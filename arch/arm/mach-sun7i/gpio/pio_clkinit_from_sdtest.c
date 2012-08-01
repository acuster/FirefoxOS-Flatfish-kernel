
#define CCM_BASE			(g_ccm_reg_vbase)

/* clock ID */
#define AXI_BUS				(0)
#define AHB1_BUS0			(1)
#define AHB1_BUS1			(2)
#define AHB1_LVDS			(3)
#define APB1_BUS0			(4)
#define APB2_BUS0			(5)

#define PIO_CKID			((APB1_BUS0 << 8) | 5)

#define CCM_AHB1_RST_REG0		(CCM_BASE+0x02C0)
#define CCM_AHB1_RST_REG1		(CCM_BASE+0x02C4)
#define CCM_AHB1_RST_REG2		(CCM_BASE+0x02C8)
#define CCM_APB1_RST_REG		(CCM_BASE+0x02D0)
#define CCM_APB2_RST_REG		(CCM_BASE+0x02D8)

#define set_wbit(addr, v)   	(*((volatile unsigned long  *)(addr)) |=  (unsigned long)(v))
#define clr_wbit(addr, v)   	(*((volatile unsigned long  *)(addr)) &= ~(unsigned long)(v))

static void ccm_module_disable(u32 clk_id)
{
	switch(clk_id>>8) {
		case AHB1_BUS0:
			clr_wbit(CCM_AHB1_RST_REG0, 0x1U<<(clk_id&0xff));
			break;
		case AHB1_BUS1:
			clr_wbit(CCM_AHB1_RST_REG1, 0x1U<<(clk_id&0xff));
			break;
		case AHB1_LVDS:
			clr_wbit(CCM_AHB1_RST_REG2, 0x1U<<(clk_id&0xff));
			break;
		case APB1_BUS0:
			clr_wbit(CCM_APB1_RST_REG, 0x1U<<(clk_id&0xff));
			break;
		case APB2_BUS0:
			clr_wbit(CCM_APB2_RST_REG, 0x1U<<(clk_id&0xff));
			break;
	}
}

static void ccm_module_enable(u32 clk_id)
{
	switch(clk_id>>8) {
		case AHB1_BUS0:
			set_wbit(CCM_AHB1_RST_REG0, 0x1U<<(clk_id&0xff));
			break;
		case AHB1_BUS1:
			set_wbit(CCM_AHB1_RST_REG1, 0x1U<<(clk_id&0xff));
			break;
		case AHB1_LVDS:
			set_wbit(CCM_AHB1_RST_REG2, 0x1U<<(clk_id&0xff));
			break;
		case APB1_BUS0:
			set_wbit(CCM_APB1_RST_REG, 0x1U<<(clk_id&0xff));
			break;
		case APB2_BUS0:
			set_wbit(CCM_APB2_RST_REG, 0x1U<<(clk_id&0xff));
			break;
	}
}

static void ccm_module_reset(u32 clk_id)
{
	ccm_module_disable(clk_id);
	ccm_module_disable(clk_id);
	ccm_module_enable(clk_id);
}

#define CCM_CPU_L2_AXI_CTRL		(CCM_BASE+0x050)
#define CCM_AHB1_APB1_CTRL		(CCM_BASE+0x054)
#define CCM_APB2_CLK_CTRL		(CCM_BASE+0x058)
#define CCM_AXI_GATE_CTRL		(CCM_BASE+0x05c)
#define CCM_AHB1_GATE0_CTRL		(CCM_BASE+0x060)
#define CCM_AHB1_GATE1_CTRL		(CCM_BASE+0x064)
#define CCM_APB1_GATE0_CTRL		(CCM_BASE+0x068)
#define CCM_APB2_GATE0_CTRL		(CCM_BASE+0x06C)

static void ccm_clock_enable(u32 clk_id)
{
	switch(clk_id>>8) {
		case AXI_BUS:
			set_wbit(CCM_AXI_GATE_CTRL, 0x1U<<(clk_id&0xff));
			break;
		case AHB1_BUS0:
			set_wbit(CCM_AHB1_GATE0_CTRL, 0x1U<<(clk_id&0xff));
			break;
		case AHB1_BUS1:
			set_wbit(CCM_AHB1_GATE1_CTRL, 0x1U<<(clk_id&0xff));
			break;
		case APB1_BUS0:
			set_wbit(CCM_APB1_GATE0_CTRL, 0x1U<<(clk_id&0xff));
			break;
		case APB2_BUS0:
			set_wbit(CCM_APB2_GATE0_CTRL, 0x1U<<(clk_id&0xff));
			break;
	}
}

static s32 gpio_clk_init(void)
{
	ccm_module_reset(PIO_CKID);
	ccm_clock_enable(PIO_CKID);
	return 0;
}

/* for r gpio below */
/*
#define R_PIO_CKID			(0)

#define R_PRCM_BASE			(g_r_prcm_reg_vbase)
//#define R_PRCM_BASE			(0x01f01400)

#define R_APB0_MOD_RST_REG		(R_PRCM_BASE + 0xb0)
#define R_APB0_CLK_GATE_REG		(R_PRCM_BASE + 0x28)
#define R_CPU0_CLK_CFG_REG		(R_PRCM_BASE + 0x00)
void r_prcm_module_enable(u32 clkid)
{
	set_wbit(R_APB0_MOD_RST_REG, 0x1U << clkid);
}

void r_prcm_module_disable(u32 clkid)
{
	clr_wbit(R_APB0_MOD_RST_REG, 0x1U << clkid);
}

void r_prcm_module_reset(u32 clkid)
{
	r_prcm_module_disable(clkid);
	r_prcm_module_enable(clkid);
}

void r_prcm_clock_enable(u32 clkid)
{
	set_wbit(R_APB0_CLK_GATE_REG, 0x1U << clkid);
}

void r_prcm_clock_disable(u32 clkid)
{
	clr_wbit(R_APB0_CLK_GATE_REG, 0x1U << clkid);
}

void r_prcm_core_init(void)
{
	u32 rval;

	//enable cpu0 clock
	rval = readl(R_CPU0_CLK_CFG_REG);
	writel(rval | 0x1, R_CPU0_CLK_CFG_REG);

}

s32 init_r_gpio(void)
{
	r_prcm_module_reset(R_PIO_CKID);
	r_prcm_clock_enable(R_PIO_CKID);
	return 0;
}

static s32 r_gpio_clk_init(void)
{
	r_prcm_core_init();
	init_r_gpio();
	return 0;
}
*/
