/*
 * arch/arm/mach-sun6i/dram-freq/mdfs/mdfs.c
 *
 * Copyright (C) 2012 - 2016 Reuuimlla Limited
 * Pan Nan <pannan@reuuimllatech.com>
 *
 * SUN6I dram frequency dynamic scaling driver
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 */

#include "mdfs.h"


static inline __u32 __uldiv(__u32 dividend, __u32 divisior)
{
    __u32   tmpDiv = divisior;
    __u32   tmpQuot = 0;
    __s32   shift = 0;

    if(!divisior)
    {
        /* divide 0 error abort */
        return 0;
    }

    while(!(tmpDiv & ((__u32)1<<31)))
    {
        tmpDiv <<= 1;
        shift ++;
    }

    do
    {
        if(dividend >= tmpDiv)
        {
            dividend -= tmpDiv;
            tmpQuot = (tmpQuot << 1) | 1;
        }
        else
        {
            tmpQuot = (tmpQuot << 1) | 0;
        }
        tmpDiv >>= 1;
        shift --;
    } while(shift >= 0);

    return tmpQuot;
}

void mdfs_memcpy(void *dest, const void *src, int n)
{
    char *tmp = dest;
    const char *s = src;

    if (!dest || !src)
        return;

    while (n--)
        *tmp++ = *s++;

    return;
}

void mdfs_memset(void *s, int c, int n)
{
    char *xs = s;

    if (!s)
        return;

    while (n--)
        *xs++ = c;

    return;
}

int raise (int signum)
{
    return 0;
}

int mdfs_start(struct aw_mdfs_info *mdfs)
{
    unsigned int reg_val, tmp, freq_div, dual_channel_en;
    unsigned int *mdfs_table;
    int i;

    dual_channel_en = mdfs->is_dual_channel;
    freq_div = mdfs->div;
    mdfs_table = &mdfs->table[0][0];

    /* wait for whether the past MDFS process has done */
    while (readl(SDR_COM_MDFSCR)&0x1);
    DEBUG_LINE;

    /* move to CFG */
    writel(0x1, SDR_SCTL);
    DEBUG_LINE;
    while ((readl(SDR_SSTAT) & 0x7) != 0x1);
    DEBUG_LINE;
    if (dual_channel_en) {
        writel(0x1, 0x1000 + SDR_SCTL);
        while ((readl(0x1000 + SDR_SSTAT) & 0x7) != 0x1);
        DEBUG_LINE;
    }
    DEBUG_LINE;

    /* pd_idle setting */
    reg_val = readl(SDR_MCFG);
    reg_val &= ~(0xff<<8);
    writel(reg_val, SDR_MCFG);
    if (dual_channel_en) {
        reg_val = readl(0x1000 + SDR_MCFG);
        reg_val &= ~(0xff<<8);
        writel(reg_val, 0x1000 + SDR_MCFG);
    }
    DEBUG_LINE;

    /* set Toggle 1us/100ns REG */
    reg_val  = (((readl(CCM_PLL5_DDR_CTRL)>>8)&0x1f)+1)*24;
    reg_val *= (((readl(CCM_PLL5_DDR_CTRL)>>4)&0x3) + 1);
    tmp = ((readl(CCM_PLL5_DDR_CTRL)>>0)&0x3) + 1;
    reg_val = __uldiv(reg_val, tmp);
    // reg_val /= (((readl(CCM_PLL5_DDR_CTRL)>>0)&0x3) + 1);
    reg_val = __uldiv(reg_val, freq_div);
    // reg_val /= freq_div;
    writel(reg_val, SDR_TOGCNT1U);                  //1us
    if (dual_channel_en) {
        writel(reg_val, 0x1000 + SDR_TOGCNT1U);     //1us
    }
    reg_val = __uldiv(reg_val, 10);
    // reg_val /= 10;
    writel(reg_val, SDR_TOGCNT100N);                //100ns
    if (dual_channel_en) {
        writel(reg_val, 0x1000 + SDR_TOGCNT100N);   //100ns
    }
    DEBUG_LINE;

    /* move to GO */
    writel(0x2, SDR_SCTL);
    while ((readl(SDR_SSTAT) & 0x7) != 0x3);
    if (dual_channel_en) {
        writel(0x2, 0x1000 + SDR_SCTL);
        while ((readl(0x1000 + SDR_SSTAT) & 0x7) != 0x3);
    }
    DEBUG_LINE;

    /* set DRAM middle & destination clock */
    reg_val = readl(CCM_DRAMCLK_CFG_CTRL);
    reg_val &= ~(0x1<<16);
    reg_val &= ~(0xf<<8);
    reg_val |= (freq_div-1)<<8;
    writel(reg_val, CCM_DRAMCLK_CFG_CTRL);
    DEBUG_LINE;

    /* set Master enable and Ready mask */
    reg_val = 0x3ff00001;
    writel(reg_val, SDR_COM_MDFSMER);
    reg_val = 0xfffffff8;
    writel(reg_val, SDR_COM_MDFSMRMR);
    DEBUG_LINE;

    /* set MDFS timing parameter */
    reg_val = 0x258;                                //3us/5ns = 600
    writel(reg_val, SDR_COM_MDFSTR0);
    reg_val = 102400*freq_div;
    tmp = (((readl(CCM_PLL5_DDR_CTRL)>>8)&0x1f)+1)*24;
    reg_val = __uldiv(reg_val, tmp);
    // reg_val /= tmp;
    reg_val ++;
    writel(reg_val, SDR_COM_MDFSTR1);               //512*200/sclk
    reg_val = 0x80;
    writel(reg_val, SDR_COM_MDFSTR2);               //fixed value : 128
    DEBUG_LINE;

    /* set MDFS DQS Gate Configuration */
    for (i=0;i<8;i++) {
        writel(*(mdfs_table + (8*(freq_div-1)) + i), SDR_COM_MDFSGCR + 4*i);
    }
    DEBUG_LINE;

    /* start hardware MDFS */
    reg_val = readl(SDR_COM_MDFSCR);
    reg_val >>= 2;
    reg_val &= ((0x1<<4) | (0x1<<8));
    reg_val |= 0x5 | (0x2u<<30);
    tmp = (((readl(CCM_PLL5_DDR_CTRL)>>8)&0x1f)+1)*24;
    tmp *= (((readl(CCM_PLL5_DDR_CTRL)>>4)&0x3) + 1);
    tmp = __uldiv(tmp, (((readl(CCM_PLL5_DDR_CTRL)>>0)&0x3) + 1));
    // tmp /= (((readl(CCM_PLL5_DDR_CTRL)>>0)&0x3) + 1);
    if (__uldiv(tmp, freq_div) < 200)
    // if ((tmp/freq_div) < 200)
        reg_val |= 0x1<<10;
    if (__uldiv(tmp, freq_div) <= 120)
    // if ((tmp/freq_div) <= 120)
        reg_val |= 0x1<<6;
    writel(reg_val, SDR_COM_MDFSCR);
    DEBUG_LINE;

	/* wait for whether the past MDFS process is done */
	while (readl(SDR_COM_MDFSCR)&0x1);
    DEBUG_LINE;

    /* move to CFG */
    writel(0x1, SDR_SCTL);
    while ((readl(SDR_SSTAT) & 0x7) != 0x1);
    if (dual_channel_en) {
        writel(0x1, 0x1000 + SDR_SCTL);
        while ((readl(0x1000 + SDR_SSTAT) & 0x7) != 0x1);
    }
    DEBUG_LINE;

    /* pd_idle setting */
    reg_val = readl(SDR_MCFG);
    reg_val |= (0x10<<8);
    writel(reg_val, SDR_MCFG);
    if (dual_channel_en) {
        reg_val = readl(0x1000 + SDR_MCFG);
        reg_val |= (0x10<<8);
        writel(reg_val, 0x1000 + SDR_MCFG);
    }
    DEBUG_LINE;

    /* move to GO */
    writel(0x2, SDR_SCTL);
    while ((readl(SDR_SSTAT) & 0x7) != 0x3);
    if (dual_channel_en) {
        writel(0x2, 0x1000 + SDR_SCTL);
        while ((readl(0x1000 + SDR_SSTAT) & 0x7) != 0x3);
    }
    DEBUG_LINE;

	return 0;
}
