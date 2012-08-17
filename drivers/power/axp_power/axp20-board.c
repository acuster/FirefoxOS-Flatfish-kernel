#include <linux/init.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/regulator/machine.h>
#include <linux/i2c.h>
#include <mach/irqs.h>
#include <linux/power_supply.h>
#include <linux/apm_bios.h>
#include <linux/apm-emulation.h>
#include <linux/mfd/axp-mfd.h>
#include <linux/module.h>

#include "axp-cfg.h"
#include <mach/sys_config.h>


int pmu_used;
int pmu_twi_id;
int pmu_irq_id;
int pmu_twi_addr;
int pmu_battery_rdc;
int pmu_battery_cap;
int pmu_init_chgcur;
int pmu_suspend_chgcur;
int pmu_resume_chgcur;
int pmu_shutdown_chgcur;
int pmu_init_chgvol;
int pmu_init_chgend_rate;
int pmu_init_chg_enabled;
int pmu_init_adc_freq;
int pmu_init_adc_freqc;
int pmu_init_chg_pretime;
int pmu_init_chg_csttime;

int pmu_bat_para1;
int pmu_bat_para2;
int pmu_bat_para3;
int pmu_bat_para4;
int pmu_bat_para5;
int pmu_bat_para6;
int pmu_bat_para7;
int pmu_bat_para8;
int pmu_bat_para9;
int pmu_bat_para10;
int pmu_bat_para11;
int pmu_bat_para12;
int pmu_bat_para13;
int pmu_bat_para14;
int pmu_bat_para15;
int pmu_bat_para16;

int pmu_usbvol_limit;
int pmu_usbvol;
int pmu_usbcur_limit;
int pmu_usbcur;

int pmu_pwroff_vol;
int pmu_pwron_vol;

int dcdc2_vol;
int dcdc3_vol;
int ldo2_vol;
int ldo3_vol;
int ldo4_vol;

int pmu_pekoff_time;
int pmu_pekoff_en;
int pmu_peklong_time;
int pmu_pekon_time;
int pmu_pwrok_time;
int pmu_pwrnoe_time;
int pmu_intotp_en;

/* Reverse engineered partly from Platformx drivers */
enum axp_regls{

	vcc_ldo1,
	vcc_ldo2,
	vcc_ldo3,
	vcc_ldo4,
	vcc_ldo5,

	vcc_buck2,
	vcc_buck3,
	vcc_ldoio0,
};

/* The values of the various regulator constraints are obviously dependent
 * on exactly what is wired to each ldo.  Unfortunately this information is
 * not generally available.  More information has been requested from Xbow
 * but as of yet they haven't been forthcoming.
 *
 * Some of these are clearly Stargate 2 related (no way of plugging
 * in an lcd on the IM2 for example!).
 */

static struct regulator_consumer_supply ldo1_data[] = {
		{
			.supply = "axp20_rtc",
		},
	};


static struct regulator_consumer_supply ldo2_data[] = {
		{
			.supply = "axp20_analog/fm",
		},
	};

static struct regulator_consumer_supply ldo3_data[] = {
		{
			.supply = "axp20_pll",
		},
	};

static struct regulator_consumer_supply ldo4_data[] = {
		{
			.supply = "axp20_hdmi",
		},
	};

static struct regulator_consumer_supply ldoio0_data[] = {
		{
			.supply = "axp20_mic",
		},
	};


static struct regulator_consumer_supply buck2_data[] = {
		{
			.supply = "axp20_core",
		},
	};

static struct regulator_consumer_supply buck3_data[] = {
		{
			.supply = "axp20_ddr",
		},
	};



static struct regulator_init_data axp_regl_init_data[] = {
	[vcc_ldo1] = {
		.constraints = { /* board default 1.25V */
			.name = "axp20_ldo1",
			.min_uV =  AXP20LDO1 * 1000,
			.max_uV =  AXP20LDO1 * 1000,
		},
		.num_consumer_supplies = ARRAY_SIZE(ldo1_data),
		.consumer_supplies = ldo1_data,
	},
	[vcc_ldo2] = {
		.constraints = { /* board default 3.0V */
			.name = "axp20_ldo2",
			.min_uV = 1800000,
			.max_uV = 3300000,
			.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE | REGULATOR_CHANGE_STATUS,
			.initial_state = PM_SUSPEND_STANDBY,
			.state_standby = {
				//.uV = ldo2_vol * 1000,
				//.enabled = 1,
			}
		},
		.num_consumer_supplies = ARRAY_SIZE(ldo2_data),
		.consumer_supplies = ldo2_data,
	},
	[vcc_ldo3] = {
		.constraints = {/* default is 1.8V */
			.name = "axp20_ldo3",
			.min_uV =  700 * 1000,
			.max_uV =  3500* 1000,
			.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE | REGULATOR_CHANGE_STATUS,
			.initial_state = PM_SUSPEND_STANDBY,
			.state_standby = {
				//.uV = ldo3_vol * 1000,
				//.enabled = 1,
			}
		},
		.num_consumer_supplies = ARRAY_SIZE(ldo3_data),
		.consumer_supplies = ldo3_data,
	},
	[vcc_ldo4] = {
		.constraints = {
			/* board default is 3.3V */
			.name = "axp20_ldo4",
			.min_uV = 1250000,
			.max_uV = 3300000,
			.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE | REGULATOR_CHANGE_STATUS,
			.initial_state = PM_SUSPEND_STANDBY,
			.state_standby = {
				//.uV = ldo4_vol * 1000,
				//.enabled = 1,
			}
		},
		.num_consumer_supplies = ARRAY_SIZE(ldo4_data),
		.consumer_supplies = ldo4_data,
	},
	[vcc_buck2] = {
		.constraints = { /* default 1.24V */
			.name = "axp20_buck2",
			.min_uV = 700 * 1000,
			.max_uV = 2275 * 1000,
			.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE | REGULATOR_CHANGE_STATUS,
			.initial_state = PM_SUSPEND_STANDBY,
			.state_standby = {
				//.uV = dcdc2_vol * 1000,
				//.enabled = 1,
			}
		},
		.num_consumer_supplies = ARRAY_SIZE(buck2_data),
		.consumer_supplies = buck2_data,
	},
	[vcc_buck3] = {
		.constraints = { /* default 2.5V */
			.name = "axp20_buck3",
			.min_uV = 700 * 1000,
			.max_uV = 3500 * 1000,
			.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE | REGULATOR_CHANGE_STATUS,
			.initial_state = PM_SUSPEND_STANDBY,
			.state_standby = {
				//.uV = dcdc3_vol * 1000,
				//.enabled = 1,
			}
		},
		.num_consumer_supplies = ARRAY_SIZE(buck3_data),
		.consumer_supplies = buck3_data,
	},
	[vcc_ldoio0] = {
		.constraints = { /* default 2.5V */
			.name = "axp20_ldoio0",
			.min_uV = 1800 * 1000,
			.max_uV = 3300 * 1000,
			.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE | REGULATOR_CHANGE_STATUS,
		},
		.num_consumer_supplies = ARRAY_SIZE(ldoio0_data),
		.consumer_supplies = ldoio0_data,
	},
};

static struct axp_funcdev_info axp_regldevs[] = {
	{
		.name = "axp20-regulator",
		.id = AXP20_ID_LDO1,
		.platform_data = &axp_regl_init_data[vcc_ldo1],
	}, {
		.name = "axp20-regulator",
		.id = AXP20_ID_LDO2,
		.platform_data = &axp_regl_init_data[vcc_ldo2],
	}, {
		.name = "axp20-regulator",
		.id = AXP20_ID_LDO3,
		.platform_data = &axp_regl_init_data[vcc_ldo3],
	}, {
		.name = "axp20-regulator",
		.id = AXP20_ID_LDO4,
		.platform_data = &axp_regl_init_data[vcc_ldo4],
	}, {
		.name = "axp20-regulator",
		.id = AXP20_ID_BUCK2,
		.platform_data = &axp_regl_init_data[vcc_buck2],
	}, {
		.name = "axp20-regulator",
		.id = AXP20_ID_BUCK3,
		.platform_data = &axp_regl_init_data[vcc_buck3],
	}, {
		.name = "axp20-regulator",
		.id = AXP20_ID_LDOIO0,
		.platform_data = &axp_regl_init_data[vcc_ldoio0],
	},
};

static struct power_supply_info battery_data ={
		.name ="PTI PL336078",
		.technology = POWER_SUPPLY_TECHNOLOGY_LiFe,
		//.voltage_max_design = pmu_init_chgvol,
		//.voltage_min_design = pmu_pwroff_vol,
		//.energy_full_design = pmu_battery_cap,
		.use_for_apm = 1,
};


static struct axp_supply_init_data axp_sply_init_data = {
	.battery_info = &battery_data,
	//.chgcur = pmu_init_chgcur,
	//.chgvol = pmu_init_chgvol,
	//.chgend = pmu_init_chgend_rate,
	//.chgen = pmu_init_chg_enabled,
	//.sample_time = pmu_init_adc_freq,
	//.chgpretime = pmu_init_chg_pretime,
	//.chgcsttime = pmu_init_chg_csttime,
};

static struct axp_funcdev_info axp_splydev[]={
	{
		.name = "axp20-supplyer",
			.id = AXP20_ID_SUPPLY,
      .platform_data = &axp_sply_init_data,
    },
};

static struct axp_funcdev_info axp_gpiodev[]={
	{   .name = "axp20-gpio",
		.id = AXP20_ID_GPIO,
    },
};

static struct axp_platform_data axp_pdata = {
	.num_regl_devs = ARRAY_SIZE(axp_regldevs),
	.num_sply_devs = ARRAY_SIZE(axp_splydev),
	.num_gpio_devs = ARRAY_SIZE(axp_gpiodev),
	.regl_devs = axp_regldevs,
	.sply_devs = axp_splydev,
	.gpio_devs = axp_gpiodev,
	.gpio_base = 0,
};

static struct i2c_board_info __initdata axp_mfd_i2c_board_info[] = {
	{
		.type = "axp20_mfd",
		//.addr = pmu_twi_addr,
		.platform_data = &axp_pdata,
		//.irq = pmu_irq_id,
	},
};

static int __init axp_board_init(void)
{
		int ret;
//Kyle added for fpga debug
		int pmu_used = 1;
		int pmu_twi_addr = 0x34;
		int pmu_twi_id = 1;
		int pmu_irq_id = 32;
		int pmu_battery_rdc          = 100;
		int pmu_battery_cap          = 2600;
		int pmu_init_chgcur          = 300;
		int pmu_earlysuspend_chgcur  = 600;
		int pmu_suspend_chgcur       = 1000;
		int pmu_resume_chgcur        = 300;
		int pmu_shutdown_chgcur      = 1000;
		int pmu_init_chgvol          = 4200;
		int pmu_init_chgend_rate     = 15;
		int pmu_init_chg_enabled     = 1;
		int pmu_init_adc_freq        = 100;
		int pmu_init_adc_freqc       = 100;
		int pmu_init_chg_pretime     = 50;
		int pmu_init_chg_csttime     = 720;

		int pmu_bat_para1            = 0;
		int pmu_bat_para2            = 0;
		int pmu_bat_para3            = 0;
		int pmu_bat_para4            = 0;
		int pmu_bat_para5            = 5;
		int pmu_bat_para6            = 13;
		int pmu_bat_para7            = 16;
		int pmu_bat_para8            = 26;
		int pmu_bat_para9            = 36;
		int pmu_bat_para10           = 46;
		int pmu_bat_para11           = 53;
		int pmu_bat_para12           = 61;
		int pmu_bat_para13           = 73;
		int pmu_bat_para14           = 84;
		int pmu_bat_para15           = 92;
		int pmu_bat_para16           = 100;

		int pmu_usbvol_limit         = 1;
		int pmu_usbcur_limit         = 0;
		int pmu_usbvol               = 4000;
		int pmu_usbcur               = 0;

		int pmu_usbvol_pc            = 4000;
		int pmu_usbcur_pc            = 0;

		int pmu_pwroff_vol           = 3300;
		int pmu_pwron_vol            = 2900;

    if (pmu_used)
    {
        axp_regl_init_data[1].constraints.state_standby.uV = ldo2_vol * 1000;
        axp_regl_init_data[2].constraints.state_standby.uV = ldo3_vol * 1000;
        axp_regl_init_data[3].constraints.state_standby.uV = ldo4_vol * 1000;
        axp_regl_init_data[5].constraints.state_standby.uV = dcdc2_vol * 1000;
        axp_regl_init_data[6].constraints.state_standby.uV = dcdc3_vol * 1000;
        axp_regl_init_data[1].constraints.state_standby.enabled = (ldo2_vol)?1:0;
        axp_regl_init_data[1].constraints.state_standby.disabled = (ldo2_vol)?0:1;
        axp_regl_init_data[2].constraints.state_standby.enabled = (ldo3_vol)?1:0;
        axp_regl_init_data[2].constraints.state_standby.disabled = (ldo3_vol)?0:1;
        axp_regl_init_data[3].constraints.state_standby.enabled = (ldo4_vol)?1:0;
        axp_regl_init_data[3].constraints.state_standby.disabled = (ldo4_vol)?0:1;
        axp_regl_init_data[5].constraints.state_standby.enabled = (dcdc2_vol)?1:0;
        axp_regl_init_data[5].constraints.state_standby.disabled = (dcdc2_vol)?0:1;
        axp_regl_init_data[6].constraints.state_standby.enabled = (dcdc3_vol)?1:0;
        axp_regl_init_data[6].constraints.state_standby.disabled = (dcdc3_vol)?0:1;
        battery_data.voltage_max_design = pmu_init_chgvol;
        battery_data.voltage_min_design = pmu_pwroff_vol;
        battery_data.energy_full_design = pmu_battery_cap;
        axp_sply_init_data.chgcur = pmu_init_chgcur;
        axp_sply_init_data.chgvol = pmu_init_chgvol;
        axp_sply_init_data.chgend = pmu_init_chgend_rate;
        axp_sply_init_data.chgen = pmu_init_chg_enabled;
        axp_sply_init_data.sample_time = pmu_init_adc_freq;
        axp_sply_init_data.chgpretime = pmu_init_chg_pretime;
        axp_sply_init_data.chgcsttime = pmu_init_chg_csttime;
        axp_mfd_i2c_board_info[0].addr = pmu_twi_addr;
        axp_mfd_i2c_board_info[0].irq = pmu_irq_id;
        //return i2c_register_board_info(pmu_twi_id, axp_mfd_i2c_board_info,
	//			ARRAY_SIZE(axp_mfd_i2c_board_info));
        ret = i2c_register_board_info(pmu_twi_id, axp_mfd_i2c_board_info,
				ARRAY_SIZE(axp_mfd_i2c_board_info));
	return ret;
		}
    else
        return -1;

}
fs_initcall(axp_board_init);

MODULE_DESCRIPTION("Axp board");
MODULE_AUTHOR("Kyle Cheung");
MODULE_LICENSE("GPL");
