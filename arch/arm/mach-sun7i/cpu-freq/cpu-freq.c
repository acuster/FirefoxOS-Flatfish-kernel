/*
 *  arch/arm/mach-sun7i/cpu-freq/cpu-freq.c
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

#include <linux/init.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/cpufreq.h>
#include <linux/cpu.h>
#include <linux/clk.h>
#include <linux/err.h>
#include <linux/regulator/consumer.h>
#include <mach/sys_config.h>
#include "cpu-freq.h"
#include <linux/pm.h>
#include <mach/ar100_cp.h>

static struct sunxi_cpu_freq_t  cpu_cur;    /* current cpu frequency configuration  */
static unsigned int last_target = ~0;       /* backup last target frequency         */

static struct clk *clk_pll; /* pll clock handler */
static struct clk *clk_cpu; /* cpu clock handler */
static struct clk *clk_axi; /* axi clock handler */


static unsigned int cpu_freq_max = SUNXI_CPUFREQ_MAX / 1000;
static unsigned int cpu_freq_min = SUNXI_CPUFREQ_MIN / 1000;


/*
 *check if the cpu frequency policy is valid;
 */
static int sunxi_cpufreq_verify(struct cpufreq_policy *policy)
{
	if (policy->cpu != 0)
		return -EINVAL;

	return 0;
}


/*
 *show cpu frequency information;
 */
static void sunxi_cpufreq_show(const char *pfx, struct sunxi_cpu_freq_t *cfg)
{
	CPUFREQ_DBG("%s: pll=%u, cpudiv=%u, axidiv=%u\n", pfx, cfg->pll, cfg->div.cpu_div, cfg->div.axi_div);
}


/*
 * adjust the frequency that cpu is currently running;
 * policy:  cpu frequency policy;
 * freq:    target frequency to be set, based on khz;
 * relation:    method for selecting the target requency;
 * return:  result, return 0 if set target frequency successed, else, return -EINVAL;
 * notes:   this function is called by the cpufreq core;
 */
static int sunxi_cpufreq_target(struct cpufreq_policy *policy, __u32 freq, __u32 relation)
{
    unsigned int            index;
    struct sunxi_cpu_freq_t freq_cfg;
    struct cpufreq_freqs    freqs;

	/* avoid repeated calls which cause a needless amout of duplicated
	 * logging output (and CPU time as the calculation process is
	 * done) */
	if (freq == last_target) {
		return 0;
	}

    /* try to look for a valid frequency value from cpu frequency table */
    if (cpufreq_frequency_table_target(policy, sunxi_freq_tbl, freq, relation, &index)) {
        CPUFREQ_ERR("%s: try to look for a valid frequency for %u failed!\n", __func__, freq);
		return -EINVAL;
	}

	if (sunxi_freq_tbl[index].frequency == last_target) {
        /* frequency is same as the value last set, need not adjust */
		return 0;
	}
	freq = sunxi_freq_tbl[index].frequency;

    /* update the target frequency */
    freq_cfg.pll = sunxi_freq_tbl[index].frequency * 1000;
    freq_cfg.div = *(struct sunxi_clk_div_t *)&sunxi_freq_tbl[index].index;
    CPUFREQ_DBG("%s: target frequency find is %u, entry %u\n", __func__, freq_cfg.pll, index);

    /* notify that cpu clock will be adjust if needed */
	if (policy) {
	    freqs.cpu = policy->cpu;
	    freqs.old = last_target;
	    freqs.new = freq;
		cpufreq_notify_transition(&freqs, CPUFREQ_PRECHANGE);
	}

    /* try to set cpu frequency */
    if (ar100_dvfs_setcpufreq(freq)) {
        /* set cpu frequency failed */
	if (policy) {
	        freqs.cpu = policy->cpu;
	        freqs.old = freqs.new;
	        freqs.new = last_target;
		    cpufreq_notify_transition(&freqs, CPUFREQ_POSTCHANGE);
	    }

        return -EINVAL;
    }

    /* notify that cpu clock will be adjust if needed */
	if (policy) {
		cpufreq_notify_transition(&freqs, CPUFREQ_POSTCHANGE);
	}

    last_target = freq;

    return 0;
}


/*
 * get the frequency that cpu currently is running;
 * cpu:    cpu number, all cpus use the same clock;
 * return: cpu frequency, based on khz;
 */
static unsigned int sunxi_cpufreq_get(unsigned int cpu)
{
	return clk_get_rate(clk_cpu) / 1000;
}


/*
 * get the frequency that cpu average is running;
 * cpu:    cpu number, all cpus use the same clock;
 * return: cpu frequency, based on khz;
 */
static unsigned int sunxi_cpufreq_getavg(struct cpufreq_policy *policy, unsigned int cpu)
{
	return clk_get_rate(clk_cpu) / 1000;
}


/*
 * cpu frequency initialise a policy;
 * policy:  cpu frequency policy;
 * result:  return 0 if init ok, else, return -EINVAL;
 */
static int sunxi_cpufreq_init(struct cpufreq_policy *policy)
{
	CPUFREQ_DBG("%s\n", __func__);

	if (policy->cpu != 0)
		return -EINVAL;

	policy->cur = sunxi_cpufreq_get(0);
	policy->min = policy->cpuinfo.min_freq = cpu_freq_min;
	policy->max = policy->cpuinfo.max_freq = cpu_freq_max;
	policy->governor = CPUFREQ_DEFAULT_GOVERNOR;

	/* feed the latency information from the cpu driver */
	policy->cpuinfo.transition_latency = SUNXI_FREQTRANS_LATENCY;

	return 0;
}


/*
 * get current cpu frequency configuration;
 * cfg:     cpu frequency cofniguration;
 * return:  result;
 */
static int sunxi_cpufreq_getcur(struct sunxi_cpu_freq_t *cfg)
{
    unsigned int    freq, freq0;

    if(!cfg) {
        return -EINVAL;
    }

	cfg->pll = clk_get_rate(clk_pll);
    freq = clk_get_rate(clk_cpu);
    cfg->div.cpu_div = cfg->pll / freq;
    freq0 = clk_get_rate(clk_axi);
    cfg->div.axi_div = freq / freq0;

	return 0;
}


#ifdef CONFIG_PM

/*
 * cpu frequency configuration suspend;
 */
static int sunxi_cpufreq_suspend(struct cpufreq_policy *policy)
{
    CPUFREQ_DBG("%s\n", __func__);
    return 0;
}

/*
 * cpu frequency configuration resume;
 */
static int sunxi_cpufreq_resume(struct cpufreq_policy *policy)
{
    /* invalidate last_target setting */
	last_target = ~0;
	CPUFREQ_DBG("%s\n", __func__);
	return 0;
}


#else   /* #ifdef CONFIG_PM */

#define sunxi_cpufreq_suspend   NULL
#define sunxi_cpufreq_resume    NULL

#endif  /* #ifdef CONFIG_PM */


static struct cpufreq_driver sunxi_cpufreq_driver = {
	.name		= "sunxi",
	.flags		= CPUFREQ_STICKY,
	.init		= sunxi_cpufreq_init,
	.verify		= sunxi_cpufreq_verify,
	.target		= sunxi_cpufreq_target,
	.get		= sunxi_cpufreq_get,
	.getavg     = sunxi_cpufreq_getavg,
	.suspend	= sunxi_cpufreq_suspend,
	.resume		= sunxi_cpufreq_resume,
};


/*
 * cpu frequency driver init
 */
static int __init sunxi_cpufreq_initcall(void)
{
	int ret = 0;

    clk_pll = clk_get(NULL, "sys_ac327");
    clk_cpu = clk_get(NULL, "sys_cpu");
    clk_axi = clk_get(NULL, "sys_axi");

	if (IS_ERR(clk_pll) || IS_ERR(clk_cpu) || IS_ERR(clk_axi)) {
		CPUFREQ_INF(KERN_ERR "%s: could not get clock(s)\n", __func__);
		return -ENOENT;
	}

	CPUFREQ_INF("%s: clocks pll=%lu,cpu=%lu,axi=%lu\n", __func__,
	       clk_get_rate(clk_pll), clk_get_rate(clk_cpu), clk_get_rate(clk_axi));

    /* initialise current frequency configuration */
	sunxi_cpufreq_getcur(&cpu_cur);
	sunxi_cpufreq_show("cur", &cpu_cur);

    /* register cpu frequency driver */
    ret = cpufreq_register_driver(&sunxi_cpufreq_driver);
    /* register cpu frequency table to cpufreq core */
    cpufreq_frequency_table_get_attr(sunxi_freq_tbl, 0);
    /* update policy for boot cpu */
    cpufreq_update_policy(0);

	return ret;
}


/*
 * cpu frequency driver exit
 */
static void __exit sunxi_cpufreq_exitcall(void)
{
    clk_put(clk_pll);
    clk_put(clk_cpu);
    clk_put(clk_axi);
	cpufreq_unregister_driver(&sunxi_cpufreq_driver);
}

/* !!!!!!!!!!!!!!!!!!!!!!! just for debug */
int ar100_dvfs_setcpufreq(unsigned long freq)
{
    return 0;
}

MODULE_DESCRIPTION("cpufreq driver for sunxi SOCs");
MODULE_LICENSE("GPL");
module_init(sunxi_cpufreq_initcall);
module_exit(sunxi_cpufreq_exitcall);
