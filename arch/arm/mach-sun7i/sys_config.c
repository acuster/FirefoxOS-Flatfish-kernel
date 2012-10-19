/*
 * arch/arch/mach-sun4i/sys_config.c
 * (C) Copyright 2010-2015
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * Benn Huang <benn@allwinnertech.com>
 *
 * sys_config utils (porting from 2.6.36)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/string.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <asm/io.h>
#include <mach/includes.h>

static script_sub_key_t *sw_cfg_get_subkey(const char *script_buf, const char *main_key, const char *sub_key)
{
    script_head_t *hd = (script_head_t *)script_buf;
    script_main_key_t *mk = (script_main_key_t *)(hd + 1);
    script_sub_key_t *sk = NULL;
    int i, j;

    for (i = 0; i < hd->main_key_count; i++) {

        if (strcmp(main_key, mk->main_name)) {
            mk++;
            continue;
        }

        for (j = 0; j < mk->lenth; j++) {
            sk = (script_sub_key_t *)(script_buf + (mk->offset<<2) + j * sizeof(script_sub_key_t));
            if (!strcmp(sub_key, sk->sub_name)) return sk;
        }
    }
    return NULL;
}

int sw_cfg_get_int(const char *script_buf, const char *main_key, const char *sub_key)
{
    script_sub_key_t *sk = NULL;
    char *pdata;
    int value;

    sk = sw_cfg_get_subkey(script_buf, main_key, sub_key);
    if (sk == NULL) {
        return -1;
    }

    if (((sk->pattern >> 16) & 0xffff) == SCIRPT_PARSER_VALUE_TYPE_SINGLE_WORD) {
        pdata = (char *)(script_buf + (sk->offset<<2));
        value = *((int *)pdata);
        return value;
    }

    return -1;
}

char *sw_cfg_get_str(const char *script_buf, const char *main_key, const char *sub_key, char *buf)
{
    script_sub_key_t *sk = NULL;
    char *pdata;

    sk = sw_cfg_get_subkey(script_buf, main_key, sub_key);
    if (sk == NULL) {
        return NULL;
    }

    if (((sk->pattern >> 16) & 0xffff) == SCIRPT_PARSER_VALUE_TYPE_STRING) {
        pdata = (char *)(script_buf + (sk->offset<<2));
        memcpy(buf, pdata, ((sk->pattern >> 0) & 0xffff));
        return (char *)buf;
    }

    return NULL;
}


/**########################################################################################
 *
 *                        Script Operations
 *
-#########################################################################################*/
static  char  *script_mod_buf = NULL; //pointer to first key
static  int    script_main_key_count = 0;

static  int   _test_str_length(char *str)
{
    int length = 0;

    while(str[length++])
    {
        if(length > 32)
        {
            length = 32;
            break;
        }
    }

    return length;
}

int script_parser_init(char *script_buf)
{
    script_head_t   *script_head;

    pr_debug("%s(%d)-%s, script_buf addr is %p:\n",__FILE__,__LINE__,__FUNCTION__, script_buf);
    if(script_buf)
    {
        script_mod_buf = script_buf;
        script_head = (script_head_t *)script_mod_buf;

        script_main_key_count = script_head->main_key_count;

        pr_debug("succeed: %s(%d)-%s\n",__FILE__,__LINE__,__FUNCTION__);
        return SCRIPT_PARSER_OK;
    }
    else
    {
        pr_warning("failed: %s(%d)-%s\n",__FILE__,__LINE__,__FUNCTION__);
        return SCRIPT_PARSER_EMPTY_BUFFER;
    }
}

int script_parser_exit(void)
{
    script_mod_buf = NULL;
    script_main_key_count = 0;

    return SCRIPT_PARSER_OK;
}

#ifdef OMIT_SCRIPT_ON_FPGA_20121018
int script_parser_fetch(char *main_name, char *sub_name, int value[], int count)
{
	pr_debug("%s: just return -4(not found)\n", __func__);
	return SCRIPT_PARSER_KEY_NOT_FIND;
}
#else
int script_parser_fetch(char *main_name, char *sub_name, int value[], int count)
{
    char   main_bkname[32], sub_bkname[32];
    char   *main_char, *sub_char;
    script_main_key_t  *main_key = NULL;
    script_sub_key_t   *sub_key = NULL;
    int    i, j;
    int    pattern, word_count;

    pr_debug("enter script parse fetch. \n");

    /* check params */
    if(!script_mod_buf)
    {
        return SCRIPT_PARSER_EMPTY_BUFFER;
    }

    if((main_name == NULL) || (sub_name == NULL))
    {
        return SCRIPT_PARSER_KEYNAME_NULL;
    }

    if(value == NULL)
    {
        return SCRIPT_PARSER_DATA_VALUE_NULL;
    }

    /* truncate string if size >31 bytes */
    main_char = main_name;
    if(_test_str_length(main_name) > 31)
    {
        memset(main_bkname, 0, 32);
        strncpy(main_bkname, main_name, 31);
        main_char = main_bkname;
    }
    sub_char = sub_name;
    if(_test_str_length(sub_name) > 31)
    {
        memset(sub_bkname, 0, 32);
        strncpy(sub_bkname, sub_name, 31);
        sub_char = sub_bkname;
    }
    pr_debug("gpio: main name is : %s, sub_name is: %s", main_char, sub_char);

    for(i=0;i<script_main_key_count;i++)
    {
        main_key = (script_main_key_t *)(script_mod_buf + (sizeof(script_head_t)) + i * sizeof(script_main_key_t));
        if(strcmp(main_key->main_name, main_char))
        {
            continue;
        }

        /* now find sub key */
        for(j=0;j<main_key->lenth;j++)
        {
            sub_key = (script_sub_key_t *)(script_mod_buf + (main_key->offset<<2) + (j * sizeof(script_sub_key_t)));
            if(strcmp(sub_key->sub_name, sub_char))
            {
                continue;
            }
            pattern    = (sub_key->pattern>>16) & 0xffff; /* get datatype */
            word_count = (sub_key->pattern>> 0) & 0xffff; /*get count of word */
            pr_debug("pattern is: 0x%x, word_count is: 0x%x, ", pattern, word_count);

            switch(pattern)
            {
                case SCIRPT_PARSER_VALUE_TYPE_SINGLE_WORD:
                    value[0] = *(int *)(script_mod_buf + (sub_key->offset<<2));
                    break;

                case SCIRPT_PARSER_VALUE_TYPE_STRING:
                    if(count < word_count)
                    {
                        word_count = count;
                    }
                    memcpy((char *)value, script_mod_buf + (sub_key->offset<<2), word_count << 2);
                    break;

                case SCIRPT_PARSER_VALUE_TYPE_MULTI_WORD:
                    break;
                case SCIRPT_PARSER_VALUE_TYPE_GPIO_WORD:
                {
                    script_gpio_set_t  *user_gpio_cfg = (script_gpio_set_t *)value;
                    /* buffer space enough? */
                    if(sizeof(script_gpio_set_t) > (count<<2))
                    {
                        return SCRIPT_PARSER_BUFFER_NOT_ENOUGH;
                    }
                    strcpy( user_gpio_cfg->gpio_name, sub_char);
                    memcpy(&user_gpio_cfg->port, script_mod_buf + (sub_key->offset<<2),  sizeof(script_gpio_set_t) - 32);
                    break;
                }
            }

            return SCRIPT_PARSER_OK;
        }
    }

    return SCRIPT_PARSER_KEY_NOT_FIND;
}
#endif /* OMIT_SCRIPT_ON_FPGA_20121018 */
EXPORT_SYMBOL(script_parser_fetch);

#ifdef OMIT_SCRIPT_ON_FPGA_20121018
int script_parser_fetch_ex(char *main_name, char *sub_name, int value[], script_parser_value_type_t *type, int count)
{
	pr_debug("%s: just return -4(not found)\n", __func__);
	return SCRIPT_PARSER_KEY_NOT_FIND;
}
#else
int script_parser_fetch_ex(char *main_name, char *sub_name, int value[], script_parser_value_type_t *type, int count)
{
    char   main_bkname[32], sub_bkname[32];
    char   *main_char, *sub_char;
    script_main_key_t  *main_key = NULL;
    script_sub_key_t   *sub_key = NULL;
    int    i, j;
    int    pattern, word_count;
    script_parser_value_type_t *value_type = type;

    pr_debug("enter script parse fetch. \n");

    if(!script_mod_buf)
    {
        return SCRIPT_PARSER_EMPTY_BUFFER;
    }

    if((main_name == NULL) || (sub_name == NULL))
    {
        return SCRIPT_PARSER_KEYNAME_NULL;
    }

    if(value == NULL)
    {
        return SCRIPT_PARSER_DATA_VALUE_NULL;
    }

    main_char = main_name;
    if(_test_str_length(main_name) > 31)
    {
        memset(main_bkname, 0, 32);
        strncpy(main_bkname, main_name, 31);
        main_char = main_bkname;
    }
    sub_char = sub_name;
    if(_test_str_length(sub_name) > 31)
    {
        memset(sub_bkname, 0, 32);
        strncpy(sub_bkname, sub_name, 31);
        sub_char = sub_bkname;
    }
    pr_debug("gpio: main name is : %s, sub_name is: %s", main_char, sub_char);

    for(i=0;i<script_main_key_count;i++)
    {
        main_key = (script_main_key_t *)(script_mod_buf + (sizeof(script_head_t)) + i * sizeof(script_main_key_t));
        if(strcmp(main_key->main_name, main_char))
        {
            continue;
        }

        for(j=0;j<main_key->lenth;j++)
        {
            sub_key = (script_sub_key_t *)(script_mod_buf + (main_key->offset<<2) + (j * sizeof(script_sub_key_t)));
            if(strcmp(sub_key->sub_name, sub_char))
            {
                continue;
            }
            pattern    = (sub_key->pattern>>16) & 0xffff;
            word_count = (sub_key->pattern>> 0) & 0xffff;
            pr_debug("pattern is: 0x%x, word_count is: 0x%x, ", pattern, word_count);

            switch(pattern)
            {
                case SCIRPT_PARSER_VALUE_TYPE_SINGLE_WORD:
                    value[0] = *(int *)(script_mod_buf + (sub_key->offset<<2));
                    *value_type = SCIRPT_PARSER_VALUE_TYPE_SINGLE_WORD;
                    break;

                case SCIRPT_PARSER_VALUE_TYPE_STRING:
                    if(count < word_count)
                    {
                        word_count = count;
                    }
                    memcpy((char *)value, script_mod_buf + (sub_key->offset<<2), word_count << 2);
                    *value_type = SCIRPT_PARSER_VALUE_TYPE_STRING;
                    break;

                case SCIRPT_PARSER_VALUE_TYPE_MULTI_WORD:
                    *value_type = SCIRPT_PARSER_VALUE_TYPE_MULTI_WORD;
                    break;
                case SCIRPT_PARSER_VALUE_TYPE_GPIO_WORD:
                {
                    script_gpio_set_t  *user_gpio_cfg = (script_gpio_set_t *)value;

                    if(sizeof(script_gpio_set_t) > (count<<2))
                    {
                        return SCRIPT_PARSER_BUFFER_NOT_ENOUGH;
                    }
                    strcpy( user_gpio_cfg->gpio_name, sub_char);
                    memcpy(&user_gpio_cfg->port, script_mod_buf + (sub_key->offset<<2),  sizeof(script_gpio_set_t) - 32);
                    *value_type = SCIRPT_PARSER_VALUE_TYPE_GPIO_WORD;
                    break;
                }
            }

            return SCRIPT_PARSER_OK;
        }
    }

    return SCRIPT_PARSER_KEY_NOT_FIND;
}
#endif /* OMIT_SCRIPT_ON_FPGA_20121018 */
EXPORT_SYMBOL(script_parser_fetch_ex);

int script_parser_subkey_count(char *main_name)
{
    char   main_bkname[32];
    char   *main_char;
    script_main_key_t  *main_key = NULL;
    int    i;

    if(!script_mod_buf)
    {
        return SCRIPT_PARSER_EMPTY_BUFFER;
    }

    if(main_name == NULL)
    {
        return SCRIPT_PARSER_KEYNAME_NULL;
    }

    main_char = main_name;
    if(_test_str_length(main_name) > 31)
    {
        memset(main_bkname, 0, 32);
        strncpy(main_bkname, main_name, 31);
        main_char = main_bkname;
    }

    for(i=0;i<script_main_key_count;i++)
    {
        main_key = (script_main_key_t *)(script_mod_buf + (sizeof(script_head_t)) + i * sizeof(script_main_key_t));
        if(strcmp(main_key->main_name, main_char))
        {
            continue;
        }

        return main_key->lenth;
    }

    return -1;
}

int script_parser_mainkey_count(void)
{
    if(!script_mod_buf)
    {
        return SCRIPT_PARSER_EMPTY_BUFFER;
    }

    return     script_main_key_count;
}

#ifdef OMIT_SCRIPT_ON_FPGA_20121018
int script_parser_mainkey_get_gpio_count(char *main_name)
{
	pr_debug("%s: just return -1(SCRIPT_PARSER_EMPTY_BUFFER)\n", __func__);
	return SCRIPT_PARSER_EMPTY_BUFFER;
}
#else
int script_parser_mainkey_get_gpio_count(char *main_name)
{
    char   main_bkname[32];
    char   *main_char;
    script_main_key_t  *main_key = NULL;
    script_sub_key_t   *sub_key = NULL;
    int    i, j;
    int    pattern, gpio_count = 0;

    if(!script_mod_buf)
    {
        return SCRIPT_PARSER_EMPTY_BUFFER;
    }

    if(main_name == NULL)
    {
        return SCRIPT_PARSER_KEYNAME_NULL;
    }

    main_char = main_name;
    if(_test_str_length(main_name) > 31)
    {
        memset(main_bkname, 0, 32);
        strncpy(main_bkname, main_name, 31);
        main_char = main_bkname;
    }

    for(i=0;i<script_main_key_count;i++)
    {
        main_key = (script_main_key_t *)(script_mod_buf + (sizeof(script_head_t)) + i * sizeof(script_main_key_t));
        if(strcmp(main_key->main_name, main_char))
        {
            continue;
        }

        for(j=0;j<main_key->lenth;j++)
        {
            sub_key = (script_sub_key_t *)(script_mod_buf + (main_key->offset<<2) + (j * sizeof(script_sub_key_t)));

            pattern    = (sub_key->pattern>>16) & 0xffff;

            if(SCIRPT_PARSER_VALUE_TYPE_GPIO_WORD == pattern)
            {
                gpio_count ++;
            }
        }
    }

    return gpio_count;
}
#endif /* OMIT_SCRIPT_ON_FPGA_20121018 */
EXPORT_SYMBOL(script_parser_mainkey_get_gpio_count);
int script_parser_mainkey_get_gpio_cfg(char *main_name, void *gpio_cfg, int gpio_count)
{
    char   main_bkname[32];
    char   *main_char;
    script_main_key_t  *main_key = NULL;
    script_sub_key_t   *sub_key = NULL;
    script_gpio_set_t  *user_gpio_cfg = (script_gpio_set_t *)gpio_cfg;
    int    i, j;
    int    pattern, user_index;

    if(!script_mod_buf)
    {
        return SCRIPT_PARSER_EMPTY_BUFFER;
    }

    if(main_name == NULL)
    {
        return SCRIPT_PARSER_KEYNAME_NULL;
    }

    memset(user_gpio_cfg, 0, sizeof(script_gpio_set_t) * gpio_count);

    main_char = main_name;
    if(_test_str_length(main_name) > 31)
    {
        memset(main_bkname, 0, 32);
        strncpy(main_bkname, main_name, 31);
        main_char = main_bkname;
    }

    for(i=0;i<script_main_key_count;i++)
    {
        main_key = (script_main_key_t *)(script_mod_buf + (sizeof(script_head_t)) + i * sizeof(script_main_key_t));
        if(strcmp(main_key->main_name, main_char))
        {
            continue;
        }

        pr_debug("mainkey name = %s\n", main_key->main_name);
        user_index = 0;
        for(j=0;j<main_key->lenth;j++)
        {
            sub_key = (script_sub_key_t *)(script_mod_buf + (main_key->offset<<2) + (j * sizeof(script_sub_key_t)));
            pr_debug("subkey name = %s\n", sub_key->sub_name);
            pattern    = (sub_key->pattern>>16) & 0xffff;
            pr_debug("subkey pattern = %d\n", pattern);

            if(SCIRPT_PARSER_VALUE_TYPE_GPIO_WORD == pattern)
            {
                strcpy( user_gpio_cfg[user_index].gpio_name, sub_key->sub_name);
                memcpy(&user_gpio_cfg[user_index].port, script_mod_buf + (sub_key->offset<<2), sizeof(script_gpio_set_t) - 32);
                user_index++;
                if(user_index >= gpio_count)
                {
                    break;
                }
            }
        }
        return SCRIPT_PARSER_OK;
    }

    return SCRIPT_PARSER_KEY_NOT_FIND;
}

