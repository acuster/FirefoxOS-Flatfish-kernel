/*
 * arch/arch/mach-sun6i/sys_config.c
 * (C) Copyright 2010-2015
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * James Deng <csjamesdeng@allwinnertech.com>
 *
 * sys_config utils (porting from 2.6.36)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 */

#include <linux/string.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <mach/sys_config.h>
#include <mach/gpio.h>
#include <linux/slab.h>

#include <linux/debugfs.h>
#include <asm/uaccess.h>

#define SCRIPT_MALLOC(x)    kzalloc((x), GFP_KERNEL)
#define SCRIPT_FREE(x)      kfree(x)

/*
 * define origin main key data structure in cript buffer
 * @name: main key name, defined left of "="
 * @items: count of sub key
 * @offset: position of the value of main key, in dword
 */
#pragma pack(1)
typedef struct {
    char name[32];
    int  sub_cnt;
    int  offset;
} script_origin_main_key_t;
#pragma pack()

/*
 * define origin sub key data structure in cript buffer
 * @name: sub key name, defined left of "="
 * @offset: sub key value position, in dword
 * @type: type of sub key, int / string / gpio
 * @cnt:  length of the value area, in dword
 */
#pragma pack(1)
typedef struct {
    char name[32];
    int  offset;
    struct {
        u32 cnt : 16;
        u32 type: 16;
    } pattern;
} script_origin_sub_key_t;
#pragma pack()

/*
 * define origin header of the script in cript buffer
 * @main_cnt: count of main keys
 * @version: script version
 * @main_key: fist main key
 */
#pragma pack(1)
typedef struct {
    int  main_cnt;
    int  version[3];
    script_origin_main_key_t    main_key;
} script_origin_head_t;
#pragma pack()

/*
 * define origin gpio data structure in cript buffer
 * @gpio_name: gpio name, defined left of '='
 * @port: gpio port number, 1-PA, 2-PB, 3-PC, ...
 * @port_num: pin number in port, 0-Px0, 1-Px1, ...
 * @mul_sel: multi-function select
 * @pull: pin status,
 * @drv_level: drive level
 * @data: gpio data value
 */
#pragma pack(1)
typedef struct {
    char    gpio_name[32];
    int     port;
    int     port_num;
    int     mul_sel;
    int     pull;
    int     drv_level;
    int     data;
} script_origin_gpio_t;
#pragma pack()

/*
 *===========================================================================================================
 * origin script defined as follows:
 *
 * |-----------------------|
 * |@main-cnt |@version[3] |
 * |-----------------------|
 * | origin main key 0:    |
 * | @name[32]             |
 * | @sub_cnt              |
 * | @offset   ------------|-----
 * |-----------------------|    |
 * | origin main key 1:    |    |
 * | @name[32]             |    |
 * | @sub_cnt              |    |
 * | @offset               |    |
 * |-----------------------|    |
 * | origin main key 2:    |    |
 * |      ......           |    |
 *                              |
 *                              |
 *                              |
 * |-----------------------|    |
 * | origin sub key 0:     |<---|
 * | @name                 |
 * | @offset   ------------|----|
 * | @type                 |    |
 * |-----------------------|    |
 * | origin sub key 0:     |    |
 * | @name                 |    |
 * | @offset               |    |
 * | @type                 |    |
 * |-----------------------|    |
 * | origin sub key 0:     |    |
 * |    ......             |    |
 *                              |
 *                              |
 *                              |
 * |-----------------------|    |
 * | origin sub key value: |<---|
 * |    ......             |
 *
 *
 *
 *===========================================================================================================
 * script parse result organized as follows:
 *
 * |-----------------------|
 * | script_main_key_t     |
 * | @name[]               |
 * | @subkey      ---------|-------------------------------------------->|-----------------------|
 * | @subkey_val  ---------|------->|-----------------------|            | script_sub_key_t      |
 * | @gpio    -------------|------->| script_item_u         |<-----|     | @name[]               |
 * | @gpio_cnt             |        | @val                  |      ------|-@value                |
 * | @hash                 |        | @str                  |            | @type                 |
 * | @next      -----------|----    | @gpio                 |            | @hash                 |
 * |-----------------------|   |    |-----------------------|            | @next    -------------|---|
 * | script_main_key_t     |<--|    | script_item_u         |<----|      |-----------------------|   |
 * | @name[]               |        | @val                  |     |      | script_sub_key_t      |<--|
 * | @subkey               |        | @str                  |--|  |      | @name[]               |
 * | @subkey_val           |        | @gpio                 |  |  -------|-@value                |
 * | @gpio                 |        |-----------------------|  |         | @type                 |
 * | @gpio_cnt             |        | script_item_u         |  |         | @hash                 |
 * | @hash                 |        | @val                  |  |         | @next                 |
 * | @next                 |        | @str                  |  |         |-----------------------|
 * |-----------------------|        | @gpio                 |  |         | script_sub_key_t      |
 * | main key 2:           |        |-----------------------|  |         |  ......               |
 * |    ......             |        | script_item_u         |  |
 *                                  | ......                |  |
 *                                                             |--------->|-----------------------|
 *                                                                        | string                |
 *                                                                        |-----------------------|
 *===========================================================================================================
 */

#define SCRIPT_NAME_SIZE_MAX    (32)

/*
 * define script item management data
 * @name: item name, which is defined left of '='
 * @value: value of the item
 * @type: type of the item, interge / string / gpio?
 * @hash: hash value of sub key name, for searching quickly
 * @next: pointer for list
 */
typedef struct {
    char                        name[SCRIPT_NAME_SIZE_MAX];
    script_item_u               *value;
    script_item_value_type_e    type;
    int                         hash;
    void                        *next;
} script_sub_key_t;

/*
 * define script main key management data
 * @name: main key name, which is defined by '[]'
 * @subkey: sub key list
 * @subkey_val: buffer for save sub keys
 * @gpio: gpio list pointer
 * @gpio_cnt: gpio conter
 * @hash: hash value of sub key name, for searching quickly
 * @next: pointer for list
 */
typedef struct {
    char                name[SCRIPT_NAME_SIZE_MAX];
    script_sub_key_t    *subkey;
    script_item_u       *subkey_val;
    script_item_u       *gpio;
    int                 gpio_cnt;
    int                 hash;
    void                *next;
} script_main_key_t;

/*
 * define script sub key type, raw from sys_config.bin
 * @SCIRPT_PARSER_VALUE_TYPE_INVALID: invalid type
 * @SCIRPT_PARSER_VALUE_TYPE_SINGLE_WORD: int item type
 * @SCIRPT_PARSER_VALUE_TYPE_STRING: string item type
 * @SCIRPT_PARSER_VALUE_TYPE_MULTI_WORD: multi int type, not used currently
 * @SCIRPT_PARSER_VALUE_TYPE_GPIO_WORD: gpio item type
 */
typedef enum {
    SCIRPT_PARSER_VALUE_TYPE_INVALID = 0,
    SCIRPT_PARSER_VALUE_TYPE_SINGLE_WORD,
    SCIRPT_PARSER_VALUE_TYPE_STRING,
    SCIRPT_PARSER_VALUE_TYPE_MULTI_WORD,
    SCIRPT_PARSER_VALUE_TYPE_GPIO_WORD
} script_parser_value_type_t;

static script_main_key_t   *g_script;

static int hash(char *string)
{
    int hash = 0;

    if (!string) {
        return 0;
    }

    while (*string) {
        hash += *string;
        string++;
    }

    return hash;
}

/**
 * port_to_index - gpio port to global index, port is from script
 * @port: gpio port group index, eg: 1 for PA, 2 for PB..., -1(0xFFFF) for axp pin
 * @port_num: port index in gpio group, eg: 0 for PA0, 1 for PA1...
 *
 * return the gpio index for the port, GPIO_INDEX_INVALID indicate err
 */
u32 port_to_index(u32 port, u32 port_num)
{
    struct pio_group {
        u32 base;
        u32 nr;
    };
    /* define gpio number translate table */
    const struct pio_group gpio_trans_tbl[] = {
        {PA_NR_BASE, PA_NR},
        {PB_NR_BASE, PB_NR},
        {PC_NR_BASE, PC_NR},
        {PD_NR_BASE, PD_NR},
        {PE_NR_BASE, PE_NR},
        {PF_NR_BASE, PF_NR},
        {PG_NR_BASE, PG_NR},
        {PH_NR_BASE, PH_NR},
        {PI_NR_BASE, PI_NR}
    };

    if (port - 1 < ARRAY_SIZE(gpio_trans_tbl) &&
        port_num < gpio_trans_tbl[port - 1].nr)
        return gpio_trans_tbl[port - 1].base + port_num;

    printk(KERN_ERR "%s err: port %d, port_num %d, please check sys_config.fex\n", __func__, port, port_num);

    return GPIO_INDEX_INVALID;
}

void dump_mainkey(script_main_key_t *pmainkey)
{
#define TYPE_TO_STR(type)   ((SCIRPT_ITEM_VALUE_TYPE_INT == (type)) ?  "int"    :   \
                 ((SCIRPT_ITEM_VALUE_TYPE_STR == (type))  ?  "string" :  \
                 ((SCIRPT_ITEM_VALUE_TYPE_PIO == (type))  ?   "gpio" : "invalid")))
    script_sub_key_t *psubkey = NULL;

    if (NULL == pmainkey || NULL == pmainkey->subkey || NULL == pmainkey->subkey_val)
        return;

    printk("=========================================================\n");
    printk("    name:      %s\n", pmainkey->name);
    printk("    sub_key:   name           type      value\n");
    psubkey = pmainkey->subkey;
    while (psubkey) {
        switch (psubkey->type) {
            case SCIRPT_ITEM_VALUE_TYPE_INT:
                printk("               %-15s%-10s%d\n", psubkey->name,
                       TYPE_TO_STR(psubkey->type), psubkey->value->val);
                break;
            case SCIRPT_ITEM_VALUE_TYPE_STR:
                printk("               %-15s%-10s\"%s\"\n", psubkey->name,
                       TYPE_TO_STR(psubkey->type), psubkey->value->str);
                break;
            case SCIRPT_ITEM_VALUE_TYPE_PIO:
                printk("               %-15s%-10s(gpio: %3d, mul: %d, pull %d, drv %d, data %d)\n",
                       psubkey->name, TYPE_TO_STR(psubkey->type), psubkey->value->gpio.gpio, psubkey->value->gpio.mul_sel,
                       psubkey->value->gpio.pull, psubkey->value->gpio.drv_level, psubkey->value->gpio.data);
                break;
            default:
                printk("               %-15sINVALID TYPE\n", psubkey->name);
                break;
        }

        psubkey = psubkey->next;
    }
    printk("=========================================================\n");
}

int script_dump_mainkey(char *main_key)
{
    int     main_hash = 0;
    script_main_key_t *pmainkey = g_script;

    if (NULL != main_key) {
        printk("%s: dump %s\n", __func__, main_key);
        main_hash = hash(main_key);
        while (pmainkey) {
            if ((pmainkey->hash == main_hash) && !strcmp(pmainkey->name, main_key)) {
                dump_mainkey(pmainkey);
                return 0;
            }
            pmainkey = pmainkey->next;
        }
        printk(KERN_ERR "%s: mainkey %s not found\n", __func__, main_key);
    } else {
        printk("%s: dump all the mainkey\n", __func__);
        while (pmainkey) {
            dump_mainkey(pmainkey);
            pmainkey = pmainkey->next;
        }
    }
    printk("%s: exit\n", __func__);
    return 0;
}
EXPORT_SYMBOL(script_dump_mainkey);

script_item_value_type_e
script_get_item(char *main_key, char *sub_key, script_item_u *item)
{
    int main_hash, sub_hash;
    script_main_key_t   *mainkey = g_script;

    if (!main_key || !sub_key || !item || !g_script) {
        return SCIRPT_ITEM_VALUE_TYPE_INVALID;
    }

    main_hash = hash(main_key);
    sub_hash = hash(sub_key);

    /* try to look for the main key from main key list */
    while (mainkey) {
        if ((mainkey->hash == main_hash) && !strcmp(mainkey->name, main_key)) {
            /* find the main key */
            script_sub_key_t    *subkey = mainkey->subkey;
            while (subkey) {
                if ((subkey->hash == sub_hash) && !strcmp(subkey->name, sub_key)) {
                    /* find the sub key */
                    *item = *subkey->value;
                    return subkey->type;
                }
                subkey = subkey->next;
            }

            /* no sub key defined under the main key */
            return SCIRPT_ITEM_VALUE_TYPE_INVALID;
        }
        mainkey = mainkey->next;
    }

    return SCIRPT_ITEM_VALUE_TYPE_INVALID;
}
EXPORT_SYMBOL(script_get_item);

int script_get_pio_list(char *main_key, script_item_u **list)
{
    int main_hash;
    script_main_key_t   *mainkey = g_script;

    if (!main_key || !list || !g_script) {
        return 0;
    }

    main_hash = hash(main_key);

    /* try to look for the main key from main key list */
    while (mainkey) {
        if ((mainkey->hash == main_hash) && !strcmp(mainkey->name, main_key)) {
            /* find the main key */
            *list = mainkey->gpio;
            return mainkey->gpio_cnt;
        }
        mainkey = mainkey->next;
    }

    /* look for main key failed */
    return 0;
}
EXPORT_SYMBOL(script_get_pio_list);

/*
 * init script
 */
static int __init script_init(void)
{
    int i, j, count;

    script_origin_head_t        *script_hdr = __va(SYS_CONFIG_MEMBASE);

    script_origin_main_key_t    *origin_main;
    script_origin_sub_key_t     *origin_sub;

    script_main_key_t           *main_key;
    script_sub_key_t            *sub_key, *tmp_sub, swap_sub;

    script_item_u               *sub_val, *tmp_val, swap_val, *pval_temp;

    printk("%s enter!\n", __func__);
    if (!script_hdr) {
        printk(KERN_ERR "script buffer is NULL!\n");
        return -1;
    }

    /* alloc memory for main keys */
    g_script = SCRIPT_MALLOC(script_hdr->main_cnt * sizeof(script_main_key_t));
    if (!g_script) {
        printk(KERN_ERR "try to alloc memory for main keys!\n");
        return -1;
    }

    origin_main = &script_hdr->main_key;
    for (i = 0; i < script_hdr->main_cnt; i++) {
        main_key = &g_script[i];

        /* copy main key name */
        strncpy(main_key->name, origin_main[i].name, SCRIPT_NAME_SIZE_MAX);
        /* calculate hash value */
        main_key->hash = hash(main_key->name);

        /* allock memory for sub-keys */
        main_key->subkey = SCRIPT_MALLOC(origin_main[i].sub_cnt * sizeof(script_sub_key_t));
        main_key->subkey_val = SCRIPT_MALLOC(origin_main[i].sub_cnt * sizeof(script_item_u));
        if (!main_key->subkey || !main_key->subkey_val) {
            printk(KERN_ERR "try alloc memory for sub keys failed!\n");
            goto err_out;
        }

        sub_key = main_key->subkey;
        sub_val = main_key->subkey_val;
        origin_sub = (script_origin_sub_key_t *)((unsigned int)script_hdr + (origin_main[i].offset << 2));

        /* process sub keys */
        for (j = 0; j < origin_main[i].sub_cnt; j++) {
            strncpy(sub_key[j].name, origin_sub[j].name, SCRIPT_NAME_SIZE_MAX);
            sub_key[j].hash = hash(sub_key[j].name);
            sub_key[j].type = (script_item_value_type_e)origin_sub[j].pattern.type;
            sub_key[j].value = &sub_val[j];
            if (origin_sub[j].pattern.type == SCIRPT_PARSER_VALUE_TYPE_SINGLE_WORD) {
                sub_val[j].val = *(int *)((unsigned int)script_hdr + (origin_sub[j].offset << 2));
                sub_key[j].type = SCIRPT_ITEM_VALUE_TYPE_INT;
            } else if (origin_sub[j].pattern.type == SCIRPT_PARSER_VALUE_TYPE_STRING) {
                sub_val[j].str = SCRIPT_MALLOC((origin_sub[j].pattern.cnt << 2) + 1);
                memcpy(sub_val[j].str, (char *)((unsigned int)script_hdr + (origin_sub[j].offset << 2)), origin_sub[j].pattern.cnt << 2);
                sub_key[j].type = SCIRPT_ITEM_VALUE_TYPE_STR;
            } else if (origin_sub[j].pattern.type == SCIRPT_PARSER_VALUE_TYPE_GPIO_WORD) {
                script_origin_gpio_t    *origin_gpio = (script_origin_gpio_t *)((unsigned int)script_hdr + (origin_sub[j].offset << 2) - 32);
                u32 gpio_tmp = port_to_index(origin_gpio->port, origin_gpio->port_num);

                if (GPIO_INDEX_INVALID == gpio_tmp)
                    printk(KERN_ERR "%s:%s->%s gpio cfg invalid, please check sys_config.fex!\n", __func__, main_key->name, sub_key[j].name);
                sub_val[j].gpio.gpio = gpio_tmp;
                sub_val[j].gpio.mul_sel = (u32)origin_gpio->mul_sel;
                sub_val[j].gpio.pull = (u32)origin_gpio->pull;
                sub_val[j].gpio.drv_level = (u32)origin_gpio->drv_level;
                sub_val[j].gpio.data = (u32)origin_gpio->data;
                sub_key[j].type = SCIRPT_ITEM_VALUE_TYPE_PIO;
            } else {
                sub_key[j].type = SCIRPT_ITEM_VALUE_TYPE_INVALID;
            }
        }

        /* process gpios */
        tmp_sub = main_key->subkey;
        tmp_val = main_key->subkey_val;
        count = 0;
        for (j = 0; j < origin_main[i].sub_cnt; j++) {
            if (sub_key[j].type == SCIRPT_ITEM_VALUE_TYPE_PIO) {
                /* swap sub key */
                swap_sub = *tmp_sub;
                *tmp_sub = sub_key[j];
                sub_key[j] = swap_sub;
                /* swap sub key value ptr */
                pval_temp = tmp_sub->value;
                tmp_sub->value = sub_key[j].value;
                sub_key[j].value = pval_temp;
                /* swap key value */
                swap_val = *tmp_val;
                *tmp_val = main_key->subkey_val[j];
                main_key->subkey_val[j] = swap_val;
                tmp_sub++;
                tmp_val++;
                count++;
            }
        }

        /* process sub key link */
        for (j = 0; j < origin_main[i].sub_cnt - 1; j++) {
            main_key->subkey[j].next = &main_key->subkey[j + 1];
        }

        /* set gpio infermation */
        main_key->gpio = main_key->subkey_val;
        main_key->gpio_cnt = count;
        main_key->next = &g_script[i + 1];
    }
    g_script[script_hdr->main_cnt - 1].next = 0;

    printk("%s exit!\n", __func__);
    return 0;

err_out:

    /* script init failed, release resource */
    printk(KERN_ERR "init sys_config script failed!\n");
    if (g_script) {
        for (i = 0; i < script_hdr->main_cnt; i++) {
            main_key = &g_script[i];

            if (main_key->subkey_val) {
                for (j = 0; j < origin_main[i].sub_cnt; j++) {
                    if (main_key->subkey[j].type == SCIRPT_ITEM_VALUE_TYPE_STR) {
                        SCRIPT_FREE(main_key->subkey_val[j].str);
                    }
                }

                SCRIPT_FREE(main_key->subkey_val);
            }
            if (main_key->subkey) {
                SCRIPT_FREE(main_key->subkey);
            }
        }

        SCRIPT_FREE(g_script);
        g_script = 0;
    }

    return -1;
}
core_initcall(script_init);

static struct dentry *script_debugfs;

static int dump_mainkey_open(struct inode *inode, struct file *filp)
{
    return 0;
}

static ssize_t dump_mainkey_write(struct file *filp, const char __user *buffer,
    size_t count, loff_t *ppos)
{
    char mainkey[SCRIPT_NAME_SIZE_MAX] = {0};

    if (!buffer) {
        printk(KERN_ERR "Usage: echo {mainkey} > dump_mainkey\n");
        return 0;
    }

    if (copy_from_user(mainkey, buffer, count)) {
        return -EFAULT;
    }

    /* omit '\n' */
    mainkey[count-1] = '\0';
    script_dump_mainkey(mainkey);

    return count;
}

static struct file_operations dump_mainkey_fops = {
    .open = dump_mainkey_open,
    .write = dump_mainkey_write
};

static int script_debugfs_init(void)
{
    script_debugfs = debugfs_create_dir("sys_config", NULL);
    if (!script_debugfs) {
        printk(KERN_ERR "create debugfs/sys_config/ failed\n");
        return -1;
    }

    if (!debugfs_create_file("dump_mainkey", 0222, script_debugfs,
            NULL, &dump_mainkey_fops)) {
        printk(KERN_ERR "create file dump_mainkey failed\n");
        return -1;
    }

    return 0;
}
core_initcall(script_debugfs_init);
