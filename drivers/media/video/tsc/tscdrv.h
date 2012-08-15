#ifndef __TSC_DRV_H__
#define __TSC_DRV_H__

#include "dvb_drv_sun7i.h"
#define DRV_VERSION               "0.01alpha"      //version
#define TS_IRQ_NO                 (35)             //interrupt number,

#ifndef TSCDEV_MAJOR
#define TSCDEV_MAJOR            (225)
#endif

#ifndef TSCDEV_MINOR
#define TSCDEV_MINOR             (0)
#endif

#define LOG_PRINT
#define __err(msg...)		{printk(KERN_ERR "L.file:%s,line:%d,",__FILE__,__LINE__);printk(msg);printk("\n");}
#ifdef LOG_PRINT
#define __wrn(msg...)       {printk(KERN_WARNING "W.file:%s,line:%d,",__FILE__,__LINE__);printk(msg);printk("\n");}
#else
#define __wrn(msg...)   (0)
#endif
/*--------------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------------*/

struct tsc_dev{
    struct cdev cdev;             /*char device struct */
    struct device *dev;           /*ptr to class device struct */
    struct class *class;          /*class for auto create device node*/
    struct semaphore sem;         /*mutual exclusion semaphore */
    spinlock_t lock;              /*spinlock to protect ioclt access*/
    wait_queue_head_t  wq;        /*wait queue for poll ops */

    struct resource *regs;        /*registers resource */
    char * regsaddr ;             /*registers address */

    unsigned int irq;                    /*tsc driver irq number */
    unsigned int irq_flag;               /*flag of tsc driver irq generated*/

    int     ts_dev_major;
    int     ts_dev_minor;           //

    struct clk * pll5_clk;
    struct clk * sdram_clk;         //sdram clock
    struct clk * tsc_clk;           //ts  clock
    struct clk * ahb_clk;           //ahb clock

    unsigned int pio_handle;		//pio handle

    char * pmem;                    //memory address
    unsigned int gsize;             //memory size

    struct intrstatus   intstatus;  //save interrupt status;
};
struct tsc_dev *tsc_devp;


/*---------------------------------------------------------------------------*/

//extern void *reserved_mem;
//static unsigned long phymem_start_addr;

#endif /*define __TSC_DRV_H__*/
