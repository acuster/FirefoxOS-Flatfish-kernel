/*
 *  arch/arm/mach-aw163x/clock/ccu_dbg.c
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

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/errno.h>
#include <linux/err.h>
#include <linux/string.h>
#include <linux/clk.h>
#include <linux/mutex.h>
#include <linux/platform_device.h>
#include <linux/cpufreq.h>
#include <linux/debugfs.h>
#include <linux/io.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include "ccm_i.h"


#define print_clk_inf(x, y)     do{printk(#x"."#y":%d\n", aw_ccu_reg->x.y);}while(0)

static void print_module0_clock(char *name, volatile __ccmu_module0_clk_t *reg)
{
    printk("\n%s clk infor:\n", name);
    printk("%s.DivM:%d\n", name, reg->DivM);
    printk("%s.OutClkCtrl:%d\n", name, reg->OutClkCtrl);
    printk("%s.DivN:%d\n", name, reg->DivN);
    printk("%s.SampClkCtrl:%d\n", name, reg->SampClkCtrl);
    printk("%s.ClkSrc:%d\n", name, reg->ClkSrc);
    printk("%s.ClkGate:%d\n", name, reg->ClkGate);
}

static void print_module1_clock(char *name, volatile __ccmu_module1_clk_t *reg)
{
    printk("\n%s clk infor:\n", name);
    printk("%s.ClkSrc:%d\n", name, reg->ClkSrc);
    printk("%s.ClkGate:%d\n", name, reg->ClkGate);
}


static void print_disp_clock(char *name, volatile __ccmu_disp_clk_t *reg)
{
    printk("\n%s clk infor:\n", name);
    printk("%s.DivM:%d\n", name, reg->DivM);
    printk("%s.ClkSrc:%d\n", name, reg->ClkSrc);
    printk("%s.ClkGate:%d\n", name, reg->ClkGate);
}


static void print_mediapll_clock(char *name, volatile __ccmu_media_pll_t *reg)
{
    printk("\n%s clk infor:\n", name);
    printk("%s.FactorM:%d\n", name, reg->FactorM);
    printk("%s.FactorN:%d\n", name, reg->FactorN);
    printk("%s.SdmEn:%d\n", name, reg->SdmEn);
    printk("%s.ModeSel:%d\n", name, reg->ModeSel);
    printk("%s.FracMod:%d\n", name, reg->FracMod);
    printk("%s.Lock:%d\n", name, reg->Lock);
    printk("%s.CtlMode:%d\n", name, reg->CtlMode);
    printk("%s.PLLEn:%d\n", name, reg->PLLEn);
}


void clk_dbg_inf(void)
{
    printk("---------------------------------------------\n");
    printk("    dump clock information                   \n");
    printk("---------------------------------------------\n");

    printk("\nPLL1 clk infor:\n");
    print_clk_inf(Pll1Ctl, FactorM  );
    print_clk_inf(Pll1Ctl, FactorK  );
    print_clk_inf(Pll1Ctl, FactorN  );
    print_clk_inf(Pll1Ctl, SigmaEn  );
    print_clk_inf(Pll1Ctl, Lock     );
    print_clk_inf(Pll1Ctl, PLLEn    );

    printk("\nPLL2 clk infor:\n");
    print_clk_inf(Pll2Ctl, FactorM  );
    print_clk_inf(Pll2Ctl, FactorN  );
    print_clk_inf(Pll2Ctl, FactorP  );
    print_clk_inf(Pll2Ctl, SdmEn    );
    print_clk_inf(Pll2Ctl, Lock     );
    print_clk_inf(Pll2Ctl, PLLEn    );

    printk("\nPLL3 clk infor:\n");
    print_clk_inf(Pll3Ctl, FactorM  );
    print_clk_inf(Pll3Ctl, FactorN  );
    print_clk_inf(Pll3Ctl, SdmEn    );
    print_clk_inf(Pll3Ctl, ModeSel  );
    print_clk_inf(Pll3Ctl, FracMod  );
    print_clk_inf(Pll3Ctl, Lock     );
    print_clk_inf(Pll3Ctl, CtlMode  );
    print_clk_inf(Pll3Ctl, PLLEn    );

    print_mediapll_clock("Pll4Ctl", &aw_ccu_reg->Pll4Ctl);

    printk("\nPll5Ctl clk infor:\n");
    print_clk_inf(Pll5Ctl, FactorM      );
    print_clk_inf(Pll5Ctl, FactorK      );
    print_clk_inf(Pll5Ctl, FactorN      );
    print_clk_inf(Pll5Ctl, PLLCfgUpdate );
    print_clk_inf(Pll5Ctl, SigmaDeltaEn );
    print_clk_inf(Pll5Ctl, Lock         );
    print_clk_inf(Pll5Ctl, PLLEn        );

    printk("\nPll6Ctl clk infor:\n");
    print_clk_inf(Pll6Ctl, FactorM      );
    print_clk_inf(Pll6Ctl, FactorK      );
    print_clk_inf(Pll6Ctl, FactorN      );
    print_clk_inf(Pll6Ctl, Pll24MPdiv   );
    print_clk_inf(Pll6Ctl, Pll24MOutEn  );
    print_clk_inf(Pll6Ctl, PllClkOutEn  );
    print_clk_inf(Pll6Ctl, PLLBypass    );
    print_clk_inf(Pll6Ctl, Lock         );
    print_clk_inf(Pll6Ctl, PLLEn        );

    print_mediapll_clock("Pll7Ctl", &aw_ccu_reg->Pll7Ctl);
    print_mediapll_clock("Pll8Ctl", &aw_ccu_reg->Pll8Ctl);

    printk("\nMipiPllCtl clk infor:\n");
    print_clk_inf(MipiPllCtl, FactorM   );
    print_clk_inf(MipiPllCtl, FactorK   );
    print_clk_inf(MipiPllCtl, FactorN   );
    print_clk_inf(MipiPllCtl, VfbSel    );
    print_clk_inf(MipiPllCtl, FeedBackDiv   );
    print_clk_inf(MipiPllCtl, SdmEn     );
    print_clk_inf(MipiPllCtl, PllSrc    );
    print_clk_inf(MipiPllCtl, Ldo2En    );
    print_clk_inf(MipiPllCtl, Ldo1En    );
    print_clk_inf(MipiPllCtl, Sel625Or750   );
    print_clk_inf(MipiPllCtl, SDiv2     );
    print_clk_inf(MipiPllCtl, FracMode  );
    print_clk_inf(MipiPllCtl, Lock      );
    print_clk_inf(MipiPllCtl, PLLEn     );

    print_mediapll_clock("Pll9Ctl", &aw_ccu_reg->Pll9Ctl);
    print_mediapll_clock("Pll10Ctl", &aw_ccu_reg->Pll10Ctl);

    printk("\nSysClkDiv clk infor:\n");
    print_clk_inf(SysClkDiv, AXIClkDiv   );
    print_clk_inf(SysClkDiv, CpuClkSrc   );

    printk("\nAhb1Div clk infor:\n");
    print_clk_inf(Ahb1Div, Ahb1Div      );
    print_clk_inf(Ahb1Div, Ahb1PreDiv   );
    print_clk_inf(Ahb1Div, Apb1Div      );
    print_clk_inf(Ahb1Div, Ahb1ClkSrc   );

    printk("\nApb2Div clk infor:\n");
    print_clk_inf(Apb2Div, DivM      );
    print_clk_inf(Apb2Div, DivN      );
    print_clk_inf(Apb2Div, ClkSrc      );

    printk("\nAxiGate clk infor:\n");
    print_clk_inf(AxiGate, Sdram      );

    printk("\nAhbGate0 clk infor:\n");
    print_clk_inf(AhbGate0, MipiCsi      );
    print_clk_inf(AhbGate0, MipiDsi );
    print_clk_inf(AhbGate0, Ss      );
    print_clk_inf(AhbGate0, Dma     );
    print_clk_inf(AhbGate0, Sd0     );
    print_clk_inf(AhbGate0, Sd1     );
    print_clk_inf(AhbGate0, Sd2     );
    print_clk_inf(AhbGate0, Sd3     );
    print_clk_inf(AhbGate0, Nand1   );
    print_clk_inf(AhbGate0, Nand0   );
    print_clk_inf(AhbGate0, Dram    );
    print_clk_inf(AhbGate0, Gmac    );
    print_clk_inf(AhbGate0, Ts      );
    print_clk_inf(AhbGate0, HsTmr   );
    print_clk_inf(AhbGate0, Spi0    );
    print_clk_inf(AhbGate0, Spi1    );
    print_clk_inf(AhbGate0, Spi2    );
    print_clk_inf(AhbGate0, Spi3    );
    print_clk_inf(AhbGate0, Otg     );
    print_clk_inf(AhbGate0, Ehci0   );
    print_clk_inf(AhbGate0, Ehci1   );
    print_clk_inf(AhbGate0, Ohci0   );
    print_clk_inf(AhbGate0, Ohci1   );
    print_clk_inf(AhbGate0, Ohci2   );

    printk("\nAhbGate1 clk infor:\n");
    print_clk_inf(AhbGate1, Ve      );
    print_clk_inf(AhbGate1, Lcd0);
    print_clk_inf(AhbGate1, Lcd1);
    print_clk_inf(AhbGate1, Csi0);
    print_clk_inf(AhbGate1, Csi1);
    print_clk_inf(AhbGate1, Hdmi);
    print_clk_inf(AhbGate1, Be0);
    print_clk_inf(AhbGate1, Be1);
    print_clk_inf(AhbGate1, Fe0);
    print_clk_inf(AhbGate1, Fe1);
    print_clk_inf(AhbGate1, Mp);
    print_clk_inf(AhbGate1, Gpu);
    print_clk_inf(AhbGate1, MsgBox);
    print_clk_inf(AhbGate1, SpinLock);
    print_clk_inf(AhbGate1, Deu0);
    print_clk_inf(AhbGate1, Deu1);
    print_clk_inf(AhbGate1, Drc0);
    print_clk_inf(AhbGate1, Drc1);
    print_clk_inf(AhbGate1, MtcAcc);

    printk("\nApb1Gate clk infor:\n");
    print_clk_inf(Apb1Gate, Adda      );
    print_clk_inf(Apb1Gate, Spdif);
    print_clk_inf(Apb1Gate, Pio);
    print_clk_inf(Apb1Gate, I2s0);
    print_clk_inf(Apb1Gate, I2s1);

    printk("\nApb2Gate clk infor:\n");
    print_clk_inf(Apb2Gate, Twi0      );
    print_clk_inf(Apb2Gate, Twi1);
    print_clk_inf(Apb2Gate, Twi2);
    print_clk_inf(Apb2Gate, Twi3);
    print_clk_inf(Apb2Gate, Uart0);
    print_clk_inf(Apb2Gate, Uart1);
    print_clk_inf(Apb2Gate, Uart2);
    print_clk_inf(Apb2Gate, Uart3);
    print_clk_inf(Apb2Gate, Uart4);
    print_clk_inf(Apb2Gate, Uart5);

    print_module0_clock("Nand0", &aw_ccu_reg->Nand0);
    print_module0_clock("Nand1", &aw_ccu_reg->Nand1);
    print_module0_clock("Sd0", &aw_ccu_reg->Sd0);
    print_module0_clock("Sd1", &aw_ccu_reg->Sd1);
    print_module0_clock("Sd2", &aw_ccu_reg->Sd2);
    print_module0_clock("Sd3", &aw_ccu_reg->Sd3);
    print_module0_clock("Sd1", &aw_ccu_reg->Ts);
    print_module0_clock("Sd1", &aw_ccu_reg->Ss);
    print_module0_clock("Spi0", &aw_ccu_reg->Spi0);
    print_module0_clock("Spi1", &aw_ccu_reg->Spi1);
    print_module0_clock("Spi2", &aw_ccu_reg->Spi2);
    print_module0_clock("Spi3", &aw_ccu_reg->Spi3);

    print_module1_clock("I2s0", &aw_ccu_reg->I2s0);
    print_module1_clock("I2s1", &aw_ccu_reg->I2s1);
    print_module1_clock("Spdif", &aw_ccu_reg->Spdif);

    printk("\nUsb clk infor:\n");
    print_clk_inf(Usb, UsbPhy0Rst      );
    print_clk_inf(Usb, UsbPhy1Rst);
    print_clk_inf(Usb, UsbPhy2Rst);
    print_clk_inf(Usb, Phy0Gate);
    print_clk_inf(Usb, Phy1Gate);
    print_clk_inf(Usb, Phy2Gate);
    print_clk_inf(Usb, Ohci0Gate);
    print_clk_inf(Usb, Ohci1Gate);
    print_clk_inf(Usb, Ohci2Gate);



    print_module0_clock("Mdfs", &aw_ccu_reg->Mdfs);

    printk("\nDramCfg clk infor:\n");
    print_clk_inf(DramCfg, Div1M      );
    print_clk_inf(DramCfg, ClkSrc1);
    print_clk_inf(DramCfg, Div0M);
    print_clk_inf(DramCfg, ClkSrc0);
    print_clk_inf(DramCfg, SdrClkUpd);
    print_clk_inf(DramCfg, CtrlerRst);


    printk("\nDramGate clk infor:\n");
    print_clk_inf(DramGate, Ve      );
    print_clk_inf(DramGate, Csi0);
    print_clk_inf(DramGate, Csi1);
    print_clk_inf(DramGate, Ts);
    print_clk_inf(DramGate, Drc0);
    print_clk_inf(DramGate, Drc1);
    print_clk_inf(DramGate, Deu0);
    print_clk_inf(DramGate, Deu1);
    print_clk_inf(DramGate, Fe0);
    print_clk_inf(DramGate, Fe1);
    print_clk_inf(DramGate, Be0);
    print_clk_inf(DramGate, Be1);
    print_clk_inf(DramGate, Mp);

    print_disp_clock("Be0", &aw_ccu_reg->Be0);
    print_disp_clock("Be1", &aw_ccu_reg->Be1);
    print_disp_clock("Fe0", &aw_ccu_reg->Fe0);
    print_disp_clock("Fe1", &aw_ccu_reg->Fe1);
    print_disp_clock("Mp", &aw_ccu_reg->Mp);
    print_disp_clock("Lcd0Ch0", &aw_ccu_reg->Lcd0Ch0);
    print_disp_clock("Lcd0Ch1", &aw_ccu_reg->Lcd0Ch1);
    print_disp_clock("Lcd1Ch0", &aw_ccu_reg->Lcd1Ch0);
    print_disp_clock("Lcd1Ch1", &aw_ccu_reg->Lcd1Ch1);

    printk("\nCsi0 clk infor:\n");
    print_clk_inf(Csi0, MClkDiv      );
    print_clk_inf(Csi0, MClkSrc);
    print_clk_inf(Csi0, MClkGate);
    print_clk_inf(Csi0, SClkDiv);
    print_clk_inf(Csi0, SClkSrc);
    print_clk_inf(Csi0, SClkGate);


    printk("\nCsi1 clk infor:\n");
    print_clk_inf(Csi1, MClkDiv      );
    print_clk_inf(Csi1, MClkSrc);
    print_clk_inf(Csi1, MClkGate);
    print_clk_inf(Csi1, SClkDiv);
    print_clk_inf(Csi1, SClkSrc);
    print_clk_inf(Csi1, SClkGate);


    printk("\nVe clk infor:\n");
    print_clk_inf(Ve, ClkDiv      );
    print_clk_inf(Ve, ClkGate      );

    print_module1_clock("Adda", &aw_ccu_reg->Adda);
    print_module1_clock("Avs", &aw_ccu_reg->Avs);

    printk("\nHdmi clk infor:\n");
    print_clk_inf(Hdmi, ClkDiv);
    print_clk_inf(Hdmi, ClkSrc);
    print_clk_inf(Hdmi, DDCGate);
    print_clk_inf(Hdmi, ClkGate);

    print_module1_clock("Ps", &aw_ccu_reg->Ps);
    print_module0_clock("MtcAcc", &aw_ccu_reg->MtcAcc);
    print_module0_clock("MBus0", &aw_ccu_reg->MBus0);
    print_module0_clock("MBus1", &aw_ccu_reg->MBus1);

    printk("\nMipiDsi clk infor:\n");
    print_clk_inf(MipiDsi, PClkDiv);
    print_clk_inf(MipiDsi, PClkSrc);
    print_clk_inf(MipiDsi, PClkGate);
    print_clk_inf(MipiDsi, SClkDiv);
    print_clk_inf(MipiDsi, SClkSrc);
    print_clk_inf(MipiDsi, SClkGate);

    printk("\nMipiCsi clk infor:\n");
    print_clk_inf(MipiCsi, PClkDiv);
    print_clk_inf(MipiCsi, PClkSrc);
    print_clk_inf(MipiCsi, PClkGate);
    print_clk_inf(MipiCsi, SClkDiv);
    print_clk_inf(MipiCsi, SClkSrc);
    print_clk_inf(MipiCsi, SClkGate);

    print_module0_clock("IepDrc0", &aw_ccu_reg->IepDrc0);
    print_module0_clock("IepDrc1", &aw_ccu_reg->IepDrc1);
    print_module0_clock("IepDeu0", &aw_ccu_reg->IepDeu0);
    print_module0_clock("IepDeu1", &aw_ccu_reg->IepDeu1);
    print_module0_clock("GpuCore", &aw_ccu_reg->GpuCore);
    print_module0_clock("GpuMem", &aw_ccu_reg->GpuMem);
    print_module0_clock("GpuHyd", &aw_ccu_reg->GpuHyd);

    printk("\nPllLock clk infor:\n");
    print_clk_inf(PllLock, LockTime);

    printk("\nAhbReset0 clk infor:\n");
    print_clk_inf(AhbReset0, MipiCsi);
    print_clk_inf(AhbReset0, MipiDsi);
    print_clk_inf(AhbReset0, Ss);
    print_clk_inf(AhbReset0, Dma);
    print_clk_inf(AhbReset0, Sd0);
    print_clk_inf(AhbReset0, Sd1);
    print_clk_inf(AhbReset0, Sd2);
    print_clk_inf(AhbReset0, Sd3);
    print_clk_inf(AhbReset0, Nand1);
    print_clk_inf(AhbReset0, Nand0);
    print_clk_inf(AhbReset0, Sdram);
    print_clk_inf(AhbReset0, Gmac);
    print_clk_inf(AhbReset0, Ts);
    print_clk_inf(AhbReset0, HsTmr);
    print_clk_inf(AhbReset0, Spi0);
    print_clk_inf(AhbReset0, Spi1);
    print_clk_inf(AhbReset0, Spi2);
    print_clk_inf(AhbReset0, Spi3);
    print_clk_inf(AhbReset0, Otg);
    print_clk_inf(AhbReset0, Ehci0);
    print_clk_inf(AhbReset0, Ehci1);
    print_clk_inf(AhbReset0, Ohci0);
    print_clk_inf(AhbReset0, Ohci1);
    print_clk_inf(AhbReset0, Ohci2);

    printk("\nAhbReset1 clk infor:\n");
    print_clk_inf(AhbReset1, Ve);
    print_clk_inf(AhbReset1, Lcd0);
    print_clk_inf(AhbReset1, Lcd1);
    print_clk_inf(AhbReset1, Csi0);
    print_clk_inf(AhbReset1, Csi1);
    print_clk_inf(AhbReset1, Hdmi);
    print_clk_inf(AhbReset1, Be0);
    print_clk_inf(AhbReset1, Be1);
    print_clk_inf(AhbReset1, Fe0);
    print_clk_inf(AhbReset1, Fe1);
    print_clk_inf(AhbReset1, Mp);
    print_clk_inf(AhbReset1, Gpu);
    print_clk_inf(AhbReset1, MsgBox);
    print_clk_inf(AhbReset1, SpinLock);
    print_clk_inf(AhbReset1, Deu0);
    print_clk_inf(AhbReset1, Deu1);
    print_clk_inf(AhbReset1, Drc0);
    print_clk_inf(AhbReset1, Drc1);
    print_clk_inf(AhbReset1, MtcAcc);

    printk("\nAhbReset2 clk infor:\n");
    print_clk_inf(AhbReset2, Lvds);

    printk("\nApb1Reset clk infor:\n");
    print_clk_inf(Apb1Reset, Adda);
    print_clk_inf(Apb1Reset, Spdif);
    print_clk_inf(Apb1Reset, Pio);
    print_clk_inf(Apb1Reset, I2s0);
    print_clk_inf(Apb1Reset, I2s1);

    printk("\nApb2Reset clk infor:\n");
    print_clk_inf(Apb2Reset, Twi0);
    print_clk_inf(Apb2Reset, Twi1);
    print_clk_inf(Apb2Reset, Twi2);
    print_clk_inf(Apb2Reset, Twi3);
    print_clk_inf(Apb2Reset, Uart0);
    print_clk_inf(Apb2Reset, Uart1);
    print_clk_inf(Apb2Reset, Uart2);
    print_clk_inf(Apb2Reset, Uart3);
    print_clk_inf(Apb2Reset, Uart4);
    print_clk_inf(Apb2Reset, Uart5);

    printk("\nClkOutA clk infor:\n");
    print_clk_inf(ClkOutA, DivM);
    print_clk_inf(ClkOutA, DivN);
    print_clk_inf(ClkOutA, ClkSrc);
    print_clk_inf(ClkOutA, ClkEn);

    printk("\nClkOutB clk infor:\n");
    print_clk_inf(ClkOutB, DivM);
    print_clk_inf(ClkOutB, DivN);
    print_clk_inf(ClkOutB, ClkSrc);
    print_clk_inf(ClkOutB, ClkEn);

    printk("\nClkOutC clk infor:\n");
    print_clk_inf(ClkOutC, DivM);
    print_clk_inf(ClkOutC, DivN);
    print_clk_inf(ClkOutC, ClkSrc);
    print_clk_inf(ClkOutC, ClkEn);
}
EXPORT_SYMBOL(clk_dbg_inf);

#ifdef CONFIG_PROC_FS

#define sprintf_clk_inf(buf, x, y)     do{seq_printf(buf, "\t"#x"."#y":%d\n", aw_ccu_reg->x.y);}while(0)
static int ccmu_stats_show(struct seq_file *m, void *unused)
{
    seq_printf(m, "---------------------------------------------\n");
    seq_printf(m, "clock information:                           \n");
    seq_printf(m, "---------------------------------------------\n");

    seq_printf(m, "\nPLL1 infor:\n");
    sprintf_clk_inf(m, Pll1Ctl, FactorM  );
    sprintf_clk_inf(m, Pll1Ctl, FactorK  );
    sprintf_clk_inf(m, Pll1Ctl, FactorN  );
    sprintf_clk_inf(m, Pll1Ctl, SigmaEn  );
    sprintf_clk_inf(m, Pll1Ctl, Lock     );
    sprintf_clk_inf(m, Pll1Ctl, PLLEn    );

	return 0;
}


static int ccmu_stats_open(struct inode *inode, struct file *file)
{
	return single_open(file, ccmu_stats_show, NULL);
}

static const struct file_operations ccmu_dbg_fops = {
	.owner = THIS_MODULE,
	.open = ccmu_stats_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

static int __init ccu_dbg_init(void)
{
	proc_create("ccmu", S_IRUGO, NULL, &ccmu_dbg_fops);
	return 0;
}

static void  __exit ccu_dbg_exit(void)
{
	remove_proc_entry("ccmu", NULL);
}

core_initcall(ccu_dbg_init);
module_exit(ccu_dbg_exit);
#endif
