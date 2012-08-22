/*
 * arch/arm/mach-sun7i/gpio/gpio_script.c
 * (C) Copyright 2010-2015
 * Reuuimlla Technology Co., Ltd. <www.reuuimllatech.com>
 * liugang <liugang@reuuimllatech.com>
 *
 * sun7i gpio driver
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 */

#include "gpio_include.h"

/*
 * old gpio api(base on script) realize
 */
extern char sys_cofig_data[];
extern char sys_cofig_data_end[];

#define CSP_OSAL_MALLOC(size) 		kmalloc((size), GFP_ATOMIC)
#define CSP_OSAL_FREE(ptr) 		kfree((ptr))

typedef struct
{
	int mul_sel;
	int pull;
	int drv_level;
	int data;
}gpio_status_set_t;

typedef struct
{
	char    	gpio_name[32];
	int 	port;
	int 	port_num;
	gpio_status_set_t user_gpio_status;
	gpio_status_set_t hardware_gpio_status;
}system_gpio_set_t;

/**
 * gpio_init - script init
 *
 * return SCRIPT_PARSER_OK if success, others if failed
 */
int gpio_script_init(void)
{
	PIO_DBG("%s, line %d, init pin", __FUNCTION__, __LINE__);

	//gpio_g_pioMemBase = (u32)CSP_OSAL_PHY_2_VIRT(CSP_PIN_PHY_ADDR_BASE , CSP_PIN_PHY_ADDR_SIZE);
#ifdef FPGA_RUNTIME_ENV
	return script_parser_init((char *)(sys_cofig_data));
#else
	return script_parser_init((char *)__va(SYS_CONFIG_MEMBASE));
#endif
}
fs_initcall(gpio_script_init);

/**
 * port_to_gpio_index - get gpio index from port and prot_num
 * @port: gpio port group index, eg: 1 for PA, 2 for PB...
 * @port_num: port index in gpio group, eg: 0 for PA0, 1 for PA1...
 *
 * return the gpio index for the port, GPIO_INDEX_INVALID indicate err
 */
static inline u32 port_to_gpio_index(u32 port, u32 port_num)
{
	u32 	usign = 0;
	struct pio_group {
		u32 	base;
		u32 	nr;
	};
	const struct pio_group pio_buf[] = {
		{PA_NR_BASE,         PA_NR},
		{PB_NR_BASE,         PB_NR},
		{PC_NR_BASE,         PC_NR},
		{PD_NR_BASE,         PD_NR},
		{PE_NR_BASE,         PE_NR},
		{PF_NR_BASE,         PF_NR},
		{PG_NR_BASE,         PG_NR},
		{PH_NR_BASE,         PH_NR},
		{PI_NR_BASE,         PI_NR}
	};

	PIO_DBG("%s: port %d, port_num %d\n", __FUNCTION__, port, port_num);

	/* check if port valid */
	if(port - 1 >= ARRAY_SIZE(pio_buf)) {
		usign = __LINE__;
		goto End;
	}
	if(port_num >= pio_buf[port - 1].nr) {
		usign = __LINE__;
		goto End;
	}

End:
	if(0 != usign) {
		PIO_ERR("%s err, line %d\n", __FUNCTION__, usign);
		return GPIO_INDEX_INVALID;
	} else {
		PIO_DBG("%s success\n", __FUNCTION__);
		return (pio_buf[port - 1].base + port_num);
	}
}

/**
 * sw_gpio_request - config a group of gpio, for each, config the mul_sel/pull/drv_level/data...
 * @gpio_list: 存放所有用到的GPIO数据的数组，GPIO将直接使用这个数组
 * @group_count_max: 数组的成员个数，GPIO设定的时候，将操作的GPIO最大不超过这个值
 *
 * return gpio handle if success, 0 if failed
 */
u32 sw_gpio_request(user_gpio_set_t *gpio_list, u32 group_count_max)
{
	char         	*user_gpio_buf;                                        //按照char类型申请
	system_gpio_set_t *user_gpio_set, *tmp_sys_gpio_data;                      //user_gpio_set将是申请内存的句柄
	user_gpio_set_t	*tmp_user_gpio_data;
	u32        	real_gpio_count = 0, first_port;                      //保存真正有效的GPIO的个数
	u32  		port, port_num;
	s32  		i;
	u32		pio_index = 0;
	u32		usign = 0;
	struct gpio_config config_stru = {0};

	// 检查gpio_list中真正有效的gpio个数
	if((!gpio_list) || (!group_count_max))
		return (u32)0;
	for(i = 0; i < group_count_max; i++) {
		tmp_user_gpio_data = gpio_list + i;                 //gpio_set依次指向每个GPIO数组成员
		if(!tmp_user_gpio_data->port)
			continue;
		real_gpio_count ++;
	}

	// 分配user_gpio_buf, 将作为句柄返回
	// fix bug: may be memory overflow in "tmp_sys_gpio_data  = user_gpio_set + i" line below.
	user_gpio_buf = (char *)CSP_OSAL_MALLOC(16 + sizeof(system_gpio_set_t) * group_count_max);   //申请内存，多申请16个字节，用于存放GPIO个数等信息
	//user_gpio_buf = (char *)CSP_OSAL_MALLOC(16 + sizeof(system_gpio_set_t) * real_gpio_count);
	if(!user_gpio_buf)
		return (u32)0;
	memset(user_gpio_buf, 0, 16 + sizeof(system_gpio_set_t) * real_gpio_count);        //首先全部清零
	*(int *)user_gpio_buf = real_gpio_count;                                           //保存有效的GPIO个数
	user_gpio_set = (system_gpio_set_t *)(user_gpio_buf + 16);                         //指向第一个结构体

	//找到gpio_list第一个有效port, 并获取其原始硬件配置信息
	for(first_port = 0; first_port < group_count_max; first_port++)	{
		tmp_user_gpio_data = gpio_list + first_port;
		port     = tmp_user_gpio_data->port;                         //读出端口数值
		port_num = tmp_user_gpio_data->port_num;                     //读出端口中的某一个GPIO
		if(!port) {
			PIO_DBG_FUN_LINE;
			continue;
		}

#if 0
		// get the orignal reg value for first valid port
		port_num_func = (port_num >> 3);
		port_num_pull = (port_num >> 4);

		tmp_group_func_addr    = PIO_REG_CFG(port, port_num_func);   //更新功能寄存器地址
		tmp_group_pull_addr    = PIO_REG_PULL(port, port_num_pull);  //更新pull寄存器
		tmp_group_dlevel_addr  = PIO_REG_DLEVEL(port, port_num_pull);//更新level寄存器
		tmp_group_data_addr    = PIO_REG_DATA(port);                 //更新data寄存器

		tmp_group_func_data    = *tmp_group_func_addr;
		tmp_group_pull_data    = *tmp_group_pull_addr;
		tmp_group_dlevel_data  = *tmp_group_dlevel_addr;
		tmp_group_data_data    = *tmp_group_data_addr;
#endif
		break;
	}
	if(first_port >= group_count_max) { //找不到有效port, 则返回
		PIO_DBG_FUN_LINE;
		return 0;
	}

	// 对gpio_list每一个有效项, 保存用户配置信息到user_gpio_buf; 对硬件进行配置;
	//	  并保存hardware_gpio_status结构(从硬件读取)
	// 对硬件配置时, 考虑了连续几项配置同一端口的情形
	for(i = first_port; i < group_count_max; i++) {
		// 若配置项无效则返回.
		tmp_sys_gpio_data  = user_gpio_set + i;             //tmp_sys_gpio_data指向申请的GPIO空间
		tmp_user_gpio_data = gpio_list + i;                 //gpio_set依次指向用户的每个GPIO数组成员
		port     = tmp_user_gpio_data->port;                //读出端口数值
		port_num = tmp_user_gpio_data->port_num;            //读出端口中的某一个GPIO
		if(!port)
			continue;

		// 保存用户配置信息
		strcpy(tmp_sys_gpio_data->gpio_name, tmp_user_gpio_data->gpio_name);
		tmp_sys_gpio_data->port                       = port;
		tmp_sys_gpio_data->port_num                   = port_num;
		tmp_sys_gpio_data->user_gpio_status.mul_sel   = tmp_user_gpio_data->mul_sel;
		tmp_sys_gpio_data->user_gpio_status.pull      = tmp_user_gpio_data->pull;
		tmp_sys_gpio_data->user_gpio_status.drv_level = tmp_user_gpio_data->drv_level;
		tmp_sys_gpio_data->user_gpio_status.data      = tmp_user_gpio_data->data;

#if 1
		/* get the gpio index */
		pio_index = port_to_gpio_index(port, port_num);

		PIO_DBG("%s: pio_index %d, mul_sel %d, pull %d, drv_level %d\n", __FUNCTION__, pio_index,
			tmp_user_gpio_data->mul_sel, tmp_user_gpio_data->pull, tmp_user_gpio_data->drv_level);

		/* backup the last config(read from hw) to hardware_gpio_status */
		tmp_sys_gpio_data->hardware_gpio_status.mul_sel = sw_gpio_getcfg(pio_index);
		tmp_sys_gpio_data->hardware_gpio_status.pull = sw_gpio_getpull(pio_index);
		tmp_sys_gpio_data->hardware_gpio_status.drv_level = sw_gpio_getdrvlevel(pio_index);

		/* config pio reg */
		config_stru.gpio = pio_index;
		config_stru.mul_sel = tmp_user_gpio_data->mul_sel;
		config_stru.pull = tmp_user_gpio_data->pull;
		config_stru.drv_level = tmp_user_gpio_data->drv_level;
		if(0 != sw_gpio_setall_range(&config_stru, 1)) {
			usign = __LINE__;
			goto End;
		}
#else
		port_num_func = (port_num >> 3);
		port_num_pull = (port_num >> 4);

		// 对硬件进行配置
		if((port_num_pull != pre_port_num_pull) || (port != pre_port)) { //如果发现当前引脚的端口不一致，或者所在的pull寄存器不一致
			if(func_change)	{
				*tmp_group_func_addr   = tmp_group_func_data;    //回写功能寄存器
				func_change = 0;
			}
			if(pull_change)	{
				pull_change = 0;
				*tmp_group_pull_addr   = tmp_group_pull_data;    //回写pull寄存器
			}
			if(dlevel_change) {
				dlevel_change = 0;
				*tmp_group_dlevel_addr = tmp_group_dlevel_data;  //回写driver level寄存器
			}
			if(data_change)	{
				data_change = 0;
				*tmp_group_data_addr   = tmp_group_data_data;    //回写
			}

			tmp_group_func_addr    = PIO_REG_CFG(port, port_num_func);   //更新功能寄存器地址
			tmp_group_pull_addr    = PIO_REG_PULL(port, port_num_pull);   //更新pull寄存器
			tmp_group_dlevel_addr  = PIO_REG_DLEVEL(port, port_num_pull); //更新level寄存器
			tmp_group_data_addr    = PIO_REG_DATA(port);                  //更新data寄存器

			tmp_group_func_data    = *tmp_group_func_addr;
			tmp_group_pull_data    = *tmp_group_pull_addr;
			tmp_group_dlevel_data  = *tmp_group_dlevel_addr;
			tmp_group_data_data    = *tmp_group_data_addr;

		}
		else if(pre_port_num_func != port_num_func) { //如果发现当前引脚的功能寄存器不一致
			*tmp_group_func_addr   = tmp_group_func_data;    //则只回写功能寄存器
			tmp_group_func_addr    = PIO_REG_CFG(port, port_num_func);   //更新功能寄存器地址

			tmp_group_func_data    = *tmp_group_func_addr;
		}

		pre_port_num_pull = port_num_pull;                      //保存当前硬件寄存器数据, 设置当前GPIO成为前一个GPIO
		pre_port_num_func = port_num_func;
		pre_port          = port;

		// 更新将要写入func reg的值
		if(tmp_user_gpio_data->mul_sel >= 0) {
			tmp_val = (port_num - (port_num_func<<3)) << 2; /* get the bits offset of cfg reg */
			tmp_sys_gpio_data->hardware_gpio_status.mul_sel = (tmp_group_func_data >> tmp_val) & 0x07;
			tmp_group_func_data &= ~(                              0x07  << tmp_val);
			tmp_group_func_data |=  (tmp_user_gpio_data->mul_sel & 0x07) << tmp_val;
			func_change = 1;
		}
		// 更新将要写入pull reg的值
		tmp_val = (port_num - (port_num_pull<<4)) << 1; /* get the bits offset of pull reg */
		if(tmp_user_gpio_data->pull >= 0) {
			tmp_sys_gpio_data->hardware_gpio_status.pull = (tmp_group_pull_data >> tmp_val) & 0x03;
			if(tmp_user_gpio_data->pull >= 0) {
				tmp_group_pull_data &= ~(                           0x03  << tmp_val);
				tmp_group_pull_data |=  (tmp_user_gpio_data->pull & 0x03) << tmp_val;
				pull_change = 1;
			}
		}
		// 更新将要写入drv level reg的值
		if(tmp_user_gpio_data->drv_level >= 0) { /* tmp_val is as above */
			tmp_sys_gpio_data->hardware_gpio_status.drv_level = (tmp_group_dlevel_data >> tmp_val) & 0x03;
			if(tmp_user_gpio_data->drv_level >= 0) {
				tmp_group_dlevel_data &= ~(                                0x03  << tmp_val);
				tmp_group_dlevel_data |=  (tmp_user_gpio_data->drv_level & 0x03) << tmp_val;
				dlevel_change = 1;
			}
		}
#endif

	// 根据用户输入，以及功能分配决定是否更新data寄存器
#if 1
		if(tmp_user_gpio_data->mul_sel == 1) {
			if(tmp_user_gpio_data->data >= 0)
				__gpio_set_value(pio_index, tmp_user_gpio_data->data);
		}
#else
		if(tmp_user_gpio_data->mul_sel == 1) {
			if(tmp_user_gpio_data->data >= 0) {
				tmp_val = tmp_user_gpio_data->data;
				tmp_val &= 1;
				tmp_group_data_data &= ~(1 << port_num);
				tmp_group_data_data |= tmp_val << port_num;
				data_change = 1;
			}
		}
#endif
	}

#if 0
	// for循环结束，如果存在还没有回写的寄存器，这里写回到硬件当中
	if(tmp_group_func_addr) {			//只要更新过寄存器地址，就可以对硬件赋值
							//那么把所有的值全部回写到硬件寄存器
	        *tmp_group_func_addr   = tmp_group_func_data;       //回写功能寄存器
		if(pull_change) {
			*tmp_group_pull_addr   = tmp_group_pull_data;    //回写pull寄存器
		}
		if(dlevel_change) {
			*tmp_group_dlevel_addr = tmp_group_dlevel_data;  //回写driver level寄存器
		}
		if(data_change) {
			*tmp_group_data_addr   = tmp_group_data_data;    //回写data寄存器
		}
	}
#endif

End:
	if(0 != usign) {
		PIO_ERR("%s err, line %d", __FUNCTION__, usign);

		/* free buf if err */
		if(NULL != user_gpio_buf) {
			CSP_OSAL_FREE(user_gpio_buf);
			user_gpio_buf = NULL;
		}
		return 0;
	} else
		return (u32)user_gpio_buf;
}
EXPORT_SYMBOL_GPL(sw_gpio_request);

/**
 * sw_gpio_request_ex - gpio request
 * @main_name: 传进的主键名称，匹配模块(驱动名称)
 * @sub_name: 传进的子键名称，如果是空，表示全部，否则寻找到匹配的单独GPIO
 *
 * return gpio handle if success, 0 if failed
 */
u32 sw_gpio_request_ex(char *main_name, const char *sub_name)  //设备申请GPIO函数扩展接口
{
	user_gpio_set_t    *gpio_list=NULL;
	user_gpio_set_t     one_gpio;
	u32               gpio_handle;
	s32               gpio_count;

	if(!sub_name) {
		gpio_count = script_parser_mainkey_get_gpio_count(main_name);
		if(gpio_count <= 0) {
			printk("err: gpio count < =0 ,gpio_count is: %d \n", gpio_count);
			return 0;
		}
		gpio_list = (user_gpio_set_t *)CSP_OSAL_MALLOC(sizeof(system_gpio_set_t) * gpio_count); //申请一片临时内存，用于保存用户数据
		if(!gpio_list) {
			printk("malloc gpio_list error \n");
			return 0;
		}
		if(!script_parser_mainkey_get_gpio_cfg(main_name,gpio_list,gpio_count)){
			gpio_handle = sw_gpio_request(gpio_list, gpio_count);
			CSP_OSAL_FREE(gpio_list);
		} else {
			return 0;
		}
	} else {
		if(script_parser_fetch((char *)main_name, (char *)sub_name, (int *)&one_gpio, (sizeof(user_gpio_set_t) >> 2)) < 0){
			printk("script parser fetch err. \n");
			return 0;
		}

		gpio_handle = sw_gpio_request(&one_gpio, 1);
	}

	return gpio_handle;
}
EXPORT_SYMBOL(sw_gpio_request_ex);

/**
 * sw_gpio_release - gpio release
 * @p_handler: gpio handler
 * @if_release_to_default_status: 是否释放到原始状态(寄存器原有状态)
 *
 * return EGPIO_SUCCESS if success, EGPIO_FAIL if failed
 */
s32 sw_gpio_release(u32 p_handler, s32 if_release_to_default_status)
{
	char               *tmp_buf;                                        //转换成char类型
	u32               group_count_max, first_port;                    //最大GPIO个数
	system_gpio_set_t  *user_gpio_set, *tmp_sys_gpio_data;
	u32               port, port_num;
	u32               i;

	u32 	usign = 0;
	u32 	pio_index = 0;
	struct gpio_config config_stru = {0};

	// 检查传进的句柄的有效性
	if(!p_handler)
		return EGPIO_FAIL;
	tmp_buf = (char *)p_handler;
	group_count_max = *(int *)tmp_buf;
	if(!group_count_max)
		return EGPIO_FAIL;
	if(if_release_to_default_status == 2) {
		//printk("gpio module :  release p_handler = %x\n",p_handler);
		CSP_OSAL_FREE((char *)p_handler);
		return EGPIO_SUCCESS;
	}
	user_gpio_set = (system_gpio_set_t *)(tmp_buf + 16);

	// 读取用户数据, 找到第一个有效项
	for(first_port = 0; first_port < group_count_max; first_port++) {
		tmp_sys_gpio_data  = user_gpio_set + first_port;
		port     = tmp_sys_gpio_data->port;                 //读出端口数值
		port_num = tmp_sys_gpio_data->port_num;             //读出端口中的某一个GPIO
		if(!port)
			continue;

#if 0
		port_num_func = (port_num >> 3);
		port_num_pull = (port_num >> 4);

		tmp_group_func_addr    = PIO_REG_CFG(port, port_num_func);   //更新功能寄存器地址
		tmp_group_pull_addr    = PIO_REG_PULL(port, port_num_pull);  //更新pull寄存器
		tmp_group_dlevel_addr  = PIO_REG_DLEVEL(port, port_num_pull);//更新level寄存器

		tmp_group_func_data    = *tmp_group_func_addr;
		tmp_group_pull_data    = *tmp_group_pull_addr;
		tmp_group_dlevel_data  = *tmp_group_dlevel_addr;
#endif
		break;
	}
	if(first_port >= group_count_max)
		return 0;

	for(i = first_port; i < group_count_max; i++) {
		tmp_sys_gpio_data  = user_gpio_set + i;             //tmp_sys_gpio_data指向申请的GPIO空间
		port     = tmp_sys_gpio_data->port;                 //读出端口数值
		port_num = tmp_sys_gpio_data->port_num;             //读出端口中的某一个GPIO

#if 1
		/* get the gpio index */
		pio_index = port_to_gpio_index(port, port_num);

		config_stru.gpio = pio_index;
		config_stru.mul_sel = 0; /* input */
		config_stru.pull = tmp_sys_gpio_data->hardware_gpio_status.pull;
		config_stru.drv_level = tmp_sys_gpio_data->hardware_gpio_status.drv_level;
		if(0 != sw_gpio_setall_range(&config_stru, 1)) {
			usign = __LINE__;
			goto End;
		}
#else
		port_num_func = (port_num >> 3);
		port_num_pull = (port_num >> 4);

		if((port_num_pull != pre_port_num_pull) || (port != pre_port)) {   //如果发现当前引脚的端口不一致，或者所在的pull寄存器不一致
			*tmp_group_func_addr   = tmp_group_func_data;    //回写功能寄存器
			*tmp_group_pull_addr   = tmp_group_pull_data;    //回写pull寄存器
			*tmp_group_dlevel_addr = tmp_group_dlevel_data;  //回写driver level寄存器

			tmp_group_func_addr    = PIO_REG_CFG(port, port_num_func);   //更新功能寄存器地址
			tmp_group_pull_addr    = PIO_REG_PULL(port, port_num_pull);   //更新pull寄存器
			tmp_group_dlevel_addr  = PIO_REG_DLEVEL(port, port_num_pull); //更新level寄存器

			tmp_group_func_data    = *tmp_group_func_addr;
			tmp_group_pull_data    = *tmp_group_pull_addr;
			tmp_group_dlevel_data  = *tmp_group_dlevel_addr;
		} else if(pre_port_num_func != port_num_func) {                      //如果发现当前引脚的功能寄存器不一致
			*tmp_group_func_addr   = tmp_group_func_data;                 //则只回写功能寄存器
			tmp_group_func_addr    = PIO_REG_CFG(port, port_num_func);   //更新功能寄存器地址
			tmp_group_func_data    = *tmp_group_func_addr;
		}

		pre_port_num_pull = port_num_pull;
		pre_port_num_func = port_num_func;
		pre_port          = port;
		//更新功能寄存器, 设置为输入
		tmp_group_func_data &= ~(0x07 << ((port_num - (port_num_func<<3)) << 2));
		//更新pull状态寄存器
		tmp_val              =  (port_num - (port_num_pull<<4)) << 1;
		tmp_group_pull_data &= ~(0x03  << tmp_val);
		tmp_group_pull_data |= (tmp_sys_gpio_data->hardware_gpio_status.pull & 0x03) << tmp_val;
		//更新driver状态寄存器
		tmp_val              =  (port_num - (port_num_pull<<4)) << 1;
		tmp_group_dlevel_data &= ~(0x03  << tmp_val);
		tmp_group_dlevel_data |= (tmp_sys_gpio_data->hardware_gpio_status.drv_level & 0x03) << tmp_val;
#endif
	}

#if 0
	if(tmp_group_func_addr) {               //只要更新过寄存器地址，就可以对硬件赋值
						//那么把所有的值全部回写到硬件寄存器
	        *tmp_group_func_addr   = tmp_group_func_data;    //回写功能寄存器
	}
	if(tmp_group_pull_addr) {
	        *tmp_group_pull_addr   = tmp_group_pull_data;
	}
	if(tmp_group_dlevel_addr) {
	        *tmp_group_dlevel_addr = tmp_group_dlevel_data;
	}
#endif

End:
	CSP_OSAL_FREE((char *)p_handler);

	if(0 != usign) {
		PIO_ERR("%s err, line %d", __FUNCTION__, usign);
		return EGPIO_FAIL;
	} else {
		return EGPIO_SUCCESS;
	}
}
EXPORT_SYMBOL(sw_gpio_release);

/**
 * sw_gpio_get_all_pin_status - 获取用户申请过的所有GPIO的状态
 * @p_handler: gpio handler
 * @gpio_status: 保存用户数据的数组
 * @gpio_count_max: 数组最大个数，避免数组越界
 * @if_get_user_set_flag: 读取标志，表示读取用户设定数据或者是实际数据
 *
 * return EGPIO_SUCCESS if success, EGPIO_FAIL if failed
 */
s32  sw_gpio_get_all_pin_status(u32 p_handler, user_gpio_set_t *gpio_status, u32 gpio_count_max, u32 if_get_from_hardware)
{
	char               *tmp_buf;                                        //转换成char类型
	u32               group_count_max, first_port;                    //最大GPIO个数
	system_gpio_set_t  *user_gpio_set, *tmp_sys_gpio_data;
	user_gpio_set_t  *script_gpio;
	u32               port, port_num;
	u32               i;

	u32 	usign = 0;
	u32 	pio_index = 0;
	struct gpio_config config_stru = {0};

	if((!p_handler) || (!gpio_status))
		return EGPIO_FAIL;
	if(gpio_count_max <= 0)
		return EGPIO_FAIL;
	tmp_buf = (char *)p_handler;
	group_count_max = *(int *)tmp_buf;
	if(group_count_max <= 0)
		return EGPIO_FAIL;
	user_gpio_set = (system_gpio_set_t *)(tmp_buf + 16);
	if(group_count_max > gpio_count_max)
		group_count_max = gpio_count_max;

	//读取用户数据
	//表示读取用户给定的数据
	if(!if_get_from_hardware) {
		for(i = 0; i < group_count_max; i++) {
			tmp_sys_gpio_data = user_gpio_set + i;             //tmp_sys_gpio_data指向申请的GPIO空间
			script_gpio       = gpio_status + i;               //script_gpio指向用户传进的空间

			script_gpio->port      = tmp_sys_gpio_data->port;                       //读出port数据
			script_gpio->port_num  = tmp_sys_gpio_data->port_num;                   //读出port_num数据
			script_gpio->pull      = tmp_sys_gpio_data->user_gpio_status.pull;      //读出pull数据
			script_gpio->mul_sel   = tmp_sys_gpio_data->user_gpio_status.mul_sel;   //读出功能数据
			script_gpio->drv_level = tmp_sys_gpio_data->user_gpio_status.drv_level; //读出驱动能力数据
			script_gpio->data      = tmp_sys_gpio_data->user_gpio_status.data;      //读出data数据
			strcpy(script_gpio->gpio_name, tmp_sys_gpio_data->gpio_name);
		}
	} else {
		/* find the first valid port index */
		for(first_port = 0; first_port < group_count_max; first_port++) {
			tmp_sys_gpio_data  = user_gpio_set + first_port;
			port     = tmp_sys_gpio_data->port;
			port_num = tmp_sys_gpio_data->port_num;

			if(!port)
				continue;
#if 0
			port_num_func = (port_num >> 3);
			port_num_pull = (port_num >> 4);
			tmp_group_func_addr    = PIO_REG_CFG(port, port_num_func);   //更新功能寄存器地址
			tmp_group_pull_addr    = PIO_REG_PULL(port, port_num_pull);   //更新pull寄存器
			tmp_group_dlevel_addr  = PIO_REG_DLEVEL(port, port_num_pull); //更新level寄存器
			tmp_group_data_addr    = PIO_REG_DATA(port);                  //更新data寄存器
#endif
			break;
		}
		if(first_port >= group_count_max)
			return 0;

		/* get all pin status from hardware */
		for(i = first_port; i < group_count_max; i++) {
			tmp_sys_gpio_data = user_gpio_set + i;             //tmp_sys_gpio_data指向申请的GPIO空间
			script_gpio       = gpio_status + i;               //script_gpio指向用户传进的空间

			port     = tmp_sys_gpio_data->port;                //读出端口数值
			port_num = tmp_sys_gpio_data->port_num;            //读出端口中的某一个GPIO

			script_gpio->port = port;                          //读出port数据
			script_gpio->port_num  = port_num;                 //读出port_num数据
			strcpy(script_gpio->gpio_name, tmp_sys_gpio_data->gpio_name);

#if 1
			/* get the gpio index */
			pio_index = port_to_gpio_index(port, port_num);

			config_stru.gpio = pio_index;
			if(0 != sw_gpio_getall_range(&config_stru, 1)) {
				usign = __LINE__;
				goto End;
			} else {
				script_gpio->mul_sel = config_stru.mul_sel;	/* get mul sel */
				script_gpio->pull = config_stru.pull;		/* get pull val */
				script_gpio->drv_level = config_stru.drv_level; /* get drv level val */
				if(script_gpio->mul_sel <= 1) /* get data val if cfg is input/output */
					script_gpio->data = __gpio_get_value(pio_index);
				else
					script_gpio->data = -1;
			}
#else
			port_num_func = (port_num >> 3);
			port_num_pull = (port_num >> 4);

			if((port_num_pull != pre_port_num_pull) || (port != pre_port)) {    //如果发现当前引脚的端口不一致，或者所在的pull寄存器不一致
				tmp_group_func_addr    = PIO_REG_CFG(port, port_num_func);   //更新功能寄存器地址
				tmp_group_pull_addr    = PIO_REG_PULL(port, port_num_pull);   //更新pull寄存器
				tmp_group_dlevel_addr  = PIO_REG_DLEVEL(port, port_num_pull); //更新level寄存器
				tmp_group_data_addr    = PIO_REG_DATA(port);                  //更新data寄存器
			}
			else if(pre_port_num_func != port_num_func) {                      //如果发现当前引脚的功能寄存器不一致
				tmp_group_func_addr    = PIO_REG_CFG(port, port_num_func);   //更新功能寄存器地址
			}

			pre_port_num_pull = port_num_pull;
			pre_port_num_func = port_num_func;
			pre_port          = port;
			//给用户控件赋值
			script_gpio->pull      = (*tmp_group_pull_addr   >> ((port_num - (port_num_pull<<4))<<1)) & 0x03;    //读出pull数据
			script_gpio->drv_level = (*tmp_group_dlevel_addr >> ((port_num - (port_num_pull<<4))<<1)) & 0x03;    //读出功能数据
			script_gpio->mul_sel   = (*tmp_group_func_addr   >> ((port_num - (port_num_func<<3))<<2)) & 0x07;    //读出功能数据
			if(script_gpio->mul_sel <= 1) {
				script_gpio->data  = (*tmp_group_data_addr   >>   port_num) & 0x01;                              //读出data数据
			} else {
				script_gpio->data = -1;
			}
#endif
		}
	}

End:
	if(0 != usign) {
		PIO_ERR("%s err, line %d", __FUNCTION__, usign);
		return EGPIO_FAIL;
	} else {
		return EGPIO_SUCCESS;
	}
}
EXPORT_SYMBOL(sw_gpio_get_all_pin_status);

/**
 * sw_gpio_get_one_pin_status - 获取用户申请过的GPIO的状态
 * @p_handler: gpio handler
 * @gpio_status: 保存用户数据的数组
 * @gpio_name: 要操作的GPIO的名称
 * @if_get_user_set_flag: 读取标志，表示读取用户设定数据或者是实际数据
 *
 * return EGPIO_SUCCESS if success, EGPIO_FAIL if failed
 */
s32  sw_gpio_get_one_pin_status(u32 p_handler, user_gpio_set_t *gpio_status, const char *gpio_name, u32 if_get_from_hardware)
{
	char              *tmp_buf;                                        //转换成char类型
	u32               group_count_max;                                //最大GPIO个数
	system_gpio_set_t  *user_gpio_set, *tmp_sys_gpio_data;
	u32               port, port_num;
	u32               i;

	u32 	usign = 0;
	u32 	pio_index = 0;
	struct gpio_config config_stru = {0};

	//检查传进的句柄的有效性
	if((!p_handler) || (!gpio_status))
		return EGPIO_FAIL;
	tmp_buf = (char *)p_handler;
	group_count_max = *(int *)tmp_buf;
	if(group_count_max <= 0)
		return EGPIO_FAIL;
	else if((group_count_max > 1) && (!gpio_name))
		return EGPIO_FAIL;
	user_gpio_set = (system_gpio_set_t *)(tmp_buf + 16);

	//读取用户数据
	//表示读取用户给定的数据
	for(i = 0; i < group_count_max; i++) {
		tmp_sys_gpio_data = user_gpio_set + i;             //tmp_sys_gpio_data指向申请的GPIO空间
		if(strcmp(gpio_name, tmp_sys_gpio_data->gpio_name))
			continue;
		strcpy(gpio_status->gpio_name, tmp_sys_gpio_data->gpio_name);
		port                   = tmp_sys_gpio_data->port;
		port_num               = tmp_sys_gpio_data->port_num;
		gpio_status->port      = port;                                              //读出port数据
		gpio_status->port_num  = port_num;                                          //读出port_num数据

		if(!if_get_from_hardware) {                                                   //当前要求读出用户设计的数据
			gpio_status->mul_sel   = tmp_sys_gpio_data->user_gpio_status.mul_sel;   //从用户传进数据中读出功能数据
			gpio_status->pull      = tmp_sys_gpio_data->user_gpio_status.pull;      //从用户传进数据中读出pull数据
			gpio_status->drv_level = tmp_sys_gpio_data->user_gpio_status.drv_level; //从用户传进数据中读出驱动能力数据
			gpio_status->data      = tmp_sys_gpio_data->user_gpio_status.data;      //从用户传进数据中读出data数据
		} else {                                                                      //当前读出寄存器实际的参数
#if 1
			/* get the gpio index */
			pio_index = port_to_gpio_index(port, port_num);

			config_stru.gpio = pio_index;
			if(0 != sw_gpio_getall_range(&config_stru, 1)) {
				usign = __LINE__;
				goto End;
			} else {
				gpio_status->mul_sel = config_stru.mul_sel;	/* get mul sel */
				gpio_status->pull = config_stru.pull;		/* get pull val */
				gpio_status->drv_level = config_stru.drv_level; /* get drv level val */
				if(gpio_status->mul_sel <= 1) /* get data val if cfg is input/output */
					gpio_status->data = __gpio_get_value(pio_index);
				else
					gpio_status->data = -1;
			}
#else
			port_num_func = (port_num >> 3);
			port_num_pull = (port_num >> 4);

			tmp_val1 = ((port_num - (port_num_func << 3)) << 2);
			tmp_val2 = ((port_num - (port_num_pull << 4)) << 1);
			gpio_status->mul_sel   = (PIO_REG_CFG_VALUE(port, port_num_func)>>tmp_val1) & 0x07;       //从硬件中读出功能寄存器
			gpio_status->pull      = (PIO_REG_PULL_VALUE(port, port_num_pull)>>tmp_val2) & 0x03;      //从硬件中读出pull寄存器
			gpio_status->drv_level = (PIO_REG_DLEVEL_VALUE(port, port_num_pull)>>tmp_val2) & 0x03;    //从硬件中读出level寄存器
			if(gpio_status->mul_sel <= 1)
			{
				gpio_status->data = (PIO_REG_DATA_VALUE(port) >> port_num) & 0x01;                     //从硬件中读出data寄存器
			}
			else
			{
				gpio_status->data = -1;
			}
#endif
		}
		break;
	}

End:
	if(0 != usign) {
		PIO_ERR("%s err, line %d", __FUNCTION__, usign);
		return EGPIO_FAIL;
	} else {
		return EGPIO_SUCCESS;
	}
}
EXPORT_SYMBOL(sw_gpio_get_one_pin_status);

/**
 * sw_gpio_set_one_pin_status - 设置用户申请过的GPIO的某一个的状态
 * @p_handler: gpio handler
 * @gpio_status: 保存用户数据的数组
 * @gpio_name: 要操作的GPIO的名称
 * @if_set_to_current_input_status: 读取标志，表示读取用户设定数据或者是实际数据
 *
 * return EGPIO_SUCCESS if success, EGPIO_FAIL if failed
 */
s32  sw_gpio_set_one_pin_status(u32 p_handler, user_gpio_set_t *gpio_status, const char *gpio_name, u32 if_set_to_current_input_status)
{
	char               *tmp_buf;                                        //转换成char类型
	u32               group_count_max;                                //最大GPIO个数
	system_gpio_set_t  *user_gpio_set, *tmp_sys_gpio_data;
	user_gpio_set_t     script_gpio;
	u32               port, port_num;
	u32               i;

	u32 	usign = 0;
	u32 	pio_index = 0;

	//检查传进的句柄的有效性
	if((!p_handler) || (!gpio_name))
		return EGPIO_FAIL;
	if((if_set_to_current_input_status) && (!gpio_status))
		return EGPIO_FAIL;
	tmp_buf = (char *)p_handler;
	group_count_max = *(int *)tmp_buf;
	if(group_count_max <= 0)
		return EGPIO_FAIL;
	user_gpio_set = (system_gpio_set_t *)(tmp_buf + 16);

	//读取用户数据
	//表示读取用户给定的数据
	for(i = 0; i < group_count_max; i++) {
		tmp_sys_gpio_data = user_gpio_set + i;             //tmp_sys_gpio_data指向申请的GPIO空间
		if(strcmp(gpio_name, tmp_sys_gpio_data->gpio_name))
			continue;

		port          = tmp_sys_gpio_data->port;                           //读出port数据
		port_num      = tmp_sys_gpio_data->port_num;                       //读出port_num数据

		if(if_set_to_current_input_status) {                               //根据当前用户设定修正
			//修改FUCN寄存器
			script_gpio.mul_sel   = gpio_status->mul_sel;
			script_gpio.pull      = gpio_status->pull;
			script_gpio.drv_level = gpio_status->drv_level;
			script_gpio.data      = gpio_status->data;
		} else {
			script_gpio.mul_sel   = tmp_sys_gpio_data->user_gpio_status.mul_sel;
			script_gpio.pull      = tmp_sys_gpio_data->user_gpio_status.pull;
			script_gpio.drv_level = tmp_sys_gpio_data->user_gpio_status.drv_level;
			script_gpio.data      = tmp_sys_gpio_data->user_gpio_status.data;
		}

#if 1
		/* get the gpio index */
		pio_index = port_to_gpio_index(port, port_num);
#endif

		if(script_gpio.mul_sel >= 0) {
#if 1
			/* set cfg */
			if(0 != sw_gpio_setcfg(pio_index, script_gpio.mul_sel)) {
				usign = __LINE__;
				goto End;
			}
#else
			tmp_addr = PIO_REG_CFG(port, port_num_func);
			reg_val = *tmp_addr;                                                       //修改FUNC寄存器
			tmp_val = (port_num - (port_num_func<<3))<<2;
			reg_val &= ~(0x07 << tmp_val);
			reg_val |=  (script_gpio.mul_sel) << tmp_val;
			*tmp_addr = reg_val;
#endif
		}

		//修改PULL寄存器
		if(script_gpio.pull >= 0) {
#if 1
			if(0 != sw_gpio_setpull(pio_index, script_gpio.pull)) {
				usign = __LINE__;
				goto End;
			}
#else
		tmp_addr = PIO_REG_PULL(port, port_num_pull);
		reg_val = *tmp_addr;                                                     //修改FUNC寄存器
		tmp_val = (port_num - (port_num_pull<<4))<<1;
		reg_val &= ~(0x03 << tmp_val);
		reg_val |=  (script_gpio.pull) << tmp_val;
		            *tmp_addr = reg_val;
#endif
		}

		//修改DLEVEL寄存器
		if(script_gpio.drv_level >= 0) {
#if 1
			if(0 != sw_gpio_setdrvlevel(pio_index, script_gpio.drv_level)) {
				usign = __LINE__;
				goto End;
			}
#else
			tmp_addr = PIO_REG_DLEVEL(port, port_num_pull);
			reg_val = *tmp_addr;                                                         //修改FUNC寄存器
			tmp_val = (port_num - (port_num_pull<<4))<<1;
			reg_val &= ~(0x03 << tmp_val);
			reg_val |=  (script_gpio.drv_level) << tmp_val;
			*tmp_addr = reg_val;
#endif
		}

		//修改data寄存器
		if(script_gpio.mul_sel == 1) {
			if(script_gpio.data >= 0) {
#if 1
				__gpio_set_value(pio_index, script_gpio.data);
#else
				tmp_addr = PIO_REG_DATA(port);
				reg_val = *tmp_addr;                                                      //修改DATA寄存器
				reg_val &= ~(0x01 << port_num);
				reg_val |=  (script_gpio.data & 0x01) << port_num;
				                *tmp_addr = reg_val;
#endif
			}
		}
break;
	}

End:
	if(0 != usign) {
		PIO_ERR("%s err, line %d", __FUNCTION__, usign);
		return EGPIO_FAIL;
	} else {
		return EGPIO_SUCCESS;
	}
}
EXPORT_SYMBOL(sw_gpio_set_one_pin_status);

/**
 * sw_gpio_set_one_pin_io_status - 修改用户申请过的GPIO中的某一个IO口的，输入输出状态
 * @p_handler: gpio handler
 * @if_set_to_output_status: 设置成输出状态还是输入状态
 * @gpio_name: 要操作的GPIO的名称
 *
 * return EGPIO_SUCCESS if success, EGPIO_FAIL if failed
 */
s32  sw_gpio_set_one_pin_io_status(u32 p_handler, u32 if_set_to_output_status, const char *gpio_name)
{
	char               *tmp_buf;                                        //转换成char类型
	u32               group_count_max;                                //最大GPIO个数
	system_gpio_set_t  *user_gpio_set = NULL, *tmp_sys_gpio_data;
	u32               port, port_num;
	u32                i;

	u32 	ucfg = 0;
	u32 	pio_index = 0;

	//检查传进的句柄的有效性
	if(!p_handler)
		return EGPIO_FAIL;
	if(if_set_to_output_status > 1)
		return EGPIO_FAIL;
	tmp_buf = (char *)p_handler;
	group_count_max = *(int *)tmp_buf;
	tmp_sys_gpio_data = (system_gpio_set_t *)(tmp_buf + 16);
	if(group_count_max == 0) {
		return EGPIO_FAIL;
	} else if(group_count_max == 1) {
		user_gpio_set = tmp_sys_gpio_data;
	} else if(gpio_name) {
		for(i=0; i<group_count_max; i++) {
			if(strcmp(gpio_name, tmp_sys_gpio_data->gpio_name)) {
				tmp_sys_gpio_data ++;
				continue;
			}
			user_gpio_set = tmp_sys_gpio_data;
			break;
		}
	}
	if(!user_gpio_set)
		return EGPIO_FAIL;

	port     = user_gpio_set->port;
	port_num = user_gpio_set->port_num;

#if 1
	/* get the gpio index */
	pio_index = port_to_gpio_index(port, port_num);

	/* set cfg */
	ucfg = (if_set_to_output_status ? 1 : 0);
	if(0 != sw_gpio_setcfg(pio_index, ucfg)) {
		PIO_ERR_FUN_LINE;
		return EGPIO_FAIL;
	}
#else
	port_num_func = port_num >> 3;

	tmp_group_func_addr = PIO_REG_CFG(port, port_num_func);
	reg_val = *tmp_group_func_addr;
	reg_val &= ~(0x07 << (((port_num - (port_num_func<<3))<<2)));
	reg_val |=   if_set_to_output_status << (((port_num - (port_num_func<<3))<<2));
	*tmp_group_func_addr = reg_val;
#endif

return EGPIO_SUCCESS;
}
EXPORT_SYMBOL(sw_gpio_set_one_pin_io_status);

/**
 * sw_gpio_set_one_pin_pull - 修改用户申请过的GPIO中的某一个IO口的，PULL状态
 * @p_handler: gpio handler
 * @if_set_to_output_status: 所设置的pull状态
 * @gpio_name: 要操作的GPIO的名称
 *
 * return EGPIO_SUCCESS if success, EGPIO_FAIL if failed
 */
s32  sw_gpio_set_one_pin_pull(u32 p_handler, u32 set_pull_status, const char *gpio_name)
{
	char               *tmp_buf;                                        //转换成char类型
	u32               group_count_max;                                //最大GPIO个数
	system_gpio_set_t  *user_gpio_set = NULL, *tmp_sys_gpio_data;
	u32               port, port_num;
	u32                i;

	u32 	pio_index = 0;

	//检查传进的句柄的有效性
	if(!p_handler)
		return EGPIO_FAIL;
	if(set_pull_status >= 4)
		return EGPIO_FAIL;
	tmp_buf = (char *)p_handler;
	group_count_max = *(int *)tmp_buf;
	tmp_sys_gpio_data = (system_gpio_set_t *)(tmp_buf + 16);
	if(group_count_max == 0) {
		return EGPIO_FAIL;
	} else if(group_count_max == 1)	{
		user_gpio_set = tmp_sys_gpio_data;
	} else if(gpio_name) {
		for(i=0; i<group_count_max; i++) {
			if(strcmp(gpio_name, tmp_sys_gpio_data->gpio_name)) {
				tmp_sys_gpio_data ++;
				continue;
			}
			user_gpio_set = tmp_sys_gpio_data;
			break;
		}
	}
	if(!user_gpio_set)
		return EGPIO_FAIL;

	port     = user_gpio_set->port;
	port_num = user_gpio_set->port_num;

#if 1
	/* get the gpio index */
	pio_index = port_to_gpio_index(port, port_num);

	if(0 != sw_gpio_setpull(pio_index, set_pull_status)) {
		PIO_ERR_FUN_LINE;
		return EGPIO_FAIL;
	}
#else
	port_num_pull = port_num >> 4;

	tmp_group_pull_addr = PIO_REG_PULL(port, port_num_pull);
	reg_val = *tmp_group_pull_addr;
	reg_val &= ~(0x03 << (((port_num - (port_num_pull<<4))<<1)));
	reg_val |=  (set_pull_status << (((port_num - (port_num_pull<<4))<<1)));
	*tmp_group_pull_addr = reg_val;
#endif

return EGPIO_SUCCESS;
}
EXPORT_SYMBOL(sw_gpio_set_one_pin_pull);

/**
 * sw_gpio_set_one_pin_driver_level - 修改用户申请过的GPIO中的某一个IO口的，驱动能力
 * @p_handler: gpio handler
 * @set_driver_level: 所设置的驱动能力等级
 * @gpio_name: 要操作的GPIO的名称
 *
 * return EGPIO_SUCCESS if success, EGPIO_FAIL if failed
 */
s32  sw_gpio_set_one_pin_driver_level(u32 p_handler, u32 set_driver_level, const char *gpio_name)
{
	char               *tmp_buf;                                        //转换成char类型
	u32               group_count_max;                                //最大GPIO个数
	system_gpio_set_t  *user_gpio_set = NULL, *tmp_sys_gpio_data;
	u32               port, port_num;
	u32                i;

	u32 	pio_index = 0;

	//检查传进的句柄的有效性
	if(!p_handler)
		return EGPIO_FAIL;
	if(set_driver_level >= 4)
		return EGPIO_FAIL;
	tmp_buf = (char *)p_handler;
	group_count_max = *(int *)tmp_buf;
	tmp_sys_gpio_data = (system_gpio_set_t *)(tmp_buf + 16);

	if(group_count_max == 0) {
		return EGPIO_FAIL;
	} else if(group_count_max == 1)	{
		user_gpio_set = tmp_sys_gpio_data;
	} else if(gpio_name) {
		for(i=0; i<group_count_max; i++) {
			if(strcmp(gpio_name, tmp_sys_gpio_data->gpio_name)) {
				tmp_sys_gpio_data ++;
				continue;
			}
			user_gpio_set = tmp_sys_gpio_data;
			break;
		}
	}
	if(!user_gpio_set)
		return EGPIO_FAIL;

	port     = user_gpio_set->port;
	port_num = user_gpio_set->port_num;

#if 1
	/* get the gpio index */
	pio_index = port_to_gpio_index(port, port_num);

	if(0 != sw_gpio_setdrvlevel(pio_index, set_driver_level)) {
		PIO_ERR_FUN_LINE;
		return EGPIO_FAIL;
	}
#else
	port_num_dlevel = port_num >> 4;

	tmp_group_dlevel_addr = PIO_REG_DLEVEL(port, port_num_dlevel);
	reg_val = *tmp_group_dlevel_addr;
	reg_val &= ~(0x03 << (((port_num - (port_num_dlevel<<4))<<1)));
	reg_val |=  (set_driver_level << (((port_num - (port_num_dlevel<<4))<<1)));
	*tmp_group_dlevel_addr = reg_val;
#endif

return EGPIO_SUCCESS;
}
EXPORT_SYMBOL(sw_gpio_set_one_pin_driver_level);

/**
 * sw_gpio_read_one_pin_value - 读取用户申请过的GPIO中的某一个IO口的端口的电平
 * @p_handler: gpio handler
 * @gpio_name: 要操作的GPIO的名称
 *
 * return the pin value if success, EGPIO_FAIL if failed
 */
s32  sw_gpio_read_one_pin_value(u32 p_handler, const char *gpio_name)
{
	char               *tmp_buf;                                        //转换成char类型
	u32               group_count_max;                                //最大GPIO个数
	system_gpio_set_t  *user_gpio_set = NULL, *tmp_sys_gpio_data;
	u32               port, port_num;
	u32                i;

	u32 	ucfg = 0;
	u32 	pio_index = 0;

	//检查传进的句柄的有效性
	if(!p_handler)
		return EGPIO_FAIL;
	tmp_buf = (char *)p_handler;
	group_count_max = *(int *)tmp_buf;
	tmp_sys_gpio_data = (system_gpio_set_t *)(tmp_buf + 16);

	if(group_count_max == 0) {
		return EGPIO_FAIL;
	} else if(group_count_max == 1)	{
		user_gpio_set = tmp_sys_gpio_data;
	} else if(gpio_name) {
		for(i=0; i<group_count_max; i++) {
			if(strcmp(gpio_name, tmp_sys_gpio_data->gpio_name)) {
				tmp_sys_gpio_data ++;
				continue;
			}
			user_gpio_set = tmp_sys_gpio_data;
			break;
		}
	}
	if(!user_gpio_set)
		return EGPIO_FAIL;

	port     = user_gpio_set->port;
	port_num = user_gpio_set->port_num;

#if 1
	/* get the gpio index */
	pio_index = port_to_gpio_index(port, port_num);

	ucfg = sw_gpio_getcfg(pio_index);
	if(0 == ucfg)
		return __gpio_get_value(pio_index);
#else
	port_num_func = port_num >> 3;

	reg_val  = PIO_REG_CFG_VALUE(port, port_num_func);
	func_val = (reg_val >> ((port_num - (port_num_func<<3))<<2)) & 0x07;
	if(func_val == 0)
	{
		reg_val = (PIO_REG_DATA_VALUE(port) >> port_num) & 0x01;

		return reg_val;
	}
#endif

	PIO_ERR_FUN_LINE;
	return EGPIO_FAIL;
}
EXPORT_SYMBOL(sw_gpio_read_one_pin_value);

/**
 * sw_gpio_write_one_pin_value - 修改用户申请过的GPIO中的某一个IO口的端口的电平
 * @p_handler: gpio handler
 * @value_to_gpio:  要设置的电平的电压
 * @gpio_name: 要操作的GPIO的名称
 *
 * return EGPIO_SUCCESS if success, EGPIO_FAIL if failed
 */
s32  sw_gpio_write_one_pin_value(u32 p_handler, u32 value_to_gpio, const char *gpio_name)
{
	char              *tmp_buf;                                        //转换成char类型
	u32               group_count_max;                                //最大GPIO个数
	system_gpio_set_t *user_gpio_set = NULL, *tmp_sys_gpio_data;
	u32               port, port_num;
	u32               i;

	u32 	ucfg = 0;
	u32 	uval = 0;
	u32 	pio_index = 0;

	//检查传进的句柄的有效性
	if(!p_handler)
		return EGPIO_FAIL;
	if(value_to_gpio >= 2)
		return EGPIO_FAIL;
	tmp_buf = (char *)p_handler;
	group_count_max = *(int *)tmp_buf;
	tmp_sys_gpio_data = (system_gpio_set_t *)(tmp_buf + 16);

	if(group_count_max == 0) {
		return EGPIO_FAIL;
	} else if(group_count_max == 1)	{
		user_gpio_set = tmp_sys_gpio_data;
	} else if(gpio_name) {
		for(i=0; i<group_count_max; i++) {
			if(strcmp(gpio_name, tmp_sys_gpio_data->gpio_name)) {
				tmp_sys_gpio_data ++;
				continue;
			}
			user_gpio_set = tmp_sys_gpio_data;
			break;
		}
	}
	if(!user_gpio_set)
		return EGPIO_FAIL;

	port     = user_gpio_set->port;
	port_num = user_gpio_set->port_num;

#if 1
	/* get the gpio index */
	pio_index = port_to_gpio_index(port, port_num);

	ucfg = sw_gpio_getcfg(pio_index);
	if(1 == ucfg) {
		uval = (value_to_gpio ? 1 : 0);
		__gpio_set_value(pio_index, value_to_gpio);
		return EGPIO_SUCCESS;
	}
#else
	port_num_func = port_num >> 3;

	reg_val  = PIO_REG_CFG_VALUE(port, port_num_func);
	func_val = (reg_val >> ((port_num - (port_num_func<<3))<<2)) & 0x07;
	if(func_val == 1)
	{
		tmp_group_data_addr = PIO_REG_DATA(port);
		reg_val = *tmp_group_data_addr;
		reg_val &= ~(1 << port_num);
		reg_val |=  (value_to_gpio << port_num);
		*tmp_group_data_addr = reg_val;

		return EGPIO_SUCCESS;
	}
#endif

	return EGPIO_FAIL;
}
EXPORT_SYMBOL(sw_gpio_write_one_pin_value);

/**
 * sw_gpio_get_index - get the global gpio index
 * @p_handler: gpio handler
 * @gpio_name: gpio name whose index will be got. when NULL,
? 		the first port of p_handler willbe treated.
 *
 * return the gpio index for the port, GPIO_INDEX_INVALID indicate err
 */
u32 sw_gpio_get_index(u32 p_handler, const char *gpio_name)
{
	char		*tmp_buf;                                        //转换成char类型
	u32		group_count_max;                                //最大GPIO个数
	system_gpio_set_t *user_gpio_set = NULL;
	u32		port, port_num;
	u32		i;
	u32		usign = 0;
	u32 		pio_index = 0;

	if(!p_handler) {
		usign = __LINE__;
		goto End;
	}
	tmp_buf = (char *)p_handler;
	group_count_max = *(int *)tmp_buf;
	if(0 == group_count_max) {
		usign = __LINE__;
		goto End;
	}

	user_gpio_set = (system_gpio_set_t *)(tmp_buf + 16);
	if(NULL == gpio_name) { /* when name is NULL, treat the first one */
		port     = user_gpio_set->port;
		port_num = user_gpio_set->port_num;
	} else {
		for(i = 0; i < group_count_max; i++, user_gpio_set++) {
			if(!strcmp(gpio_name, user_gpio_set->gpio_name)) {
				port     = user_gpio_set->port;
				port_num = user_gpio_set->port_num;
				break;
			}
		}
		if(i == group_count_max) { /* cannot find the gpio_name item */
			usign = __LINE__;
			goto End;
		}
	}

	/* get the gpio index */
	pio_index = port_to_gpio_index(port, port_num);
End:
	if(0 != usign) {
		PIO_ERR("%s err, line %d", __FUNCTION__, usign);
		return GPIO_INDEX_INVALID;
	} else {
		return pio_index;
	}
}
EXPORT_SYMBOL(sw_gpio_get_index);

/**
 * sw_gpio_port_to_index - get gpio index from port and prot_num
 * @port: gpio port group index, eg: 1 for PA, 2 for PB...
 * @port_num: port index in gpio group, eg: 0 for PA0, 1 for PA1...
 *
 * return the gpio index for the port, GPIO_INDEX_INVALID indicate err
 */
u32 sw_gpio_port_to_index(u32 port, u32 port_num)
{
	return port_to_gpio_index(port, port_num);
}
EXPORT_SYMBOL(sw_gpio_port_to_index);
