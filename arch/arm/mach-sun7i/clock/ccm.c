/*
 *  arch/arm/mach-sun7i/clock/ccm.c
 *
 * Copyright (c) 2012 Softwinner.
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


static __aw_ccu_clk_t aw_ccu_clk_tbl[] =
{
    make_aw_clk_inf(AW_SYS_CLK_NONE,        "sys_none"          ),
    make_aw_clk_inf(AW_SYS_CLK_LOSC,        "losc"              ),
    make_aw_clk_inf(AW_SYS_CLK_HOSC,        "hosc"              ),
    make_aw_clk_inf(AW_SYS_CLK_PLL1,        "core_pll"          ),
    make_aw_clk_inf(AW_SYS_CLK_PLL2,        "audio_pll"         ),
    make_aw_clk_inf(AW_SYS_CLK_PLL2X8,      "audio_pllx8"       ),
    make_aw_clk_inf(AW_SYS_CLK_PLL3,        "video_pll0"        ),
    make_aw_clk_inf(AW_SYS_CLK_PLL3X2,      "video_pll0x2"      ),
    make_aw_clk_inf(AW_SYS_CLK_PLL4,        "ve_pll"            ),
    make_aw_clk_inf(AW_SYS_CLK_PLL5,        "sdram_pll"         ),
    make_aw_clk_inf(AW_SYS_CLK_PLL5M,       "sdram_pll_m"       ),
    make_aw_clk_inf(AW_SYS_CLK_PLL5P,       "sdram_pll_p"       ),
    make_aw_clk_inf(AW_SYS_CLK_PLL6,        "sata_pll"          ),
    make_aw_clk_inf(AW_SYS_CLK_PLL6M,       "sata_pll_m"        ),
    make_aw_clk_inf(AW_SYS_CLK_PLL62,       "sata_pll_2"        ),
    make_aw_clk_inf(AW_SYS_CLK_PLL7,        "video_pll1"        ),
    make_aw_clk_inf(AW_SYS_CLK_PLL7X2,      "video_pll1x2"      ),
    make_aw_clk_inf(AW_SYS_CLK_PLL8,        "gpu_pll"           ),
    make_aw_clk_inf(AW_SYS_CLK_CPU,         "cpu"               ),
    make_aw_clk_inf(AW_SYS_CLK_AXI,         "axi"               ),
    make_aw_clk_inf(AW_SYS_CLK_ATB,         "atb"               ),
    make_aw_clk_inf(AW_SYS_CLK_AHB,         "ahb"               ),
    make_aw_clk_inf(AW_SYS_CLK_APB0,        "apb"               ),
    make_aw_clk_inf(AW_SYS_CLK_APB1,        "apb1"              ),
    make_aw_clk_inf(AW_CCU_CLK_NULL,        "mod_none"          ),
    make_aw_clk_inf(AW_MOD_CLK_NFC,         "nfc"               ),
    make_aw_clk_inf(AW_MOD_CLK_MSC,         "msc"               ),
    make_aw_clk_inf(AW_MOD_CLK_SDC0,        "sdc0"              ),
    make_aw_clk_inf(AW_MOD_CLK_SDC1,        "sdc1"              ),
    make_aw_clk_inf(AW_MOD_CLK_SDC2,        "sdc2"              ),
    make_aw_clk_inf(AW_MOD_CLK_SDC3,        "sdc3"              ),
    make_aw_clk_inf(AW_MOD_CLK_TS,          "ts"                ),
    make_aw_clk_inf(AW_MOD_CLK_SS,          "ss"                ),
    make_aw_clk_inf(AW_MOD_CLK_SPI0,        "spi0"              ),
    make_aw_clk_inf(AW_MOD_CLK_SPI1,        "spi1"              ),
    make_aw_clk_inf(AW_MOD_CLK_SPI2,        "spi2"              ),
    make_aw_clk_inf(AW_MOD_CLK_PATA,        "pata"              ),
    make_aw_clk_inf(AW_MOD_CLK_IR0,         "ir0"               ),
    make_aw_clk_inf(AW_MOD_CLK_IR1,         "ir1"               ),
    make_aw_clk_inf(AW_MOD_CLK_I2S,         "i2s"               ),
    make_aw_clk_inf(AW_MOD_CLK_AC97,        "ac97"              ),
    make_aw_clk_inf(AW_MOD_CLK_SPDIF,       "spdif"             ),
    make_aw_clk_inf(AW_MOD_CLK_KEYPAD,      "key_pad"           ),
    make_aw_clk_inf(AW_MOD_CLK_SATA,        "sata"              ),
    make_aw_clk_inf(AW_MOD_CLK_USBPHY,      "usb_phy"           ),
    make_aw_clk_inf(AW_MOD_CLK_USBPHY0,     "usb_phy0"          ),
    make_aw_clk_inf(AW_MOD_CLK_USBPHY1,     "usb_phy1"          ),
    make_aw_clk_inf(AW_MOD_CLK_USBPHY2,     "usb_phy2"          ),
    make_aw_clk_inf(AW_MOD_CLK_USBOHCI0,    "usb_ohci0"         ),
    make_aw_clk_inf(AW_MOD_CLK_USBOHCI1,    "usb_ohci1"         ),
    make_aw_clk_inf(AW_MOD_CLK_GPS,         "com"               ),
    make_aw_clk_inf(AW_MOD_CLK_SPI3,        "spi3"              ),
    make_aw_clk_inf(AW_MOD_CLK_DEBE0,       "de_image0"         ),
    make_aw_clk_inf(AW_MOD_CLK_DEBE1,       "de_image1"         ),
    make_aw_clk_inf(AW_MOD_CLK_DEFE0,       "de_scale0"         ),
    make_aw_clk_inf(AW_MOD_CLK_DEFE1,       "de_scale1"         ),
    make_aw_clk_inf(AW_MOD_CLK_DEMIX,       "de_mix"            ),
    make_aw_clk_inf(AW_MOD_CLK_LCD0CH0,     "lcd0_ch0"          ),
    make_aw_clk_inf(AW_MOD_CLK_LCD1CH0,     "lcd1_ch0"          ),
    make_aw_clk_inf(AW_MOD_CLK_CSIISP,      "csi_isp"           ),
    make_aw_clk_inf(AW_MOD_CLK_TVDMOD1,     "tvdmod1"           ),
    make_aw_clk_inf(AW_MOD_CLK_TVDMOD2,     "tvdmod2"           ),
    make_aw_clk_inf(AW_MOD_CLK_LCD0CH1_S1,  "lcd0_ch1_s1"       ),
    make_aw_clk_inf(AW_MOD_CLK_LCD0CH1_S2,  "lcd0_ch1_s2"       ),
    make_aw_clk_inf(AW_MOD_CLK_LCD1CH1_S1,  "lcd1_ch1_s1"       ),
    make_aw_clk_inf(AW_MOD_CLK_LCD1CH1_S2,  "lcd1_ch1_s2"       ),
    make_aw_clk_inf(AW_MOD_CLK_CSI0,        "csi0"              ),
    make_aw_clk_inf(AW_MOD_CLK_CSI1,        "csi1"              ),
    make_aw_clk_inf(AW_MOD_CLK_VE,          "ve"                ),
    make_aw_clk_inf(AW_MOD_CLK_ADDA,        "audio_codec"       ),
    make_aw_clk_inf(AW_MOD_CLK_AVS,         "avs"               ),
    make_aw_clk_inf(AW_MOD_CLK_ACE,         "ace"               ),
    make_aw_clk_inf(AW_MOD_CLK_LVDS,        "lvds"              ),
    make_aw_clk_inf(AW_MOD_CLK_HDMI,        "hdmi"              ),
    make_aw_clk_inf(AW_MOD_CLK_MALI,        "mali"              ),
    make_aw_clk_inf(AW_MOD_CLK_TWI0,        "twi0"              ),
    make_aw_clk_inf(AW_MOD_CLK_TWI1,        "twi1"              ),
    make_aw_clk_inf(AW_MOD_CLK_TWI2,        "twi2"              ),
    make_aw_clk_inf(AW_MOD_CLK_CAN,         "can"               ),
    make_aw_clk_inf(AW_MOD_CLK_SCR,         "scr"               ),
    make_aw_clk_inf(AW_MOD_CLK_PS20,        "ps0"               ),
    make_aw_clk_inf(AW_MOD_CLK_PS21,        "ps1"               ),
    make_aw_clk_inf(AW_MOD_CLK_UART0,       "uart0"             ),
    make_aw_clk_inf(AW_MOD_CLK_UART1,       "uart1"             ),
    make_aw_clk_inf(AW_MOD_CLK_UART2,       "uart2"             ),
    make_aw_clk_inf(AW_MOD_CLK_UART3,       "uart3"             ),
    make_aw_clk_inf(AW_MOD_CLK_UART4,       "uart4"             ),
    make_aw_clk_inf(AW_MOD_CLK_UART5,       "uart5"             ),
    make_aw_clk_inf(AW_MOD_CLK_UART6,       "uart6"             ),
    make_aw_clk_inf(AW_MOD_CLK_UART7,       "uart7"             ),
    make_aw_clk_inf(AW_MOD_CLK_SMPTWD,      "smp_twd"           ),
    make_aw_clk_inf(AW_MOD_CLK_MBUS,        "mbus"              ),
    make_aw_clk_inf(AW_MOD_CLK_OUTA,        "clkout_a"          ),
    make_aw_clk_inf(AW_MOD_CLK_OUTB,        "clkout_b"          ),
    make_aw_clk_inf(AW_MOD_CLK_AHB_USB0,    "ahb_usb0"          ),
    make_aw_clk_inf(AW_MOD_CLK_AHB_EHCI0,   "ahb_ehci0"         ),
    make_aw_clk_inf(AW_MOD_CLK_AHB_OHCI0,   "ahb_ohci0"         ),
    make_aw_clk_inf(AW_MOD_CLK_AHB_SS,      "ahb_ss"            ),
    make_aw_clk_inf(AW_MOD_CLK_AHB_DMA,     "ahb_dma"           ),
    make_aw_clk_inf(AW_MOD_CLK_AHB_BIST,    "ahb_bist"          ),
    make_aw_clk_inf(AW_MOD_CLK_AHB_SDMMC0,  "ahb_sdc0"          ),
    make_aw_clk_inf(AW_MOD_CLK_AHB_SDMMC1,  "ahb_sdc1"          ),
    make_aw_clk_inf(AW_MOD_CLK_AHB_SDMMC2,  "ahb_sdc2"          ),
    make_aw_clk_inf(AW_MOD_CLK_AHB_SDMMC3,  "ahb_sdc3"          ),
    make_aw_clk_inf(AW_MOD_CLK_AHB_MS,      "ahb_msc"           ),
    make_aw_clk_inf(AW_MOD_CLK_AHB_NAND,    "ahb_nfc"           ),
    make_aw_clk_inf(AW_MOD_CLK_AHB_SDRAM,   "ahb_sdramc"        ),
    make_aw_clk_inf(AW_MOD_CLK_AHB_ACE,     "ahb_ace"           ),
    make_aw_clk_inf(AW_MOD_CLK_AHB_EMAC,    "ahb_emac"          ),
    make_aw_clk_inf(AW_MOD_CLK_AHB_TS,      "ahb_ts"            ),
    make_aw_clk_inf(AW_MOD_CLK_AHB_SPI0,    "ahb_spi0"          ),
    make_aw_clk_inf(AW_MOD_CLK_AHB_SPI1,    "ahb_spi1"          ),
    make_aw_clk_inf(AW_MOD_CLK_AHB_SPI2,    "ahb_spi2"          ),
    make_aw_clk_inf(AW_MOD_CLK_AHB_SPI3,    "ahb_spi3"          ),
    make_aw_clk_inf(AW_MOD_CLK_AHB_PATA,    "ahb_pata"          ),
    make_aw_clk_inf(AW_MOD_CLK_AHB_SATA,    "ahb_sata"          ),
    make_aw_clk_inf(AW_MOD_CLK_AHB_GPS,     "ahb_com"           ),
    make_aw_clk_inf(AW_MOD_CLK_AHB_VE,      "ahb_ve"            ),
    make_aw_clk_inf(AW_MOD_CLK_AHB_TVD,     "ahb_tvd"           ),
    make_aw_clk_inf(AW_MOD_CLK_AHB_TVE0,    "ahb_tve0"          ),
    make_aw_clk_inf(AW_MOD_CLK_AHB_TVE1,    "ahb_tve1"          ),
    make_aw_clk_inf(AW_MOD_CLK_AHB_LCD0,    "ahb_lcd0"          ),
    make_aw_clk_inf(AW_MOD_CLK_AHB_LCD1,    "ahb_lcd1"          ),
    make_aw_clk_inf(AW_MOD_CLK_AHB_CSI0,    "ahb_csi0"          ),
    make_aw_clk_inf(AW_MOD_CLK_AHB_CSI1,    "ahb_csi1"          ),
    make_aw_clk_inf(AW_MOD_CLK_AHB_HDMI,    "ahb_hdmi"          ),
    make_aw_clk_inf(AW_MOD_CLK_AHB_DEBE0,   "ahb_de_image0"     ),
    make_aw_clk_inf(AW_MOD_CLK_AHB_DEBE1,   "ahb_de_image1"     ),
    make_aw_clk_inf(AW_MOD_CLK_AHB_DEFE0,   "ahb_de_scale0"     ),
    make_aw_clk_inf(AW_MOD_CLK_AHB_DEFE1,   "ahb_de_scale1"     ),
    make_aw_clk_inf(AW_MOD_CLK_AHB_MP,      "ahb_de_mix"        ),
    make_aw_clk_inf(AW_MOD_CLK_AHB_MALI,    "ahb_mali"          ),
    make_aw_clk_inf(AW_MOD_CLK_AHB_EHCI1,   "ahb_ehci1"         ),
    make_aw_clk_inf(AW_MOD_CLK_AHB_OHCI1,   "ahb_ohci1"         ),
    make_aw_clk_inf(AW_MOD_CLK_AHB_STMR,    "ahb_stmr"          ),
    make_aw_clk_inf(AW_MOD_CLK_APB_ADDA,    "apb_audio_codec"   ),
    make_aw_clk_inf(AW_MOD_CLK_APB_SPDIF,   "apb_spdif"         ),
    make_aw_clk_inf(AW_MOD_CLK_APB_AC97,    "apb_ac97"          ),
    make_aw_clk_inf(AW_MOD_CLK_APB_I2S,     "apb_i2s"           ),
    make_aw_clk_inf(AW_MOD_CLK_APB_PIO,     "apb_pio"           ),
    make_aw_clk_inf(AW_MOD_CLK_APB_IR0,     "apb_ir0"           ),
    make_aw_clk_inf(AW_MOD_CLK_APB_IR1,     "apb_ir1"           ),
    make_aw_clk_inf(AW_MOD_CLK_APB_KEYPAD,  "apb_key_pad"       ),
    make_aw_clk_inf(AW_MOD_CLK_APB_TWI0,    "apb_twi0"          ),
    make_aw_clk_inf(AW_MOD_CLK_APB_TWI1,    "apb_twi1"          ),
    make_aw_clk_inf(AW_MOD_CLK_APB_TWI2,    "apb_twi2"          ),
    make_aw_clk_inf(AW_MOD_CLK_APB_CAN,     "apb_can"           ),
    make_aw_clk_inf(AW_MOD_CLK_APB_SCR,     "apb_scr"           ),
    make_aw_clk_inf(AW_MOD_CLK_APB_PS20,    "apb_ps0"           ),
    make_aw_clk_inf(AW_MOD_CLK_APB_PS21,    "apb_ps1"           ),
    make_aw_clk_inf(AW_MOD_CLK_APB_UART0,   "apb_uart0"         ),
    make_aw_clk_inf(AW_MOD_CLK_APB_UART1,   "apb_uart1"         ),
    make_aw_clk_inf(AW_MOD_CLK_APB_UART2,   "apb_uart2"         ),
    make_aw_clk_inf(AW_MOD_CLK_APB_UART3,   "apb_uart3"         ),
    make_aw_clk_inf(AW_MOD_CLK_APB_UART4,   "apb_uart4"         ),
    make_aw_clk_inf(AW_MOD_CLK_APB_UART5,   "apb_uart5"         ),
    make_aw_clk_inf(AW_MOD_CLK_APB_UART6,   "apb_uart6"         ),
    make_aw_clk_inf(AW_MOD_CLK_APB_UART7,   "apb_uart7"         ),
    make_aw_clk_inf(AW_MOD_CLK_SDRAM_VE,    "sdram_ve"          ),
    make_aw_clk_inf(AW_MOD_CLK_SDRAM_CSI0,  "sdram_csi0"        ),
    make_aw_clk_inf(AW_MOD_CLK_SDRAM_CSI1,  "sdram_csi1"        ),
    make_aw_clk_inf(AW_MOD_CLK_SDRAM_TS,    "sdram_ts"          ),
    make_aw_clk_inf(AW_MOD_CLK_SDRAM_TVD,   "sdram_tvd"         ),
    make_aw_clk_inf(AW_MOD_CLK_SDRAM_TVE0,  "sdram_tve0"        ),
    make_aw_clk_inf(AW_MOD_CLK_SDRAM_TVE1,  "sdram_tve1"        ),
    make_aw_clk_inf(AW_MOD_CLK_SDRAM_DEFE0, "sdram_de_scale0"   ),
    make_aw_clk_inf(AW_MOD_CLK_SDRAM_DEFE1, "sdram_de_scale1"   ),
    make_aw_clk_inf(AW_MOD_CLK_SDRAM_DEBE0, "sdram_de_image0"   ),
    make_aw_clk_inf(AW_MOD_CLK_SDRAM_DEBE1, "sdram_de_image1"   ),
    make_aw_clk_inf(AW_MOD_CLK_SDRAM_DEMP,  "sdram_de_mix"      ),
    make_aw_clk_inf(AW_MOD_CLK_SDRAM_ACE,   "sdram_ace"         ),
    make_aw_clk_inf(AW_CCU_CLK_CNT,         "none"              ),
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
    aw_ccu_reg = (__ccmu_reg_list_t *)SW_VA_CCM_IO_BASE;

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
