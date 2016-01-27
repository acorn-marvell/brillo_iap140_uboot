/*
 * f_loopback.c - USB peripheral loopback configuration driver
 *
 * Copyright (C) 2003-2008 David Brownell
 * Copyright (C) 2008 by Nokia Corporation
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/* #define VERBOSE_DEBUG */

#include <common.h>
#include <fastboot.h>
#include <asm/errno.h>
#include <linux/usb/ch9.h>
#include <linux/usb/cdc.h>
#include <linux/usb/gadget.h>
#include "gadget_chips.h"

#ifdef CONFIG_USB_GADGET_DUALSPEED
#define DEVSPEED	USB_SPEED_HIGH
#else
#define DEVSPEED	USB_SPEED_FULL
#endif

#define buflen 4096

struct loopback_dev {
	struct usb_gadget	*gadget;
	struct usb_request	*req;		/* for control responses */
	u8			config;
	struct usb_ep		*in_ep, *out_ep;
	const struct usb_endpoint_descriptor
				*in, *out;
	struct usb_request	*tx_req, *rx_req;
};

static u8 control_req[512];
static struct loopback_dev l_lbdev;

static struct usb_device_descriptor device_desc = {
	.bLength = sizeof(device_desc),
	.bDescriptorType = USB_DT_DEVICE,
	.bcdUSB = cpu_to_le16(0x0200),
	.bDeviceClass = USB_CLASS_VENDOR_SPEC,

	.idVendor = cpu_to_le16(CONFIG_USBD_VENDORID),
	.idProduct = cpu_to_le16(CONFIG_USBD_PRODUCTID),
	.iManufacturer = 0x1,
	.iProduct = 0x2,
	.iSerialNumber = 0x3,
	.bNumConfigurations = 1,
};

static struct usb_config_descriptor loopback_config = {
	.bLength = sizeof(struct usb_config_descriptor),
	.bDescriptorType = USB_DT_CONFIG,

	.bNumInterfaces = 1,
	.bConfigurationValue = 1,
	.iConfiguration = 4,
	.bmAttributes = USB_CONFIG_ATT_ONE|USB_CONFIG_ATT_SELFPOWER,
	.bMaxPower = 0x80
};

static struct usb_interface_descriptor loopback_intf = {
	.bLength = sizeof(loopback_intf),
	.bDescriptorType = USB_DT_INTERFACE,
	.bInterfaceNumber = 0,
	.bNumEndpoints = 2,
	.bInterfaceClass = USB_CLASS_VENDOR_SPEC,
	/* .iInterface = DYNAMIC */
};

/* full speed support: */

static struct usb_endpoint_descriptor fs_loop_source_desc = {
	.bLength = USB_DT_ENDPOINT_SIZE,
	.bDescriptorType = USB_DT_ENDPOINT,

	.bEndpointAddress = 0x81,
	.bmAttributes = USB_ENDPOINT_XFER_BULK,
};

static struct usb_endpoint_descriptor fs_loop_sink_desc = {
	.bLength = USB_DT_ENDPOINT_SIZE,
	.bDescriptorType = USB_DT_ENDPOINT,

	.bEndpointAddress = 0x2,
	.bmAttributes =	USB_ENDPOINT_XFER_BULK,
};

static const struct usb_descriptor_header *fs_loopback_descs[] = {
	(struct usb_descriptor_header *)&loopback_intf,
	(struct usb_descriptor_header *)&fs_loop_sink_desc,
	(struct usb_descriptor_header *)&fs_loop_source_desc,
	NULL,
};

/* high speed support: */

static struct usb_endpoint_descriptor hs_loop_source_desc = {
	.bLength = USB_DT_ENDPOINT_SIZE,
	.bDescriptorType = USB_DT_ENDPOINT,

	.bEndpointAddress = 0x81,
	.bmAttributes = USB_ENDPOINT_XFER_BULK,
	.wMaxPacketSize = cpu_to_le16(512),
};

static struct usb_endpoint_descriptor hs_loop_sink_desc = {
	.bLength = USB_DT_ENDPOINT_SIZE,
	.bDescriptorType = USB_DT_ENDPOINT,

	.bEndpointAddress = 0x2,
	.bmAttributes =	USB_ENDPOINT_XFER_BULK,
	.wMaxPacketSize = cpu_to_le16(512),
};

static const struct usb_descriptor_header *hs_loopback_descs[] = {
	(struct usb_descriptor_header *)&loopback_intf,
	(struct usb_descriptor_header *)&hs_loop_source_desc,
	(struct usb_descriptor_header *)&hs_loop_sink_desc,
	NULL,
};

/* function-specific strings: */

static struct usb_string strings_loopback[] = {
	{ 1,	CONFIG_USBD_MANUFACTURER, },
	{ 2,	CONFIG_USBD_PRODUCT_NAME, },
	{ 3,	CONFIG_SERIAL_NUM, },
	{ 4,	CONFIG_USBD_CONFIGURATION_STR, },
	{0, NULL},
};

static struct usb_gadget_strings stringtab_loop = {
	.language	= 0x0409,	/* en-us */
	.strings	= strings_loopback,
};

static struct usb_qualifier_descriptor dev_qualifier = {
	.bLength = sizeof(dev_qualifier),
	.bDescriptorType = USB_DT_DEVICE_QUALIFIER,

	.bcdUSB = __constant_cpu_to_le16(0x0200),
	.bDeviceClass =	 USB_CLASS_VENDOR_SPEC,
	.bMaxPacketSize0 = 64,

	.bNumConfigurations = 1,
};

/* maxpacket and other transfer characteristics vary by speed. */
static inline struct usb_endpoint_descriptor *
ep_desc(struct usb_gadget *g, struct usb_endpoint_descriptor *hs,
	struct usb_endpoint_descriptor *fs)
{
	if (gadget_is_dualspeed(g) && g->speed == USB_SPEED_HIGH)
		return hs;
	return fs;
}

static void loopback_setup_complete(struct usb_ep *ep, struct usb_request *req)
{
}

/*-------------------------------------------------------------------------*/

static int loopback_bind(struct usb_gadget *gadget)
{
	struct loopback_dev *dev = &l_lbdev;
	struct usb_ep	*in_ep, *out_ep;
	int gcnum;
	gcnum = usb_gadget_controller_number(gadget);
	if (gcnum >= 0) {
		device_desc.bcdDevice = cpu_to_le16(0x0300 + gcnum);
	} else {
		/*
		 * can't assume CDC works.  don't want to default to
		 * anything less functional on CDC-capable hardware,
		 * so we fail in this case.
		 */
		error("controller '%s' not recognized", gadget->name);
		return -ENODEV;
	}

	/* all we really need is bulk IN/OUT */
	usb_ep_autoconfig_reset(gadget);
	in_ep = usb_ep_autoconfig(gadget, &fs_loop_source_desc);
	if (!in_ep) {
		error("can't autoconfigure on %s\n", gadget->name);
		return -ENODEV;
	}
	in_ep->driver_data = in_ep;	/* claim */

	out_ep = usb_ep_autoconfig(gadget, &fs_loop_sink_desc);
	if (!out_ep) {
		error("can't autoconfigure on %s\n", gadget->name);
		return -ENODEV;
	}
	out_ep->driver_data = out_ep;	/* claim */
	device_desc.bMaxPacketSize0 = gadget->ep0->maxpacket;
	usb_gadget_set_selfpowered(gadget);
	dev->in_ep = in_ep;
	dev->out_ep = out_ep;

	dev->gadget = gadget;
	set_gadget_data(gadget, dev);
	gadget->ep0->driver_data = dev;
	dev->req = usb_ep_alloc_request(gadget->ep0, 0);
	dev->req->buf = control_req;
	dev->req->complete = loopback_setup_complete;
	dev->tx_req = usb_ep_alloc_request(dev->in_ep, 0);
	dev->rx_req = usb_ep_alloc_request(dev->out_ep, 0);
	return 0;
}

static void
loopback_unbind(struct usb_gadget *gadget)
{
}

static void loopback_complete(struct usb_ep *ep, struct usb_request *req)
{
	int status = req->status;

	switch (status) {
	case 0:				/* normal completion? */
		if (ep == l_lbdev.out_ep) {
			/* loop this OUT packet back IN to the host */
			req->zero = (req->actual < req->length);
			req->length = req->actual;
			status = usb_ep_queue(l_lbdev.in_ep, req, 0);
			if (status == 0)
				return;

			/* "should never get here" */
			printf("can't loop %s to %s: %d\n", ep->name, l_lbdev.in_ep->name, status);
		}

		/* queue the buffer for some later OUT packet */
		req->length = buflen;
		status = usb_ep_queue(l_lbdev.out_ep, req, 0);
		if (status == 0)
			return;

		/* "should never get here" */
		/* FALLTHROUGH */

	default:
		printf("%s loop complete: %d, %d/%d\n", ep->name, status, req->actual, req->length);
		/* FALLTHROUGH */

	/* NOTE:  since this driver doesn't maintain an explicit record
	 * of requests it submitted (just maintains qlen count), we
	 * rely on the hardware driver to clean up on disconnect or
	 * endpoint disable.
	 */
	case -ECONNABORTED:		/* hardware forced ep reset */
	case -ECONNRESET:		/* request dequeued */
	case -ESHUTDOWN:		/* disconnect from host */
		return;
	}
}

static int
loopback_setup(struct usb_gadget *gadget, const struct usb_ctrlrequest *ctrl)
{
	struct loopback_dev		*dev = get_gadget_data(gadget);
	struct usb_request	*req = dev->req;
	int			value = -EOPNOTSUPP;
	u16			wValue = le16_to_cpu(ctrl->wValue);
	u16			wLength = le16_to_cpu(ctrl->wLength);

	switch (ctrl->bRequest) {
	case USB_REQ_GET_DESCRIPTOR:
		if (ctrl->bRequestType != USB_DIR_IN)
			break;
		switch (wValue >> 8) {
		case USB_DT_DEVICE:
			value = min(wLength, (u16) sizeof(device_desc));
			memcpy(req->buf, &device_desc, value);
			break;
		case USB_DT_OTHER_SPEED_CONFIG:
			if (!gadget_is_dualspeed(gadget))
				break;
			loopback_config.bDescriptorType = USB_DT_OTHER_SPEED_CONFIG;
			value  = usb_gadget_config_buf(&loopback_config, req->buf,
				64, fs_loopback_descs);
			if (value >= 0)
				value = min(wLength, (u16) value);
			break;
		case USB_DT_CONFIG:
			loopback_config.bDescriptorType = USB_DT_CONFIG;
			value  = usb_gadget_config_buf(&loopback_config, req->buf,
					512, hs_loopback_descs);
			if (value >= 0)
				value = min(wLength, (u16) value);
			break;

		case USB_DT_STRING:
			value = usb_gadget_get_string(&stringtab_loop,
					wValue & 0xff, req->buf);

			if (value >= 0)
				value = min(wLength, (u16) value);

			break;
		case USB_DT_DEVICE_QUALIFIER:
			value = min(wLength, (u16) sizeof(dev_qualifier));
			memcpy(req->buf, &dev_qualifier, value);
			break;
		}
		break;
	case USB_REQ_SET_CONFIGURATION:
		dev->out = ep_desc(gadget, &hs_loop_sink_desc, &fs_loop_sink_desc);
		dev->in = ep_desc(gadget, &hs_loop_source_desc, &fs_loop_source_desc);
		dev->rx_req->length = 512;
		dev->rx_req->complete = loopback_complete;
		usb_ep_enable(dev->out_ep, dev->out);
		usb_ep_enable(dev->in_ep, dev->in);
		dev->config = wValue & 0xff;
		value = 0;
		break;
	case USB_REQ_GET_CONFIGURATION:
		if (ctrl->bRequestType != USB_DIR_IN)
			break;
		*(u8 *)req->buf = dev->config;
		value = min(wLength, (u16) 1);
		break;
	}

	/* respond with data transfer before status phase? */
	if (value >= 0) {
		req->length = value;
		req->zero = value < wLength && (value % gadget->ep0->maxpacket) == 0;
		value = usb_ep_queue(gadget->ep0, req, 0);
		if (value < 0) {
			printf("ep_queue --> %d\n", value);
			req->status = 0;
		}
	}

	/* host either stalls (value < 0) or reports success */
	return value;
}
static void loopback_disconnect(struct usb_gadget *gadget)
{}

static struct usb_gadget_driver loopback_driver = {
	.speed = DEVSPEED,

	.bind = loopback_bind,
	.unbind = loopback_unbind,

	.setup = loopback_setup,
	.disconnect = loopback_disconnect,

};

void loopback_init(void)
{
	struct loopback_dev *dev = &l_lbdev;
	struct usb_gadget *gadget;
	usb_gadget_register_driver(&loopback_driver);
	gadget = dev->gadget;
	usb_gadget_connect(gadget);
	while (1) {
		usb_gadget_handle_interrupts();
		udelay(100);
	}
}

