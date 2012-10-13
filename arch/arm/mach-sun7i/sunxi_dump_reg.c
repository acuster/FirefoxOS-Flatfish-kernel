/*
 * arch/arm/mach-sun7i/clock/sunxi_dump_reg.c
 * (C) Copyright 2010-2015
 * Reuuimlla Technology Co., Ltd. <www.reuuimllatech.com>
 * liugang <liugang@reuuimllatech.com>
 *
 * sunxi dump sysfs driver
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 */
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/kdev_t.h>
#include <linux/clk.h>
#include <linux/io.h>
#include <linux/slab.h>

#include <mach/includes.h>

#if 1
    #define DUMP_DBG(format, args...)		printk(format, ##args)
    #define DUMP_INF(format, args...)		printk(format, ##args)
    #define DUMP_ERR(format, args...)		printk(format, ##args)
#else
    #define DUMP_DBG(...)
    #define DUMP_INF(format, args...)		printk(format, ##args) /* should not be NULL */
    #define DUMP_ERR(...)			printk(format, ##args)
#endif

/**
 * is_reg - check if the addr is reg addr
 * @addr: addr to judge
 *
 * return true if the addr is register addr, false if not.
 */
bool is_reg(u32 addr)
{
	if(addr >= AW_IO_PHYS_BASE && addr < AW_IO_PHYS_BASE + AW_IO_SIZE)
		return true;
	if(addr >= AW_SRAM_A1_BASE && addr < AW_SRAM_A1_BASE + AW_SRAM_A1_SIZE)
		return true;
	if(addr >= AW_SRAM_A2_BASE && addr < AW_SRAM_A2_BASE + AW_SRAM_A2_SIZE)
		return true;
	return false;
}

/**
 * first_str_to_int - convert substring of pstr to int, the substring is
 * 		from head of pstr to the first occurance of ch in pstr
 * @pstr: the string to convert
 * @ch: a char in pstr
 * @pout: store the convert result
 *
 * return the first occurance of ch in pstr on success, NULL if failed.
 */
char * first_str_to_u32(char *pstr, char ch, u32 *pout)
{
	char 	*pret = NULL;
	char 	str_tmp[260] = {0};

	pret = strchr(pstr, ch);
	if(NULL != pret) {
		memcpy(str_tmp, pstr, pret - pstr);
		if(strict_strtoul(str_tmp, 16, (long unsigned int *)pout)) {
			DUMP_ERR("%s err, line %d\n", __func__, __LINE__);
			return NULL;
		}
	} else
		*pout = 0;

	return pret;
}

/**
 * parse_dump_str - parse the input string for dump attri.
 * @buf:     the input string, eg: "0x01c20000,0x01c20300".
 * @size:    buf size.
 * @start:   store the start reg's addr parsed from buf, eg 0x01c20000.
 * @end:     store the end reg's addr parsed from buf, eg 0x01c20300.
 *
 * return 0 if success, otherwise failed.
 */
int parse_dump_str(const char *buf, size_t size, u32 *start, u32 *end)
{
	char 	*ptr = (char *)buf;

	if(NULL == strchr(ptr, ',')) { /* only one reg to dump */
		if(strict_strtoul(ptr, 16, (long unsigned int *)start))
			return -EINVAL;
		*end = *start;
		return 0;
	}

	ptr = first_str_to_u32(ptr, ',', start);
	if(NULL == ptr)
		return -EINVAL;

	ptr += 1;
	if(strict_strtoul(ptr, 16, (long unsigned int *)end))
		return -EINVAL;

	return 0;
}

/**
 * sunxi_dump_regs - dump a range of registers' value.
 * @start_reg:   physcal address of start reg.
 * @end_reg:     physcal address of end reg.
 */
void sunxi_dump_regs(u32 start_reg, u32 end_reg)
{
	int 	itemp;
	u32 	first_addr = 0, end_addr = 0;

	if(start_reg == end_reg) { /* only one to dump */
		DUMP_INF("0x%08x: 0x%08x\n", start_reg, readl(IO_ADDRESS(start_reg)));
		return;
	}

	first_addr = start_reg & (~0xf);
	end_addr   = end_reg   & (~0xf);

	DUMP_INF("0x%08x: ", first_addr);

	for(itemp = first_addr; itemp < end_addr + 0xf; itemp += 4) {
		if(itemp < start_reg || itemp > end_reg)
			DUMP_INF("           "); /* "0x12345678 ", 11 space*/
		else
			DUMP_INF("0x%08x ", readl(IO_ADDRESS(itemp)));

		if((itemp & 0xc) == 0xc) {
			DUMP_INF("\n");
			if(itemp + 4 < end_addr + 0xf) /* avoid the last blank line */
				DUMP_INF("0x%08x: ", itemp + 4);
		}
	}
}
EXPORT_SYMBOL(sunxi_dump_regs);

/**
 * dump_store - store func of dump attribute.
 * @class:   class ptr.
 * @attr:    attribute ptr.
 * @buf:     the input buf which contain the start and end reg. eg: "0x01c20000,0x01c20100\n"
 * @size:    buf size.
 */
ssize_t dump_store(struct class *class, struct class_attribute *attr,
			const char *buf, size_t size)
{
	u32 	start_reg = 0, end_reg = 0;

	if(0 != parse_dump_str((char *)buf, size, &start_reg, &end_reg)) {
		DUMP_ERR("%s err, invalid para, parse_dump_str failed\n", __func__);
		return -EINVAL;
	}
	//DUMP_INF("%s: get start_reg 0x%08x, end_reg 0x%08x\n", __func__, start_reg, end_reg);

	if(!is_reg(start_reg) || !is_reg(end_reg)) {
		DUMP_ERR("%s err, invalid para, the addr is not reg\n", __func__);
		return -EINVAL;
	}

	sunxi_dump_regs(start_reg, end_reg);
	return size;
}

/**
 * parse_compare_str - parse the input string for compare attri.
 * @str:     string to be parsed, eg: "0x01c20000 0x80000011 0x00000011".
 * @reg_addr:   store the reg address. eg: 0x01c20000.
 * @val_expect: store the expect value. eg: 0x80000011.
 * @val_mask:   store the mask value. eg: 0x00000011.
 *
 * return 0 if success, otherwise failed.
 */
int parse_compare_str(char *str, u32 *reg_addr,
		u32 *val_expect, u32 *val_mask)
{
	char *ptr = str;

	ptr = first_str_to_u32(ptr, ' ', reg_addr);
	if(NULL == ptr)
		return -EINVAL;

	ptr += 1;
	ptr = first_str_to_u32(ptr, ' ', val_expect);
	if(NULL == ptr)
		return -EINVAL;

	ptr += 1;
	if(strict_strtoul(ptr, 16, (long unsigned int *)val_mask))
		return -EINVAL;

	return 0;
}

/**
 * compare_item_init - init for compare attri. parse input string, and construct compare struct.
 * @buf:     the input string, eg: "0x01c20000 0x80000011 0x00000011,0x01c20004 0x0000c0a4 0x0000c0a0...".
 * @size:    buf size.
 * @ppgroup: store the struct allocated, the struct contains items parsed from input buf.
 *
 * return 0 if success, otherwise failed.
 */
int compare_item_init(const char *buf, size_t size, struct compare_group **ppgroup)
{
	int 	itemp = 0;
	char 	str_temp[256] = {0};
	char 	*ptr = NULL, *ptr2 = NULL;
	u32 	reg_addr = 0, val_expect = 0, val_mask = 0;
	struct compare_group *pgroup = NULL;

	/* alloc item buffer */
	pgroup = kmalloc(sizeof(struct compare_group), GFP_KERNEL);
	if(NULL == pgroup)
		return -EINVAL;
	pgroup->pitem = kmalloc(sizeof(struct compare_item) * MAX_COMPARE_ITEM, GFP_KERNEL);
	if(NULL == pgroup->pitem) {
		kfree(pgroup);
		return -EINVAL;
	}

	pgroup->num = 0;

	/* get item from buf */
	ptr = (char *)buf;
	while((ptr2 = strchr(ptr, ',')) != NULL) {
		itemp = ptr2 - ptr;
		memcpy(str_temp, ptr, itemp);
		str_temp[itemp] = 0;
		if(0 != parse_compare_str(str_temp, &reg_addr, &val_expect, &val_mask))
			DUMP_ERR("%s err, line %d, str_temp %s\n", __func__, __LINE__, str_temp);
		else {
			//DUMP_DBG("%s: reg_addr 0x%08x, val_expect 0x%08x, val_mask 0x%08x\n",
			//	__func__, reg_addr, val_expect, val_mask);
			if(pgroup->num < MAX_COMPARE_ITEM) {
				pgroup->pitem[pgroup->num].reg_addr = reg_addr;
				pgroup->pitem[pgroup->num].val_expect = val_expect;
				pgroup->pitem[pgroup->num].val_mask = val_mask;
				pgroup->num++;
			} else {
				DUMP_ERR("%s err, line %d, pgroup->num %d exceed %d\n",
					__func__, __LINE__, pgroup->num, MAX_COMPARE_ITEM);
				break;
			}
		}

		ptr = ptr2 + 1;
	}

	/* the last item */
	if(0 != parse_compare_str(ptr, &reg_addr, &val_expect, &val_mask))
		DUMP_ERR("%s err, line %d, ptr %s\n", __func__, __LINE__, ptr);
	else {
		//DUMP_DBG("%s: line %d, reg_addr 0x%08x, val_expect 0x%08x, val_mask 0x%08x\n",
		//	__func__, __LINE__, reg_addr, val_expect, val_mask);
		if(pgroup->num < MAX_COMPARE_ITEM) {
			pgroup->pitem[pgroup->num].reg_addr = reg_addr;
			pgroup->pitem[pgroup->num].val_expect = val_expect;
			pgroup->pitem[pgroup->num].val_mask = val_mask;
			pgroup->num++;
		}
	}

	/* free buffer if no valid item */
	if(0 == pgroup->num) {
		kfree(pgroup->pitem);
		kfree(pgroup);
		return -EINVAL;
	}

	*ppgroup = pgroup;
	return 0;
}

/**
 * compare_item_deinit - release memory that created by compare_item_init.
 * @pgroup: the compare struct allocated in compare_item_init.
 */
void compare_item_deinit(struct compare_group *pgroup)
{
	if(NULL != pgroup) {
		if(NULL != pgroup->pitem)
			kfree(pgroup->pitem);
		kfree(pgroup);
	}
}

/**
 * sunxi_compare_regs - dump values for compare items.
 * @pgroup: the compare struct which contain items that will be dumped.
 */
void sunxi_compare_regs(struct compare_group *pgroup)
{
	int 	i = 0;
	u32 	reg = 0, expect = 0, actual = 0, mask = 0;

	DUMP_DBG("reg         expect      actual      mask        result\n");
	for(i = 0; i < pgroup->num; i++) {
		reg    = pgroup->pitem[i].reg_addr;
		expect = pgroup->pitem[i].val_expect;
		actual = readl(IO_ADDRESS(reg));
		mask   = pgroup->pitem[i].val_mask;
		if((actual & mask) == (expect & mask))
			DUMP_DBG("0x%08x  0x%08x  0x%08x  0x%08x  OK\n", reg, expect, actual, mask);
		else
			DUMP_DBG("0x%08x  0x%08x  0x%08x  0x%08x  ERR\n", reg, expect, actual, mask);
	}
}
EXPORT_SYMBOL(sunxi_compare_regs);

/**
 * compare_store - store func of compare attribute.
 * @class:   class ptr.
 * @attr:    attribute ptr.
 * @buf:     the input buf which contain the items to compared.
 * 		eg: "0x01c20000 0x01c20100\n"
 * @size:    buf size.
 */
ssize_t compare_store(struct class *class, struct class_attribute *attr,
			const char *buf, size_t size)
{
	struct compare_group *item_group = NULL;

	/* parse input buf for items that will be dumped */
	if(compare_item_init(buf, size, &item_group) < 0)
		return -EINVAL;

	/* dump the items */
	sunxi_compare_regs(item_group);

	/* release struct memory */
	if(NULL != item_group)
		compare_item_deinit(item_group);
	return size;
}

/**
 * parse_write_str - parse the input string for write attri.
 * @str:     string to be parsed, eg: "0x01c20818 0x55555555".
 * @reg_addr:   store the reg address. eg: 0x01c20818.
 * @val: store the expect value. eg: 0x55555555.
 *
 * return 0 if success, otherwise failed.
 */
int parse_write_str(char *str, u32 *reg_addr, u32 *val)
{
	char *ptr = str;

	ptr = first_str_to_u32(ptr, ' ', reg_addr);
	if(NULL == ptr)
		return -EINVAL;

	ptr += 1;
	if(strict_strtoul(ptr, 16, (long unsigned int *)val))
		return -EINVAL;

	return 0;
}

/**
 * write_item_init - init for write attri. parse input string, and construct write struct.
 * @buf:     the input string, eg: "0x01c20800 0x00000031,0x01c20818 0x55555555,...".
 * @size:    buf size.
 * @ppgroup: store the struct allocated, the struct contains items parsed from input buf.
 *
 * return 0 if success, otherwise failed.
 */
int write_item_init(const char *buf, size_t size, struct write_group **ppgroup)
{
	int 	itemp = 0;
	char 	str_temp[256] = {0};
	char 	*ptr = NULL, *ptr2 = NULL;
	u32 	reg_addr = 0, val;
	struct write_group *pgroup = NULL;

	/* alloc item buffer */
	pgroup = kmalloc(sizeof(struct write_group), GFP_KERNEL);
	if(NULL == pgroup)
		return -EINVAL;
	pgroup->pitem = kmalloc(sizeof(struct write_item) * MAX_WRITE_ITEM, GFP_KERNEL);
	if(NULL == pgroup->pitem) {
		kfree(pgroup);
		return -EINVAL;
	}

	pgroup->num = 0;

	/* get item from buf */
	ptr = (char *)buf;
	while((ptr2 = strchr(ptr, ',')) != NULL) {
		itemp = ptr2 - ptr;
		memcpy(str_temp, ptr, itemp);
		str_temp[itemp] = 0;
		if(0 != parse_write_str(str_temp, &reg_addr, &val))
			DUMP_ERR("%s err, line %d, str_temp %s\n", __func__, __LINE__, str_temp);
		else {
			//DUMP_DBG("%s: reg_addr 0x%08x, val 0x%08x\n", __func__, reg_addr, val);
			if(pgroup->num < MAX_WRITE_ITEM) {
				pgroup->pitem[pgroup->num].reg_addr = reg_addr;
				pgroup->pitem[pgroup->num].val = val;
				pgroup->num++;
			} else {
				DUMP_ERR("%s err, line %d, pgroup->num %d exceed %d\n",
					__func__, __LINE__, pgroup->num, MAX_WRITE_ITEM);
				break;
			}
		}

		ptr = ptr2 + 1;
	}

	/* the last item */
	if(0 != parse_write_str(ptr, &reg_addr, &val))
		DUMP_ERR("%s err, line %d, ptr %s\n", __func__, __LINE__, ptr);
	else {
		//DUMP_DBG("%s: line %d, reg_addr 0x%08x, val 0x%08x\n", __func__, __LINE__, reg_addr, val);
		if(pgroup->num < MAX_WRITE_ITEM) {
			pgroup->pitem[pgroup->num].reg_addr = reg_addr;
			pgroup->pitem[pgroup->num].val = val;
			pgroup->num++;
		}
	}

	/* free buffer if no valid item */
	if(0 == pgroup->num) {
		kfree(pgroup->pitem);
		kfree(pgroup);
		return -EINVAL;
	}

	*ppgroup = pgroup;
	return 0;
}

/**
 * write_item_deinit - release memory that created by write_item_init.
 * @pgroup: the write struct allocated in write_item_init.
 */
void write_item_deinit(struct write_group *pgroup)
{
	if(NULL != pgroup) {
		if(NULL != pgroup->pitem)
			kfree(pgroup->pitem);
		kfree(pgroup);
	}
}

/**
 * sunxi_write_regs - write a group of regs' value.
 * @pgroup: the write struct which contain items that will be write.
 */
void sunxi_write_regs(struct write_group *pgroup)
{
	int 	i = 0;
	u32 	reg = 0, val = 0, readback = 0;

	DUMP_DBG("reg         to_write    after_write \n");
	for(i = 0; i < pgroup->num; i++) {
		reg    	= pgroup->pitem[i].reg_addr;
		val 	= pgroup->pitem[i].val;
		writel(val, IO_ADDRESS(reg));
		readback = readl(IO_ADDRESS(reg));
		DUMP_DBG("0x%08x  0x%08x  0x%08x\n", reg, val, readback);
	}
}
EXPORT_SYMBOL(sunxi_write_regs);

/**
 * write_store - store func of dump attribute.
 * @class:   class ptr.
 * @attr:    attribute ptr.
 * @buf:     the input buf which contain reg&val to write.
 * 		eg: "0x01c20800 0x00000031,0x01c20818 0x55555555,...\n"
 * @size:    buf size.
 */
ssize_t write_store(struct class *class, struct class_attribute *attr,
			const char *buf, size_t size)
{
	struct write_group *item_group = NULL;

	/* parse input buf for items that will be dumped */
	if(write_item_init(buf, size, &item_group) < 0)
		return -EINVAL;

	/* write the items */
	sunxi_write_regs(item_group);

	/* release struct memory */
	if(NULL != item_group)
		write_item_deinit(item_group);
	return size;
}

static struct class_attribute dump_class_attrs[] = {
	__ATTR(dump, 	0200, NULL, dump_store),
	__ATTR(compare,	0200, NULL, compare_store),
	__ATTR(write,	0200, NULL, write_store),
	__ATTR_NULL,
};

static struct class dump_class = {
	.name		= "sunxi_dump",
	.owner		= THIS_MODULE,
	.class_attrs	= dump_class_attrs,
};

static int __init sunxi_dump_init(void)
{
	int	status;

	status = class_register(&dump_class);
	if(status < 0)
		DUMP_ERR("%s err, status %d\n", __func__, status);
	else
		DUMP_DBG("%s success\n", __func__);

	return status;
}
postcore_initcall(sunxi_dump_init);
