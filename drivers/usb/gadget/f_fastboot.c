/*
 * (C) Copyright 2008 - 2009
 * Windriver, <www.windriver.com>
 * Tom Rix <Tom.Rix@windriver.com>
 *
 * Copyright 2011 Sebastian Andrzej Siewior <bigeasy@linutronix.de>
 *
 * Copyright 2014 Linaro, Ltd.
 * Rob Herring <robh@kernel.org>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#include <config.h>
#include <common.h>
#include <errno.h>
#include <malloc.h>
#include <linux/usb/ch9.h>
#include <linux/usb/gadget.h>
#include <linux/usb/composite.h>
#include <linux/compiler.h>
#include <version.h>
#include <g_dnl.h>
#ifdef CONFIG_FASTBOOT_FLASH_MMC_DEV
#include <fb_mmc.h>
#endif
#ifdef CONFIG_MRVL_BOOT
#include <mv_boot.h>
#endif
#include <fastboot.h>
#ifdef CONFIG_MULTIPLE_SLOTS
#include <bootctrl.h>
#endif

#define FASTBOOT_VERSION		"0.4"

#define FASTBOOT_INTERFACE_CLASS	0xff
#define FASTBOOT_INTERFACE_SUB_CLASS	0x42
#define FASTBOOT_INTERFACE_PROTOCOL	0x03

#define RX_ENDPOINT_MAXIMUM_PACKET_SIZE_2_0  (0x0200)
#define RX_ENDPOINT_MAXIMUM_PACKET_SIZE_1_1  (0x0040)
#define TX_ENDPOINT_MAXIMUM_PACKET_SIZE      (0x0040)

/* The 64 defined bytes plus \0 */
#define RESPONSE_LEN	(64 + 1)

#define EP_BUFFER_SIZE			4096
DECLARE_GLOBAL_DATA_PTR;

#ifndef CONFIG_FB_RESV
#define CONFIG_FB_RESV  64
#endif

struct f_fastboot {
	struct usb_function usb_function;

	/* IN/OUT EP's and corresponding requests */
	struct usb_ep *in_ep, *out_ep;
	struct usb_request *in_req, *out_req;
};

static inline struct f_fastboot *func_to_fastboot(struct usb_function *f)
{
	return container_of(f, struct f_fastboot, usb_function);
}

static struct f_fastboot *fastboot_func;
static unsigned int download_size;
static unsigned int download_bytes;
static unsigned ramdisk_addr, ramdisk_size, kernel_addr, kernel_size;

static struct usb_endpoint_descriptor fs_ep_in = {
	.bLength		= USB_DT_ENDPOINT_SIZE,
	.bDescriptorType	= USB_DT_ENDPOINT,
	.bEndpointAddress	= USB_DIR_IN,
	.bmAttributes		= USB_ENDPOINT_XFER_BULK,
	.wMaxPacketSize		= TX_ENDPOINT_MAXIMUM_PACKET_SIZE,
	.bInterval		= 0x00,
};

static struct usb_endpoint_descriptor fs_ep_out = {
	.bLength		= USB_DT_ENDPOINT_SIZE,
	.bDescriptorType	= USB_DT_ENDPOINT,
	.bEndpointAddress	= USB_DIR_OUT,
	.bmAttributes		= USB_ENDPOINT_XFER_BULK,
	.wMaxPacketSize		= RX_ENDPOINT_MAXIMUM_PACKET_SIZE_1_1,
	.bInterval		= 0x00,
};

static struct usb_endpoint_descriptor hs_ep_out = {
	.bLength		= USB_DT_ENDPOINT_SIZE,
	.bDescriptorType	= USB_DT_ENDPOINT,
	.bEndpointAddress	= USB_DIR_OUT,
	.bmAttributes		= USB_ENDPOINT_XFER_BULK,
	.wMaxPacketSize		= RX_ENDPOINT_MAXIMUM_PACKET_SIZE_2_0,
	.bInterval		= 0x00,
};

static struct usb_interface_descriptor interface_desc = {
	.bLength		= USB_DT_INTERFACE_SIZE,
	.bDescriptorType	= USB_DT_INTERFACE,
	.bInterfaceNumber	= 0x00,
	.bAlternateSetting	= 0x00,
	.bNumEndpoints		= 0x02,
	.bInterfaceClass	= FASTBOOT_INTERFACE_CLASS,
	.bInterfaceSubClass	= FASTBOOT_INTERFACE_SUB_CLASS,
	.bInterfaceProtocol	= FASTBOOT_INTERFACE_PROTOCOL,
};

static struct usb_descriptor_header *fb_runtime_descs[] = {
	(struct usb_descriptor_header *)&interface_desc,
	(struct usb_descriptor_header *)&fs_ep_in,
	(struct usb_descriptor_header *)&hs_ep_out,
	NULL,
};

/*
 * static strings, in UTF-8
 */
static const char fastboot_name[] = "Android Fastboot";

static struct usb_string fastboot_string_defs[] = {
	[0].s = fastboot_name,
	{  }			/* end of list */
};

static struct usb_gadget_strings stringtab_fastboot = {
	.language	= 0x0409,	/* en-us */
	.strings	= fastboot_string_defs,
};

static struct usb_gadget_strings *fastboot_strings[] = {
	&stringtab_fastboot,
	NULL,
};

static void rx_handler_command(struct usb_ep *ep, struct usb_request *req);

static void fastboot_complete(struct usb_ep *ep, struct usb_request *req)
{
	int status = req->status;
	if (!status)
		return;
	printf("status: %d ep '%s' trans: %d\n", status, ep->name, req->actual);
}

static int fastboot_bind(struct usb_configuration *c, struct usb_function *f)
{
	int id;
	struct usb_gadget *gadget = c->cdev->gadget;
	struct f_fastboot *f_fb = func_to_fastboot(f);

	char *s;
	s = getenv("fb_serial");
	if (s != NULL) {
		printf("Fastboot serial no set as:%s\n", s);
		g_dnl_set_serialnumber(s);
	}

	/* DYNAMIC interface numbers assignments */
	id = usb_interface_id(c, f);
	if (id < 0)
		return id;
	interface_desc.bInterfaceNumber = id;

	id = usb_string_id(c->cdev);
	if (id < 0)
		return id;
	fastboot_string_defs[0].id = id;
	interface_desc.iInterface = id;

	f_fb->in_ep = usb_ep_autoconfig(gadget, &fs_ep_in);
	if (!f_fb->in_ep)
		return -ENODEV;
	f_fb->in_ep->driver_data = c->cdev;

	f_fb->out_ep = usb_ep_autoconfig(gadget, &fs_ep_out);
	if (!f_fb->out_ep)
		return -ENODEV;
	f_fb->out_ep->driver_data = c->cdev;

#ifdef CONFIG_USB_GADGET_DUALSPEED
	fs_ep_in.wMaxPacketSize = __constant_cpu_to_le16(512);
#endif

	hs_ep_out.bEndpointAddress = fs_ep_out.bEndpointAddress;

	return 0;
}

static void fastboot_unbind(struct usb_configuration *c, struct usb_function *f)
{
	memset(fastboot_func, 0, sizeof(*fastboot_func));
}

static void fastboot_disable(struct usb_function *f)
{
	struct f_fastboot *f_fb = func_to_fastboot(f);

	usb_ep_disable(f_fb->out_ep);
	usb_ep_disable(f_fb->in_ep);

	if (f_fb->out_req) {
		free(f_fb->out_req->buf);
		usb_ep_free_request(f_fb->out_ep, f_fb->out_req);
		f_fb->out_req = NULL;
	}
	if (f_fb->in_req) {
		free(f_fb->in_req->buf);
		usb_ep_free_request(f_fb->in_ep, f_fb->in_req);
		f_fb->in_req = NULL;
	}
}

static struct usb_request *fastboot_start_ep(struct usb_ep *ep)
{
	struct usb_request *req;

	req = usb_ep_alloc_request(ep, 0);
	if (!req)
		return NULL;

	req->length = EP_BUFFER_SIZE;
	req->buf = memalign(CONFIG_SYS_CACHELINE_SIZE, EP_BUFFER_SIZE);
	if (!req->buf) {
		usb_ep_free_request(ep, req);
		return NULL;
	}

	memset(req->buf, 0, req->length);
	return req;
}

static int fastboot_set_alt(struct usb_function *f,
			    unsigned interface, unsigned alt)
{
	int ret;
	struct usb_composite_dev *cdev = f->config->cdev;
	struct usb_gadget *gadget = cdev->gadget;
	struct f_fastboot *f_fb = func_to_fastboot(f);

	debug("%s: func: %s intf: %d alt: %d\n",
	      __func__, f->name, interface, alt);

	/* make sure we don't enable the ep twice */
	if (gadget->speed == USB_SPEED_HIGH)
		ret = usb_ep_enable(f_fb->out_ep, &hs_ep_out);
	else
		ret = usb_ep_enable(f_fb->out_ep, &fs_ep_out);
	if (ret) {
		puts("failed to enable out ep\n");
		return ret;
	}

	f_fb->out_req = fastboot_start_ep(f_fb->out_ep);
	if (!f_fb->out_req) {
		puts("failed to alloc out req\n");
		ret = -EINVAL;
		goto err;
	}
	f_fb->out_req->complete = rx_handler_command;

	ret = usb_ep_enable(f_fb->in_ep, &fs_ep_in);
	if (ret) {
		puts("failed to enable in ep\n");
		goto err;
	}

	f_fb->in_req = fastboot_start_ep(f_fb->in_ep);
	if (!f_fb->in_req) {
		puts("failed alloc req in\n");
		ret = -EINVAL;
		goto err;
	}
	f_fb->in_req->complete = fastboot_complete;

	ret = usb_ep_queue(f_fb->out_ep, f_fb->out_req, 0);
	if (ret)
		goto err;

	return 0;
err:
	fastboot_disable(f);
	return ret;
}

static int fastboot_add(struct usb_configuration *c)
{
	struct f_fastboot *f_fb = fastboot_func;
	int status;

	debug("%s: cdev: 0x%p\n", __func__, c->cdev);

	if (!f_fb) {
		f_fb = memalign(CONFIG_SYS_CACHELINE_SIZE, sizeof(*f_fb));
		if (!f_fb)
			return -ENOMEM;

		fastboot_func = f_fb;
		memset(f_fb, 0, sizeof(*f_fb));
	}

	f_fb->usb_function.name = "f_fastboot";
	f_fb->usb_function.hs_descriptors = fb_runtime_descs;
	f_fb->usb_function.bind = fastboot_bind;
	f_fb->usb_function.unbind = fastboot_unbind;
	f_fb->usb_function.set_alt = fastboot_set_alt;
	f_fb->usb_function.disable = fastboot_disable;
	f_fb->usb_function.strings = fastboot_strings;

	status = usb_add_function(c, &f_fb->usb_function);
	if (status) {
		free(f_fb);
		fastboot_func = f_fb;
	}

	return status;
}
DECLARE_GADGET_BIND_CALLBACK(usb_dnl_fastboot, fastboot_add);

static int fastboot_tx_write(const char *buffer, unsigned int buffer_size)
{
	struct usb_request *in_req = fastboot_func->in_req;
	int ret;

	memcpy(in_req->buf, buffer, buffer_size);
	in_req->length = buffer_size;
	ret = usb_ep_queue(fastboot_func->in_ep, in_req, 0);
	if (ret)
		printf("Error %d on queue\n", ret);
	return 0;
}

static int fastboot_tx_write_str(const char *buffer)
{
	return fastboot_tx_write(buffer, strlen(buffer));
}

#define BOOT_MAGIC "ANDROID!"
#define BOOT_MAGIC_SIZE 8
#define BOOT_NAME_SIZE 16
#define BOOT_ARGS_SIZE 512

struct boot_img_hdr {
	unsigned char magic[BOOT_MAGIC_SIZE];

	unsigned kernel_size;  /* size in bytes */
	unsigned kernel_addr;  /* physical load addr */

	unsigned ramdisk_size; /* size in bytes */
	unsigned ramdisk_addr; /* physical load addr */

	unsigned second_size;  /* size in bytes */
	unsigned second_addr;  /* physical load addr */

	unsigned tags_addr;    /* physical addr for kernel tags */
	unsigned page_size;    /* flash page size we assume */
	unsigned unused[2];    /* future expansion: should be 0 */

	unsigned char name[BOOT_NAME_SIZE]; /* asciiz product name */

	unsigned char cmdline[BOOT_ARGS_SIZE];

	unsigned id[8]; /* timestamp / checksum / sha1 / etc */
};

static int init_boot_linux(void)
{
	struct boot_img_hdr *hdr =
		(void *)(uintptr_t)CONFIG_USB_FASTBOOT_BUF_ADDR;
	unsigned page_mask = 2047;
	unsigned kernel_actual;
	unsigned ramdisk_actual;
	unsigned second_actual;

	if (kernel_size < 2048) {
		printf("bootimg: bad header, kernel_size is wrong\n");
		return -1;
	}

	if (memcmp(hdr->magic, BOOT_MAGIC, BOOT_MAGIC_SIZE)) {
		printf("bootimg: bad header\n");
		return -1;
	}

	if (hdr->page_size != 2048) {
		printf("bootimg: invalid page size\n");
		return -1;
	}

	kernel_actual = (hdr->kernel_size + page_mask) & (~page_mask);
	ramdisk_actual = (hdr->ramdisk_size + page_mask) & (~page_mask);
	second_actual = (hdr->second_size + page_mask) & (~page_mask);

	if (kernel_size !=
	    (kernel_actual + ramdisk_actual + second_actual + 2048)) {
		printf("bootimg: invalid image size");
		return -1;
	}

	/* XXX process commandline here */
	if (hdr->cmdline[0]) {
		hdr->cmdline[BOOT_ARGS_SIZE - 1] = 0;
		printf("cmdline is: %s\n", hdr->cmdline);
		setenv("bootargs", (char *)hdr->cmdline);
	}

	/* XXX how to validate addresses? */
	kernel_addr = (uintptr_t)hdr + 2048;
	kernel_size = hdr->kernel_size;
	ramdisk_size = hdr->ramdisk_size;
	if (ramdisk_size > 0)
		ramdisk_addr = kernel_addr + 2048 + kernel_size;
	else
		ramdisk_addr = 0;

	printf("bootimg: kernel addr=0x%x size=0x%x\n",
	       kernel_addr, kernel_size);
	printf("bootimg: ramdisk addr=0x%x size=0x%x\n",
	       ramdisk_addr, ramdisk_size);

	return 0;
}

static void compl_do_reset(struct usb_ep *ep, struct usb_request *req)
{
	do_reset(NULL, 0, 0, NULL);
}

static void cb_reboot(struct usb_ep *ep, struct usb_request *req)
{
	char *cmd = req->buf;

	if (strncmp(cmd + 6, "-bootloader", 11) == 0)
		stop_fastboot = 1;
	else
		fastboot_func->in_req->complete = compl_do_reset;
	fastboot_tx_write_str("OKAY");
}

static int strcmp_l1(const char *s1, const char *s2)
{
	if (!s1 || !s2)
		return -1;
	return strncmp(s1, s2, strlen(s1));
}

static void var_partition_type(const char *part, char *response)
{
	block_dev_desc_t *dev = get_dev("mmc", CONFIG_FASTBOOT_FLASH_MMC_DEV);
	disk_partition_t info;

	if (!dev) {
		strcpy(response, "FAILfailed to read mmc");
		return;
	}

	if (get_partition_info_efi_by_name(dev, part, &info)) {
		strcpy(response, "FAILpartition not found");
		return;
	}

	snprintf(response, RESPONSE_LEN, "OKAY%s", info.type);
}

static void var_partition_size(const char *part, char *response)
{
	block_dev_desc_t *dev = get_dev("mmc", CONFIG_FASTBOOT_FLASH_MMC_DEV);
	disk_partition_t info;

	if (!dev) {
		fastboot_tx_write_str("FAILfailed to read mmc");
		return;
	}

	if (get_partition_info_efi_by_name(dev, part, &info)) {
		fastboot_tx_write_str("FAILpartition not found");
		return;
	}

	snprintf(response, RESPONSE_LEN, "OKAY0x%016llx", info.size * info.blksz);
}

static void cb_getvar(struct usb_ep *ep, struct usb_request *req)
{
	char *cmd = req->buf;
	char response[RESPONSE_LEN];
	const char *s;
	size_t chars_left;

	strcpy(response, "OKAY");
	chars_left = sizeof(response) - strlen(response) - 1;

	strsep(&cmd, ":");
	if (!cmd) {
		error("missing variable\n");
		fastboot_tx_write_str("FAILmissing var");
		return;
	}

	if (!strcmp_l1("version", cmd)) {
		strncat(response, FASTBOOT_VERSION, chars_left);
	} else if (!strcmp_l1("bootloader-version", cmd)) {
		strncat(response, U_BOOT_VERSION, chars_left);
	} else if (!strcmp_l1("downloadsize", cmd) ||
		!strcmp_l1("max-download-size", cmd)) {
		char str_num[12];

		sprintf(str_num, "0x%08x", (CONFIG_FB_RESV*1024*1024));
		strncat(response, str_num, chars_left);
	} else if (!strcmp_l1("serialno", cmd)) {
		//s = getenv("serial#");
		s = getenv("fb_serial");
		if (s)
			strncat(response, s, chars_left);
		else
			strcpy(response, "FAILValue not set");
#ifdef CONFIG_MULTIPLE_SLOTS
	} else if (!strcmp_l1("slot-suffixes", cmd)) {
		int i;
		char str_suffixes[12] = {0};

		for(i= 0; i< bootctrl_get_boot_slots_num(); i++)
			sprintf(str_suffixes+strlen(str_suffixes), "%s,", bootctrl_get_boot_slot_suffix(i));

		strncat(response, str_suffixes, chars_left);
	} else if (!strcmp_l1("has-slot", cmd)) {
		strsep(&cmd, ":");
		if (!cmd) {
			error("missing variable\n");
			fastboot_tx_write_str("FAILmissing var");
			return;
		}
		if(bootctrl_is_partition_support_slot(cmd)){
			strncat(response, "yes", chars_left);
		}else{
			strncat(response, "no", chars_left);
		}
	} else if (!strcmp_l1("current-slot", cmd)) {
		strncat(response, bootctrl_get_active_slot_suffix(), chars_left);
#endif
	} else if(!strcmp_l1("product",cmd)){
		strncat(response, CONFIG_PRODUCT_STRING, chars_left);
	} else if (!strcmp_l1("partition-type", cmd)) {
		var_partition_type(cmd + 15, response);
	} else if (!strcmp_l1("partition-size", cmd)) {
		var_partition_size(cmd + 15, response);
	} else {
		error("unknown variable: %s\n", cmd);
		strcpy(response, "FAILVariable not implemented");
	}
	fastboot_tx_write_str(response);
}

static unsigned int rx_bytes_expected(void)
{
	int rx_remain = download_size - download_bytes;
	if (rx_remain < 0)
		return 0;
	if (rx_remain > EP_BUFFER_SIZE)
		return EP_BUFFER_SIZE;
	return rx_remain;
}

#define BYTES_PER_DOT	0x20000
static void rx_handler_dl_image(struct usb_ep *ep, struct usb_request *req)
{
	char response[RESPONSE_LEN];
	unsigned int transfer_size = download_size - download_bytes;
	const unsigned char *buffer = req->buf;
	unsigned int buffer_size = req->actual;
	unsigned int pre_dot_num, now_dot_num;

	if (req->status != 0) {
		printf("Bad status: %d\n", req->status);
		return;
	}

	if (buffer_size < transfer_size)
		transfer_size = buffer_size;

	memcpy((void *)CONFIG_USB_FASTBOOT_BUF_ADDR + download_bytes,
	       buffer, transfer_size);

	pre_dot_num = download_bytes / BYTES_PER_DOT;
	download_bytes += transfer_size;
	now_dot_num = download_bytes / BYTES_PER_DOT;

	if (pre_dot_num != now_dot_num) {
		putc('.');
		if (!(now_dot_num % 74))
			putc('\n');
	}

	/* Check if transfer is done */
	if (download_bytes >= download_size) {
		/*
		 * Reset global transfer variable, keep download_bytes because
		 * it will be used in the next possible flashing command
		 */
		kernel_size = download_size;
		download_size = 0;
		req->complete = rx_handler_command;
		req->length = EP_BUFFER_SIZE;

		sprintf(response, "OKAY");
		fastboot_tx_write_str(response);

		printf("\ndownloading of %d bytes finished\n", download_bytes);
	} else {
		req->length = rx_bytes_expected();
		if (req->length < ep->maxpacket)
			req->length = ep->maxpacket;
	}

	req->actual = 0;
	usb_ep_queue(ep, req, 0);
}

static void cb_download(struct usb_ep *ep, struct usb_request *req)
{
	char *cmd = req->buf;
	char response[RESPONSE_LEN];

	strsep(&cmd, ":");
	download_size = simple_strtoul(cmd, NULL, 16);
	download_bytes = 0;

	printf("Starting download of %d bytes\n", download_size);

	if (0 == download_size) {
		sprintf(response, "FAILdata invalid size");
	} else if (download_size > (CONFIG_FB_RESV*1024*1024)) {
		download_size = 0;
		sprintf(response, "FAILdata too large");
	} else {
		sprintf(response, "DATA%08x", download_size);
		req->complete = rx_handler_dl_image;
		req->length = rx_bytes_expected();
		if (req->length < ep->maxpacket)
			req->length = ep->maxpacket;
	}
	fastboot_tx_write_str(response);
}

#ifdef CONFIG_MRVL_BOOT
static void do_boot_on_complete(struct usb_ep *ep, struct usb_request *req)
{
	char response[RESPONSE_LEN];

	puts("Booting kernel...\n");

	image_load_notify(kernel_addr);
	sprintf(response, "boot");
	printf("cmd:%s\n", response);
	run_command(response, 0);
	fastboot_tx_write_str("FAIL Not zImage");
}
#else
static void do_bootm_on_complete(struct usb_ep *ep, struct usb_request *req)
{
	char boot_addr_start[12];
	char *bootm_args[] = { "bootm", boot_addr_start, NULL };

	puts("Booting kernel..\n");

	sprintf(boot_addr_start, "0x%lx", load_addr);
	do_bootm(NULL, 0, 2, bootm_args);

	/* This only happens if image is somehow faulty so we start over */
	do_reset(NULL, 0, 0, NULL);
}
#endif
static void cb_boot(struct usb_ep *ep, struct usb_request *req)
{
	if (init_boot_linux()) {
		fastboot_tx_write_str("FAIL invalid boot image");
	} else {
#ifdef CONFIG_MRVL_BOOT
		fastboot_func->in_req->complete = do_boot_on_complete;
#else
		fastboot_func->in_req->complete = do_bootm_on_complete;
#endif
	}
	fastboot_tx_write_str("OKAY");
}

static void cb_senddata(struct usb_ep *ep, struct usb_request *req)
{
	char response[RESPONSE_LEN];
	int i, ret;
	char *p;

	printf("!!!!Attention: old version fastboot is used!!!!\n");
	printf("Please get the latest version in the following folder:\n");
	printf("//sh-fs03/Embedded_OS/XScaleLinux/crash_utility/fastboot\n");
	ret = sprintf(response, "OKAY");
	p = response + ret;
	for (i = 0; i < CONFIG_NR_DRAM_BANKS; i++) {
		if (!gd->bd->bi_dram[i].size)
			continue;

		ret = sprintf(p, "%lx:%lx:",
			      gd->bd->bi_dram[i].start,
			      gd->bd->bi_dram[i].size);
		p += ret;
	}

	fastboot_tx_write_str(response);
	return;
}

static void cb_check_arch_and_meminfo(struct usb_ep *ep,
				      struct usb_request *req)
{
	char response[RESPONSE_LEN];
	int i, ret;
	int is_arm64;
	char *p;

	ret = sprintf(response, "OKAY");
	p = response + ret;

	is_arm64 = (((read_cpuid_id() >> 4) & 0xfff) > 0xd00);
#ifdef CONFIG_ENV_RE_OFFSET
	if (is_arm64 && (get_cpu_arch() == IH_ARCH_ARM))
		is_arm64 = 0;
#endif

	ret = sprintf(p, "%s:", is_arm64 ? "arm64" : "arm");
	p += ret;

	for (i = 0; i < CONFIG_NR_DRAM_BANKS; i++) {
		if (!gd->bd->bi_dram[i].size)
			continue;

		ret = sprintf(p, "%lx:%lx:",
			      gd->bd->bi_dram[i].start,
			      gd->bd->bi_dram[i].size);
		p += ret;
	}

	fastboot_tx_write_str(response);
	return;
}

static unsigned fb_packet_sent;
static void tx_complete(struct usb_ep *ep, struct usb_request *req)
{
	fb_packet_sent = 1;
}

static int fastboot_tx_upload(void *buffer, unsigned int buffer_size)
{
	struct usb_request *in_req = fastboot_func->in_req;
	unsigned int in_ep_maxpacket = fastboot_func->in_ep->maxpacket;
	int ret;

	while (buffer_size) {
		in_req->length = (buffer_size > in_ep_maxpacket)
			? in_ep_maxpacket : buffer_size;
		memcpy(in_req->buf, buffer, in_req->length);
		in_req->context = NULL;
		in_req->complete = tx_complete;
		fb_packet_sent = 0;
		ret = usb_ep_queue(fastboot_func->in_ep, in_req, 0);
		if (ret)
			printf("Error %d on queue\n", ret);
		while (!fb_packet_sent)
			usb_gadget_handle_interrupts();
		buffer += in_req->length;
		buffer_size -= in_req->length;
	}
	printf("uploading finished.\n");
	return 0;
}

static void cb_upload(struct usb_ep *ep, struct usb_request *req)
{
	char *cmd = req->buf;
	char *s = cmd + 6, *p;
	unsigned long upload_start, upload_sz;

	p = strchr(s, ':');
	if (!p) {
		fastboot_tx_write_str("FAILno valid start pos");
		return;
	}
	*p = 0;
	upload_start = simple_strtoul(s, (char **)&s, 16);
	s = p + 1;
	p = strchr(s, ':');
	if (!p) {
		fastboot_tx_write_str("FAILno valid start pos");
		return;
	}
	*p = 0;
	upload_sz = simple_strtoul(s, (char **)&s, 16);
	if (!upload_sz) {
		fastboot_tx_write_str("FAILno valid upload sz");
		return;
	}

	fastboot_tx_upload((void *)upload_start, upload_sz);

	return;
}

#ifdef CONFIG_MULTIPLE_SLOTS
static void cb_set_active(struct usb_ep *ep, struct usb_request *req)
{
	char *cmd = req->buf;
	char response[RESPONSE_LEN];

	strsep(&cmd, ":");
	if (!cmd) {
		error("missing slot_suffix\n");
		fastboot_tx_write_str("FAILmissing slot_suffix");
		return;
	}

	bootctrl_set_active_slot(cmd, response);
	
	fastboot_tx_write_str(response);
}
#endif

#ifdef CONFIG_FASTBOOT_FLASH
#define SECTOR_SIZE 512
/*Remove it because it is not called*/
#if 0
static int mkbootimg(void *kernel)
{
#ifdef CONFIG_ANDROID_BOOT_IMAGE
	unsigned long ramdisk_addr = RAMDISK_LOADADDR;
#endif
	struct boot_img_hdr *hdr = (struct boot_img_hdr *)kernel;
	struct boot_img_hdr *ahdr = (void *)ramdisk_addr;
	unsigned ramdisk_offsize;
	char cmd[128];
	unsigned long ramdisk_size = RAMDISK_SIZE;
	unsigned long base_addr = BOOTIMG_EMMC_ADDR;
	base_addr /= SECTOR_SIZE;

	/* If the burning image is not bootimg or already have ramdisk*/
	if (memcmp(BOOT_MAGIC, hdr->magic, BOOT_MAGIC_SIZE) ||
	    hdr->ramdisk_size) {
		return 0;
	}

	/* parse the raw image header and read the ramdisk to memory*/
	sprintf(cmd, "%s; mmc read %lx %lx 0x8",
		CONFIG_MMC_BOOT_DEV, ramdisk_addr, base_addr);
	run_command(cmd, 0);
	if (!memcmp(BOOT_MAGIC, ahdr->magic, BOOT_MAGIC_SIZE) &&
	    ahdr->ramdisk_size) {
		hdr->ramdisk_size = ahdr->ramdisk_size;
		ramdisk_offsize = ALIGN(ahdr->kernel_size, ahdr->page_size)
			+ ahdr->page_size;
		ramdisk_offsize = ramdisk_offsize / SECTOR_SIZE;
		ramdisk_size = ahdr->ramdisk_size;
		ramdisk_size /= SECTOR_SIZE;
		sprintf(cmd, "%s; mmc read 0x%08lx %lx %lx",
			CONFIG_MMC_BOOT_DEV, ramdisk_addr,
			base_addr + ramdisk_offsize, ramdisk_size);
		run_command(cmd, 0);
	} else {
		/*In the flash, there is no boot.img*/
		printf("there's no boot.img in the flash\n");
		return 0;
	}
	/*write ramdisk.img to flash */
	ramdisk_offsize = hdr->page_size
		+ ALIGN(hdr->kernel_size, hdr->page_size);
	ramdisk_offsize = ramdisk_offsize / SECTOR_SIZE;
	sprintf(cmd, "%s; mmc write %lx %lx %lx",
		CONFIG_MMC_BOOT_DEV, ramdisk_addr,
		base_addr + ramdisk_offsize, ramdisk_size);
	run_command(cmd, 0);
	return 1;
}
#endif

static void cb_flash(struct usb_ep *ep, struct usb_request *req)
{
	char *cmd = req->buf;
	char response[RESPONSE_LEN];

	strsep(&cmd, ":");
	if (!cmd) {
		error("missing partition name\n");
		fastboot_tx_write_str("FAILmissing partition name");
		return;
	}

	//strcpy(response, "FAILno flash device defined");
#ifdef CONFIG_FASTBOOT_FLASH_MMC_DEV
	//mkbootimg((void *)(uintptr_t)CONFIG_USB_FASTBOOT_BUF_ADDR);
	fb_mmc_flash_write(cmd, (void *)CONFIG_USB_FASTBOOT_BUF_ADDR,
			   download_bytes, response);
#endif
	fastboot_tx_write_str(response);
}

static void cb_erase(struct usb_ep *ep, struct usb_request *req)
{
	char *cmd = req->buf;
	char response[RESPONSE_LEN];

	strsep(&cmd, ":");
	if (!cmd) {
		error("missing partition name\n");
		fastboot_tx_write_str("FAILmissing partition name");
		return;
	}

	strcpy(response, "FAILno flash device defined");
#ifdef CONFIG_FASTBOOT_FLASH_MMC_DEV
	fb_mmc_erase(cmd, response);
#endif
	fastboot_tx_write_str(response);
}
#endif

struct cmd_dispatch_info {
	char *cmd;
	void (*cb)(struct usb_ep *ep, struct usb_request *req);
};

static const struct cmd_dispatch_info cmd_dispatch_info[] = {
	{
		.cmd = "reboot",
		.cb = cb_reboot,
	}, {
		.cmd = "getvar:",
		.cb = cb_getvar,
	}, {
		.cmd = "download:",
		.cb = cb_download,
	}, {
		.cmd = "boot",
		.cb = cb_boot,
	}, {
		.cmd = "senddata",
		.cb = cb_senddata,
	}, {
		.cmd = "check_arch_and_meminfo",
		.cb = cb_check_arch_and_meminfo,
	}, {
		.cmd = "upload",
		.cb = cb_upload,
	},
#ifdef CONFIG_MULTIPLE_SLOTS
	{
		.cmd = "set_active",
		.cb = cb_set_active,
	},
#endif
#ifdef CONFIG_FASTBOOT_FLASH
	{
		.cmd = "flash",
		.cb = cb_flash,
	}, {
		.cmd = "erase",
		.cb = cb_erase,
	}
#endif
};

static void rx_handler_command(struct usb_ep *ep, struct usb_request *req)
{
	char *cmdbuf = req->buf;
	void (*func_cb)(struct usb_ep *ep, struct usb_request *req) = NULL;
	int i;

	for (i = 0; i < ARRAY_SIZE(cmd_dispatch_info); i++) {
		if (!strcmp_l1(cmd_dispatch_info[i].cmd, cmdbuf)) {
			func_cb = cmd_dispatch_info[i].cb;
			break;
		}
	}

	if (!func_cb) {
		error("unknown command: %s\n", cmdbuf);
		fastboot_tx_write_str("FAILunknown command");
	} else {
		if (req->actual < req->length) {
			u8 *buf = (u8 *)req->buf;
			buf[req->actual] = 0;
			printf(">command: %s\n", cmdbuf);
			func_cb(ep, req);
		} else {
			error("buffer overflow\n");
			fastboot_tx_write_str("FAILbuffer overflow");
		}
	}

	if (req->status == 0) {
		*cmdbuf = '\0';
		req->actual = 0;
		usb_ep_queue(ep, req, 0);
	}
}
