/*
 *  arch/arm/mach-sun6i/clock/ccm.c
 *
 * Copyright (c) 2012 Allwinner.
 * kevin.z.m (kevin@allwinnertech.com)
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
#include <mach/platform.h>
#include <mach/clock.h>
#include "ccm_i.h"

#define make_aw_clk_inf(clk_id, clk_name)   {.id = clk_id, .name = clk_name}
__ccmu_reg_list_t       *aw_ccu_reg;
__ccmu_reg_cpu0_list_t  *aw_cpu0_reg;


static __aw_ccu_clk_t aw_ccu_clk_tbl[] =
{
    make_aw_clk_inf(AW_SYS_CLK_NONE,        "sys_none"      ),
    make_aw_clk_inf(AW_SYS_CLK_LOSC,        "sys_losc"      ),
    make_aw_clk_inf(AW_SYS_CLK_HOSC,        "sys_hosc"      ),
    make_aw_clk_inf(AW_SYS_CLK_PLL1,        "sys_pll1"      ),
    make_aw_clk_inf(AW_SYS_CLK_PLL2,        "sys_pll2"      ),
    make_aw_clk_inf(AW_SYS_CLK_PLL3,        "sys_pll3"      ),
    make_aw_clk_inf(AW_SYS_CLK_PLL4,        "sys_pll4"      ),
    make_aw_clk_inf(AW_SYS_CLK_PLL5,        "sys_pll5"      ),
    make_aw_clk_inf(AW_SYS_CLK_PLL6,        "sys_pll6"      ),
    make_aw_clk_inf(AW_SYS_CLK_PLL7,        "sys_pll7"      ),
    make_aw_clk_inf(AW_SYS_CLK_PLL8,        "sys_pll8"      ),
    make_aw_clk_inf(AW_SYS_CLK_PLL9,        "sys_pll9"      ),
    make_aw_clk_inf(AW_SYS_CLK_PLL10,       "sys_pll10"     ),
    make_aw_clk_inf(AW_SYS_CLK_PLL2X8,      "sys_pll2x8"    ),
    make_aw_clk_inf(AW_SYS_CLK_PLL3X2,      "sys_pll3x2"    ),
    make_aw_clk_inf(AW_SYS_CLK_PLL6x2,      "sys_pll6x2"    ),
    make_aw_clk_inf(AW_SYS_CLK_PLL7X2,      "sys_pll7x2"    ),
    make_aw_clk_inf(AW_SYS_CLK_MIPIPLL,     "sys_mipi-pll"  ),
    make_aw_clk_inf(AW_SYS_CLK_AC327,       "sys_ac327"     ),
    make_aw_clk_inf(AW_SYS_CLK_AR100,       "sys_ar100"     ),
    make_aw_clk_inf(AW_SYS_CLK_AXI,         "sys_axi"       ),
    make_aw_clk_inf(AW_SYS_CLK_AHB0,        "sys_ahb0"      ),
    make_aw_clk_inf(AW_SYS_CLK_AHB1,        "sys_ahb1"      ),
    make_aw_clk_inf(AW_SYS_CLK_APB0,        "sys_apb0"      ),
    make_aw_clk_inf(AW_SYS_CLK_APB1,        "sys_apb1"      ),
    make_aw_clk_inf(AW_SYS_CLK_APB2,        "sys_apb2"      ),
    make_aw_clk_inf(AW_CCU_CLK_NULL,        "null"          ),
    make_aw_clk_inf(AW_MOD_CLK_NAND0,       "mod_nand0"     ),
    make_aw_clk_inf(AW_MOD_CLK_NAND1,       "mod_nand1"     ),
    make_aw_clk_inf(AW_MOD_CLK_SDC0,        "mod_sdc0"      ),
    make_aw_clk_inf(AW_MOD_CLK_SDC1,        "mod_sdc1"      ),
    make_aw_clk_inf(AW_MOD_CLK_SDC2,        "mod_sdc2"      ),
    make_aw_clk_inf(AW_MOD_CLK_SDC3,        "mod_sdc3"      ),
    make_aw_clk_inf(AW_MOD_CLK_TS,          "mod_ts"        ),
    make_aw_clk_inf(AW_MOD_CLK_SS,          "mod_ss"        ),
    make_aw_clk_inf(AW_MOD_CLK_SPI0,        "mod_spi0"      ),
    make_aw_clk_inf(AW_MOD_CLK_SPI1,        "mod_spi1"      ),
    make_aw_clk_inf(AW_MOD_CLK_SPI2,        "mod_spi2"      ),
    make_aw_clk_inf(AW_MOD_CLK_SPI3,        "mod_spi3"      ),
    make_aw_clk_inf(AW_MOD_CLK_I2S0,        "mod_i2s0"      ),
    make_aw_clk_inf(AW_MOD_CLK_I2S1,        "mod_i2s1"      ),
    make_aw_clk_inf(AW_MOD_CLK_SPDIF,       "mod_spdif"     ),
    make_aw_clk_inf(AW_MOD_CLK_USBPHY0,     "mod_usbphy0"   ),
    make_aw_clk_inf(AW_MOD_CLK_USBPHY1,     "mod_usbphy1"   ),
    make_aw_clk_inf(AW_MOD_CLK_USBPHY2,     "mod_usbphy2"   ),
    make_aw_clk_inf(AW_MOD_CLK_USBEHCI0,    "mod_usbehci0"  ),
    make_aw_clk_inf(AW_MOD_CLK_USBEHCI1,    "mod_usbehci1"  ),
    make_aw_clk_inf(AW_MOD_CLK_USBOHCI0,    "mod_usbohci0"  ),
    make_aw_clk_inf(AW_MOD_CLK_USBOHCI1,    "mod_usbohci1"  ),
    make_aw_clk_inf(AW_MOD_CLK_USBOHCI2,    "mod_usbohci2"  ),
    make_aw_clk_inf(AW_MOD_CLK_USBOTG,      "mod_usbotg"    ),
    make_aw_clk_inf(AW_MOD_CLK_MDFS,        "mod_mdfs"      ),
    make_aw_clk_inf(AW_MOD_CLK_DEBE0,       "mod_debe0"     ),
    make_aw_clk_inf(AW_MOD_CLK_DEBE1,       "mod_debe1"     ),
    make_aw_clk_inf(AW_MOD_CLK_DEFE0,       "mod_defe0"     ),
    make_aw_clk_inf(AW_MOD_CLK_DEFE1,       "mod_defe1"     ),
    make_aw_clk_inf(AW_MOD_CLK_DEMIX,       "mod_demp"      ),
    make_aw_clk_inf(AW_MOD_CLK_LCD0CH0,     "mod_lcd0ch0"   ),
    make_aw_clk_inf(AW_MOD_CLK_LCD0CH1,     "mod_lcd0ch1"   ),
    make_aw_clk_inf(AW_MOD_CLK_LCD1CH0,     "mod_lcd1ch0"   ),
    make_aw_clk_inf(AW_MOD_CLK_LCD1CH1,     "mod_lcd1ch1"   ),
    make_aw_clk_inf(AW_MOD_CLK_CSI0S,       "mod_csi0s"     ),
    make_aw_clk_inf(AW_MOD_CLK_CSI0M,       "mod_csi0m"     ),
    make_aw_clk_inf(AW_MOD_CLK_CSI1S,       "mod_csi1s"     ),
    make_aw_clk_inf(AW_MOD_CLK_CSI1M,       "mod_csi1m"     ),
    make_aw_clk_inf(AW_MOD_CLK_VE,          "mod_ve"        ),
    make_aw_clk_inf(AW_MOD_CLK_ADDA,        "mod_adda"      ),
    make_aw_clk_inf(AW_MOD_CLK_AVS,         "mod_avs"       ),
    make_aw_clk_inf(AW_MOD_CLK_HDMI,        "mod_hdmi"      ),
    make_aw_clk_inf(AW_MOD_CLK_PS,          "mod_ps"        ),
    make_aw_clk_inf(AW_MOD_CLK_MTCACC,      "mod_mtcacc"    ),
    make_aw_clk_inf(AW_MOD_CLK_MBUS0,       "mod_mbus0"     ),
    make_aw_clk_inf(AW_MOD_CLK_MBUS1,       "mod_mbus1"     ),
    make_aw_clk_inf(AW_MOD_CLK_DRAM,        "mod_dram"      ),
    make_aw_clk_inf(AW_MOD_CLK_MIPIDSIS,    "mod_mipidsis"  ),
    make_aw_clk_inf(AW_MOD_CLK_MIPIDSIP,    "mod_mipidsip"  ),
    make_aw_clk_inf(AW_MOD_CLK_MIPICSIS,    "mod_mipicsis"  ),
    make_aw_clk_inf(AW_MOD_CLK_MIPICSIP,    "mod_mipicsip"  ),
    make_aw_clk_inf(AW_MOD_CLK_IEPDRC0,     "mod_iepdrc0"   ),
    make_aw_clk_inf(AW_MOD_CLK_IEPDRC1,     "mod_iepdrc1"   ),
    make_aw_clk_inf(AW_MOD_CLK_IEPDEU0,     "mod_iepdeu0"   ),
    make_aw_clk_inf(AW_MOD_CLK_IEPDEU1,     "mod_iepdeu1"   ),
    make_aw_clk_inf(AW_MOD_CLK_GPUCORE,     "mod_gpucore"   ),
    make_aw_clk_inf(AW_MOD_CLK_GPUMEM,      "mod_gpumem"    ),
    make_aw_clk_inf(AW_MOD_CLK_GPUHYD,      "mod_gpuhyd"    ),
    make_aw_clk_inf(AW_MOD_CLK_TWI0,        "mod_twi0"      ),
    make_aw_clk_inf(AW_MOD_CLK_TWI1,        "mod_twi1"      ),
    make_aw_clk_inf(AW_MOD_CLK_TWI2,        "mod_twi2"      ),
    make_aw_clk_inf(AW_MOD_CLK_TWI3,        "mod_twi3"      ),
    make_aw_clk_inf(AW_MOD_CLK_UART0,       "mod_uart0"     ),
    make_aw_clk_inf(AW_MOD_CLK_UART1,       "mod_uart1"     ),
    make_aw_clk_inf(AW_MOD_CLK_UART2,       "mod_uart2"     ),
    make_aw_clk_inf(AW_MOD_CLK_UART3,       "mod_uart3"     ),
    make_aw_clk_inf(AW_MOD_CLK_UART4,       "mod_uart4"     ),
    make_aw_clk_inf(AW_MOD_CLK_UART5,       "mod_uart5"     ),
    make_aw_clk_inf(AW_MOD_CLK_GMAC,        "mod_gmac"      ),
    make_aw_clk_inf(AW_MOD_CLK_DMA,         "mod_dma"       ),
    make_aw_clk_inf(AW_MOD_CLK_HSTMR,       "mod_hstmr"     ),
    make_aw_clk_inf(AW_MOD_CLK_MSGBOX,      "mod_msgbox"    ),
    make_aw_clk_inf(AW_MOD_CLK_SPINLOCK,    "mod_spinlock"  ),
    make_aw_clk_inf(AW_MOD_CLK_LVDS,        "mod_lvds"      ),
    make_aw_clk_inf(AW_MOD_CLK_SMPTWD,      "smp_twd"       ),
    make_aw_clk_inf(AW_AXI_CLK_DRAM,        "axi_dram"      ),
    make_aw_clk_inf(AW_AHB_CLK_MIPICSI,     "ahb_mipicsi"   ),
    make_aw_clk_inf(AW_AHB_CLK_MIPIDSI,     "ahb_mipidsi"   ),
    make_aw_clk_inf(AW_AHB_CLK_SS,          "ahb_ss"        ),
    make_aw_clk_inf(AW_AHB_CLK_DMA,         "ahb_dma"       ),
    make_aw_clk_inf(AW_AHB_CLK_SDMMC0,      "ahb_sdmmc0"    ),
    make_aw_clk_inf(AW_AHB_CLK_SDMMC1,      "ahb_sdmmc1"    ),
    make_aw_clk_inf(AW_AHB_CLK_SDMMC2,      "ahb_sdmmc2"    ),
    make_aw_clk_inf(AW_AHB_CLK_SDMMC3,      "ahb_sdmmc3"    ),
    make_aw_clk_inf(AW_AHB_CLK_NAND1,       "ahb_nand1"     ),
    make_aw_clk_inf(AW_AHB_CLK_NAND0,       "ahb_nand0"     ),
    make_aw_clk_inf(AW_AHB_CLK_SDRAM,       "ahb_sdram"     ),
    make_aw_clk_inf(AW_AHB_CLK_GMAC,        "ahb_gmac"      ),
    make_aw_clk_inf(AW_AHB_CLK_TS,          "ahb_ts"        ),
    make_aw_clk_inf(AW_AHB_CLK_HSTMR,       "ahb_hstmr"     ),
    make_aw_clk_inf(AW_AHB_CLK_SPI0,        "ahb_spi0"      ),
    make_aw_clk_inf(AW_AHB_CLK_SPI1,        "ahb_spi1"      ),
    make_aw_clk_inf(AW_AHB_CLK_SPI2,        "ahb_spi2"      ),
    make_aw_clk_inf(AW_AHB_CLK_SPI3,        "ahb_spi3"      ),
    make_aw_clk_inf(AW_AHB_CLK_OTG,         "ahb_otg"       ),
    make_aw_clk_inf(AW_AHB_CLK_EHCI0,       "ahb_ehci0"     ),
    make_aw_clk_inf(AW_AHB_CLK_EHCI1,       "ahb_ehci1"     ),
    make_aw_clk_inf(AW_AHB_CLK_OHCI0,       "ahb_ohci0"     ),
    make_aw_clk_inf(AW_AHB_CLK_OHCI1,       "ahb_ohci1"     ),
    make_aw_clk_inf(AW_AHB_CLK_OHCI2,       "ahb_ohci2"     ),
    make_aw_clk_inf(AW_AHB_CLK_VE,          "ahb_ve"        ),
    make_aw_clk_inf(AW_AHB_CLK_LCD0,        "ahb_lcd0"      ),
    make_aw_clk_inf(AW_AHB_CLK_LCD1,        "ahb_lcd1"      ),
    make_aw_clk_inf(AW_AHB_CLK_CSI0,        "ahb_csi0"      ),
    make_aw_clk_inf(AW_AHB_CLK_CSI1,        "ahb_csi1"      ),
    make_aw_clk_inf(AW_AHB_CLK_HDMID,       "ahb_hdmid"     ),
    make_aw_clk_inf(AW_AHB_CLK_DEBE0,       "ahb_debe0"     ),
    make_aw_clk_inf(AW_AHB_CLK_DEBE1,       "ahb_debe1"     ),
    make_aw_clk_inf(AW_AHB_CLK_DEFE0,       "ahb_defe0"     ),
    make_aw_clk_inf(AW_AHB_CLK_DEFE1,       "ahb_defe1"     ),
    make_aw_clk_inf(AW_AHB_CLK_MP,          "ahb_mp"        ),
    make_aw_clk_inf(AW_AHB_CLK_GPU,         "ahb_gpu"       ),
    make_aw_clk_inf(AW_AHB_CLK_MSGBOX,      "ahb_msgbox"    ),
    make_aw_clk_inf(AW_AHB_CLK_SPINLOCK,    "ahb_spinlock"  ),
    make_aw_clk_inf(AW_AHB_CLK_DEU0,        "ahb_deu0"      ),
    make_aw_clk_inf(AW_AHB_CLK_DEU1,        "ahb_deu1"      ),
    make_aw_clk_inf(AW_AHB_CLK_DRC0,        "ahb_drc0"      ),
    make_aw_clk_inf(AW_AHB_CLK_DRC1,        "ahb_drc1"      ),
    make_aw_clk_inf(AW_AHB_CLK_MTCACC,      "ahb_mtcacc"    ),
    make_aw_clk_inf(AW_APB_CLK_ADDA,        "apb_adda"      ),
    make_aw_clk_inf(AW_APB_CLK_SPDIF,       "apb_spdif"     ),
    make_aw_clk_inf(AW_APB_CLK_PIO,         "apb_pio"       ),
    make_aw_clk_inf(AW_APB_CLK_I2S0,        "apb_i2s0"      ),
    make_aw_clk_inf(AW_APB_CLK_I2S1,        "apb_i2s1"      ),
    make_aw_clk_inf(AW_APB_CLK_TWI0,        "apb_twi0"      ),
    make_aw_clk_inf(AW_APB_CLK_TWI1,        "apb_twi1"      ),
    make_aw_clk_inf(AW_APB_CLK_TWI2,        "apb_twi2"      ),
    make_aw_clk_inf(AW_APB_CLK_TWI3,        "apb_twi3"      ),
    make_aw_clk_inf(AW_APB_CLK_UART0,       "apb_uart0"     ),
    make_aw_clk_inf(AW_APB_CLK_UART1,       "apb_uart1"     ),
    make_aw_clk_inf(AW_APB_CLK_UART2,       "apb_uart2"     ),
    make_aw_clk_inf(AW_APB_CLK_UART3,       "apb_uart3"     ),
    make_aw_clk_inf(AW_APB_CLK_UART4,       "apb_uart4"     ),
    make_aw_clk_inf(AW_APB_CLK_UART5,       "apb_uart5"     ),
    make_aw_clk_inf(AW_DRAM_CLK_VE,         "dram_ve"       ),
    make_aw_clk_inf(AW_DRAM_CLK_CSI0,       "dram_csi0"     ),
    make_aw_clk_inf(AW_DRAM_CLK_CSI1,       "dram_csi1"     ),
    make_aw_clk_inf(AW_DRAM_CLK_TS,         "dram_ts"       ),
    make_aw_clk_inf(AW_DRAM_CLK_DRC0,       "dram_drc0"     ),
    make_aw_clk_inf(AW_DRAM_CLK_DRC1,       "dram_drc1"     ),
    make_aw_clk_inf(AW_DRAM_CLK_DEU0,       "dram_deu0"     ),
    make_aw_clk_inf(AW_DRAM_CLK_DEU1,       "dram_deu1"     ),
    make_aw_clk_inf(AW_DRAM_CLK_DEFE0,      "dram_defe0"    ),
    make_aw_clk_inf(AW_DRAM_CLK_DEFE1,      "dram_defe1"    ),
    make_aw_clk_inf(AW_DRAM_CLK_DEBE0,      "dram_debe0"    ),
    make_aw_clk_inf(AW_DRAM_CLK_DEBE1,      "dram_debe1"    ),
    make_aw_clk_inf(AW_DRAM_CLK_MP,         "dram_mp"       ),
    make_aw_clk_inf(AW_CCU_CLK_CNT,         "count"         ),
};


/*
*********************************************************************************************************
*                           aw_ccu_init
*
*Description: initialise clock mangement unit;
*
*Arguments  : none
*
*Return     : result,
*               AW_CCMU_OK,     initialise ccu successed;
*               AW_CCMU_FAIL,   initialise ccu failed;
*
*Notes      :
*
*********************************************************************************************************
*/
int aw_ccu_init(void)
{
    CCU_DBG("%s\n", __func__);

    /* initialise the CCU io base */
    aw_ccu_reg = (__ccmu_reg_list_t *)ioremap_nocache(AW_CCM_BASE, 0x400);
    aw_cpu0_reg = (__ccmu_reg_cpu0_list_t *)ioremap_nocache(AW_R_CPUCFG_BASE, 0x200);;
    return 0;
}


/*
*********************************************************************************************************
*                           aw_ccu_exit
*
*Description: exit clock managment unit;
*
*Arguments  : none
*
*Return     : result,
*               AW_CCMU_OK,     exit ccu successed;
*               AW_CCMU_FAIL,   exit ccu failed;
*
*Notes      :
*
*********************************************************************************************************
*/
int aw_ccu_exit(void)
{
    CCU_DBG("%s\n", __func__);
    aw_ccu_reg = NULL;
    aw_cpu0_reg = NULL;

    return 0;
}


/*
*********************************************************************************************************
*                           aw_ccu_get__clk
*
*Description: get clock information by clock id.
*
*Arguments  : id    clock id;
*
*Return     : clock handle, return NULL if get clock information failed;
*
*Notes      :
*
*********************************************************************************************************
*/
int aw_ccu_get_clk(__aw_ccu_clk_id_e id, __ccu_clk_t *clk)
{
    __aw_ccu_clk_t  *tmp_clk;

    if(clk && (id < AW_CCU_CLK_NULL)) {
        tmp_clk = &aw_ccu_clk_tbl[id];

        /* set clock operation handle */
        clk->ops = &sys_clk_ops;
        clk->aw_clk = tmp_clk;

        /* query system clock information from hardware */
        tmp_clk->parent = sys_clk_ops.get_parent(id);
        tmp_clk->onoff  = sys_clk_ops.get_status(id);
        tmp_clk->rate   = sys_clk_ops.get_rate(id);
        tmp_clk->hash   = ccu_clk_calc_hash(tmp_clk->name);
    }
    else if(clk && (id < AW_CCU_CLK_CNT)) {
        tmp_clk = &aw_ccu_clk_tbl[id];

        /* set clock operation handle */
        clk->ops = &mod_clk_ops;
        clk->aw_clk = tmp_clk;

        /* query system clock information from hardware */
        tmp_clk->parent = mod_clk_ops.get_parent(id);
        tmp_clk->onoff  = mod_clk_ops.get_status(id);
        tmp_clk->reset  = mod_clk_ops.get_reset(id);
        tmp_clk->rate   = mod_clk_ops.get_rate(id);
        tmp_clk->hash   = ccu_clk_calc_hash(tmp_clk->name);
    }
    else {
        CCU_ERR("clock id is invalid when get clk info!\n");
        return -1;
    }

    return 0;
}
