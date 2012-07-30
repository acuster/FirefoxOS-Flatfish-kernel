/*
 * tscdrv.c
 * driver for tsc and demux module
 */


#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/semaphore.h>
#include <linux/fs.h>
#include <linux/dma-mapping.h>
#include <linux/sched.h>   //wake_up_process()
#include <linux/kthread.h> //kthread_create()、kthread_run()
#include <linux/err.h> //IS_ERR()、PTR_ERR()
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/errno.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/dma-mapping.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/clk.h>
#include <linux/cdev.h>
#include <linux/types.h>
#include <linux/spinlock.h>
#include <linux/preempt.h>
#include <linux/rmap.h>
#include <linux/wait.h>
#include <linux/poll.h>
#include <linux/module.h>
#include <linux/ioctl.h>
#include <linux/device.h>
#include <linux/list.h>
#include <linux/jiffies.h>

#include <asm/uaccess.h>
#include <asm/memory.h>
#include <asm/unistd.h>
#include <asm-generic/int-ll64.h>
#include <asm/uaccess.h>
#include <asm/io.h>
#include <asm/dma.h>
#include <asm/system.h>
#include <asm/page.h>

//#include <mach/sys_config.h>
#include <mach/gpio.h>
#include <mach/clock.h>
#include <mach/system.h>
#include <mach/hardware.h>

#include "tscdrv.h"

//module_param(ts_dev_major, int, S_IRUGO);
//module_param(ts_dev_minor, int, S_IRUGO);


static DECLARE_WAIT_QUEUE_HEAD(wait_proc);

/*
 * unmap/release iomem
 */
void tsiomem_unregister(struct tsc_dev *devp)
{
    /* Release io mem */
    if (devp->regs) {
        release_resource(devp->regs);
        devp->regs = NULL;
    }
    iounmap(devp->regsaddr);
    devp->regsaddr = NULL;
}

/*
 * ioremap and request iomem
 */
void register_tsiomem(struct tsc_dev *devp)
{
    char * addr;

    int ret;
    struct resource *res;

    ret = check_mem_region(REGS_BASE, REGS_SIZE);
    __wrn(" check mem region %d", ret);
    res = request_mem_region(REGS_BASE, REGS_SIZE, "ts regs");
    if (res == NULL)    {
        __err(" Cannot reserve region for register");
        goto err_out;
    }
    devp->regs = res;

    addr = ioremap(REGS_BASE, REGS_SIZE);
    if (!addr){
        __err(" cannot map region for register");
        goto err_out;
    }

    devp->regsaddr = addr;
    __wrn(" devp->regsaddr %x", (unsigned int)devp->regsaddr);

err_out:
    if (devp->regs) {
        release_resource(devp->regs);
        devp->regs = NULL;
    }
}


/*
 * interrupt service routine
 * To wake up  wait queue
 */
static irqreturn_t tscdriverinterrupt(int irq, void *dev_id)
{
    tsc_devp->intstatus.port0pcr    = ioread32((void *)(tsc_devp->regsaddr + 0x88));
    tsc_devp->intstatus.port0chan   = ioread32((void *)(tsc_devp->regsaddr + 0x98));
    tsc_devp->intstatus.port1pcr    = ioread32((void *)(tsc_devp->regsaddr + 0x108));
    tsc_devp->intstatus.port1chan   = ioread32((void *)(tsc_devp->regsaddr + 0x118));

    iowrite32(tsc_devp->intstatus.port0pcr, (void *)(tsc_devp->regsaddr + 0x88));
    iowrite32(tsc_devp->intstatus.port0chan, (void *)(tsc_devp->regsaddr + 0x98));
    iowrite32(tsc_devp->intstatus.port1pcr, (void *)(tsc_devp->regsaddr + 0x108));
    iowrite32(tsc_devp->intstatus.port1chan, (void *)(tsc_devp->regsaddr + 0x118));
    tsc_devp->irq_flag = 1;
    wake_up_interruptible(&wait_proc);        //ioctl

    return IRQ_HANDLED;
}

/*
 * poll operateion for wait for TS irq
 */
unsigned int tscdev_poll(struct file *filp, struct poll_table_struct *wait)
{
    int mask = 0;
    struct tsc_dev *devp = filp->private_data;

    poll_wait(filp, &devp->wq, wait);

    if (devp->irq_flag == 1) {
        mask |= POLLIN | POLLRDNORM;
    }

    return mask;
}

/*
 * ioctl function
 */
long tscdev_ioctl( struct file *filp, unsigned int cmd, unsigned long arg)
{
    long   ret;
    spinlock_t *lock;
    struct tsc_dev *devp;
    struct clk_para temp_clk;
    struct clk *clk_hdle;

    struct intrstatus statusdata;
    ret = 0;
    devp = filp->private_data;
    lock = &devp->lock;

    if(_IOC_TYPE(cmd) != TSCDEV_IOC_MAGIC)
	return -EINVAL;
    if(_IOC_NR(cmd) > TSCDEV_IOC_MAXNR)
	return -EINVAL;

    switch (cmd)
    {
        case TSCDEV_WAIT_INT:
		ret = wait_event_interruptible_timeout(wait_proc, devp->irq_flag, HZ*1);
		if(!ret && !devp->irq_flag) {
			//case:wait timeout.
			__wrn("wait timeout");
			memset(&statusdata, 0, sizeof(statusdata));
		} else {
			//case:interrupt occured.
                devp->irq_flag = 0;
                statusdata.port0chan    = devp->intstatus.port0chan;
                statusdata.port0pcr     = devp->intstatus.port0pcr;
                statusdata.port1chan    = devp->intstatus.port1chan;
                statusdata.port1pcr     = devp->intstatus.port1pcr;
		}
            //copy status data to user
            if(copy_to_user((struct intrstatus *)arg, &(devp->intstatus),
                        sizeof(struct intrstatus))){
                return -EFAULT;
            }
            break;

        case TSCDEV_GET_PHYSICS:
            return virt_to_phys((void *)devp->pmem);

        case TSCDEV_ENABLE_INT:
            enable_irq(devp->irq);
            break;

        case TSCDEV_DISABLE_INT:
		tsc_devp->irq_flag = 1;
		wake_up_interruptible(&wait_proc);
		disable_irq(devp->irq);
            break;

        case TSCDEV_RELEASE_SEM:
		tsc_devp->irq_flag = 1;
		wake_up_interruptible(&wait_proc);
		break;

        case TSCDEV_GET_CLK:
            if (copy_from_user(&temp_clk, (struct clk_para __user *)arg,
			sizeof(struct clk_para))) {
                __err(" get clk err");
                return -EFAULT;
            }
            clk_hdle = clk_get(devp->dev, temp_clk.clk_name);

            if (copy_to_user((char *)&((struct clk_para*)arg)->handle,
                        &clk_hdle, 4)) {
                __err(" get clk err");
                return -EFAULT;
            }
            break;

        case TSCDEV_PUT_CLK:
            if (copy_from_user(&temp_clk, (struct clk_para __user *)arg,
                        sizeof(struct clk_para))) {
                __err(" put clk err");
                return -EFAULT;
            }
            clk_put((struct clk *)temp_clk.handle);

            break;

        case TSCDEV_ENABLE_CLK:
            if (copy_from_user(&temp_clk, (struct clk_para __user *)arg,
                        sizeof(struct clk_para))) {
                __err(" enable clk err");
                return -EFAULT;
            }

            clk_enable((struct clk *)temp_clk.handle);

            break;

        case TSCDEV_DISABLE_CLK:
            if (copy_from_user(&temp_clk, (struct clk_para __user *)arg,
                        sizeof(struct clk_para))) {
                __err(" disable clk err");
                return -EFAULT;
            }

            clk_disable((struct clk *)temp_clk.handle);

            break;

        case TSCDEV_GET_CLK_FREQ:
            if (copy_from_user(&temp_clk, (struct clk_para __user *)arg,
                        sizeof(struct clk_para))) {
                __err(" get clk freq err");
                return -EFAULT;
            }
            temp_clk.clk_rate = clk_get_rate((struct clk *)temp_clk.handle);

            if (copy_to_user((char *)&((struct clk_para*)arg)->clk_rate,
                        &temp_clk.clk_rate, 4)) {
                __err(" get clk freq err");
                return -EFAULT;
            }
            break;

        case TSCDEV_SET_SRC_CLK_FREQ:
            break;

        case TSCDEV_SET_CLK_FREQ:
            if (copy_from_user(&temp_clk, (struct clk_para __user *)arg,
                        sizeof(struct clk_para))) {
                __err(" set clk err");
                return -EFAULT;
            }

            clk_set_rate((struct clk *)temp_clk.handle, temp_clk.clk_rate);

            break;

        default:
            __err(" invalid cmd ");
            ret = -EINVAL;
            break;
    }
    return ret;
}

static int tscdev_open(struct inode *inode, struct file *filp)
{
	int ret;
    struct tsc_dev *devp;
    struct clk_para tmp_clk;

    devp = container_of(inode->i_cdev, struct tsc_dev, cdev);
    filp->private_data = devp;

    if (down_interruptible(&devp->sem)) {
        return -ERESTARTSYS;
    }

    //open ahb clock
    strcpy(tmp_clk.clk_name, "ahb_ts");
    tsc_devp->ahb_clk = clk_get(tsc_devp->dev, tmp_clk.clk_name);
    if(clk_enable(tsc_devp->ahb_clk) < 0){
        __wrn(" enable ahb clk error");
        ret = -EINVAL;
        goto _err_exit;
    }
	 __wrn("ahb_ts clk rate:%lx", clk_get_rate(tsc_devp->ahb_clk));
    //open sdram clock
    strcpy(tmp_clk.clk_name, "sdram_ts");
    tsc_devp->sdram_clk = clk_get(tsc_devp->dev, tmp_clk.clk_name);
    if(clk_enable(tsc_devp->sdram_clk) < 0){
        __wrn(" enable sdram clk error");
        ret = -EINVAL;
        goto _err_exit;
    }

    //open ts clock
    strcpy(tmp_clk.clk_name, "ts");
    tsc_devp->tsc_clk = clk_get(tsc_devp->dev, tmp_clk.clk_name);
    if(clk_enable(tsc_devp->tsc_clk) < 0){
	__wrn(" enable tsc clk error");
	ret = -EINVAL;
	goto _err_exit;
    }
    strcpy(tmp_clk.clk_name, "sdram_pll_p");
    tsc_devp->pll5_clk = clk_get(tsc_devp->dev, tmp_clk.clk_name);
    clk_set_parent(tsc_devp->tsc_clk, tsc_devp->pll5_clk);

    //set ts clock rate
    tmp_clk.clk_rate = clk_get_rate(tsc_devp->pll5_clk);
    __wrn("parent clock rate %d", tmp_clk.clk_rate);
    tmp_clk.clk_rate /= 2;
    __wrn(" clock rate %d", tmp_clk.clk_rate);
    if(clk_set_rate(tsc_devp->tsc_clk, tmp_clk.clk_rate)<0){
        __wrn(" set clk rate error");
        ret = -EINVAL;
        goto _err_exit;
    }
    tmp_clk.clk_rate = clk_get_rate(tsc_devp->tsc_clk);
    __wrn("clock rate %d", tmp_clk.clk_rate);

    /* init other resource here */
    devp->irq_flag = 0;

    up(&devp->sem);

    nonseekable_open(inode, filp);
    return 0;

_err_exit:
		if(devp->tsc_clk){
			clk_disable(tsc_devp->tsc_clk);
			devp->tsc_clk = NULL;
		}
		if(devp->ahb_clk){
			clk_disable(tsc_devp->ahb_clk);
			devp->ahb_clk = NULL;
		}
		if(devp->sdram_clk){
			clk_disable(tsc_devp->sdram_clk);
			devp->sdram_clk = NULL;
		}
		return ret;

}

static int tscdev_release(struct inode *inode, struct file *filp)
{
    struct tsc_dev *devp;

    devp = filp->private_data;


    if (down_interruptible(&devp->sem)) {
        return -ERESTARTSYS;
    }

		if(devp->tsc_clk){
			clk_disable(tsc_devp->tsc_clk);
			devp->tsc_clk = NULL;
		}
		if(devp->ahb_clk){
			clk_disable(tsc_devp->ahb_clk);
			devp->ahb_clk = NULL;
		}
		if(devp->sdram_clk){
			clk_disable(tsc_devp->sdram_clk);
			devp->sdram_clk = NULL;
		}

    /* release other resource here */
    devp->irq_flag = 1;

    up(&devp->sem);
    return 0;
}

void tscdev_vma_open(struct vm_area_struct *vma)
{
    return;
}

void tscdev_vma_close(struct vm_area_struct *vma)
{
    return;
}

static struct vm_operations_struct tscdev_remap_vm_ops = {
    .open  = tscdev_vma_open,
    .close = tscdev_vma_close,
};

static int tscdev_mmap(struct file *filp, struct vm_area_struct *vma)
{
    unsigned int  mypfn;
    unsigned int  psize;
    unsigned int  offset;
    unsigned int  physics;
    unsigned int  vmsize;
    unsigned int  regsaddr ;

    struct tsc_dev *devp = filp->private_data;

    regsaddr = (unsigned int)devp->regsaddr ;
    offset = vma->vm_pgoff << PAGE_SHIFT;
    vmsize = vma->vm_end - vma->vm_start;
    psize  = devp->gsize - offset;
    if(vmsize <= PAGE_SIZE){//if mmap registers ,vmsize must be less than PAGE_SIZE
        physics = __pa(regsaddr);
        mypfn = REGS_BASE >> 12;

        vma->vm_flags |= VM_RESERVED |VM_IO;
        vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);
        if(remap_pfn_range(vma, vma->vm_start, mypfn, vmsize, vma->vm_page_prot)){
            return -EAGAIN;
        }
    }else {
        physics =  __pa(devp->pmem);
        mypfn = physics >>PAGE_SHIFT;
        if(vmsize > psize)
            return -ENXIO;

        vma->vm_flags |= VM_RESERVED ;
        vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);
        if(remap_pfn_range(vma, vma->vm_start, mypfn, vmsize, vma->vm_page_prot)){
            return -EAGAIN;
        }
    }
    vma->vm_ops = &tscdev_remap_vm_ops;
    tscdev_vma_open(vma);

    return 0;
}

static unsigned int request_tsc_pio(void)
{
    unsigned int handle = 0;
	handle = sw_gpio_request_ex("tsc_para", NULL);
	return handle;
}

static void release_tsc_pio(unsigned int handle, unsigned int reset)
{
	sw_gpio_release(handle, reset);
}


static struct file_operations tscdev_fops = {
    .owner   = THIS_MODULE,
    .mmap    = tscdev_mmap,
    .poll    = tscdev_poll,
    .open    = tscdev_open,
    .release = tscdev_release,
    .llseek  = no_llseek,
    .unlocked_ioctl   = tscdev_ioctl,
};

static int size_multiply_arr[4] = {2, 4, 8, 16};

static int __init tscdev_init(void)
{
    int ret;
    int devno;              //device NO.

    unsigned int videobufsize;
    unsigned int audiobufsize;
    unsigned int subtitlebufsize;
    unsigned int sectionbufsize;
    unsigned int tsdatabufsize;
    unsigned int pcrbufsize;
    unsigned int chan32bufsize;

    unsigned int sizemultiply;  //
    char * viraddr;
    char * pbuf;
    int gsize;

    int i;
    int tsc_used = 0;
    static user_gpio_set_t gpio_set[12];

    dev_t dev = 0;

    ret = script_parser_fetch("tsc_para", "tsc_used", &tsc_used, sizeof(tsc_used)/sizeof(int));
    __wrn(" ret %d, tsc_used %d", ret, tsc_used);
    if (ret)
    {
	__wrn(" tsc init fetch emac using configuration failed");
    }
    if(tsc_used == 0)
    {
        __err(" tsc driver is disabled ");
        return 0;
    }

    tsc_devp = kmalloc(sizeof(struct tsc_dev), GFP_KERNEL);
    if (tsc_devp == NULL) {
        __err(" malloc mem for tsc device err");
        ret = -ENOMEM;
        goto _err_exit;
    }
    memset(tsc_devp, 0, sizeof(struct tsc_dev));

    tsc_devp->ts_dev_major  = TSCDEV_MAJOR;
    tsc_devp->ts_dev_minor  = TSCDEV_MINOR;

    dev = MKDEV(tsc_devp->ts_dev_major,tsc_devp->ts_dev_minor);
    //register char device
    if (tsc_devp->ts_dev_major) {
        ret = register_chrdev_region(dev, 1, "tsc_dev");
    } else {
        ret = alloc_chrdev_region(&dev, tsc_devp->ts_dev_minor, 1, "tsc_dev");
        tsc_devp->ts_dev_major = MAJOR(dev);
        tsc_devp->ts_dev_minor = MINOR(dev);
    }

    if (ret < 0) {
        printk(KERN_WARNING "[tsc driver]: can't get major %d",tsc_devp->ts_dev_major);
        goto _err_exit;
    }
    sizemultiply = size_multiply_arr[TSF_INTR_THRESHOLD];
    //calculate total buffer size of filters
    videobufsize    = (VIDEO_NOTIFY_PKT_NUM * 188 * sizemultiply + 0x3ff)& ~0x3ff;
    audiobufsize    = (AUDIO_NOTIFY_PKT_NUM * 188 * sizemultiply+ 0x3ff)& ~0x3ff;
    subtitlebufsize = (SUBTITLE_NOTIFY_PKT_NUM * 188 * sizemultiply + 0x3ff)& ~0x3ff;
    sectionbufsize  = (SECTION_NOTIFY_PKT_NUM * 188 * sizemultiply + 0x3ff)& ~0x3ff;
    tsdatabufsize   = (TS_DATA_NOTIFY_PKT_NUM * 188 * sizemultiply + 0x3ff)& ~0x3ff;
    pcrbufsize      = (1 * 188 * sizemultiply + 0x3ff) & ~0x3ff;;
    chan32bufsize   = (1 * 188 * sizemultiply + 0x3ff) & ~0x3ff;;

    gsize = videobufsize * MAX_VIDEO_CHAN +
			audiobufsize * MAX_AUDIO_CHAN +
			subtitlebufsize * MAX_SUBTITLE_CHAN +
			sectionbufsize *  MAX_SECTION_CHAN +
			tsdatabufsize  *  MAX_TS_DATA_CHAN +
			pcrbufsize +
			chan32bufsize;

    gsize = (gsize + 0xfff) & ~0xfff;
    //alloc contiguous buffer
    __wrn("kmalloc memory, size %x", gsize);
    pbuf = (char *)kmalloc(gsize, GFP_DMA | GFP_KERNEL);
    if(!pbuf)
    {
        __wrn(" Allocate memory failed!");
        ret = -ENOMEM;
        goto _err_exit;
    }
    //set pages reversed
    for (viraddr = pbuf; viraddr < pbuf + gsize; viraddr += PAGE_SIZE){
        SetPageReserved(virt_to_page(viraddr));
    }

    tsc_devp->pmem = pbuf;
    tsc_devp->gsize = gsize;

    sema_init(&tsc_devp->sem, 1);
    init_waitqueue_head(&tsc_devp->wq);

    /* request TS irq */

    tsc_devp->pio_handle = request_tsc_pio();
    __wrn(" request tsc pio return %u", tsc_devp->pio_handle);
    if(tsc_devp->pio_handle == 0){
	printk("[tsc drvier]: request pio failed");
	ret = -EINVAL;
	goto _err_exit;
    }

    ret = request_irq(TS_IRQ_NO, tscdriverinterrupt, 0, "tsc_dev", NULL);
    if (ret < 0){
	__wrn("request irq err");
	ret = -EINVAL;
	goto _err_exit;
    }
    tsc_devp->irq = TS_IRQ_NO;


    sw_gpio_get_all_pin_status(tsc_devp->pio_handle, gpio_set, 12, 0);
    for(i = 0; i< 12; i++){
	__wrn("name %-8s, port %d, port_num %d, mul_sel %d", gpio_set[i].gpio_name, gpio_set[i].port, gpio_set[i].port_num, gpio_set[i].mul_sel);
    }

    /* Create char device */
    devno = MKDEV(tsc_devp->ts_dev_major,tsc_devp->ts_dev_minor);
    cdev_init(&tsc_devp->cdev, &tscdev_fops);
    tsc_devp->cdev.owner = THIS_MODULE;
    tsc_devp->cdev.ops = &tscdev_fops;
    ret = cdev_add(&tsc_devp->cdev, devno, 1);
    if (ret) {
        __err("Err:%d add tscdev", ret);
        ret = -EINVAL;
        goto _err_exit;
    }

    tsc_devp->class = class_create(THIS_MODULE, "tsc_dev");
    tsc_devp->dev   = device_create(tsc_devp->class, NULL, devno, NULL, "tsc_dev");

    register_tsiomem(tsc_devp);

    return 0;
_err_exit:
	 if(tsc_devp == NULL)
		 return -EINVAL;
	 free_irq(TS_IRQ_NO, NULL);

	 if(tsc_devp->pio_handle){
		 release_tsc_pio(tsc_devp->pio_handle, 1);
	 }

    pbuf   = tsc_devp->pmem;
    gsize  = tsc_devp->gsize;
    if(tsc_devp->pmem){
	//release reserved pages
	for (viraddr = pbuf; viraddr < pbuf + gsize; viraddr += PAGE_SIZE){
		ClearPageReserved(virt_to_page(viraddr));
	}
	kfree(pbuf);
	pbuf = NULL;
    }
    /* Unregister iomem and iounmap */

    tsiomem_unregister(tsc_devp);

    /* Destroy char device */
    if (tsc_devp) {
        cdev_del(&tsc_devp->cdev);
        device_destroy(tsc_devp->class, dev);
        class_destroy(tsc_devp->class);
    }

    unregister_chrdev_region(dev, 1);

    if (tsc_devp) {
        kfree(tsc_devp);
    }
	return ret;
}
module_init(tscdev_init);

static void __exit tscdev_exit(void)
{
    dev_t dev;
    char * viraddr;
    char *pbuf;
    int gsize;

    int ret;
    int tsc_used = 0;

    dev = MKDEV(tsc_devp->ts_dev_major, tsc_devp->ts_dev_minor);

    ret = script_parser_fetch("tsc_para","tsc_used", &tsc_used, sizeof(int));
    if (ret)
    {
	__wrn(" tsc init fetch emac using configuration failed");
    }
    if(tsc_used == 0)
    {
        __wrn(" tsc driver is disabled ");
        return ;
    }

    /* Release TS irq */
    free_irq(TS_IRQ_NO, NULL);
    if(tsc_devp == NULL)
		 return ;

    if(tsc_devp->pio_handle){
	release_tsc_pio(tsc_devp->pio_handle, 1);
    }

    pbuf   = tsc_devp->pmem;
    gsize  = tsc_devp->gsize;
    if(tsc_devp->pmem){
	//release reserved pages
	for (viraddr = pbuf; viraddr < pbuf + gsize; viraddr += PAGE_SIZE){
		ClearPageReserved(virt_to_page(viraddr));
	}
	kfree(pbuf);
	pbuf = NULL;
    }
    /* Unregister iomem and iounmap */

    tsiomem_unregister(tsc_devp);
    /* Destroy char device */
    if (tsc_devp) {
        cdev_del(&tsc_devp->cdev);
        device_destroy(tsc_devp->class, dev);
        class_destroy(tsc_devp->class);
    }

    unregister_chrdev_region(dev, 1);

    if (tsc_devp) {
        kfree(tsc_devp);
    }
}
module_exit(tscdev_exit);

MODULE_AUTHOR("Soft-Allwinner");
MODULE_DESCRIPTION("User mode tsc and demux device interface");
MODULE_LICENSE("GPL");
MODULE_VERSION(DRV_VERSION);
