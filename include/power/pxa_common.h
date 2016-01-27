#ifndef PXA_COMMON_H
#define PXA_COMMON_H
#include <errno.h>
#include <common.h>
#include <command.h>
#include <asm/io.h>
#include <asm/arch/features.h>

#define MHZ			(1000 * 1000)
#define MHZ_TO_KHZ		(1000)
#define MASK(n) ((1 << n) - 1)
#define REG_WIDTH_1BIT	1
#define REG_WIDTH_2BIT	2
#define REG_WIDTH_3BIT	3
#define REG_WIDTH_4BIT	4

#define CLK_SET_BITS(addr, set, clear)	{	\
	unsigned long tmp;			\
	tmp = __raw_readl(addr);		\
	tmp &= ~(clear);			\
	tmp |= (set);				\
	__raw_writel(tmp, addr);		\
}						\

struct peri_reg_info {
	void		*reg_addr;
	/* peripheral device source select */
	u32		src_sel_shift;
	u32		src_sel_width;
	u32		src_sel_mask;
	/* peripheral device divider set */
	u32		div_shift;
	u32		div_width;
	u32		div_mask;
	/* peripheral device freq change trigger */
	u32		fcreq_shift;
	u32		fcreq_width;
	u32		fcreq_mask;
};

struct periph_clk_tbl {
	unsigned long fclk_rate;	/* fclk rate */
	const char *fclk_parent;	/* fclk parent */
	u32 fclk_mul;			/* fclk src select */
	u32 fclk_div;			/* fclk src divider */
	unsigned long aclk_rate;	/* aclk rate */
	const char *aclk_parent;	/* aclk parent */
	u32 aclk_mul;			/* aclk src select */
	u32 aclk_div;			/* aclk src divider */
};

struct periph_parents {
	const char  *name;
	unsigned long rate;
	unsigned long hw_sel;
};

extern unsigned int peri_get_parent_index(const char *parent,
	struct periph_parents *table, int size);

extern void peri_set_mux_div(struct peri_reg_info *reg_info,
	unsigned long mux, unsigned long div);

extern int peri_set_clk(unsigned long freq, struct periph_clk_tbl *tbl,
	int tbl_size, struct peri_reg_info *clk_reg_info,
	struct periph_parents *periph_parents, int parent_size);

extern int peri_setrate(unsigned long freq, struct periph_clk_tbl *tbl,
	int tbl_size, struct peri_reg_info *aclk_reg_info,
	struct peri_reg_info *fclk_reg_info,
	struct periph_parents *periph_aparents, int aparent_size,
	struct periph_parents *periph_fparents, int fparent_size);

#endif /* PXA_COMMON_H */
