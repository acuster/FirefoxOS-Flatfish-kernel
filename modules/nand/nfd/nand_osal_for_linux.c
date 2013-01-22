/*
*********************************************************************************************************
*											        eBIOS
*						            the Easy Portable/Player Develop Kits
*									           dma sub system
*
*						        (c) Copyright 2006-2008, David China
*											All	Rights Reserved
*
* File    : clk_for_nand.c
* By      : Richard
* Version : V1.00
*********************************************************************************************************
*/
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/list.h>
#include <linux/fs.h>
#include <linux/blkdev.h>
#include <linux/blkpg.h>
#include <linux/spinlock.h>
#include <linux/hdreg.h>
#include <linux/init.h>
#include <linux/semaphore.h>
#include <linux/platform_device.h>
#include <linux/clk.h>
#include <linux/interrupt.h>
#include <asm/uaccess.h>
#include <linux/timer.h>
#include <linux/delay.h>
#include <linux/clk.h>
#include <linux/mutex.h>
#include <mach/clock.h>
#include <mach/sys_config.h>
#include <mach/dma.h>
#include <linux/wait.h>
#include <linux/sched.h>
#include <asm/cacheflush.h>
#include "nand_lib.h"
#include "nand_blk.h"

static struct clk *ahb_nand_clk = NULL;
static struct clk *mod_nand_clk = NULL;


static DECLARE_WAIT_QUEUE_HEAD(DMA_wait);
dma_hdl_t dma_hdle = (dma_hdl_t)NULL;

int seq=0;
int nand_handle=0;
dma_cb_t done_cb;
dma_config_t dma_config;

static int nanddma_completed_flag = 1;

static int dma_start_flag = 0;

static int nandrb_ready_flag = 1;

static DECLARE_WAIT_QUEUE_HEAD(NAND_RB_WAIT);

//#define RB_INT_MSG_ON
#ifdef  RB_INT_MSG_ON
#define dbg_rbint(fmt, args...) printk(fmt, ## args)
#else
#define dbg_rbint(fmt, ...)  ({})
#endif

//#define RB_INT_WRN_ON
#ifdef  RB_INT_WRN_ON
#define dbg_rbint_wrn(fmt, args...) printk(fmt, ## args)
#else
#define dbg_rbint_wrn(fmt, ...)  ({})
#endif

/*
*********************************************************************************************************
*                                               DMA TRANSFER END ISR
*
* Description: dma transfer end isr.
*
* Arguments  : none;
*
* Returns    : EPDK_TRUE/ EPDK_FALSE
*********************************************************************************************************
*/
//struct sw_dma_client nand_dma_client = {
//	.name="NAND_DMA",
//};

int NAND_ClkRequest(void)
{
    printk("[NAND] nand clk request start\n");
	ahb_nand_clk = clk_get(NULL,"ahb_nfc");
	if(!ahb_nand_clk) {
		return -1;
	}
	mod_nand_clk = clk_get(NULL,"nfc");
		if(!mod_nand_clk) {
		return -1;
	}
	printk("[NAND] nand clk request ok!\n");
	return 0;
}

void NAND_ClkRelease(void)
{
	clk_put(ahb_nand_clk);
	clk_put(mod_nand_clk);
}


int NAND_AHBEnable(void)
{
	return clk_enable(ahb_nand_clk);
}

int NAND_ClkEnable(void)
{
	return clk_enable(mod_nand_clk);
}

void NAND_AHBDisable(void)
{
	clk_disable(ahb_nand_clk);
}

void NAND_ClkDisable(void)
{
	clk_disable(mod_nand_clk);
}

int NAND_SetClk(__u32 nand_clk)
{
	return clk_set_rate(mod_nand_clk, nand_clk*2000000);
}

int NAND_GetClk(void)
{
	return (clk_get_rate(mod_nand_clk)/2000000);
}


#ifdef  __LINUX_SUPPORT_DMA_INT__
void nanddma_buffdone(dma_hdl_t dma_hdle, void *parg)
{
	nanddma_completed_flag = 1;
	wake_up( &DMA_wait );
	//printk("buffer done. nanddma_completed_flag: %d\n", nanddma_completed_flag);
}

__s32 NAND_WaitDmaFinish(void)
{
	 wait_event(DMA_wait, nanddma_completed_flag);

    return 0;
}


#else
void nanddma_buffdone(dma_hdl_t dma_hdle, void *parg)
{
	return 0;
}

__s32 NAND_WaitDmaFinish(void)
{

    return 0;
}

#endif
int  nanddma_opfn(dma_hdl_t dma_hdle, void *parg){
	//if(op_code == SW_DMAOP_START)
	//	nanddma_completed_flag = 0;

	//printk("buffer opfn: %d, nanddma_completed_flag: %d\n", (int)op_code, nanddma_completed_flag);

	return 0;
}

dma_hdl_t NAND_RequestDMA(void)
{
	dma_hdle = sw_dma_request("NAND_DMA",CHAN_DEDICATE);
	if(dma_hdle == NULL)
	{
		printk("[NAND DMA] request dma fail\n");
		return dma_hdle;
	}
	printk("[NAND DMA] request dma success\n");

//	sw_dma_set_opfn(dma_hdle, nanddma_opfn);
//	sw_dma_set_buffdone_fn(dma_hdle, nanddma_buffdone);

	/* set full done callback */
	done_cb.func = nanddma_buffdone;
	done_cb.parg = NULL;
	if(0 != sw_dma_ctl(dma_hdle, DMA_OP_SET_FD_CB, (void *)&done_cb)) {
		printk("[NAND DMA] set fulldone_cb fail\n");
	}
	printk("[NAND DMA] set fulldone_cb success\n");


	return dma_hdle;

}


__s32  NAND_ReleaseDMA(void)
{
    if(dma_start_flag==1)
    {
        dma_start_flag=0;
        if(0 != sw_dma_ctl(dma_hdle, DMA_OP_STOP, NULL))
	{
		printk("[NAND DMA] stop dma fail\n");
	}
    }

    if(0!= sw_dma_release(dma_hdle))
    {
        printk("[NAND DMA] release dma fail\n");
    }
	return 0;
}


//__s32 NAND_SettingDMA(void * pArg)
//{
//	sw_dma_setflags(dma_hdle, SW_DMAF_AUTOSTART);
//	return sw_dma_config(dma_hdle, (struct dma_hw_conf*)pArg);
//	return 0;
//}

//int NAND_StartDMA(int rw, __u32 saddr, __u32 daddr, __u32 bytes)
//{
//	return 0;
//}

void eLIBs_CleanFlushDCacheRegion_nand(void *adr, size_t bytes)
{
	__cpuc_flush_dcache_area(adr, bytes + (1 << 5) * 2 - 2);
}


//__s32 NAND_DMAEqueueBuf(__u32 buff_addr, __u32 len)
//{
//	eLIBs_CleanFlushDCacheRegion_nand((void *)buff_addr, (size_t)len);

//	nanddma_completed_flag = 0;
//	return sw_dma_enqueue((int)dma_hdle, (void*)(seq++), buff_addr, len);
//}

int NAND_QueryDmaStat(void)
{
	return 0;
}

void NAND_DMAConfigStart(int rw, unsigned int buff_addr, int len)
{
	__u32 buff_phy_addr = 0;

#if 0
	struct dma_hw_conf nand_hwconf = {
		.xfer_type = DMAXFER_D_BWORD_S_BWORD,
		.hf_irq = SW_DMA_IRQ_FULL,
		.cmbk = 0x7f077f07,
	};

	nand_hwconf.dir = rw+1;

	if(rw == 0){
		nand_hwconf.from = 0x01C03030,
		nand_hwconf.address_type = DMAADDRT_D_LN_S_IO,
		nand_hwconf.drqsrc_type = DRQ_TYPE_NAND;
	} else {
		nand_hwconf.to = 0x01C03030,
		nand_hwconf.address_type = DMAADDRT_D_IO_S_LN,
		nand_hwconf.drqdst_type = DRQ_TYPE_NAND;
	}

	NAND_SettingDMA((void*)&nand_hwconf);
	NAND_DMAEqueueBuf(buff_addr, len);
#endif
//////////////////////////////////////////////////////////////////////////////
//config dma
	if(rw == 0)//read from nand
	{
		/* config para */
		memset(&dma_config, 0, sizeof(dma_config));
		dma_config.xfer_type.src_data_width 	= DATA_WIDTH_32BIT;
		dma_config.xfer_type.src_bst_len 	= DATA_BRST_4;
		dma_config.xfer_type.dst_data_width 	= DATA_WIDTH_32BIT;
		dma_config.xfer_type.dst_bst_len 	= DATA_BRST_4;
		dma_config.address_type.src_addr_mode 	= DDMA_ADDR_IO;
		dma_config.address_type.dst_addr_mode 	= DDMA_ADDR_LINEAR;
		dma_config.src_drq_type 	= D_DST_NAND;
		dma_config.dst_drq_type 	= D_DST_SDRAM;
		dma_config.bconti_mode 		= false;
		dma_config.irq_spt 		= CHAN_IRQ_FD;
//        printk("nand read config done!");
	}
	else //write to nand
	{
		/* config para */
		memset(&dma_config, 0, sizeof(dma_config));
		dma_config.xfer_type.src_data_width 	= DATA_WIDTH_32BIT;
		dma_config.xfer_type.src_bst_len 	= DATA_BRST_4;
		dma_config.xfer_type.dst_data_width 	= DATA_WIDTH_32BIT;
		dma_config.xfer_type.dst_bst_len 	= DATA_BRST_4;
		dma_config.address_type.src_addr_mode 	= DDMA_ADDR_LINEAR;
		dma_config.address_type.dst_addr_mode 	= DDMA_ADDR_IO;
		dma_config.src_drq_type 	= D_DST_SDRAM;
		dma_config.dst_drq_type 	= D_DST_NAND;
		dma_config.bconti_mode 		= false;
		dma_config.irq_spt 		= CHAN_IRQ_FD;
//        printk("nand write config done!");
	}


	if(0 != sw_dma_config(dma_hdle, &dma_config)) {
		printk("[NAND DMA] config dma fail\n");
	}


	{
		dma_para_t para;
		para.src_blk_sz 	= 0x7f;
		para.src_wait_cyc 	= 0x07;
		para.dst_blk_sz 	= 0x7f;
		para.dst_wait_cyc 	= 0x07;
		if(0 != sw_dma_ctl(dma_hdle, DMA_OP_SET_PARA_REG, &para))
		{
			printk("[NAND DMA] set dma para fail\n");
		}

	}
	nanddma_completed_flag = 0;

//enqueue buf
	if(rw == 0)//read
	{
		eLIBs_CleanFlushDCacheRegion_nand((void *)buff_addr, (size_t)len);

		buff_phy_addr = virt_to_phys(buff_addr);

		/* enqueue  buf */
  //      printk("%s:%d: buff_addr=%x,len= %d\n",__FUNCTION__,__LINE__,buff_addr,len);
//		printk("%s:%d: buff_phy_addr=%x,len= %d\n",__FUNCTION__,__LINE__,buff_phy_addr,len);
		if(0 != sw_dma_enqueue(dma_hdle, 0x01c03030, buff_phy_addr, len))
		{
			printk("[NAND DMA] enqueue buffer fail\n");
		}

	}
	else//write
	{
		eLIBs_CleanFlushDCacheRegion_nand((void *)buff_addr, (size_t)len);

		buff_phy_addr = virt_to_phys(buff_addr);

		/* enqueue  buf */
		if(0 != sw_dma_enqueue(dma_hdle, buff_phy_addr, 0x01c03030, len))
		{
			printk("[NAND DMA] enqueue buffer fail\n");
		}

	}
//start dma
    if(dma_start_flag==0)
    {
        dma_start_flag=1;
        printk("[NAND DMA] start dma***************************************** \n");
        if(0 != sw_dma_ctl(dma_hdle, DMA_OP_START, NULL))
	{
		printk("[NAND DMA] start dma fail\n");
	}
    }
}




void NAND_EnRbInt(void)
{
	//clear interrupt
	NFC_RbIntClearStatus();

	nandrb_ready_flag = 0;

	//enable interrupt
	NFC_RbIntEnable();

	dbg_rbint("rb int en\n");
}


void NAND_ClearRbInt(void)
{

	//disable interrupt
	NFC_RbIntDisable();;

	dbg_rbint("rb int clear\n");

	//clear interrupt
	NFC_RbIntClearStatus();

	//check rb int status
	if(NFC_RbIntGetStatus())
	{
		dbg_rbint_wrn("nand  clear rb int status error in int clear \n");
	}

	nandrb_ready_flag = 0;
}


void NAND_RbInterrupt(void)
{

	dbg_rbint("rb int occor! \n");
	if(!NFC_RbIntGetStatus())
	{
		dbg_rbint_wrn("nand rb int late \n");
	}

    NAND_ClearRbInt();

    nandrb_ready_flag = 1;
	wake_up( &NAND_RB_WAIT );

}


__s32 NAND_WaitRbReady(void)
{
	__u32 rb;

	NAND_EnRbInt();

	//wait_event(NAND_RB_WAIT, nandrb_ready_flag);
	dbg_rbint("rb wait \n");

	if(nandrb_ready_flag)
	{
		dbg_rbint("fast rb int\n");
		NAND_ClearRbInt();
		return 0;
	}

	rb=  NFC_GetRbSelect();
	if(!rb)
	{
		if(NFC_GetRbStatus(rb))
		{
			dbg_rbint("rb %u fast ready \n", rb);
			NAND_ClearRbInt();
			return 0;
		}
	}
	else
	{
		if(NFC_GetRbStatus(rb))
		{
			dbg_rbint("rb %u fast ready \n", rb);
			NAND_ClearRbInt();
			return 0;
		}
	}



	if(wait_event_timeout(NAND_RB_WAIT, nandrb_ready_flag, 1*HZ)==0)
	{
		dbg_rbint_wrn("nand wait rb int time out\n");
		NAND_ClearRbInt();

	}
	else
	{
		dbg_rbint("nand wait rb ready ok\n");
	}

	return 0;

}


#if 0
void NAND_PIORequest(void)
{
	printk("[NAND] nand gpio_request\n");
	nand_handle = gpio_request_ex("nand_para",NULL);
	if(!nand_handle)
	{
		printk("[NAND] nand gpio_request ok\n");
	}
	else
	{
	    printk("[NAND] nand gpio_request fail\n");
	}


}
#else
void NAND_PIORequest(void){};
#endif

void NAND_PIORelease(void)
{

	//printk("[NAND] nand gpio_release\n");
	//gpio_release("nand_para",NULL);

}
void NAND_Memset(void* pAddr, unsigned char value, unsigned int len)
{
    memset(pAddr, value, len);
}

void NAND_Memcpy(void* pAddr_dst, void* pAddr_src, unsigned int len)
{
    memcpy(pAddr_dst, pAddr_src, len);
}

void* NAND_Malloc(unsigned int Size)
{
    return kmalloc(Size, GFP_KERNEL);
}

void NAND_Free(void *pAddr, unsigned int Size)
{
    kfree(pAddr);
}

int NAND_Print(const char * str, ...)
{
    printk(str);

    return 0;
}

void *NAND_IORemap(unsigned int base_addr, unsigned int size)
{
    return (void *)0xf1c03000;
}

__u32 NAND_GetIOBaseAddr(void)
{
	return 0xf1c03000;
}


int NAND_get_storagetype()
{
    script_item_value_type_e script_ret;
    script_item_u storage_type;

    script_ret = script_get_item("target","storage_type", &storage_type);
    if(script_ret!=SCIRPT_ITEM_VALUE_TYPE_INT)
    {
           printk("nand init fetch storage_type failed\n");
           storage_type.val=0;
           return storage_type.val;
    }

    return storage_type.val;


}
