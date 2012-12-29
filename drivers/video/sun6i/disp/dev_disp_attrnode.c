#include "dev_disp.h"


extern struct device	*display_dev;
extern fb_info_t g_fbi;

extern __s32 disp_video_set_dit_mode(__u32 scaler_index, __u32 mode);
extern __s32 disp_video_get_dit_mode(__u32 scaler_index);


static __u32 sel;
static __u32 hid;

#define ____SEPARATOR_GLABOL_NODE____

static ssize_t disp_sel_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d", sel);
}

static ssize_t disp_sel_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	int err;
    unsigned long val;
    
	err = strict_strtoul(buf, 10, &val);
	if (err) {
		printk("Invalid size\n");
		return err;
	}

    if((val>1))
    {
        printk("Invalid value, 0/1 is expected!\n");
    }else
    {
        printk("%ld\n", val);
        sel = val;
	}
    
	return count;
}

static ssize_t disp_hid_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d", HANDTOID(hid));
}

static ssize_t disp_hid_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	int err;
    unsigned long val;
    
	err = strict_strtoul(buf, 10, &val);
	if (err) {
		printk("Invalid size\n");
		return err;
	}

    if((val>3) || (val < 0))
    {
        printk("Invalid value, 0~3 is expected!\n");
    }else
    {
        printk("%ld\n", val);
        hid = IDTOHAND(val);
	}
    
	return count;
}
static DEVICE_ATTR(sel, S_IRUGO|S_IWUSR|S_IWGRP,
		disp_sel_show, disp_sel_store);

static DEVICE_ATTR(hid, S_IRUGO|S_IWUSR|S_IWGRP,
		disp_hid_show, disp_hid_store);


#define ____SEPARATOR_REG_DUMP____
static ssize_t disp_reg_dump_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%s", "there is nothing here!");
}

static ssize_t disp_reg_dump_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	int err;
    unsigned long val;
    
	err = strict_strtoul(buf, 10, &val);
	if (err) {
		printk("Invalid size\n");
		return err;
	}

    if((val>17))
    {
        printk("Invalid value, <18 is expected!\n");
    }else
    {
        BSP_disp_print_reg(1, (unsigned int)val);
	}
    
	return count;
}

static DEVICE_ATTR(reg_dump, S_IRUGO|S_IWUSR|S_IWGRP,
		disp_reg_dump_show, disp_reg_dump_store);

#define ____SEPARATOR_SCRIPT_DUMP____
static ssize_t disp_script_dump_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%s", "there is nothing here!");
}

static ssize_t disp_script_dump_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	char main_key[32];

    if(strlen(buf) == 0) {
		printk("Invalid para\n");
		return -1;
	}

    memcpy(main_key, buf, strlen(buf)+1);

    script_dump_mainkey(main_key);

	return count;
}

static DEVICE_ATTR(script_dump, S_IRUGO|S_IWUSR|S_IWGRP,
		disp_script_dump_show, disp_script_dump_store);


#define ____SEPARATOR_LCD____
static ssize_t disp_lcd_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%s", "there is nothing here!");
}

static ssize_t disp_lcd_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	int err;
    unsigned long val;
    
	err = strict_strtoul(buf, 10, &val);
	if (err) {
		printk("Invalid size\n");
		return err;
	}

    if((val==0))
    {
        DRV_lcd_close(sel);
    }else
    {
        BSP_disp_hdmi_close(sel);
        DRV_lcd_open(sel);
	}
    
	return count;
}

static DEVICE_ATTR(lcd, S_IRUGO|S_IWUSR|S_IWGRP,
		disp_lcd_show, disp_lcd_store);

static ssize_t disp_lcd_bl_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d", BSP_disp_lcd_get_bright(sel));
}

static ssize_t disp_lcd_bl_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	int err;
    unsigned long val;
    
	err = strict_strtoul(buf, 10, &val);
	if (err) {
		printk("Invalid size\n");
		return err;
	}

    if((val < 0) || (val > 255))
    {
        printk("Invalid value, 0~255 is expected!\n");
    }else
    {
        BSP_disp_lcd_set_bright(sel, val, 0);
	}
    
	return count;
}

static DEVICE_ATTR(lcd_bl, S_IRUGO|S_IWUSR|S_IWGRP,
		disp_lcd_bl_show, disp_lcd_bl_store);


#define ____SEPARATOR_HDMI____
static ssize_t disp_hdmi_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%s", "there is nothing here!");
}

static ssize_t disp_hdmi_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	int err;
    unsigned long val;
    
	err = strict_strtoul(buf, 10, &val);
	if (err) {
		printk("Invalid size\n");
		return err;
	}

    if((val==0xff))
    {
        BSP_disp_hdmi_close(sel);
    }else
    {
        BSP_disp_hdmi_close(sel);
        if(BSP_disp_hdmi_set_mode(sel,(__disp_tv_mode_t)val) == 0)
        {
            BSP_disp_hdmi_open(sel);
        }
	}
    
	return count;
}

static DEVICE_ATTR(hdmi, S_IRUGO|S_IWUSR|S_IWGRP,
		disp_hdmi_show, disp_hdmi_store);


#define ____SEPARATOR_VSYNC_EVENT____
static ssize_t disp_vsync_event_enable_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return 0xff;
}

static ssize_t disp_vsync_event_enable_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	int err;
    unsigned long val;
    
	err = strict_strtoul(buf, 10, &val);
	if (err) {
		printk("Invalid size\n");
		return err;
	}

    if((val>1))
    {
        printk("Invalid value, 0/1 is expected!\n");
    }else
    {
        BSP_disp_vsync_event_enable(sel, val);
	}
    
	return count;
}

static DEVICE_ATTR(vsync_event_enable, S_IRUGO|S_IWUSR|S_IWGRP,
		disp_vsync_event_enable_show, disp_vsync_event_enable_store);


#define ____SEPARATOR_LAYER____
static ssize_t disp_layer_mode_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
    __disp_layer_info_t para;
    int ret;

    ret = BSP_disp_layer_get_para(sel, hid, &para);
    if(0 == ret)
	{
	    return sprintf(buf, "%d\n", para.mode);
    }else
    {
        return sprintf(buf, "%s\n", "not used!");
    }
}

static ssize_t disp_layer_mode_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	int err;
    unsigned long val;
    int ret;
    __disp_layer_info_t para;
    
	err = strict_strtoul(buf, 10, &val);
	if (err) {
		printk("Invalid size\n");
		return err;
	}

    if((val>4))
    {
        printk("Invalid value, <5 is expected!\n");
        
    }else
    {
        ret = BSP_disp_layer_get_para(sel, hid, &para);
        if(0 == ret)
        {
            para.mode = (__disp_layer_work_mode_t)val;
            if(para.mode == DISP_LAYER_WORK_MODE_SCALER)
            {
                para.scn_win.width = BSP_disp_get_screen_width(sel);
                para.scn_win.height = BSP_disp_get_screen_height(sel);
            }
            BSP_disp_layer_set_para(sel, hid, &para);
        }else
        {
            printk("not used!\n");
        }
	}
    
	return count;
}

static DEVICE_ATTR(layer_mode, S_IRUGO|S_IWUSR|S_IWGRP,
		disp_layer_mode_show, disp_layer_mode_store);



#define ____SEPARATOR_VIDEO_NODE____
static ssize_t disp_video_dit_mode_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d", (unsigned int)disp_video_get_dit_mode(sel));
}

static ssize_t disp_video_dit_mode_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	int err;
    unsigned long val;
    
	err = strict_strtoul(buf, 10, &val);
	if (err) {
		printk("Invalid size\n");
		return err;
	}

    if((val>3))
    {
        printk("Invalid value, 0~3 is expected!\n");
    }else
    {
        printk("%ld\n", val);
        disp_video_set_dit_mode(sel, (unsigned int)val);
	}
    
	return count;
}
static DEVICE_ATTR(video_dit_mode, S_IRUGO|S_IWUSR|S_IWGRP,
		disp_video_dit_mode_show, disp_video_dit_mode_store);


#define ____SEPARATOR_DEU_NODE____
static ssize_t disp_deu_enable_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d", BSP_disp_deu_get_enable(sel, hid));
}

static ssize_t disp_deu_enable_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	int err;
    unsigned long val;
    
	err = strict_strtoul(buf, 10, &val);
	if (err) {
		printk("Invalid size\n");
		return err;
	}

    if((val>1))
    {
        printk("Invalid value, 0/1 is expected!\n");
    }else
    {
        printk("%ld\n", val);
        BSP_disp_deu_enable(sel, hid, (unsigned int)val);
	}
    
	return count;
}


static ssize_t disp_deu_luma_sharp_level_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d", BSP_disp_deu_get_luma_sharp_level(sel, hid));
}

static ssize_t disp_deu_luma_sharp_level_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	int err;
    unsigned long val;
    
	err = strict_strtoul(buf, 10, &val);
	if (err) {
		printk("Invalid size\n");
		return err;
	}

    if(val > 4)
    {
        printk("Invalid value, 0~4 is expected!\n");
    }else
    {
        printk("%ld\n", val);
        BSP_disp_deu_set_luma_sharp_level(sel, hid, (unsigned int)val);
	}
    
	return count;
}

static ssize_t disp_deu_chroma_sharp_level_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d", BSP_disp_deu_get_chroma_sharp_level(sel, hid));
}

static ssize_t disp_deu_chroma_sharp_level_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	int err;
    unsigned long val;
    
	err = strict_strtoul(buf, 10, &val);
	if (err) {
		printk("Invalid size\n");
		return err;
	}

    if(val > 4)
    {
        printk("Invalid value, 0~4 is expected!\n");
    }else
    {
        printk("%ld\n", val);
        BSP_disp_deu_set_chroma_sharp_level(sel, hid, (unsigned int)val);
	}
    
	return count;
}

static ssize_t disp_deu_black_exten_level_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d", BSP_disp_deu_get_black_exten_level(sel, hid));
}

static ssize_t disp_deu_black_exten_level_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	int err;
    unsigned long val;
    
	err = strict_strtoul(buf, 10, &val);
	if (err) {
		printk("Invalid size\n");
		return err;
	}

    if(val > 4)
    {
        printk("Invalid value, 0~4 is expected!\n");
    }else
    {
        printk("%ld\n", val);
        BSP_disp_deu_set_black_exten_level(sel, hid, (unsigned int)val);
	}
    
	return count;
}

static ssize_t disp_deu_white_exten_level_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d", BSP_disp_deu_get_white_exten_level(sel, hid));
}

static ssize_t disp_deu_white_exten_level_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	int err;
    unsigned long val;
    
	err = strict_strtoul(buf, 10, &val);
	if (err) {
		printk("Invalid size\n");
		return err;
	}

    if(val > 4)
    {
        printk("Invalid value, 0~4 is expected!\n");
    }else
    {
        printk("%ld\n", val);
        BSP_disp_deu_set_white_exten_level(sel, hid, (unsigned int)val);
	}
    
	return count;
}

static DEVICE_ATTR(deu_en, S_IRUGO|S_IWUSR|S_IWGRP,
		disp_deu_enable_show, disp_deu_enable_store);

static DEVICE_ATTR(deu_luma_level, S_IRUGO|S_IWUSR|S_IWGRP,
		disp_deu_luma_sharp_level_show, disp_deu_luma_sharp_level_store);

static DEVICE_ATTR(deu_chroma_level, S_IRUGO|S_IWUSR|S_IWGRP,
		disp_deu_chroma_sharp_level_show, disp_deu_chroma_sharp_level_store);

static DEVICE_ATTR(deu_black_level, S_IRUGO|S_IWUSR|S_IWGRP,
		disp_deu_black_exten_level_show, disp_deu_black_exten_level_store);

static DEVICE_ATTR(deu_white_level, S_IRUGO|S_IWUSR|S_IWGRP,
		disp_deu_white_exten_level_show, disp_deu_white_exten_level_store);



#define ____SEPARATOR_LAYER_ENHANCE_NODE____
static ssize_t disp_layer_enhance_enable_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d", BSP_disp_cmu_layer_get_enable(sel, hid));
}

static ssize_t disp_layer_enhance_enable_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	int err;
    unsigned long bright_val;
    
	err = strict_strtoul(buf, 10, &bright_val);
    printk("string=%s, int=%ld\n", buf, bright_val);
	if (err) {
		printk("Invalid size\n");
		return err;
	}

    if(bright_val > 100)
    {
        printk("Invalid value, 0~100 is expected!\n");
    }else
    {
        printk("%ld\n", bright_val);
        BSP_disp_cmu_layer_enable(sel, hid, (unsigned int)bright_val);
	}
    
	return count;
}


static ssize_t disp_layer_bright_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d", BSP_disp_cmu_layer_get_bright(sel, hid));
}

static ssize_t disp_layer_bright_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	int err;
    unsigned long bright_val;
    
	err = strict_strtoul(buf, 10, &bright_val);
    printk("string=%s, int=%ld\n", buf, bright_val);
	if (err) {
		printk("Invalid size\n");
		return err;
	}

    if(bright_val > 100)
    {
        printk("Invalid value, 0~100 is expected!\n");
    }else
    {
        printk("%ld\n", bright_val);
        BSP_disp_cmu_layer_set_bright(sel, hid, (unsigned int)bright_val);
	}
    
	return count;
}

static ssize_t disp_layer_contrast_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d", BSP_disp_cmu_layer_get_contrast(sel, hid));
}

static ssize_t disp_layer_contrast_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	int err;
    unsigned long contrast_val;
    
	err = strict_strtoul(buf, 10, &contrast_val);

	if (err) {
		printk("Invalid size\n");
		return err;
	}

    if(contrast_val > 100)
    {
        printk("Invalid value, 0~100 is expected!\n");
    }else
    {
        printk("%ld\n", contrast_val);
        BSP_disp_cmu_layer_set_contrast(sel, hid, (unsigned int)contrast_val);
	}
    
	return count;
}

static ssize_t disp_layer_saturation_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d", BSP_disp_cmu_layer_get_saturation(sel, hid));
}

static ssize_t disp_layer_saturation_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	int err;
    unsigned long saturation_val;
    
	err = strict_strtoul(buf, 10, &saturation_val);

	if (err) {
		printk("Invalid size\n");
		return err;
	}

    if(saturation_val > 100)
    {
        printk("Invalid value, 0~100 is expected!\n");
    }else
    {
        printk("%ld\n", saturation_val);
        BSP_disp_cmu_layer_set_saturation(sel, hid,(unsigned int)saturation_val);
	}
    
	return count;
}

static ssize_t disp_layer_hue_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d", BSP_disp_cmu_layer_get_hue(sel,hid));
}

static ssize_t disp_layer_hue_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	int err;
    unsigned long hue_val;
    
	err = strict_strtoul(buf, 10, &hue_val);

	if (err) {
		printk("Invalid size\n");
		return err;
	}

    if(hue_val > 100)
    {
        printk("Invalid value, 0~100 is expected!\n");
    }else
    {
        printk("%ld\n", hue_val);
        BSP_disp_cmu_layer_set_hue(sel, hid,(unsigned int)hue_val);
	}
    
	return count;
}

static ssize_t disp_layer_enhance_mode_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d", BSP_disp_cmu_layer_get_mode(sel,hid));
}

static ssize_t disp_layer_enhance_mode_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	int err;
    unsigned long mode_val;
    
	err = strict_strtoul(buf, 10, &mode_val);

	if (err) {
		printk("Invalid size\n");
		return err;
	}

    if(mode_val > 100)
    {
        printk("Invalid value, 0~100 is expected!\n");
    }else
    {
        printk("%ld\n", mode_val);
        BSP_disp_cmu_layer_set_mode(sel, hid,(unsigned int)mode_val);
	}
    
	return count;
}

#define ____SEPARATOR_SCREEN_ENHANCE_NODE____
static ssize_t disp_enhance_enable_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d", BSP_disp_cmu_get_enable(sel));
}

static ssize_t disp_enhance_enable_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	int err;
    unsigned long bright_val;
    
	err = strict_strtoul(buf, 10, &bright_val);
    printk("string=%s, int=%ld\n", buf, bright_val);
	if (err) {
		printk("Invalid size\n");
		return err;
	}

    if(bright_val > 100)
    {
        printk("Invalid value, 0~100 is expected!\n");
    }else
    {
        printk("%ld\n", bright_val);
        BSP_disp_cmu_enable(sel,(unsigned int)bright_val);
	}
    
	return count;
}



static ssize_t disp_bright_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d", BSP_disp_cmu_get_bright(sel));
}

static ssize_t disp_bright_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	int err;
    unsigned long bright_val;
    
	err = strict_strtoul(buf, 10, &bright_val);
    printk("string=%s, int=%ld\n", buf, bright_val);
	if (err) {
		printk("Invalid size\n");
		return err;
	}

    if(bright_val > 100)
    {
        printk("Invalid value, 0~100 is expected!\n");
    }else
    {
        printk("%ld\n", bright_val);
        BSP_disp_cmu_set_bright(sel, (unsigned int)bright_val);
	}
    
	return count;
}

static ssize_t disp_contrast_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d", BSP_disp_cmu_get_contrast(sel));
}

static ssize_t disp_contrast_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	int err;
    unsigned long contrast_val;
    
	err = strict_strtoul(buf, 10, &contrast_val);

	if (err) {
		printk("Invalid size\n");
		return err;
	}

    if(contrast_val > 100)
    {
        printk("Invalid value, 0~100 is expected!\n");
    }else
    {
        printk("%ld\n", contrast_val);
        BSP_disp_cmu_set_contrast(sel, (unsigned int)contrast_val);
	}
    
	return count;
}

static ssize_t disp_saturation_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d", BSP_disp_cmu_get_saturation(sel));
}

static ssize_t disp_saturation_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	int err;
    unsigned long saturation_val;
    
	err = strict_strtoul(buf, 10, &saturation_val);

	if (err) {
		printk("Invalid size\n");
		return err;
	}

    if(saturation_val > 100)
    {
        printk("Invalid value, 0~100 is expected!\n");
    }else
    {
        printk("%ld\n", saturation_val);
        BSP_disp_cmu_set_saturation(sel, (unsigned int)saturation_val);
	}
    
	return count;
}

static ssize_t disp_hue_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d", BSP_disp_cmu_get_hue(sel));
}

static ssize_t disp_hue_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	int err;
    unsigned long hue_val;
    
	err = strict_strtoul(buf, 10, &hue_val);

	if (err) {
		printk("Invalid size\n");
		return err;
	}

    if(hue_val > 100)
    {
        printk("Invalid value, 0~100 is expected!\n");
    }else
    {
        printk("%ld\n", hue_val);
        BSP_disp_cmu_set_hue(sel, (unsigned int)hue_val);
	}
    
	return count;
}

static ssize_t disp_enhance_mode_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d", BSP_disp_cmu_get_mode(sel));
}

static ssize_t disp_enhance_mode_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	int err;
    unsigned long mode_val;
    
	err = strict_strtoul(buf, 10, &mode_val);

	if (err) {
		printk("Invalid size\n");
		return err;
	}

    if(mode_val > 100)
    {
        printk("Invalid value, 0~100 is expected!\n");
    }else
    {
        printk("%ld\n", mode_val);
        BSP_disp_cmu_set_mode(sel, (unsigned int)mode_val);
	}
    
	return count;
}

static DEVICE_ATTR(layer_enhance_en, S_IRUGO|S_IWUSR|S_IWGRP,
		disp_layer_enhance_enable_show, disp_layer_enhance_enable_store);

static DEVICE_ATTR(layer_bright, S_IRUGO|S_IWUSR|S_IWGRP,
		disp_layer_bright_show, disp_layer_bright_store);

static DEVICE_ATTR(layer_contrast, S_IRUGO|S_IWUSR|S_IWGRP,
		disp_layer_contrast_show, disp_layer_contrast_store);

static DEVICE_ATTR(layer_saturation, S_IRUGO|S_IWUSR|S_IWGRP,
		disp_layer_saturation_show, disp_layer_saturation_store);

static DEVICE_ATTR(layer_hue, S_IRUGO|S_IWUSR|S_IWGRP,
		disp_layer_hue_show, disp_layer_hue_store);

static DEVICE_ATTR(layer_enhance_mode, S_IRUGO|S_IWUSR|S_IWGRP,
		disp_layer_enhance_mode_show, disp_layer_enhance_mode_store);


static DEVICE_ATTR(screen_enhance_en, S_IRUGO|S_IWUSR|S_IWGRP,
		disp_enhance_enable_show, disp_enhance_enable_store);

static DEVICE_ATTR(screen_bright, S_IRUGO|S_IWUSR|S_IWGRP,
		disp_bright_show, disp_bright_store);

static DEVICE_ATTR(screen_contrast, S_IRUGO|S_IWUSR|S_IWGRP,
		disp_contrast_show, disp_contrast_store);

static DEVICE_ATTR(screen_saturation, S_IRUGO|S_IWUSR|S_IWGRP,
		disp_saturation_show, disp_saturation_store);

static DEVICE_ATTR(screen_hue, S_IRUGO|S_IWUSR|S_IWGRP,
		disp_hue_show, disp_hue_store);

static DEVICE_ATTR(screen_enhance_mode, S_IRUGO|S_IWUSR|S_IWGRP,
		disp_enhance_mode_show, disp_enhance_mode_store);


#define ____SEPARATOR_DRC_NODE____
static ssize_t disp_drc_enable_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d", BSP_disp_drc_get_enable(sel));
}

static ssize_t disp_drc_enable_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	int err;
    unsigned long val;
    
	err = strict_strtoul(buf, 10, &val);

	if (err) {
		printk("Invalid size\n");
		return err;
	}

    if((val != 0) && (val != 1))
    {
        printk("Invalid value, 0/1 is expected!\n");
    }else
    {
        printk("%ld\n", val);
        BSP_disp_drc_enable(sel, (unsigned int)val);
	}
    
	return count;
}


static DEVICE_ATTR(drc_en, S_IRUGO|S_IWUSR|S_IWGRP,
		disp_drc_enable_show, disp_drc_enable_store);

#define ____SEPARATOR_COLORBAR____
static ssize_t disp_colorbar_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%s", "there is nothing here!");
}

extern __s32 fb_draw_colorbar(__u32 base, __u32 width, __u32 height, struct fb_var_screeninfo *var);

static ssize_t disp_colorbar_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	int err;
    unsigned long val;
    
	err = strict_strtoul(buf, 10, &val);
	if (err) {
		printk("Invalid size\n");
		return err;
	}

    if((val>7)) {
        printk("Invalid value, 0~7 is expected!\n");
    }
    else {
        fb_draw_colorbar((__u32)g_fbi.fbinfo[val]->screen_base, g_fbi.fbinfo[val]->var.xres, 
            g_fbi.fbinfo[val]->var.yres, &(g_fbi.fbinfo[val]->var));;
	}
    
	return count;
}

static DEVICE_ATTR(colorbar, S_IRUGO|S_IWUSR|S_IWGRP,
		disp_colorbar_show, disp_colorbar_store);



static struct attribute *disp_attributes[] = {
    &dev_attr_screen_enhance_en.attr,
    &dev_attr_screen_bright.attr,
    &dev_attr_screen_contrast.attr,
    &dev_attr_screen_saturation.attr,
    &dev_attr_screen_hue.attr,
    &dev_attr_screen_enhance_mode.attr,
    &dev_attr_layer_enhance_en.attr,
    &dev_attr_layer_bright.attr,
    &dev_attr_layer_contrast.attr,
    &dev_attr_layer_saturation.attr,
    &dev_attr_layer_hue.attr,
    &dev_attr_layer_enhance_mode.attr,
    &dev_attr_drc_en.attr,
    &dev_attr_deu_en.attr,
    &dev_attr_deu_luma_level.attr,
    &dev_attr_deu_chroma_level.attr,
    &dev_attr_deu_black_level.attr,
    &dev_attr_deu_white_level.attr,
    &dev_attr_video_dit_mode.attr,
    &dev_attr_sel.attr,
    &dev_attr_hid.attr,
    &dev_attr_reg_dump.attr,
    &dev_attr_layer_mode.attr,
    &dev_attr_vsync_event_enable.attr,
    &dev_attr_lcd.attr,
    &dev_attr_lcd_bl.attr,
    &dev_attr_hdmi.attr,
    &dev_attr_script_dump.attr,
    &dev_attr_colorbar.attr,
	NULL
};

static struct attribute_group disp_attribute_group = {
	.name = "attr",
	.attrs = disp_attributes
};

int disp_attr_node_init(void)
{
    unsigned int ret;

    ret = sysfs_create_group(&display_dev->kobj,
                             &disp_attribute_group);
    sel = 0;
    hid = 100;
    return 0;
}

int disp_attr_node_exit(void)
{
    return 0;
}
