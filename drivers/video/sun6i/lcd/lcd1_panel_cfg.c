
#include "lcd_panel_cfg.h"
#include "lcd_bak/lcd_edp_anx9804.h"
#include "lcd_bak/lcd_edp_anx6345.h"

//delete this line if you want to use the lcd para define in sys_config1.fex
//#define LCD_PARA_USE_CONFIG

static void LCD_power_on(__u32 sel);
static void LCD_power_off(__u32 sel);
static void LCD_bl_open(__u32 sel);
static void LCD_bl_close(__u32 sel);

static void LCD_panel_init(__u32 sel);
static void LCD_panel_exit(__u32 sel);

#ifdef LCD_PARA_USE_CONFIG
static __u8 g_gamma_tbl[][2] = 
{
//{input value, corrected value}
    {0, 0},
    {15, 15},
    {30, 30},
    {45, 45},
    {60, 60},
    {75, 75},
    {90, 90},
    {105, 105},
    {120, 120},
    {135, 135},
    {150, 150},
    {165, 165},
    {180, 180},
    {195, 195},
    {210, 210},
    {225, 225},
    {240, 240},
    {255, 255},
};

static void LCD_cfg_panel_info(__panel_para_t * info)
{
    __u32 i = 0, j=0;
    
    memset(info,0,sizeof(__panel_para_t));

    info->lcd_if              = 3;        //0:hv(sync+de); 1:8080; 2:ttl;  3:lvds; 4:dsi

    info->lcd_x               = 1280;
    info->lcd_y               = 800;
    info->lcd_dclk_freq       = 70;       //MHz 
    info->lcd_io_phase        = 0x0000;   //0:noraml; 1:intert phase(0~3bit: vsync phase; 4~7bit:hsync phase; 8~11bit:dclk phase; 12~15bit:de phase)
    info->lcd_pwm_freq        = 50000;     //Hz
    info->lcd_pwm_pol         = 1;

    info->lcd_hbp             = 20;      //hsync back porch
    info->lcd_ht              = 1418;     //hsync total cycle
    info->lcd_hspw            = 0;        //hsync plus width
    info->lcd_vbp             = 10;       //vsync back porch
    info->lcd_vt              = 814;      //vysnc total cycle
    info->lcd_vspw            = 0;        //vysnc plus width

    info->lcd_hv_if           = 0;        //0:parallel  hv; 8:serial hv; 12:ccir656 
    info->lcd_hv_srgb_seq     = 0;        //serial RGB output sequence
    info->lcd_hv_syuv_seq     = 0;        //serial YUV output sequence
    info->lcd_hv_syuv_fdly    = 0;        //serial YUV output F line delay

    info->lcd_lvds_if         = 0;        //0:single channel; 1:dual channel    
    info->lcd_lvds_mode       = 0;        //0:NS mode; 1:JEIDA mode    
    info->lcd_lvds_colordepth = 1;        //0:8bit; 1:6bit    
    info->lcd_lvds_io_polarity= 0;        //0:normal; 1:pn cross
    
    info->lcd_dsi_if          = 0;        //0: video mode;   1:command mode
    info->lcd_dsi_lane        = 0;        //0: 1 lnae;  1: 2lane; 2:3lane;  3:4lane
    info->lcd_dsi_format      = 0;        //0: RGB888;  1:RGB666;  2:RGB666P;   3: RGB565
    info->lcd_dsi_eotp        = 0;        //0: no ending symbol;  1: insert  ending symbol
    info->lcd_dsi_te          = 0;        //0: disable; 1: rising te mode;  2:falling te mode

    info->lcd_cpu_if          = 0;        //0: 18bit; 8:16bit
    info->lcd_cpu_te          = 0;        //0: disable; 1: rising te mode;  2:falling te mode

    info->lcd_edp_tx_ic       = 0;        //0:anx9804;  1:anx6345
    info->lcd_edp_tx_rate     = 0;        //1:1.62G;   2:2.7G;    3:5.4G
    info->lcd_edp_tx_lane     = 4;        //  1/2/4lane

    info->lcd_frm             = 0;        //0: disable; 1: enable rgb666 dither; 2:enable rgb656 dither


    info->lcd_gamma_en = 0;
    if(info->lcd_gamma_en)
    {
        __u32 items = sizeof(g_gamma_tbl)/2;
        
        for(i=0; i<items-1; i++)
        {
            __u32 num = g_gamma_tbl[i+1][0] - g_gamma_tbl[i][0];

            for(j=0; j<num; j++)
            {
                __u32 value = 0;

                value = g_gamma_tbl[i][1] + ((g_gamma_tbl[i+1][1] - g_gamma_tbl[i][1]) * j)/num;
                info->lcd_gamma_tbl[g_gamma_tbl[i][0] + j] = (value<<16) + (value<<8) + value;
            }
        }
        info->lcd_gamma_tbl[255] = (g_gamma_tbl[items-1][1]<<16) + (g_gamma_tbl[items-1][1]<<8) + g_gamma_tbl[items-1][1];
    }
}
#endif

static __s32 LCD_open_flow(__u32 sel)
{
	LCD_OPEN_FUNC(sel, LCD_power_on, 50);   //open lcd power, and delay 50ms
	LCD_OPEN_FUNC(sel, LCD_panel_init,	200);   //open lcd power, than delay 200ms
	LCD_OPEN_FUNC(sel, TCON_open, 500);     //open lcd controller, and delay 500ms
	LCD_OPEN_FUNC(sel, LCD_bl_open, 0);     //open lcd backlight, and delay 0ms

	return 0;
}

static __s32 LCD_close_flow(__u32 sel)
{	
	LCD_CLOSE_FUNC(sel, LCD_bl_close, 0);       //close lcd backlight, and delay 0ms
	LCD_CLOSE_FUNC(sel, TCON_close, 0);         //close lcd controller, and delay 0ms
	LCD_CLOSE_FUNC(sel, LCD_panel_exit,	200);   //open lcd power, than delay 200ms
	LCD_CLOSE_FUNC(sel, LCD_power_off, 1000);   //close lcd power, and delay 1000ms

	return 0;
}

static void LCD_power_on(__u32 sel)
{
    LCD_POWER_EN(sel, 1);//config lcd_power pin to open lcd power
}

static void LCD_power_off(__u32 sel)
{
    LCD_POWER_EN(sel, 0);//config lcd_power pin to close lcd power
}

static void LCD_bl_open(__u32 sel)
{
    LCD_PWM_EN(sel, 1);//open pwm module
    LCD_BL_EN(sel, 1);//config lcd_bl_en pin to open lcd backlight
}

static void LCD_bl_close(__u32 sel)
{
    LCD_BL_EN(sel, 0);//config lcd_bl_en pin to close lcd backlight
    LCD_PWM_EN(sel, 0);//close pwm module
}

static void LCD_panel_init(__u32 sel)
{
    __panel_para_t *info = kmalloc(sizeof(__panel_para_t), GFP_KERNEL | __GFP_ZERO);

    lcd_get_panel_para(sel, info);
    if((info->lcd_if == LCD_IF_EDP) && (info->lcd_edp_tx_ic == 0))
    {
        __inf("edp select anx9804 ic\n");
        anx9804_init(info);
    }
    else if((info->lcd_if == LCD_IF_EDP) && (info->lcd_edp_tx_ic == 1))
    {
        __inf("edp select anx6345 ic\n");
        anx6345_init(info);
    }
    else
    {
        __inf("== panel is not edp interface ===!\n");
    }

    kfree(info);

}

static void LCD_panel_exit(__u32 sel)
{
	return;
}

//sel: 0:lcd0; 1:lcd1
static __s32 LCD_user_defined_func(__u32 sel, __u32 para1, __u32 para2, __u32 para3)
{
    return 0;
}

void LCD_get_panel_funs_1(__lcd_panel_fun_t * fun)
{
    __inf("LCD_get_panel_funs_1\n");
#ifdef LCD_PARA_USE_CONFIG
    fun->cfg_panel_info = LCD_cfg_panel_info;//delete this line if you want to use the lcd para define in sys_config1.fex
#endif
    fun->cfg_open_flow = LCD_open_flow;
    fun->cfg_close_flow = LCD_close_flow;
    fun->lcd_user_defined_func = LCD_user_defined_func;
}
EXPORT_SYMBOL(LCD_get_panel_funs_1);

