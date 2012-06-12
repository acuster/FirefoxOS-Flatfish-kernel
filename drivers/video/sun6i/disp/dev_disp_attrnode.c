#include "dev_disp.h"

extern struct device	*display_dev;

#define ____SEPARATOR_LAYER_ENHANCE_NODE____
static ssize_t disp_layer_enhance_enable_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d", BSP_disp_cmu_layer_get_enable(0, 100));
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
        BSP_disp_cmu_layer_enable(0, 100, (unsigned int)bright_val);
	}

	return count;
}


static ssize_t disp_layer_bright_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d", BSP_disp_cmu_layer_get_bright(0, 100));
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
        BSP_disp_cmu_layer_set_bright(0, 100, (unsigned int)bright_val);
	}

	return count;
}

static ssize_t disp_layer_contrast_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d", BSP_disp_cmu_layer_get_contrast(0, 100));
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
        BSP_disp_cmu_layer_set_contrast(0, 100, (unsigned int)contrast_val);
	}

	return count;
}

static ssize_t disp_layer_saturation_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d", BSP_disp_cmu_layer_get_saturation(0, 100));
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
        BSP_disp_cmu_layer_set_saturation(0, 100,(unsigned int)saturation_val);
	}

	return count;
}

static ssize_t disp_layer_hue_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d", BSP_disp_cmu_layer_get_hue(0,100));
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
        BSP_disp_cmu_layer_set_hue(0, 100,(unsigned int)hue_val);
	}

	return count;
}

static ssize_t disp_layer_mode_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d", BSP_disp_cmu_layer_get_mode(0,100));
}

static ssize_t disp_layer_mode_store(struct device *dev,
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
        BSP_disp_cmu_layer_set_mode(0, 100,(unsigned int)mode_val);
	}

	return count;
}

#define ____SEPARATOR_SCREEN_ENHANCE_NODE____
static ssize_t disp_enhance_enable_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d", BSP_disp_cmu_get_enable(0));
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
        BSP_disp_cmu_enable(0,(unsigned int)bright_val);
	}

	return count;
}



static ssize_t disp_bright_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d", BSP_disp_cmu_get_bright(0));
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
        BSP_disp_cmu_set_bright(0, (unsigned int)bright_val);
	}

	return count;
}

static ssize_t disp_contrast_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d", BSP_disp_cmu_get_contrast(0));
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
        BSP_disp_cmu_set_contrast(0, (unsigned int)contrast_val);
	}

	return count;
}

static ssize_t disp_saturation_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d", BSP_disp_cmu_get_saturation(0));
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
        BSP_disp_cmu_set_saturation(0, (unsigned int)saturation_val);
	}

	return count;
}

static ssize_t disp_hue_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d", BSP_disp_cmu_get_hue(0));
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
        BSP_disp_cmu_set_hue(0, (unsigned int)hue_val);
	}

	return count;
}

static ssize_t disp_mode_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d", BSP_disp_cmu_get_mode(0));
}

static ssize_t disp_mode_store(struct device *dev,
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
        BSP_disp_cmu_set_mode(0, (unsigned int)mode_val);
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

static DEVICE_ATTR(layer_mode, S_IRUGO|S_IWUSR|S_IWGRP,
		disp_layer_mode_show, disp_layer_mode_store);


static DEVICE_ATTR(enhance_en, S_IRUGO|S_IWUSR|S_IWGRP,
		disp_enhance_enable_show, disp_enhance_enable_store);

static DEVICE_ATTR(bright, S_IRUGO|S_IWUSR|S_IWGRP,
		disp_bright_show, disp_bright_store);

static DEVICE_ATTR(contrast, S_IRUGO|S_IWUSR|S_IWGRP,
		disp_contrast_show, disp_contrast_store);

static DEVICE_ATTR(saturation, S_IRUGO|S_IWUSR|S_IWGRP,
		disp_saturation_show, disp_saturation_store);

static DEVICE_ATTR(hue, S_IRUGO|S_IWUSR|S_IWGRP,
		disp_hue_show, disp_hue_store);

static DEVICE_ATTR(mode, S_IRUGO|S_IWUSR|S_IWGRP,
		disp_mode_show, disp_mode_store);


#define ____SEPARATOR_DRC_NODE____
static ssize_t disp_drc_enable_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d", BSP_disp_drc_get_enable(0));
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
        BSP_disp_drc_enable(0, (unsigned int)val);
	}

	return count;
}


static DEVICE_ATTR(drc_en, S_IRUGO|S_IWUSR|S_IWGRP,
		disp_drc_enable_show, disp_drc_enable_store);



static struct attribute *disp_attributes[] = {
    &dev_attr_enhance_en.attr,
    &dev_attr_bright.attr,
    &dev_attr_contrast.attr,
    &dev_attr_saturation.attr,
    &dev_attr_hue.attr,
    &dev_attr_mode.attr,
    &dev_attr_layer_enhance_en.attr,
    &dev_attr_layer_bright.attr,
    &dev_attr_layer_contrast.attr,
    &dev_attr_layer_saturation.attr,
    &dev_attr_layer_hue.attr,
    &dev_attr_layer_mode.attr,
    &dev_attr_drc_en.attr,
	NULL
};

static struct attribute_group disp_attribute_group = {
	.name = "dispattributes",
	.attrs = disp_attributes
};

int disp_attr_node_init(void)
{
    unsigned int ret;

    ret = sysfs_create_group(&display_dev->kobj,
                             &disp_attribute_group);

    return 0;
}

int disp_attr_node_exit(void)
{
    return 0;
}
