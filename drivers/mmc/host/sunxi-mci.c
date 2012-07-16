/*
 * drivers/mmc/sunxi-host/host_op.c
 * (C) Copyright 2007-2011
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * Aaron.Maoye <leafy.myeh@allwinnertech.com>
 *
 * description for this code
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/io.h>
#include <linux/device.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/clk.h>
#include <linux/platform_device.h>
#include <linux/spinlock.h>
#include <linux/scatterlist.h>
#include <linux/dma-mapping.h>
#include <linux/slab.h>

#include <linux/mmc/host.h>
#include <linux/mmc/mmc.h>
#include <linux/mmc/core.h>
#include <linux/mmc/card.h>

#include <asm/cacheflush.h>
#include <asm/uaccess.h>

#include <mach/hardware.h>
#include <mach/platform.h>
#ifndef MMC_FPGA
#include <mach/dma.h>
#include <mach/sys_config.h>
#include <mach/clock.h>
#endif

#include "sunxi-mci.h"

struct sunxi_mmc_host* sw_host[4] = {NULL, NULL, NULL, NULL};

#if 0
static void dumphex32(char* name, char* base, int len)
{
	u32 i;

	printk("dump %s registers:", name);
	for (i=0; i<len; i+=4) {
		if (!(i&0xf))
			printk("\n0x%p : ", base + i);
		printk("0x%08x ", readl(base + i));
	}
	printk("\n");
}

static void hexdump(char* name, char* base, int len)
{
	u32 i;

	printk("%s :", name);
	for (i=0; i<len; i++) {
		if (!(i&0x1f))
			printk("\n0x%p : ", base + i);
		if (!(i&0xf))
			printk(" ");
		printk("%02x ", readb(base + i));
	}
	printk("\n");
}
#endif

static s32 sw_mci_init_host(struct sunxi_mmc_host* smc_host)
{
	u32 rval;

	SMC_DBG(smc_host, "MMC Driver init host %d\n", smc_host->pdev->id);

	/* reset controller */
	rval = mci_readl(smc_host, REG_GCTRL) | SDXC_HWReset;
	mci_writel(smc_host, REG_GCTRL, rval);

	mci_writel(smc_host, REG_FTRGL, 0x70008);
	mci_writel(smc_host, REG_TMOUT, 0xffffffff);
	mci_writel(smc_host, REG_RINTR, 0xffffffff);
	mci_writel(smc_host, REG_DBGC, 0xdeb);
	mci_writel(smc_host, REG_FUNS, 0xceaa0000);
	rval = mci_readl(smc_host, REG_GCTRL)|SDXC_INTEnb|SDXC_WaitMemAccessDone;
	mci_writel(smc_host, REG_GCTRL, rval);

	/* alloc idma descriptor structure */
	smc_host->sg_cpu = dma_alloc_writecombine(NULL, PAGE_SIZE,
					&smc_host->sg_dma, GFP_KERNEL);
	if (smc_host->sg_cpu == NULL) {
		SMC_ERR(smc_host, "alloc dma des failed\n");
		return -1;
	}
	smc_host->power_on = 1;
	smc_host->debuglevel = 3;
	return 0;
}

s32 sw_mci_exit_host(struct sunxi_mmc_host* smc_host)
{
	/* free idma descriptor structrue */
	if (smc_host->sg_cpu) {
		dma_free_coherent(NULL, PAGE_SIZE,
				  smc_host->sg_cpu, smc_host->sg_dma);
		smc_host->sg_cpu = NULL;
		smc_host->sg_dma = 0;
	}

	return 0;
}

s32 sw_mci_update_clk(struct sunxi_mmc_host* smc_host)
{
	u32 rval;
	s32 expire = jiffies + msecs_to_jiffies(30);	//30ms timeout
	s32 ret = 0;
	u32 imask;

	imask = mci_readl(smc_host, REG_IMASK);
	mci_writew(smc_host, REG_IMASK, 0);

	rval = SDXC_Start|SDXC_UPCLKOnly|SDXC_WaitPreOver;
	mci_writel(smc_host, REG_CMDR, rval);

	do {
		rval = mci_readl(smc_host, REG_CMDR);
	} while (jiffies < expire && (rval & SDXC_Start));

	if (jiffies > expire)
		ret = -1;

	mci_writel(smc_host, REG_RINTR, 0xffff);
	mci_writew(smc_host, REG_IMASK, imask);

	return ret;
}

static void sw_mci_send_cmd(struct sunxi_mmc_host* smc_host, struct mmc_command* cmd)
{
	u32 imask = SDXC_CmdDone|SDXC_IntErrBit;
	u32 cmd_val = SDXC_Start|(cmd->opcode&0x3f);

	smc_host->wait = SDC_WAIT_CMD_DONE;
	if (cmd->opcode == MMC_GO_IDLE_STATE)
		cmd_val |= SDXC_SendInitSeq;

	if (cmd->flags & MMC_RSP_PRESENT) {
		cmd_val |= SDXC_RspExp;
		if (cmd->flags & MMC_RSP_136)
			cmd_val |= SDXC_LongRsp;
		if (cmd->flags & MMC_RSP_CRC)
			cmd_val |= SDXC_CheckRspCRC;

		if ((cmd->flags & MMC_CMD_MASK) == MMC_CMD_ADTC) {
			cmd_val |= SDXC_DataExp | SDXC_WaitPreOver;
			smc_host->wait = SDC_WAIT_DATA_OVER;
			imask |= SDXC_DataOver;
			if (cmd->data->flags & MMC_DATA_STREAM) {
				imask |= SDXC_AutoCMDDone;
				cmd_val |= SDXC_Seqmod | SDXC_SendAutoStop;
				smc_host->wait = SDC_WAIT_AUTOCMD_DONE;
			}
			if (cmd->data->stop) {
				imask |= SDXC_AutoCMDDone;
				cmd_val |= SDXC_SendAutoStop;
				smc_host->wait = SDC_WAIT_AUTOCMD_DONE;
			}
			else
				imask |= SDXC_WaitPreOver;
			if (cmd->data->flags & MMC_DATA_WRITE)
				cmd_val |= SDXC_Write;
		}
	}

	SMC_DBG(smc_host, "smc %d send cmd %d(%08x), imask = 0x%08x, wait = %d\n",
		smc_host->pdev->id, cmd_val&0x3f, cmd_val, imask, smc_host->wait);
	mci_writew(smc_host, REG_IMASK, imask);
	mci_writel(smc_host, REG_CARG, cmd->arg);

	smc_host->state = SDC_STATE_SENDCMD;
	smp_wmb();
	mci_writel(smc_host, REG_CMDR, cmd_val);
}

static void sw_mci_init_idma_des(struct sunxi_mmc_host* smc_host, struct mmc_data* data)
{
	struct sunxi_mmc_idma_des* pdes = (struct sunxi_mmc_idma_des*)smc_host->sg_cpu;
	struct sunxi_mmc_idma_des* pdes_pa = (struct sunxi_mmc_idma_des*)smc_host->sg_dma;
	u32 des_idx = 0;
	u32 buff_frag_num = 0;
	u32 remain;
	u32 i, j;
	u32 config;

	for (i=0; i<data->sg_len; i++) {
		buff_frag_num = data->sg[i].length >> SDXC_DES_NUM_SHIFT;
		remain = data->sg[i].length & (SDXC_DES_BUFFER_MAX_LEN-1);
		if (remain)
			buff_frag_num ++;
		else
			remain = SDXC_DES_BUFFER_MAX_LEN;

		for (j=0; j < buff_frag_num; j++, des_idx++) {
			memset((void*)&pdes[des_idx], 0, sizeof(struct sunxi_mmc_idma_des));
			config = SDXC_IDMAC_DES0_CH|SDXC_IDMAC_DES0_OWN|SDXC_IDMAC_DES0_DIC;

			if (buff_frag_num > 1 && j != buff_frag_num-1)
				pdes[des_idx].data_buf1_sz = SDXC_DES_BUFFER_MAX_LEN;
			else
				pdes[des_idx].data_buf1_sz = remain;

			pdes[des_idx].buf_addr_ptr1 = sg_dma_address(&data->sg[i])
							+ j * SDXC_DES_BUFFER_MAX_LEN;
			if (i==0 && j==0)
				config |= SDXC_IDMAC_DES0_FD;

			if ((i == data->sg_len-1) && (j == buff_frag_num-1)) {
				config &= ~SDXC_IDMAC_DES0_DIC;
				config |= SDXC_IDMAC_DES0_LD|SDXC_IDMAC_DES0_ER;
				pdes[des_idx].buf_addr_ptr2 = 0;
			} else {
				pdes[des_idx].buf_addr_ptr2 = (u32)&pdes_pa[des_idx+1];
			}
			pdes[des_idx].config = config;
			SMC_DBG(smc_host, "sg %d, frag %d, remain %d, des[%d](%08x): "
				"[0] = %08x, [1] = %08x, [2] = %08x, [3] = %08x\n", i, j, remain,
				des_idx, (u32)&pdes[des_idx],
				(u32)((u32*)&pdes[des_idx])[0], (u32)((u32*)&pdes[des_idx])[1],
				(u32)((u32*)&pdes[des_idx])[2], (u32)((u32*)&pdes[des_idx])[3]);
		}
	}
	return;
}


static int sw_mci_prepare_dma(struct sunxi_mmc_host* smc_host, struct mmc_data* data)
{
	u32 dma_len;
	u32 i;
	u32 temp;
	struct scatterlist *sg;

	if (smc_host->sg_cpu == NULL)
		return -ENOMEM;

	dma_len = dma_map_sg(mmc_dev(smc_host->mmc), data->sg, data->sg_len,
			(data->flags & MMC_DATA_WRITE) ? DMA_TO_DEVICE : DMA_FROM_DEVICE);
	if (dma_len == 0) {
		SMC_ERR(smc_host, "no dma map memory\n");
		return -ENOMEM;
	}

	for_each_sg(data->sg, sg, data->sg_len, i) {
		if (sg->offset & 3 || sg->length & 3)
			return -EINVAL;
	}

	sw_mci_init_idma_des(smc_host, data);
	temp = mci_readl(smc_host, REG_GCTRL);
	temp |= SDXC_DMAEnb;
	mci_writel(smc_host, REG_GCTRL, temp);
	temp |= SDXC_DMAReset;
	mci_writel(smc_host, REG_GCTRL, temp);
	mci_writel(smc_host, REG_DMAC, SDXC_IDMACSoftRST);
	temp = SDXC_IDMACFixBurst|SDXC_IDMACIDMAOn;
	mci_writel(smc_host, REG_DMAC, temp);
	temp = mci_readl(smc_host, REG_IDIE);
	temp &= ~(SDXC_IDMACReceiveInt|SDXC_IDMACTransmitInt);
	if (data->flags & MMC_DATA_WRITE)
		temp |= SDXC_IDMACTransmitInt;
	else
		temp |= SDXC_IDMACReceiveInt;
	mci_writel(smc_host, REG_IDIE, temp);

	//write descriptor address to register
	mci_writel(smc_host, REG_DLBA, smc_host->sg_dma);
	mci_writel(smc_host, REG_FTRGL, (2U<<28)|(7<<16)|8);

	return 0;
}

int sw_mci_check_r1_ready(struct sunxi_mmc_host* smc_host)
{
	return mci_readl(smc_host, REG_STAS) & SDXC_CardDataBusy ? 0 : 1;
}

int sw_mci_send_manual_stop(struct sunxi_mmc_host* smc_host, struct mmc_request* request)
{
	struct mmc_data* data = request->data;
	u32 cmd_val = SDXC_Start | SDXC_RspExp | SDXC_CheckRspCRC | MMC_STOP_TRANSMISSION;
	u32 iflags = 0;
	u32 temp;
	int ret = 0;

	if (!data || !data->stop) {
		SMC_ERR(smc_host, "no stop cmd request\n");
		return -1;
	}
	/* disable interrupt */
	temp = mci_readl(smc_host, REG_GCTRL) & (~SDXC_INTEnb);
	mci_writel(smc_host, REG_GCTRL, temp);

	mci_writel(smc_host, REG_CARG, 0);
	mci_writel(smc_host, REG_CMDR, cmd_val);
	do {
		iflags = mci_readl(smc_host, REG_RINTR);
	} while(!(iflags & (SDXC_CmdDone | SDXC_IntErrBit)));

	if (iflags & SDXC_IntErrBit) {
		SMC_ERR(smc_host, "sdc %d send stop command failed\n", smc_host->pdev->id);
		data->stop->error = -ETIMEDOUT;
		ret = -1;
	}
	mci_writel(smc_host, REG_RINTR, iflags & (~SDXC_SDIOInt));
	data->stop->resp[0] = mci_readl(smc_host, REG_RESP0);

	/* enable interrupt */
	temp = mci_readl(smc_host, REG_GCTRL) | SDXC_INTEnb;
	mci_writel(smc_host, REG_GCTRL, temp);

	return ret;
}

void sw_mci_dump_errinfo(struct sunxi_mmc_host* smc_host)
{
	SMC_ERR(smc_host, "smc %d err, cmd %d, %s%s%s%s%s%s%s%s%s%s !!\n",
		smc_host->pdev->id, smc_host->mrq->cmd->opcode,
		smc_host->int_sum & SDXC_RespErr     ? " RE"     : "",
		smc_host->int_sum & SDXC_RespCRCErr  ? " RCE"    : "",
		smc_host->int_sum & SDXC_DataCRCErr  ? " DCE"    : "",
		smc_host->int_sum & SDXC_RespTimeout ? " RTO"    : "",
		smc_host->int_sum & SDXC_DataTimeout ? " DTO"    : "",
		smc_host->int_sum & SDXC_DataStarve  ? " DS"     : "",
		smc_host->int_sum & SDXC_FIFORunErr  ? " FE"     : "",
		smc_host->int_sum & SDXC_HardWLocked ? " HL"     : "",
		smc_host->int_sum & SDXC_StartBitErr ? " SBE"    : "",
		smc_host->int_sum & SDXC_EndBitErr   ? " EBE"    : ""
		);
}

s32 sw_mci_request_done(struct sunxi_mmc_host* smc_host)
{
	struct mmc_request* req = smc_host->mrq;
	u32 temp;
	s32 ret = 0;

	if (smc_host->int_sum & SDXC_IntErrBit) {
		sw_mci_dump_errinfo(smc_host);
		if (req->data)
			SMC_ERR(smc_host, "In data %s operation\n",
				req->data->flags & MMC_DATA_WRITE ? "write" : "read");
		ret = -1;
		goto __out;
	}

	if (req->cmd) {
		if (req->cmd->flags & MMC_RSP_136) {
			req->cmd->resp[0] = mci_readl(smc_host, REG_RESP3);
			req->cmd->resp[1] = mci_readl(smc_host, REG_RESP2);
			req->cmd->resp[2] = mci_readl(smc_host, REG_RESP1);
			req->cmd->resp[3] = mci_readl(smc_host, REG_RESP0);
		} else {
			req->cmd->resp[0] = mci_readl(smc_host, REG_RESP0);
		}
	}

__out:
	if (req->data) {
		struct mmc_data* data = req->data;
		if (!(req->data->flags & MMC_DATA_WRITE)
			&& (mci_readl(smc_host, REG_STAS) & SDXC_DataFSMBusy)) {
			if ((mci_readl(smc_host, REG_STAS) & SDXC_DataFSMBusy)
				&& (mci_readl(smc_host, REG_STAS) & SDXC_DataFSMBusy)
				&& (mci_readl(smc_host, REG_STAS) & SDXC_DataFSMBusy)
				&& (mci_readl(smc_host, REG_STAS) & SDXC_DataFSMBusy)
				&& (mci_readl(smc_host, REG_STAS) & SDXC_DataFSMBusy))
				SMC_DBG(smc_host, "mmc %d fsm busy 0x%x len %d\n",
					smc_host->pdev->id, mci_readl(smc_host, REG_STAS),
					data->blksz * data->blocks);
		}
		smc_host->dma_done = 0;
		mci_writel(smc_host, REG_IDST, 0x337);
		mci_writel(smc_host, REG_IDIE, 0);
		mci_writel(smc_host, REG_DMAC, 0);
		temp = mci_readl(smc_host, REG_GCTRL);
		mci_writel(smc_host, REG_GCTRL, temp|SDXC_DMAReset);
		temp &= ~SDXC_DMAEnb;
		mci_writel(smc_host, REG_GCTRL, temp);
		temp |= SDXC_FIFOReset;
		mci_writel(smc_host, REG_GCTRL, temp);
		dma_unmap_sg(mmc_dev(smc_host->mmc), data->sg, data->sg_len,
                                data->flags & MMC_DATA_WRITE ? DMA_TO_DEVICE : DMA_FROM_DEVICE);
	}

	temp = mci_readl(smc_host, REG_STAS);
	if ((temp & SDXC_DataFSMBusy)
		|| (smc_host->int_sum & (SDXC_RespErr | SDXC_HardWLocked | SDXC_RespTimeout))) {
		SMC_DBG(smc_host, "sdc %d abnormal status: %s %s\n", smc_host->pdev->id,
			temp & SDXC_DataFSMBusy ? "DataFSMBusy" : "",
			smc_host->int_sum & SDXC_HardWLocked ? "HardWLocked" : "RespErr");
		mci_writew(smc_host, REG_IMASK, 0);
		temp = mci_readl(smc_host, REG_GCTRL) | SDXC_HWReset;
		mci_writel(smc_host, REG_GCTRL, temp);
		mci_writew(smc_host, REG_RINTR, 0xffff);
		sw_mci_update_clk(smc_host);
	}

	mci_writew(smc_host, REG_IMASK, 0);
	mci_writew(smc_host, REG_RINTR, 0xffff);

	SMC_DBG(smc_host, "smc %d done, resp %08x %08x %08x %08x\n", smc_host->pdev->id,
		req->cmd->resp[0], req->cmd->resp[1], req->cmd->resp[2], req->cmd->resp[3]);

	if (req->data && req->data->stop && (smc_host->int_sum & SDXC_IntErrBit)) {
		SMC_MSG(smc_host, "found data error, need to send stop command !!\n");
		sw_mci_send_manual_stop(smc_host, req);
	}

	return ret;
}

/* static s32 sw_mci_set_clk(struct sunxi_mmc_host* smc_host, u32 clk);
 * set clock and the phase of output/input clock incording on
 * the different timing condition
 */
static int sw_mci_set_clk(struct sunxi_mmc_host* smc_host, u32 clk)
{
	struct clk *sclk = NULL;
	u32 mod_clk = 0;
	u32 idiv = 0;
	u32 temp;
	u32 oclk_dly = 0;
	u32 iclk_dly = 0;

	if (clk <= 400000) {
		mod_clk = 24000000;
		sclk = clk_get(&smc_host->pdev->dev, "hosc");
		idiv = 24000000 / clk / 2;
	} else {
		mod_clk = clk;
		sclk = clk_get(&smc_host->pdev->dev, "sata_pll_2");
	}
	if (IS_ERR(sclk)) {
		SMC_ERR(smc_host, "Error to get source clock for clk %dHz\n", clk);
		#ifndef MMC_FPGA
		return -1;
		#endif
	}
	clk_set_parent(smc_host->mclk, sclk);
	clk_set_rate(smc_host->mclk, mod_clk);
	clk_enable(smc_host->mclk);
	clk_enable(smc_host->hclk);
	clk_put(sclk);

	/* set internal divider */
	temp = mci_readl(smc_host, REG_CLKCR);
	temp &= ~0xff;
	temp |= idiv | SDXC_CardClkOn;
	mci_writel(smc_host, REG_CLKCR, temp);
	sw_mci_update_clk(smc_host);
	return 0;
}

static int sw_mci_resource_request(struct sunxi_mmc_host *smc_host)
{
	struct platform_device *pdev = smc_host->pdev;
	u32 smc_no = pdev->id;
	char hclk_name[16] = {0};
	char mclk_name[8] = {0};
	char pio_para[16] = {0};
	struct resource* res = NULL;
	s32 ret = 0;

	/* get sys_config1.fex configuration */
	sprintf(pio_para, "mmc%d_para", smc_no);
	#ifndef MMC_FPGA
	ret = script_parser_fetch(mmc_para, "sdc_detmode", &smc_host->cd_mode, sizeof(int));
	if (ret)
		SMC_ERR(smc_host, "sdc fetch card detect mode failed\n");
	#else
	smc_host->cd_mode = CARD_ALWAYS_PRESENT;
	#endif

	#ifndef MMC_FPGA
	smc_host->pio_hdle = gpio_request_ex(pio_para, NULL);
	if (!smc_host->pio_hdle) {
		SMC_ERR(smc_host, "sdc %d request pio parameter failed\n", smc_no);
		goto out;
	}
	#endif

	res = request_mem_region(SMC_BASE(smc_no), SMC_BASE_OS, pdev->name);
	if (!res) {
		SMC_ERR(smc_host, "Failed to request io memory region.\n");
		ret = -ENOENT;
		goto release_pin;
	}
	smc_host->reg_base = ioremap(res->start, SMC_BASE_OS);
	if (!smc_host->reg_base) {
		SMC_ERR(smc_host, "Failed to ioremap() io memory region.\n");
		ret = -EINVAL;
		goto free_mem_region;
	}

	sprintf(hclk_name, "ahb_sdc%d", smc_no);
	smc_host->hclk = clk_get(&pdev->dev, hclk_name);
	if (IS_ERR(smc_host->hclk)) {
		ret = PTR_ERR(smc_host->hclk);
		SMC_ERR(smc_host, "Error to get ahb clk for %s\n", hclk_name);
		#ifndef MMC_FPGA
		goto iounmap;
		#endif
	}

	sprintf(mclk_name, "sdc%d", smc_no);
	smc_host->mclk = clk_get(&pdev->dev, mclk_name);
	if (IS_ERR(smc_host->mclk)) {
		ret = PTR_ERR(smc_host->mclk);
		SMC_ERR(smc_host, "Error to get clk for mux_mmc\n");
		#ifndef MMC_FPGA
		goto free_hclk;
		#endif
	}

	goto out;

free_hclk:
	clk_put(smc_host->hclk);
iounmap:
	iounmap(smc_host->reg_base);
free_mem_region:
	release_mem_region(SMC_BASE(smc_no), SMC_BASE_OS);
release_pin:
	#ifndef MMC_FPGA
	gpio_release(smc_host->pio_hdle, 1);
	#endif
out:
	return ret;
}


static int sw_mci_resource_release(struct sunxi_mmc_host *smc_host)
{
	clk_disable(smc_host->hclk);
	clk_put(smc_host->hclk);
	clk_disable(smc_host->mclk);
	clk_put(smc_host->mclk);

	iounmap(smc_host->reg_base);
	release_mem_region(SMC_BASE(smc_host->pdev->id), SMC_BASE_OS);

	#ifndef MMC_FPGA
	gpio_release(smc_host->pio_hdle, 1);
	#endif
	return 0;
}

static void sw_mci_suspend_pins(struct sunxi_mmc_host* smc_host)
{
	#ifndef MMC_FPGA
	int ret;
	user_gpio_set_t suspend_gpio_set_io = {"suspend_pins_sdio", 0, 0, 0, 2, 1, 0};     //for sdio
	user_gpio_set_t suspend_gpio_set_card = {"suspend_pins_mmc", 0, 0, 0, 0, 1, 0};    //for mmc card
	user_gpio_set_t *gpio_set = smc_host->pdev->id == 3
					? &suspend_gpio_set_io : &suspend_gpio_set_card;
	u32 i;

	SMC_DBG(smc_host, "mmc %d suspend pins\n", smc_host->pdev->id);
	/* backup gpios' current config */
	ret = gpio_get_all_pin_status(smc_host->pio_hdle, smc_host->bak_gpios, 6, 1);
	if (ret) {
		SMC_ERR(smc_host, "fail to fetch current gpio cofiguration\n");
		return;
	}

	for (i=0; i<6; i++) {
		ret = gpio_set_one_pin_status(smc_host->pio_hdle,
			gpio_set, smc_host->bak_gpios[i].gpio_name, 1);
		if (ret) {
			SMC_ERR(smc_host, "fail to set IO(%s) into suspend status\n",
				smc_host->bak_gpios[i].gpio_name);
		}
	}

	#endif
	smc_host->gpio_suspend_ok = 1;

	return;
}

static void sw_mci_resume_pins(struct sunxi_mmc_host* smc_host)
{
	#ifndef MMC_FPGA
	int ret;
	u32 i;

	SMC_DBG(smc_host, "mmc %d resume pins\n", smc_host->pdev->id);
	/* restore gpios' backup configuration */
	if (smc_host->gpio_suspend_ok) {
		smc_host->gpio_suspend_ok = 0;
		for (i=0; i<6; i++) {
			ret = gpio_set_one_pin_status(smc_host->pio_hdle,
				&smc_host->bak_gpios[i], smc_host->bak_gpios[i].gpio_name, 1);
			if (ret) {
			    SMC_ERR(smc_host, "fail to restore IO(%s) to resume status\n",
					smc_host->bak_gpios[i].gpio_name);
			}
		}
	}
	#endif
}

static void sw_mci_finalize_request(struct sunxi_mmc_host *smc_host)
{
	struct mmc_request* mrq = smc_host->mrq;

	if (smc_host->wait != SDC_WAIT_FINALIZE) {
		SMC_MSG(smc_host, "nothing finalize\n");
		return;
	}

	SMC_DBG(smc_host, "request finalize !!\n");
	sw_mci_request_done(smc_host);

	if (smc_host->error) {
		mrq->cmd->error = -ETIMEDOUT;
		if (mrq->data)
			mrq->data->error = -ETIMEDOUT;
		if (mrq->stop)
			mrq->data->error = -ETIMEDOUT;
	} else {
		if (mrq->data)
			mrq->data->bytes_xfered = (mrq->data->blocks * mrq->data->blksz);
	}

	smc_host->wait = SDC_WAIT_NONE;
	smc_host->mrq = NULL;
	smc_host->error = 0;
	smc_host->state = SDC_STATE_IDLE;
	mmc_request_done(smc_host->mmc, mrq);

	return;
}

static s32 sw_mci_get_ro(struct mmc_host *mmc)
{
	#ifndef MMC_FPGA
	struct sunxi_mmc_host *smc_host = mmc_priv(mmc);
	char mmc_para[16] = {0};
	int card_wp = 0;
	int ret;
	u32 gpio_val;

	sprintf(mmc_para, "mmc%d_para", smc_host->pdev->id);
	ret = script_parser_fetch(mmc_para, "sdc_use_wp", &card_wp, sizeof(int));
	if (ret)
		SMC_ERR(smc_host, "sdc fetch card write protect mode failed\n");
	if (card_wp) {
		gpio_val = gpio_read_one_pin_value(smc_host->pio_hdle, "sdc_wp");
		SMC_DBG(smc_host, "sdc fetch card wp pin status: %d \n", gpio_val);
		if (!gpio_val) {
			smc_host->read_only = 0;
			return 0;
		} else {
			SMC_MSG(smc_host, "Card is write-protected\n");
			smc_host->read_only = 1;
			return 1;
		}
	} else {
		smc_host->read_only = 0;
		return 0;
	}
	#endif
	return 0;//fpga
}

static void sw_mci_cd_timer(unsigned long data)
{
	#ifndef MMC_FPGA
	struct sunxi_mmc_host *smc_host = (struct sunxi_mmc_host *)data;
	u32 gpio_val = 0;
	u32 present;
	u32 i = 0;

	for (i=0; i<5; i++) {
		gpio_val += gpio_read_one_pin_value(smc_host->pio_hdle, "sdc_det");
		mdelay(1);
	}
	if (gpio_val==5)
		present = 0;
	else if (gpio_val==0)
		present = 1;
	else
		goto modtimer;
	SMC_DBG(smc_host, "cd %d, host present %d, cur present %d\n",
			gpio_val, smc_host->present, present);

	if (smc_host->present ^ present) {
		SMC_MSG(smc_host, "mmc %d detect change, present %d\n",
				smc_host->pdev->id, present);
		smc_host->present = present;
		if (smc_host->present)
			mmc_detect_change(smc_host->mmc, msecs_to_jiffies(300));
		else
			mmc_detect_change(smc_host->mmc, msecs_to_jiffies(10));
	}

modtimer:
	mod_timer(&smc_host->cd_timer, jiffies + 30);
	#else
	SMC_ERR(smc_host, "ignore for fpga !!\n");
	#endif
	return;
}

static int sw_mci_card_present(struct mmc_host *mmc)
{
	struct sunxi_mmc_host *smc_host = mmc_priv(mmc);
	return smc_host->present;
}

static irqreturn_t sw_mci_irq(int irq, void *dev_id)
{
	struct sunxi_mmc_host *smc_host = dev_id;
	unsigned long iflags;
	u32 sdio_int = 0;
	u32 raw_int;
	u32 msk_int;
	u32 idma_inte;
	u32 idma_int;

	spin_lock_irqsave(&smc_host->lock, iflags);

	idma_int  = mci_readl(smc_host, REG_IDST);
	idma_inte = mci_readl(smc_host, REG_IDIE);
	msk_int   = mci_readl(smc_host, REG_MISTA);
	raw_int   = mci_readl(smc_host, REG_RINTR);
	smc_host->int_sum |= raw_int;
	SMC_DBG(smc_host, "smc %d irq, ri %08x(%08x) mi %08x ie %08x idi %08x\n",
		smc_host->pdev->id, raw_int, smc_host->int_sum,
		msk_int, idma_inte, idma_int);

	if (msk_int & SDXC_SDIOInt) {
		sdio_int = 1;
		mci_writel(smc_host, REG_RINTR, SDXC_SDIOInt);
	}

	if (smc_host->wait == SDC_WAIT_NONE && !sdio_int) {
		SMC_ERR(smc_host, "smc %x, nothing to complete, raw_int = %08x, "
			"mask_int = %08x\n", smc_host->pdev->id, raw_int, msk_int);

		mci_writew(smc_host, REG_IMASK, 0);
		goto irq_out;
	}

	if ((raw_int & SDXC_IntErrBit) || (idma_int & SDXC_IDMA_ERR)) {
		smc_host->error = raw_int & SDXC_IntErrBit;
		smc_host->wait = SDC_WAIT_FINALIZE;
		goto irq_out;
	}

	if ((smc_host->wait == SDC_WAIT_AUTOCMD_DONE && (msk_int&SDXC_AutoCMDDone))
		|| (smc_host->wait == SDC_WAIT_DATA_OVER && (msk_int&SDXC_DataOver))
		|| (smc_host->wait == SDC_WAIT_CMD_DONE && (msk_int&SDXC_CmdDone)
			&& !(smc_host->int_sum&SDXC_IntErrBit))) {
		smc_host->wait = SDC_WAIT_FINALIZE;
		smc_host->state = SDC_STATE_CMDDONE;
	}

irq_out:
	mci_writel(smc_host, REG_RINTR, msk_int&(~SDXC_SDIOInt));
	mci_writel(smc_host, REG_IDST, idma_int);

	smp_wmb();
	if (smc_host->wait == SDC_WAIT_FINALIZE)
		tasklet_schedule(&smc_host->tasklet);

	spin_unlock_irqrestore(&smc_host->lock, iflags);

	if (sdio_int)
		mmc_signal_sdio_irq(smc_host->mmc);

	return IRQ_HANDLED;
}

static void sw_mci_tasklet(unsigned long data)
{
	struct sunxi_mmc_host *smc_host = (struct sunxi_mmc_host *) data;
	sw_mci_finalize_request(smc_host);
}

static void sw_mci_set_ios(struct mmc_host *mmc, struct mmc_ios *ios)
{
	struct sunxi_mmc_host *smc_host = mmc_priv(mmc);
	u32 temp;

	SMC_MSG(smc_host, "smc %d set ios, bus %x timing %x clk %d power %d\n", smc_host->pdev->id,
		1 << ios->bus_width, ios->timing, ios->clock, ios->power_mode);
	/* set bus width */
	switch (ios->bus_width) {
		case MMC_BUS_WIDTH_1:
			mci_writel(smc_host, REG_WIDTH, SDXC_WIDTH1);
			smc_host->bus_width = 1;
			break;
		case MMC_BUS_WIDTH_4:
			mci_writel(smc_host, REG_WIDTH, SDXC_WIDTH4);
			smc_host->bus_width = 4;
			break;
		case MMC_BUS_WIDTH_8:
			mci_writel(smc_host, REG_WIDTH, SDXC_WIDTH8);
			smc_host->bus_width = 8;
			break;
	}

	/* set ddr mode */
	temp = mci_readl(smc_host, REG_GCTRL);
	if (ios->timing == MMC_TIMING_UHS_DDR50) {
		temp |= SDXC_DDR_MODE;
		smc_host->ddr = 1;
	} else {
		temp &= ~SDXC_DDR_MODE;
		smc_host->ddr = 0;
	}
	mci_writel(smc_host, REG_GCTRL, temp);

	/* set up clock */
	if (ios->clock) {
		/* 8bit ddr, mod_clk = 2 * card_clk */
		if (smc_host->ddr && smc_host->bus_width == 8)
			smc_host->mod_clk = ios->clock << 1;
		else
			smc_host->mod_clk = ios->clock;
		smc_host->card_clk = ios->clock;

		sw_mci_set_clk(smc_host, smc_host->card_clk);
	}

	/* Set the power state */
	switch (ios->power_mode) {
		case MMC_POWER_ON:
		case MMC_POWER_UP:
			if (!smc_host->power_on) {
				SMC_MSG(smc_host, "mmc %d power on !!\n", smc_host->pdev->id);
				sw_mci_resume_pins(smc_host);
				clk_enable(smc_host->hclk);
				clk_enable(smc_host->mclk);
				sw_mci_update_clk(smc_host);
				enable_irq(smc_host->irq);
				smc_host->power_on = 1;
			}
			break;
		case MMC_POWER_OFF:
			if (smc_host->power_on) {
				SMC_MSG(smc_host, "mmc %d power off !!\n", smc_host->pdev->id);
				disable_irq(smc_host->irq);
				clk_disable(smc_host->mclk);
				clk_disable(smc_host->hclk);
				sw_mci_suspend_pins(smc_host);
				smc_host->power_on = 0;
				smc_host->ferror = 0;
			}
		break;
	}
}

static void sw_mci_enable_sdio_irq(struct mmc_host *mmc, int enable)
{
	struct sunxi_mmc_host *smc_host = mmc_priv(mmc);
	unsigned long flags;
	u32 imask;

	spin_lock_irqsave(&smc_host->lock, flags);
	imask = mci_readl(smc_host, REG_IMASK);
	if (enable)
		imask |= SDXC_SDIOInt;
	else
		imask &= ~SDXC_SDIOInt;
	mci_writel(smc_host, REG_IMASK, imask);
	spin_unlock_irqrestore(&smc_host->lock, flags);
}

void sw_mci_hw_reset(struct mmc_host *mmc)
{
	struct sunxi_mmc_host *smc_host = mmc_priv(mmc);
	u32 id = smc_host->pdev->id;

	if (id == 2 || id == 3) {
		mci_writel(smc_host, REG_HWRST, 0);
		udelay(10);
		mci_writel(smc_host, REG_HWRST, 1);
		udelay(300);
	}
}

static void sw_mci_request(struct mmc_host *mmc, struct mmc_request *mrq)
{
	struct sunxi_mmc_host *smc_host = mmc_priv(mmc);
	struct mmc_command* cmd = mrq->cmd;
	struct mmc_data* data = mrq->data;
	u32 byte_cnt = 0;
	int ret;

	if (sw_mci_card_present(mmc) == 0 || smc_host->ferror || !smc_host->power_on) {
		SMC_DBG(smc_host, "no medium present, ferr %d, pwd %d\n",
			    smc_host->ferror, smc_host->power_on);
		smc_host->mrq->cmd->error = -ENOMEDIUM;
		mmc_request_done(mmc, mrq);
		return;
	}

	SMC_DBG(smc_host, "smc %d cmd %d arg %08x\n", smc_host->pdev->id, cmd->opcode, cmd->arg);
	smc_host->mrq = mrq;
	smc_host->int_sum = 0;
	smc_host->state = SDC_STATE_IDLE;
	if (data) {
		byte_cnt = data->blksz * data->blocks;
		SMC_DBG(smc_host, "-> with data %d bytes, sg_len %d\n", byte_cnt, data->sg_len);

		mci_writel(smc_host, REG_BLKSZ, data->blksz);
		mci_writel(smc_host, REG_BCNTR, byte_cnt);
		ret = sw_mci_prepare_dma(smc_host, data);
		if (ret < 0) {
			SMC_ERR(smc_host, "smc %d prepare DMA failed\n", smc_host->pdev->id);
			cmd->error = ret;
			cmd->data->error = ret;
			mmc_request_done(smc_host->mmc, mrq);
			return;
		}
	}
	sw_mci_send_cmd(smc_host, cmd);
}

void sw_mci_rescan_card(unsigned id, unsigned insert)
{
	struct sunxi_mmc_host *smc_host = NULL;

	BUG_ON(id > 3);
	BUG_ON(sw_host[id] == NULL);
	smc_host = sw_host[id];

	smc_host->present = insert ? 1 : 0;
	mmc_detect_change(smc_host->mmc, 0);
	return;
}
EXPORT_SYMBOL_GPL(sw_mci_rescan_card);

static struct mmc_host_ops sw_mci_ops = {
	.request	= sw_mci_request,
	.set_ios	= sw_mci_set_ios,
	.get_ro		= sw_mci_get_ro,
	.get_cd		= sw_mci_card_present,
	.enable_sdio_irq= sw_mci_enable_sdio_irq,
	.hw_reset	= sw_mci_hw_reset
};

#ifdef CONFIG_PROC_FS
#include <linux/proc_fs.h>
static const char sw_mci_drv_version[] = DRIVER_VERSION;
static int sw_mci_proc_drvversion(char *page, char **start, off_t off,
					int count, int *eof, void *data)
{
	char *p = page;

	p += sprintf(p, "%s\n", sw_mci_drv_version);
	return p - page;
}

static int sw_mci_proc_hostinfo(char *page, char **start, off_t off,
					int count, int *eof, void *data)
{
	char *p = page;
	struct sunxi_mmc_host *smc_host = (struct sunxi_mmc_host *)data;
	struct device* dev = &smc_host->pdev->dev;
	char* cd_mode[] = {"none", "gpio mode", "data3 mode", "always in", "manual"};
	char* state[] = {"Idle", "sending cmd", "cmd done"};

	p += sprintf(p, "%s Host Info:\n", dev_name(dev));
	p += sprintf(p, "Reg Base  : %p\n", smc_host->reg_base);
	p += sprintf(p, "Mod Clock : %d\n", smc_host->mod_clk);
	p += sprintf(p, "Card Clock: %d\n", smc_host->card_clk);
	p += sprintf(p, "Bus Width : %d\n", smc_host->bus_width);
	p += sprintf(p, "DDR Mode  : %d\n", smc_host->ddr);
	p += sprintf(p, "Present   : %d\n", smc_host->present);
	p += sprintf(p, "CD Mode   : %s\n", cd_mode[smc_host->cd_mode]);
	p += sprintf(p, "Read Only : %d\n", smc_host->read_only);
	p += sprintf(p, "State     : %s\n", state[smc_host->state]);

	return p - page;
}

static int sw_mci_proc_read_regs(char *page, char **start, off_t off,
				int count, int *eof, void *data)
{
	char *p = page;
	struct sunxi_mmc_host *smc_host = (struct sunxi_mmc_host *)data;
	u32 i;

	p += sprintf(p, "Dump smc regs:\n");
	for (i=0; i<0x100; i+=4) {
		if (!(i&0xf))
			p += sprintf(p, "\n0x%08x : ", i);
		p += sprintf(p, "%08x ", readl(smc_host->reg_base + i));
	}
	p += sprintf(p, "\n");

	p += sprintf(p, "Dump ccmu regs:\n");
	for (i=0; i<0x200; i+=4) {
		if (!(i&0xf))
			p += sprintf(p, "\n0x%08x : ", i);
		p += sprintf(p, "%08x ", readl(IO_ADDRESS(AW_CCM_BASE) + i));
	}
	p += sprintf(p, "\n");

	p += sprintf(p, "Dump gpio regs:\n");
	for (i=0; i<0x200; i+=4) {
		if (!(i&0xf))
			p += sprintf(p, "\n0x%08x : ", i);
		p += sprintf(p, "%08x ", readl(IO_ADDRESS(AW_PIO_BASE)+ i));
	}
	p += sprintf(p, "\n");


	return p - page;
}

static int sw_mci_proc_read_dbglevel(char *page, char **start, off_t off,
					int count, int *eof, void *data)
{
	char *p = page;
	struct sunxi_mmc_host *smc_host = (struct sunxi_mmc_host *)data;

	p += sprintf(p, "Debug-Level : 0- msg&err, 1- +info, 2- +dbg, 3- all\n");
	p += sprintf(p, "current debug-level : %d\n", smc_host->debuglevel);
	return p - page;
}

static int sw_mci_proc_write_dbglevel(struct file *file, const char __user *buffer,
					unsigned long count, void *data)
{
	u32 smc_debug;
	struct sunxi_mmc_host *smc_host = (struct sunxi_mmc_host *)data;
	smc_debug = simple_strtoul(buffer, NULL, 10);

	smc_host->debuglevel = smc_debug;
	return sizeof(smc_debug);
}

static int sw_mci_proc_read_insert_status(char *page, char **start, off_t off,
					int coutn, int *eof, void *data)
{
	char *p = page;
	struct sunxi_mmc_host *smc_host = (struct sunxi_mmc_host *)data;

	p += sprintf(p, "Usage: \"echo 1 > insert\" to scan card and "
			"\"echo 0 > insert\" to remove card\n");
	if (smc_host->cd_mode != CARD_DETECT_BY_FS)
		p += sprintf(p, "Sorry, this node if only for manual "
				"attach mode(cd mode 4)\n");

	p += sprintf(p, "card attach status: %s\n",
		smc_host->present ? "inserted" : "removed");


	return p - page;
}

static int sw_mci_proc_card_insert_ctrl(struct file *file, const char __user *buffer,
					unsigned long count, void *data)
{
	u32 insert = simple_strtoul(buffer, NULL, 10);
	struct sunxi_mmc_host *smc_host = (struct sunxi_mmc_host *)data;
	u32 present = insert ? 1 : 0;

	if (smc_host->present ^ present) {
		smc_host->present = present;
		mmc_detect_change(smc_host->mmc, msecs_to_jiffies(300));
	}

	return sizeof(insert);
}

void sw_mci_procfs_attach(struct sunxi_mmc_host *smc_host)
{
	struct device *dev = &smc_host->pdev->dev;
	char sw_mci_proc_rootname[32] = {0};

	//make mmc dir in proc fs path
	snprintf(sw_mci_proc_rootname, sizeof(sw_mci_proc_rootname),
			"driver/%s", dev_name(dev));
	smc_host->proc_root = proc_mkdir(sw_mci_proc_rootname, NULL);
	if (IS_ERR(smc_host->proc_root))
		SMC_MSG(smc_host, "%s: failed to create procfs \"driver/mmc\".\n", dev_name(dev));

	smc_host->proc_drvver = create_proc_read_entry("drv-version", 0444,
				smc_host->proc_root, sw_mci_proc_drvversion, NULL);
	if (IS_ERR(smc_host->proc_root))
		SMC_MSG(smc_host, "%s: failed to create procfs \"drv-version\".\n", dev_name(dev));

	smc_host->proc_hostinfo = create_proc_read_entry("hostinfo", 0444,
				smc_host->proc_root, sw_mci_proc_hostinfo, smc_host);
	if (IS_ERR(smc_host->proc_hostinfo))
		SMC_MSG(smc_host, "%s: failed to create procfs \"hostinfo\".\n", dev_name(dev));

	smc_host->proc_regs = create_proc_read_entry("register", 0444,
				smc_host->proc_root, sw_mci_proc_read_regs, smc_host);
	if (IS_ERR(smc_host->proc_regs))
		SMC_MSG(smc_host, "%s: failed to create procfs \"hostinfo\".\n", dev_name(dev));

	smc_host->proc_dbglevel = create_proc_entry("debug-level", 0644, smc_host->proc_root);
	if (IS_ERR(smc_host->proc_dbglevel))
		SMC_MSG(smc_host, "%s: failed to create procfs \"debug-level\".\n", dev_name(dev));

	smc_host->proc_dbglevel->data = smc_host;
	smc_host->proc_dbglevel->read_proc = sw_mci_proc_read_dbglevel;
	smc_host->proc_dbglevel->write_proc = sw_mci_proc_write_dbglevel;

	smc_host->proc_insert = create_proc_entry("insert", 0644, smc_host->proc_root);
	if (IS_ERR(smc_host->proc_insert))
		SMC_MSG(smc_host, "%s: failed to create procfs \"insert\".\n", dev_name(dev));

	smc_host->proc_insert->data = smc_host;
	smc_host->proc_insert->read_proc = sw_mci_proc_read_insert_status;
	smc_host->proc_insert->write_proc = sw_mci_proc_card_insert_ctrl;

}

void sw_mci_procfs_remove(struct sunxi_mmc_host *smc_host)
{
	struct device *dev = &smc_host->pdev->dev;
	char sw_mci_proc_rootname[32] = {0};

	snprintf(sw_mci_proc_rootname, sizeof(sw_mci_proc_rootname),
		"driver/%s", dev_name(dev));
	remove_proc_entry("insert", smc_host->proc_root);
	remove_proc_entry("debug-level", smc_host->proc_root);
	remove_proc_entry("register", smc_host->proc_root);
	remove_proc_entry("hostinfo", smc_host->proc_root);
	remove_proc_entry("drv-version", smc_host->proc_root);
	remove_proc_entry(sw_mci_proc_rootname, NULL);
}

#else

void sw_mci_procfs_attach(struct sunxi_mmc_host *smc_host) { }
void sw_mci_procfs_remove(struct sunxi_mmc_host *smc_host) { }

#endif	//PROC_FS

static int __devinit sw_mci_probe(struct platform_device *pdev)
{
	struct sunxi_mmc_host *smc_host = NULL;
	struct mmc_host	*mmc = NULL;
	int ret = 0;

	mmc = mmc_alloc_host(sizeof(struct sunxi_mmc_host), &pdev->dev);
	if (!mmc) {
		SMC_ERR(smc_host, "mmc alloc host failed\n");
		ret = -ENOMEM;
		goto probe_out;
	}

	smc_host = mmc_priv(mmc);
	memset((void*)smc_host, 0, sizeof(smc_host));

	smc_host->mmc	= mmc;
	smc_host->pdev	= pdev;

	mmc->ops        = &sw_mci_ops;
	mmc->ocr_avail	= MMC_VDD_32_33 | MMC_VDD_33_34;
	mmc->caps	= MMC_CAP_4_BIT_DATA|MMC_CAP_MMC_HIGHSPEED
			|MMC_CAP_SD_HIGHSPEED|MMC_CAP_SDIO_IRQ;
	mmc->f_min	= 400000;
	mmc->f_max      = 150000000;
	if (pdev->id==3)
//		mmc->pm_flags = MMC_PM_IGNORE_PM_NOTIFY;
		mmc->pm_flags = MMC_PM_KEEP_POWER;	//fpga

	mmc->max_blk_count	= 512;
	mmc->max_blk_size	= 65536;
	mmc->max_req_size	= mmc->max_blk_size * mmc->max_blk_count;
	mmc->max_seg_size	= mmc->max_req_size;
	mmc->max_segs	    	= 64;

	spin_lock_init(&smc_host->lock);
	tasklet_init(&smc_host->tasklet, sw_mci_tasklet, (unsigned long) smc_host);

	if (sw_mci_resource_request(smc_host)) {
		SMC_ERR(smc_host, "%s: Failed to get resouce.\n", dev_name(&pdev->dev));
		#ifndef MMC_FPGA
		goto probe_free_host;
		#endif
	}

	sw_mci_init_host(smc_host);
	sw_mci_procfs_attach(smc_host);

	smc_host->irq = SMC_IRQNO(pdev->id);
	if (request_irq(smc_host->irq, sw_mci_irq, 0, DRIVER_NAME, smc_host)) {
		SMC_ERR(smc_host, "Failed to request smc card interrupt.\n");
		ret = -ENOENT;
		goto probe_free_resource;
	}

	if (smc_host->cd_mode == CARD_ALWAYS_PRESENT)
		smc_host->present = 1;
	else if (smc_host->cd_mode == CARD_DETECT_BY_GPIO) {
		init_timer(&smc_host->cd_timer);
		smc_host->cd_timer.expires = jiffies + 1*HZ;
		smc_host->cd_timer.function = &sw_mci_cd_timer;
		smc_host->cd_timer.data = (unsigned long)smc_host;
		add_timer(&smc_host->cd_timer);
		smc_host->present = 0;
	}

	ret = mmc_add_host(mmc);
	if (ret) {
		SMC_ERR(smc_host, "Failed to add mmc host.\n");
		goto probe_free_irq;
	}
	platform_set_drvdata(pdev, mmc);
	sw_host[pdev->id] = smc_host;

	SMC_MSG(smc_host, "mmc%d Probe: base:0x%p irq:%u sg_cpu:%p(%x) ret %d.\n",
		pdev->id, smc_host->reg_base, smc_host->irq,
		smc_host->sg_cpu, smc_host->sg_dma, ret);

	goto probe_out;

probe_free_irq:
	if (smc_host->irq)
		free_irq(smc_host->irq, smc_host);
probe_free_resource:
	sw_mci_resource_release(smc_host);
probe_free_host:
	mmc_free_host(mmc);
probe_out:
	return ret;
}

static int __devexit sw_mci_remove(struct platform_device *pdev)
{
	struct mmc_host    	*mmc  = platform_get_drvdata(pdev);
	struct sunxi_mmc_host	*smc_host = mmc_priv(mmc);

	SMC_MSG(smc_host, "%s: Remove.\n", dev_name(&pdev->dev));

	sw_mci_exit_host(smc_host);

	sw_mci_procfs_remove(smc_host);
	mmc_remove_host(mmc);

	tasklet_disable(&smc_host->tasklet);
	free_irq(smc_host->irq, smc_host);
	if (smc_host->cd_mode == CARD_DETECT_BY_GPIO)
		del_timer(&smc_host->cd_timer);

	sw_mci_resource_release(smc_host);

	mmc_free_host(mmc);
	sw_host[pdev->id] = NULL;

	return 0;
}

#ifdef CONFIG_PM

void sw_mci_regs_save(struct sunxi_mmc_host* smc_host)
{
	struct sunximmc_ctrl_regs* bak_regs = &smc_host->bak_regs;

	bak_regs->gctrl		= mci_readl(smc_host, REG_GCTRL);
	bak_regs->clkc		= mci_readl(smc_host, REG_CLKCR);
	bak_regs->timeout	= mci_readl(smc_host, REG_TMOUT);
	bak_regs->buswid	= mci_readl(smc_host, REG_WIDTH);
	bak_regs->waterlvl	= mci_readl(smc_host, REG_FTRGL);
	bak_regs->funcsel	= mci_readl(smc_host, REG_FUNS);
	bak_regs->debugc	= mci_readl(smc_host, REG_DBGC);
	bak_regs->idmacc	= mci_readl(smc_host, REG_DMAC);
}

void sw_mci_regs_restore(struct sunxi_mmc_host* smc_host)
{
	struct sunximmc_ctrl_regs* bak_regs = &smc_host->bak_regs;

	mci_writel(smc_host, REG_GCTRL, bak_regs->gctrl   );
	mci_writel(smc_host, REG_CLKCR, bak_regs->clkc    );
	mci_writel(smc_host, REG_TMOUT, bak_regs->timeout );
	mci_writel(smc_host, REG_WIDTH, bak_regs->buswid  );
	mci_writel(smc_host, REG_FTRGL, bak_regs->waterlvl);
	mci_writel(smc_host, REG_FUNS , bak_regs->funcsel );
	mci_writel(smc_host, REG_DBGC , bak_regs->debugc  );
	mci_writel(smc_host, REG_DMAC , bak_regs->idmacc  );
}


static int sw_mci_suspend(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct mmc_host *mmc = platform_get_drvdata(pdev);
	int ret = 0;

	if (mmc) {
		struct sunxi_mmc_host *smc_host = mmc_priv(mmc);

		if (mmc->card && (mmc->card->type!=MMC_TYPE_SDIO || mmc_pm_io_shd_suspend_host()))
			ret = mmc_suspend_host(mmc);

		if (smc_host->power_on) {
			/* disable irq */
			disable_irq(smc_host->irq);
			sw_mci_regs_save(smc_host);
			clk_disable(smc_host->mclk);
			if (mmc->card && mmc->card->type!=MMC_TYPE_SDIO)
				clk_disable(smc_host->hclk);
			sw_mci_suspend_pins(smc_host);
		}
	}

	SMC_DBG(smc_host, "smc %d suspend\n", pdev->id);
	return ret;
}

static int sw_mci_resume(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct mmc_host *mmc = platform_get_drvdata(pdev);
	int ret = 0;

	if (mmc) {
		struct sunxi_mmc_host *smc_host = mmc_priv(mmc);

		if (smc_host->power_on) {
			/* resume pins to correct status */
			sw_mci_resume_pins(smc_host);
			if (mmc->card && mmc->card->type!=MMC_TYPE_SDIO)
				clk_enable(smc_host->hclk);
			clk_enable(smc_host->mclk);
			if (mmc->card && mmc->card->type!=MMC_TYPE_SDIO)
				sw_mci_regs_restore(smc_host);
			sw_mci_update_clk(smc_host);
			enable_irq(smc_host->irq);
		}

		if (mmc->card && (mmc->card->type!=MMC_TYPE_SDIO || mmc_pm_io_shd_suspend_host()))
			ret = mmc_resume_host(mmc);
	}

	SMC_DBG(smc_host, "smc %d resume\n", pdev->id);
	return ret;
}

static const struct dev_pm_ops sw_mci_pm = {
	.suspend	= sw_mci_suspend,
	.resume		= sw_mci_resume,
};
#define sw_mci_pm_ops &sw_mci_pm

#else /* CONFIG_PM */

#define sw_mci_pm_ops NULL

#endif /* CONFIG_PM */

static struct platform_device awmmc_device[4] = {
	[0] = {.name = DRIVER_NAME, .id = 0},
	[1] = {.name = DRIVER_NAME, .id = 1},
	[2] = {.name = DRIVER_NAME, .id = 2},
	[3] = {.name = DRIVER_NAME, .id = 3},
};

static struct platform_driver sw_mci_driver = {
	.driver.name    = DRIVER_NAME,
	.driver.owner   = THIS_MODULE,
	.driver.pm	= sw_mci_pm_ops,
	.probe          = sw_mci_probe,
	.remove         = __devexit_p(sw_mci_remove),
};

static int sdc_used;
static int __init sw_mci_init(void)
{
	int ret;
	int i;
	char mmc_para[16] = {0};
	int used = 0;

	SMC_MSG(smc_host, "sw_mci_init\n");
	for (i=0; i<4; i++) {
		memset(mmc_para, 0, sizeof(mmc_para));
		sprintf(mmc_para, "mmc%d_para", i);
		used = 0;
		#ifndef MMC_FPGA
		ret = script_parser_fetch(mmc_para,"sdc_used", &used, sizeof(int));
		if (ret)
			printk("sw_mci_init fetch mmc%d using configuration failed\n", i);
		#else
		ret = ret;
		used = i==2;
		#endif

		if (used) {
			sdc_used |= 1 << i;
			platform_device_register(&awmmc_device[i]);
		}

	}

	SMC_MSG(smc_host, "sunxi mmc host using config : 0x%x\n", sdc_used);

	if (sdc_used)
		return platform_driver_register(&sw_mci_driver);

	return 0;
}

static void __exit sw_mci_exit(void)
{
	SMC_MSG(smc_host, "sw_mci_exit\n");
	if (sdc_used) {
		sdc_used = 0;
		platform_driver_unregister(&sw_mci_driver);
	}
}


module_init(sw_mci_init);
module_exit(sw_mci_exit);

MODULE_DESCRIPTION("Winner's SD/MMC Card Controller Driver");
MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Aaron.maoye<leafy.myeh@allwinnertech.com>");
MODULE_ALIAS("platform:sunxi-mmc");
