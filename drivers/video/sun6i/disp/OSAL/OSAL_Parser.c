#include "OSAL_Parser.h"

#ifndef __FPGA_DEBUG__

int OSAL_Script_FetchParser_Data(char *main_name, char *sub_name, int value[], int count)
{
	return script_parser_fetch(main_name, sub_name, value, count);
}

int OSAL_sw_get_ic_ver(void)
{
    enum sw_ic_ver ic_ver;
    int ret = 0xA;

    ic_ver = sw_get_ic_ver();
    if(ic_ver == MAGIC_VER_A)
    {
        ret = 0xA;
    }
    else if(ic_ver == MAGIC_VER_B)
    {
        ret = 0xB;
    }
    if(ic_ver == MAGIC_VER_C)
    {
        ret = 0xC;
    }

    return ret;
}

#else

int OSAL_Script_FetchParser_Data(char *main_name, char *sub_name, int value[], int count)
{
	return 0;
}

int OSAL_sw_get_ic_ver(void)
{
    return 0;
}

#endif
