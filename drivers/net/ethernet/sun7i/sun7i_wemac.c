/*
 *  WEMAC Fast Ethernet driver for Linux.
 * 	Copyright (C) 2012 ShugeLinux@gmail.com
 *
 * 	This program is free software; you can redistribute it and/or
 * 	modify it under the terms of the GNU General Public License
 * 	as published by the Free Software Foundation; either version 2
 * 	of the License, or (at your option) any later version.
 *
 * 	This program is distributed in the hope that it will be useful,
 * 	but WITHOUT ANY WARRANTY; without even the implied warranty of
 * 	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * 	GNU General Public License for more details.
 *
 */

#include <linux/module.h>
#include <linux/ioport.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/init.h>
#include <linux/skbuff.h>
#include <linux/spinlock.h>
#include <linux/crc32.h>
#include <linux/mii.h>
#include <linux/ethtool.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/clk.h>
#include <linux/ctype.h>
#include <linux/pm.h>

#include <asm/cacheflush.h>
#include <asm/delay.h>
#include <asm/irq.h>
#include <asm/io.h>

#include <mach/dma.h>
#include <mach/sys_config.h>
#include <mach/clock.h>
#include <mach/platform.h>

#include "sun7i_wemac.h"

#define EMAC_ON_FPGA

#undef	PHY_POWER
#undef	DYNAMIC_MAC_SYSCONFIG
#undef	SYSCONFIG_GPIO
//#undef	SYSCONFIG_CCMU
#define	SYSCONFIG_CCMU

/* Board/System/Debug information/definition ---------------- */

#define CARDNAME	"wemac"
#define DRV_VERSION	"3.00"
#define DMA_CPU_TRRESHOLD 2000

/* debug code */
#define CONFIG_WEMAC_DEBUGLEVEL 12

#ifdef DEBUG

#define wemac_dbg(db, lev, msg...) do {		\
	if ((lev) < CONFIG_WEMAC_DEBUGLEVEL){	\
		dev_dbg(db->dev, msg);			\
	}						\
} while (0)

#else

#define wemac_dbg(db, lev, msg...) do {		\
} while (0)

#endif

#ifndef PHY_MAX_ADDR
#define PHY_MAX_ADDR 0x1f
#endif

#define MAC_ADDRESS "00:00:00:00:00:00"
static char *mac_str = MAC_ADDRESS;
module_param(mac_str, charp, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(mac_str, "MAC Address String, ex. xx:xx:xx:xx:xx:xx");

/*
 * Transmit timeout, default 5 seconds.
 */
static int 	watchdog = 5000;
module_param(watchdog, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(watchdog, "Transmit timeout in milliseconds");

static int phy_addr = -1;
module_param(phy_addr, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(phy_addr, "Physical device address");

/* WEMAC register address locking.
 *
 * The WEMAC uses an address register to control where data written
 * to the data register goes. This means that the address register
 * must be preserved over interrupts or similar calls.
 *
 * During interrupt and other critical calls, a spinlock is used to
 * protect the system, but the calls themselves save the address
 * in the address register in case they are interrupting another
 * access to the device.
 *
 * For general accesses a lock is provided so that calls which are
 * allowed to sleep are serialised so that the address register does
 * not need to be saved. This lock also serves to serialise access
 * to the EEPROM and PHY access registers which are shared between
 * these two devices.
 */

/* The driver supports the original WEMACE, and now the two newer
 * devices, WEMACA and WEMACB.
 */

/* Structure/enum declaration ------------------------------- */
typedef struct wemac_board_info {

	void __iomem	*emac_vbase;	/* mac I/O base address */
	void __iomem	*sram_vbase;	/* sram control I/O base address */
#ifndef SYSCONFIG_GPIO
	void __iomem	*gpio_vbase;	/* gpio I/O base address */
#else
	int gpio_hd;
#endif

#ifndef SYSCONFIG_CCMU
	void __iomem	*ccmu_vbase;	/* ccmu I/O base address */
#else
	struct clk *emac_clk;
#endif

	u16		tx_fifo_stat;
	u32		phy_reg0;
	u32		link_up;

	void (*inblk)(void __iomem *port, void *data, int length);
	void (*outblk)(void __iomem *port, void *data, int length);
	void (*dumpblk)(void __iomem *port, int length);

	struct device	*dev;	     /* parent device */

	struct mutex	addr_lock;	/* phy and eeprom access lock */
	spinlock_t		lock;

	struct delayed_work phy_poll;
	struct net_device  *ndev;

	struct mii_if_info mii;
	u32		msg_enable;

#ifdef PHY_POWER
	user_gpio_set_t *mos_gpio;
	u32 mos_pin_handler;
#endif

	struct standby_data *reg_saves;
	u32 multi_filter[2];
} wemac_board_info_t;

static inline wemac_board_info_t *to_wemac_board(struct net_device *dev)
{
	return netdev_priv(dev);
}

static int  wemac_phy_read(struct net_device *dev, int phyaddr_unused, int reg);
static void wemac_phy_write(struct net_device *dev,
							int phyaddr_unused, int reg, int value);
static void wemac_rx(struct net_device *dev);
static void wemac_get_macaddr(wemac_board_info_t *db);
static void emac_reg_dump(wemac_board_info_t *db);

struct sw_dma_client emacrx_dma_client = {
	.name="EMACRX_DMA",
};

struct sw_dma_client emactx_dma_client = {
	.name="EMACTX_DMA",
};

int ch_rx, ch_tx;
static int emacrx_dma_completed_flag = 1;
static int emactx_dma_completed_flag = 1;
static int emacrx_completed_flag = 1;

void emacrx_dma_buffdone(struct sw_dma_chan * ch, void *buf
							,int size,enum sw_dma_buffresult result)
{
	struct net_device *dev = ch->dev_id;
	wemac_rx(dev);
}

int  emacrx_dma_opfn(struct sw_dma_chan *ch, enum sw_chan_op op_code){
	if(op_code == SW_DMAOP_START)
		emacrx_dma_completed_flag = 0;
	return 0;
}

void emactx_dma_buffdone(struct sw_dma_chan *ch, void *buf
							,int size,enum sw_dma_buffresult result){
	emactx_dma_completed_flag = 1;
}

int  emactx_dma_opfn(struct sw_dma_chan *ch, enum sw_chan_op op_code){
	if(op_code == SW_DMAOP_START)
		emactx_dma_completed_flag = 0;
	return 0;
}

//__hdle emac_RequestDMA  (__u32 dmatype)
//{
//	__hdle ch;
//
//	ch = sw_dma_request(dmatype, &nand_dma_client, NULL);
//	if(ch < 0)
//		return ch;
//
//	sw_dma_set_opfn(ch, nanddma_opfn);
//	sw_dma_set_buffdone_fn(ch, nanddma_buffdone);
//
//	return ch;
//}


void eLIBs_CleanFlushDCacheRegion(void *adr, __u32 bytes)
{
	__cpuc_flush_dcache_area(adr, bytes + (1 << 5) * 2 - 2);
}

int seq_rx=0;
__s32 emacrx_DMAEqueueBuf(int hDma,  void * buff_addr, __u32 len)
{
	eLIBs_CleanFlushDCacheRegion((void *)buff_addr, len);

	emacrx_dma_completed_flag = 0;
	return sw_dma_enqueue(hDma, (void*)(seq_rx++), (dma_addr_t)buff_addr, len);
}

int seq_tx=0;
__s32 emactx_DMAEqueueBuf(int hDma, void *buff_addr, __u32 len)
{
	eLIBs_CleanFlushDCacheRegion(buff_addr, len);

	emactx_dma_completed_flag = 0;
	return sw_dma_enqueue(hDma, (void*)(seq_tx++), (dma_addr_t)buff_addr, len);
}

int wemac_dma_config_start(__u8 rw, void *buff_addr, __u32 len)
{
	int ret;
	if(rw == 0){
		struct dma_hw_conf emac_hwconf = {
			.xfer_type = DMAXFER_D_SWORD_S_SWORD,
			.hf_irq = SW_DMA_IRQ_FULL,
			.cmbk = 0x03030303,
			.dir = SW_DMA_RDEV,
			.from = 0x01C0B04C,
			.address_type = DMAADDRT_D_LN_S_IO,
			.drqsrc_type = DRQ_TYPE_EMAC
		};

		ret = sw_dma_setflags(ch_rx, SW_DMAF_AUTOSTART);
		if(ret!=0)
			return ret;
		ret = sw_dma_config(ch_rx, &emac_hwconf);
		if(ret!=0)
			return ret;
		ret = emacrx_DMAEqueueBuf(ch_rx, buff_addr, len);
		if(ret!=0)
			return ret;
		ret = sw_dma_ctrl(ch_rx, SW_DMAOP_START);
	} else {
		struct dma_hw_conf emac_hwconf = {
			.xfer_type = DMAXFER_D_SWORD_S_SWORD,
			.hf_irq = SW_DMA_IRQ_FULL,
			.cmbk = 0x03030303,
			.dir = SW_DMA_WDEV,
			.to = 0x01C0B024,
			.address_type = DMAADDRT_D_IO_S_LN,
			.drqdst_type = DRQ_TYPE_EMAC
		};

		ret = sw_dma_setflags(ch_tx, SW_DMAF_AUTOSTART);
		if(ret!=0)
			return ret;
		ret = sw_dma_config(ch_tx, &emac_hwconf);
		if(ret!=0)
			return ret;
		ret = emactx_DMAEqueueBuf(ch_tx, buff_addr, len);
		if(ret!=0)
			return ret;
		ret = sw_dma_ctrl(ch_tx, SW_DMAOP_START);
	}
	return 0;
}

__s32 emacrx_WaitDmaFinish(void)
{
	unsigned long flags;
	int i;

	while(1){
		local_irq_save(flags);
		if (emacrx_dma_completed_flag){
			local_irq_restore(flags);
			break;
		}
		for(i=0;i<1000;i++);
		local_irq_restore(flags);
	}

	return 0;
}

__s32 emactx_WaitDmaFinish(void)
{
	while(1){
		poll_dma_pending(ch_tx);
		if (emactx_dma_completed_flag) break;
	}

	return 0;
}

/* WEMAC network board routine ---------------------------- */
static void wemac_reset(wemac_board_info_t * db)
{
	printk(KERN_WARNING "Resetting device\n");

	/* RESET device */
	writel(0, db->emac_vbase + EMAC_CTL_REG);
	udelay(100);
	writel(1, db->emac_vbase + EMAC_CTL_REG);
	udelay(100);
}

#if 0
static void wemac_outblk_dma(void __iomem *reg, void *data, int count)
{
	wemac_dma_config_start(1, data, count);
	emactx_WaitDmaFinish();
}
#else
static int wemac_inblk_dma(void __iomem * reg,void * data,int count)
{
	return wemac_dma_config_start(0, data, count);
}
#endif
static void wemac_outblk_32bit(void __iomem *reg, void *data, int count)
{
	writesl(reg, data, (count+3) >> 2);
}

static void wemac_inblk_32bit(void __iomem *reg, void *data, int count)
{
	readsl(reg, data, (count+3) >> 2);
}

static void wemac_dumpblk_32bit(void __iomem *reg, int count)
{
	int i;
	int tmp;

	count = (count + 3) >> 2;
	for (i = 0; i < count; i++)
		tmp = readl(reg);
}

static void wemac_schedule_poll(wemac_board_info_t *db)
{
	schedule_delayed_work(&db->phy_poll, HZ * 1);
}

static int wemac_ioctl(struct net_device *dev, struct ifreq *req, int cmd)
{
	wemac_board_info_t *dm = to_wemac_board(dev);

	if (!netif_running(dev))
		return -EINVAL;

	return generic_mii_ioctl(&dm->mii, if_mii(req), cmd, NULL);
}

/* ethtool ops */
static void wemac_get_drvinfo(struct net_device *dev,
		struct ethtool_drvinfo *info)
{
	wemac_board_info_t *dm = to_wemac_board(dev);

	strcpy(info->driver, CARDNAME);
	strcpy(info->version, DRV_VERSION);
	strcpy(info->bus_info, to_platform_device(dm->dev)->name);
}

static u32 wemac_get_msglevel(struct net_device *dev)
{
	wemac_board_info_t *dm = to_wemac_board(dev);

	return dm->msg_enable;
}

static void wemac_set_msglevel(struct net_device *dev, u32 value)
{
	wemac_board_info_t *dm = to_wemac_board(dev);

	dm->msg_enable = value;
}

static int wemac_get_settings(struct net_device *dev, struct ethtool_cmd *cmd)
{
	wemac_board_info_t *dm = to_wemac_board(dev);

	mii_ethtool_gset(&dm->mii, cmd);
	return 0;
}

static int wemac_set_settings(struct net_device *dev, struct ethtool_cmd *cmd)
{
	wemac_board_info_t *dm = to_wemac_board(dev);
	return mii_ethtool_sset(&dm->mii, cmd);
}

static int wemac_nway_reset(struct net_device *dev)
{
	wemac_board_info_t *dm = to_wemac_board(dev);
	return mii_nway_restart(&dm->mii);
}

static u32 wemac_get_link(struct net_device *dev)
{
	wemac_board_info_t *dm = to_wemac_board(dev);
	u32 ret;

		ret = mii_link_ok(&dm->mii);
		//ret = wemac_phy_read(dev, 0, 1)  & 0x04 ? 1 : 0;

	return ret;
}

static int wemac_get_eeprom_len(struct net_device *dev)
{
	return 128;
}

static int wemac_get_eeprom(struct net_device *dev,
		struct ethtool_eeprom *ee, u8 *data)
{

	return 0;
}

static int wemac_set_eeprom(struct net_device *dev,
		struct ethtool_eeprom *ee, u8 *data)
{

	return 0;
}

static const struct ethtool_ops wemac_ethtool_ops = {
	.get_drvinfo		= wemac_get_drvinfo,
	.get_settings		= wemac_get_settings,
	.set_settings		= wemac_set_settings,
	.get_msglevel		= wemac_get_msglevel,
	.set_msglevel		= wemac_set_msglevel,
	.nway_reset			= wemac_nway_reset,
	.get_link			= wemac_get_link,
	.get_eeprom_len		= wemac_get_eeprom_len,
	.get_eeprom			= wemac_get_eeprom,
	.set_eeprom			= wemac_set_eeprom,
};

void emac_sys_setup(wemac_board_info_t * db)
{
	unsigned int reg_val;

	/*  map SRAM to EMAC  */
	reg_val = readl(db->sram_vbase + SRAMC_CFG_REG);
	reg_val |= 0x5<<2;
	writel(reg_val, db->sram_vbase + SRAMC_CFG_REG);

#ifndef SYSCONFIG_GPIO
	/*  PA0~PA7  */
	reg_val = readl(db->gpio_vbase + PA_CFG0_REG);
	reg_val &= 0xAAAAAAAA;
	reg_val |= 0x22222222;
	writel(reg_val, db->gpio_vbase + PA_CFG0_REG);

	/*  PA8~PA15  */
	reg_val = readl(db->gpio_vbase + PA_CFG1_REG);
	reg_val &= 0xAAAAAAAA;
	reg_val |= 0x22222222;
	writel(reg_val, db->gpio_vbase + PA_CFG1_REG);

	/*  PA16~PA17  */
	reg_val = readl(db->gpio_vbase + PA_CFG2_REG);
	reg_val &= 0xffffffAA;
	reg_val |= 0x00000022;
	writel(reg_val, db->gpio_vbase + PA_CFG2_REG);
#else
	db->gpio_hd = gpio_request_ex("emac_para", NULL);
	if(!db->gpio_hd)
		printk(KERN_ERR "ERROR: Request gpio resources is failed!!!\n");
#endif

#ifdef SYSCONFIG_CCMU
	/*  set up clock gating  */
	db->emac_clk = clk_get(db->dev, "ahb_emac");
	if(db->emac_clk != NULL)
		clk_enable(db->emac_clk);
#else
	reg_val = readl(db->ccmu_vbase + CCM_AHB_GATING_REG0);
	reg_val |= 0x1<<17;			//EMAC
	writel(reg_val, db->ccmu_vbase + CCM_AHB_GATING_REG0);
#endif

}

unsigned int emac_setup(struct net_device *ndev )
{
	unsigned int reg_val;
	wemac_board_info_t * db = netdev_priv(ndev);

	wemac_dbg(db, 3, "EMAC seting ==>\n"
			"PHY_AUTO_NEGOTIOATION  %x  0: Normal        1: Auto                 \n"
			"PHY_SPEED              %x  0: 10M           1: 100M                 \n"
			"EMAC_MAC_FULL          %x  0: Half duplex   1: Full duplex          \n"
			"EMAC_TX_TM             %x  0: CPU           1: DMA                  \n"
			"EMAC_TX_AB_M           %x  0: Disable       1: Aborted frame enable \n"
			"EMAC_RX_TM             %x  0: CPU           1: DMA                  \n"
			"EMAC_RX_DRQ_MODE       %x  0: DRQ asserted  1: DRQ automatically    \n"
			,PHY_AUTO_NEGOTIOATION
			,PHY_SPEED
			,EMAC_MAC_FULL
			,EMAC_TX_TM
			,EMAC_TX_AB_M
			,EMAC_RX_TM
			,EMAC_RX_DRQ_MODE);

	/* set up TX */
	reg_val = readl(db->emac_vbase + EMAC_TX_MODE_REG);
	CONFIG_REG(EMAC_TX_AB_M, reg_val, 0);
	CONFIG_REG(EMAC_TX_TM, reg_val, 1);
	writel(reg_val, db->emac_vbase + EMAC_TX_MODE_REG);

	/* set up RX */
	reg_val = readl(db->emac_vbase + EMAC_RX_CTL_REG);
	CONFIG_REG(EMAC_RX_DRQ_MODE, reg_val, 1);
	CONFIG_REG(EMAC_RX_TM, reg_val, 2);
	CONFIG_REG(EMAC_RX_PCF, reg_val, 5);
	CONFIG_REG(EMAC_RX_PCRCE, reg_val, 6);
	CONFIG_REG(EMAC_RX_PLE, reg_val, 7);
	CONFIG_REG(EMAC_RX_POR, reg_val, 8);

	CONFIG_REG(EMAC_RX_UCAD, reg_val, 16);
	CONFIG_REG(EMAC_RX_DAF, reg_val, 17);
	CONFIG_REG(EMAC_RX_MCO, reg_val, 20);
	CONFIG_REG(EMAC_RX_MHF, reg_val, 21);
	CONFIG_REG(EMAC_RX_BCO, reg_val, 22);

	CONFIG_REG(EMAC_RX_SAF, reg_val, 24);
	CONFIG_REG(EMAC_RX_SAIF, reg_val, 25);

	writel(reg_val, db->emac_vbase + EMAC_RX_CTL_REG);

	/* set MAC CTL0 */
	reg_val = readl(db->emac_vbase + EMAC_MAC_CTL0_REG);
	CONFIG_REG(EMAC_MAC_TFC, reg_val, 3);
	CONFIG_REG(EMAC_MAC_RFC, reg_val, 2);
	writel(reg_val, db->emac_vbase + EMAC_MAC_CTL0_REG);

	/* set MAC CTL1 */
	reg_val = readl(db->emac_vbase + EMAC_MAC_CTL1_REG);

	CONFIG_REG(EMAC_MAC_FLC, reg_val, 1);
	CONFIG_REG(EMAC_MAC_HF, reg_val, 2);
	CONFIG_REG(EMAC_MAC_DCRC, reg_val, 3);
	CONFIG_REG(EMAC_MAC_CRC, reg_val, 4);
	CONFIG_REG(EMAC_MAC_PC, reg_val, 5);
	CONFIG_REG(EMAC_MAC_VC, reg_val, 6);
	CONFIG_REG(EMAC_MAC_ADP, reg_val, 7);
	CONFIG_REG(EMAC_MAC_PRE, reg_val, 8);
	CONFIG_REG(EMAC_MAC_LPE, reg_val, 9);
	CONFIG_REG(EMAC_MAC_NB, reg_val, 12);
	CONFIG_REG(EMAC_MAC_BNB, reg_val, 13);
	CONFIG_REG(EMAC_MAC_ED, reg_val, 14);
	writel(reg_val, db->emac_vbase + EMAC_MAC_CTL1_REG);

	/* set up IPGT */
	reg_val = EMAC_MAC_IPGT;
	writel(reg_val, db->emac_vbase + EMAC_MAC_IPGT_REG);

	/* set up IPGR */
	reg_val = EMAC_MAC_NBTB_IPG2;
	reg_val |= (EMAC_MAC_NBTB_IPG1<<8);
	writel(reg_val, db->emac_vbase + EMAC_MAC_IPGR_REG);

	/* set up Collison window */
	reg_val = EMAC_MAC_RM;
	reg_val |= (EMAC_MAC_CW<<8);
	writel(reg_val, db->emac_vbase + EMAC_MAC_CLRT_REG);

	/* set up Max Frame Length */
	reg_val = EMAC_MAC_MFL;
	writel(reg_val, db->emac_vbase + EMAC_MAC_MAXF_REG);

	return (1);
}

unsigned int wemac_powerup(struct net_device *ndev )
{
	wemac_board_info_t * db = netdev_priv(ndev);
	unsigned int reg_val;

	/* initial EMAC and Enable emac control */
	writel(readl(db->emac_vbase + EMAC_CTL_REG) & 0x1
			,db->emac_vbase + EMAC_CTL_REG);

	/* flush  RX FIFO */
	reg_val = readl(db->emac_vbase + EMAC_RX_CTL_REG);
	reg_val |= 0x8;
	writel(reg_val, db->emac_vbase + EMAC_RX_CTL_REG);

	/* Soft reset MAC */
	reg_val = readl(db->emac_vbase + EMAC_MAC_CTL0_REG);
	reg_val &= ((0x1<<15) | (0x1<<14) | (0x1<<11) | (0x1<<9));
	writel(reg_val, db->emac_vbase + EMAC_MAC_CTL0_REG);

	udelay(20);

	/* set MII clock */
	reg_val = readl(db->emac_vbase + EMAC_MAC_MCFG_REG);
	reg_val &= (~(0xf<<2));
	reg_val |= (0xD<<2);
	writel(reg_val, db->emac_vbase + EMAC_MAC_MCFG_REG);

	/* disable all interrupt and clear interrupt status */
	writel(0, db->emac_vbase + EMAC_INT_CTL_REG);
	reg_val = readl(db->emac_vbase + EMAC_INT_STA_REG);
	writel(reg_val, db->emac_vbase + EMAC_INT_STA_REG);

	/* set up EMAC */
	emac_setup(ndev);

	writel(ndev->dev_addr[0]<<16 | ndev->dev_addr[1]<<8 | ndev->dev_addr[2],
			db->emac_vbase + EMAC_MAC_A1_REG);
	writel(ndev->dev_addr[3]<<16 | ndev->dev_addr[4]<<8 | ndev->dev_addr[5],
			db->emac_vbase + EMAC_MAC_A0_REG);

	return (1);
}

static void wemac_phy_check(wemac_board_info_t *db)
{
	struct net_device *ndev = db->ndev;
	struct mii_if_info *mii_if = &db->mii;
	u32 duplex = 0;
	u32 ctl1 = 0;
	u32 speed = 0;

	duplex = mii_check_media(mii_if, netif_msg_link(db), mii_if->advertising ? 0 : 1);

	if(netif_carrier_ok(ndev)){

		if(duplex || db->link_up == 0){

			unsigned long flags;

			spin_lock_irqsave(&db->lock, flags);
			ctl1 = readl(db->emac_vbase + EMAC_MAC_CTL1_REG);
			speed = readl(db->emac_vbase + EMAC_MAC_SUPP_REG);

			if(mii_if->full_duplex){
				writel(0x15, db->emac_vbase + EMAC_MAC_IPGT_REG);
				ctl1 |= 0x00000001;
			} else {
				writel(0x12, db->emac_vbase + EMAC_MAC_IPGT_REG);
				ctl1 &= 0xfffffffe;
			}

			if(mii_if->advertising & (ADVERTISED_100baseT_Half |
						ADVERTISED_100baseT_Full))
				speed |= 0x00000100;
			else
				speed &= 0xfffffeff;

			writel(ctl1, db->emac_vbase + EMAC_MAC_CTL1_REG);
			writel(speed, db->emac_vbase + EMAC_MAC_SUPP_REG);
			spin_unlock_irqrestore(&db->lock, flags);

			emac_reg_dump(db);
		}

		if(db->link_up == 0){
			netif_wake_queue(ndev);
			db->link_up = 1;
		}
	} else if(db->link_up == 1){
		netif_stop_queue(ndev);
		db->link_up = 0;
		mii_if->advertising = 0;
	}
}

static void wemac_poll_work(struct work_struct *w)
{
	struct delayed_work *dw = container_of(w, struct delayed_work, work);
	wemac_board_info_t *db = container_of(dw, wemac_board_info_t, phy_poll);
	struct net_device *ndev = db->ndev;

	wemac_phy_check(db);
	if (netif_running(ndev))
		wemac_schedule_poll(db);
}

/* wemac_release_board
 *
 * release a board, and any mapped resources
 */
static void wemac_release_board(struct platform_device *pdev,
									struct wemac_board_info *db)
{
	/* unmap our resources */
	struct resource *iomem;

	/* release the resources */
	if(db->emac_vbase){
		iounmap(db->emac_vbase);
		iomem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
		release_mem_region(iomem->start, resource_size(iomem));
	}

	if(db->sram_vbase){
		iounmap(db->sram_vbase);
		iomem = platform_get_resource(pdev, IORESOURCE_MEM, 1);
		release_mem_region(iomem->start, resource_size(iomem));
	}

#ifndef SYSCONFIG_CCMU
	if(db->ccmu_vbase){
		iounmap(db->ccmu_vbase);
		iomem = platform_get_resource(pdev, IORESOURCE_MEM, 3);
		release_mem_region(iomem->start, resource_size(iomem));
	}
#else
	if (db->emac_clk){
		clk_disable(db->emac_clk);
		clk_put(db->emac_clk);
	}
#endif

#ifndef SYSCONFIG_GPIO
	if(db->gpio_vbase){
		iounmap(db->gpio_vbase);
		iomem = platform_get_resource(pdev, IORESOURCE_MEM, 2);
		release_mem_region(iomem->start, resource_size(iomem));
	}
#else
	if(db->gpio_hd)
		gpio_release(db->gpio_hd, 0);
#endif

#ifdef PHY_POWER
	if(db->mos_pin_handler)
		gpio_release(db->mos_pin_handler, 0);
	if(db->mos_gpio)
		kfree(db->mos_gpio);
#endif
}

/*
 *  Set WEMAC multicast address
 */
	static void
wemac_set_rx_mode(struct net_device *dev)
{
	wemac_board_info_t *db = netdev_priv(dev);
	struct netdev_hw_addr *ha;
	unsigned long flags;
	int i = 0;
	u32 multi_filter[2];
	u32 bit_nr = 0;
	u32 rcr = 0;

	spin_lock_irqsave(&db->lock, flags);

	rcr = readl(db->emac_vbase + EMAC_RX_CTL_REG);

	/* uicast packet and DA filtering */
	rcr |= 1 << 16;
	rcr |= 1 << 17;

	/* EMAC_RX_PA must set */
	if (dev->flags & IFF_PROMISC) {
		rcr |= 1 << 4;
	} else if (dev->flags & IFF_ALLMULTI
			|| netdev_mc_count(dev) > EMAC_MAX_MCAST) {
		printk("The flaga is IFF_ALLMULTI\n");
		rcr |= 1 << 20;
		rcr |= 1 << 21;
		writel(0xffffffff, db->emac_vbase + EMAC_RX_HASH0_REG);
		writel(0xffffffff, db->emac_vbase + EMAC_RX_HASH1_REG);
	} else if (!netdev_mc_empty(dev)) {
		rcr |= 1 << 21;
		rcr |= 1 << 20;
		memset(multi_filter, 0, sizeof(multi_filter));

		netdev_for_each_mc_addr(ha, dev) {
			bit_nr = ether_crc(ETH_ALEN, ha->addr) >> 26;
			multi_filter[bit_nr >> 5] |= 1 << (bit_nr & 0x1f);
		}

		writel(multi_filter[0], db->emac_vbase + EMAC_RX_HASH0_REG);
		writel(multi_filter[1], db->emac_vbase + EMAC_RX_HASH1_REG);
	}

	if (dev->flags & IFF_BROADCAST) {
		/* Rx Brocast Packet Accept */
		rcr |= 1 << 22;
		rcr |= 1 << 20;
	}


	if (netdev_uc_count(dev) > 4) {
		rcr |= 1 << 4;
		rcr &= ~(1 << 24);
	} else if (!(dev->flags & IFF_PROMISC)) {
		rcr |= 1 << 16;
		netdev_for_each_uc_addr(ha, dev) {
			writel(ha->addr[0] << 16 | ha->addr[1] << 8 |
					ha->addr[2], db->emac_vbase + EMAC_SAFX_H_REG0 + i);
			writel(ha->addr[3] << 16 | ha->addr[4] << 8 |
					ha->addr[5], db->emac_vbase + EMAC_SAFX_L_REG0 + i);
			i += 8;
		}
	}

	writel(rcr, db->emac_vbase + EMAC_RX_CTL_REG);
	spin_unlock_irqrestore(&db->lock,flags);
}

static int wemac_set_mac_address(struct net_device *dev, void *p)
{
	struct sockaddr *addr = p;
	wemac_board_info_t *db = netdev_priv(dev);

	if (netif_running(dev))
		return -EBUSY;

	if (!is_valid_ether_addr(addr->sa_data)) {
		dev_err(&dev->dev, "not setting invalid mac address %pM\n", addr->sa_data);
		return -EADDRNOTAVAIL;
	}

	memcpy(dev->dev_addr, addr->sa_data, ETH_ALEN);

	writel(dev->dev_addr[0]<<16 | dev->dev_addr[1]<<8 | dev->dev_addr[2],
			db->emac_vbase + EMAC_MAC_A1_REG);
	writel(dev->dev_addr[3]<<16 | dev->dev_addr[4]<<8 | dev->dev_addr[5],
			db->emac_vbase + EMAC_MAC_A0_REG);

	return 0;
}

static int wemac_phy_init(wemac_board_info_t *db)
{
	struct mii_if_info *mii_if = &db->mii;
	int i;
	u16 phy_status;
	u16 phy_reg;
	u32 reg_val;

#ifdef PHY_POWER
	if(db->mos_pin_handler){
		db->mos_gpio->data = 1;
		gpio_set_one_pin_status(db->mos_pin_handler, db->mos_gpio, "emac_power", 1);
	}
#endif

	reg_val = readl(db->emac_vbase + EMAC_MAC_SUPP_REG);
	reg_val |= 0x1<<15;
	writel(reg_val, db->emac_vbase + EMAC_MAC_SUPP_REG);

	if(phy_addr == -1){
		/* scan phy and find the address */
		for (i = 0; i < PHY_MAX_ADDR; i++) {
			phy_status = (*mii_if->mdio_read)(mii_if->dev, i, MII_BMSR);
			wemac_dbg(db, 6, "[emac]: Read the phy_status: (0x%04x), "
									"addr: (0x%04x)\n", phy_status, i);

			if(phy_status == 0xffff || phy_status == 0x0000)
				continue;

			mii_if->phy_id = i;
			wemac_dbg(db, 6, "[emac]: Scan the phy adrr: (0x%04x)\n", mii_if->phy_id);
			break;
		}
	} else {
		mii_if->phy_id = phy_addr;
	}

	/* soft reset phy */
	phy_reg = (*mii_if->mdio_read)(mii_if->dev, mii_if->phy_id, MII_BMCR);
	(*mii_if->mdio_write)(mii_if->dev, mii_if->phy_id, MII_BMCR, (~BMCR_PDOWN) & phy_reg);

	udelay(500);

	phy_reg = (*mii_if->mdio_read)(mii_if->dev, mii_if->phy_id, MII_BMCR);
	(*mii_if->mdio_write)(mii_if->dev, mii_if->phy_id, MII_BMCR, BMCR_RESET | phy_reg);
	while((*mii_if->mdio_read)(mii_if->dev, mii_if->phy_id, MII_BMCR) & BMCR_RESET);

#ifdef EMAC_ON_FPGA
	/* set In 10M/full-duplex */
	phy_reg = (*mii_if->mdio_read)(mii_if->dev, mii_if->phy_id, MII_BMCR);
	phy_reg &= ~BMCR_ANENABLE;
	(*mii_if->mdio_write)(mii_if->dev, mii_if->phy_id, MII_BMCR, phy_reg);

	phy_reg = (*mii_if->mdio_read)(mii_if->dev, mii_if->phy_id, MII_BMCR);
	phy_reg &= ~BMCR_SPEED100;
	(*mii_if->mdio_write)(mii_if->dev, mii_if->phy_id, MII_BMCR, phy_reg);

	phy_reg = (*mii_if->mdio_read)(mii_if->dev, mii_if->phy_id, MII_BMCR);
	phy_reg |= BMCR_FULLDPLX;
	(*mii_if->mdio_write)(mii_if->dev, mii_if->phy_id, MII_BMCR, phy_reg);
#endif

	return 0;
}

/*
 * Initilize wemac board
 */
static void wemac_init_wemac(struct net_device *dev)
{
	wemac_board_info_t *db = netdev_priv(dev);
	unsigned int reg_val;

	/* Init Driver variable */
	db->tx_fifo_stat = 0;
	dev->trans_start = 0;

	/* enable RX/TX0/RX Hlevel interrup */
	reg_val = readl(db->emac_vbase + EMAC_INT_CTL_REG);
	reg_val |= (0xf<<0) | (0x01<<8);
	writel(reg_val, db->emac_vbase + EMAC_INT_CTL_REG);

	/* enable RX/TX */
	reg_val = readl(db->emac_vbase + EMAC_CTL_REG);
	reg_val |= 0x6;
	writel(reg_val, db->emac_vbase + EMAC_CTL_REG);
}

/* Our watchdog timed out. Called by the networking layer */
static void wemac_timeout(struct net_device *dev)
{
	wemac_board_info_t *db = netdev_priv(dev);

	if(netif_msg_timer(db))
		dev_err(db->dev, "tx time out.\n");

	emac_reg_dump(db);

	netif_stop_queue(dev);
	wemac_reset(db);
	wemac_init_wemac(dev);

	/* We can accept TX packets again */
	dev->trans_start = jiffies;
	netif_wake_queue(dev);
}

#define PINGPANG_BUF 1
/*
 *  Hardware start transmission.
 *  Send a packet to media from the upper layer.
 */
static int wemac_start_xmit(struct sk_buff *skb, struct net_device *dev)
{
	unsigned long channal;
	unsigned long flags;
	wemac_board_info_t *db = netdev_priv(dev);

#if PINGPANG_BUF
	if ((channal = (db->tx_fifo_stat & 3)) == 3){
		return 1;
	}

	channal = (channal == 1 ? 1 : 0);
#else
	if (db->tx_fifo_stat >0){
		return 1;
	}
	channal = 0;
#endif

	spin_lock_irqsave(&db->lock, flags);

	writel(channal, db->emac_vbase + EMAC_TX_INS_REG);

	(db->outblk)(db->emac_vbase + EMAC_TX_IO_DATA_REG,skb->data, skb->len);
	dev->stats.tx_bytes += skb->len;

	db->tx_fifo_stat |= 1 << channal;
	/* TX control: First packet immediately send, second packet queue */
	if (channal == 0) {
		/* set TX len */
		writel(skb->len, db->emac_vbase + EMAC_TX_PL0_REG);
		/* start translate from fifo to phy */
		writel(readl(db->emac_vbase + EMAC_TX_CTL0_REG)
				| 1, db->emac_vbase + EMAC_TX_CTL0_REG);

		dev->trans_start = jiffies;	/* save the time stamp */
	} else if (channal == 1) {
		/* set TX len */
		writel(skb->len, db->emac_vbase + EMAC_TX_PL1_REG);
		/* start translate from fifo to phy */
		writel(readl(db->emac_vbase + EMAC_TX_CTL1_REG)
				| 1, db->emac_vbase + EMAC_TX_CTL1_REG);
		dev->trans_start = jiffies;	/* save the time stamp */
	}

	if ((db->tx_fifo_stat & 3) == 3) {
		/* Second packet */
		netif_stop_queue(dev);
	}

	spin_unlock_irqrestore(&db->lock, flags);

	/* free this SKB */
	dev_kfree_skb(skb);

	return 0;
}

/*
 * WEMAC interrupt handler
 * receive the packet to upper layer, free the transmitted packet
 */
static void wemac_tx_done(struct net_device *dev,
							wemac_board_info_t *db, unsigned int tx_status)
{
	/* One packet sent complete */
	db->tx_fifo_stat &= ~(tx_status & 3);
	if(3==(tx_status&3))
		dev->stats.tx_packets+=2;
	else
		dev->stats.tx_packets++;

	if (netif_msg_tx_done(db))
		dev_dbg(db->dev, "Tx done, NSR %02x\n", tx_status);

	//		/* Queue packet check & send */
	//		if (db->tx_fifo_stat > 0) {
	//			/* set TX len */
	//			writel(db->queue_pkt_len, db->emac_vbase + EMAC_TX_PL0_REG);
	//			/* start translate from fifo to mac */
	//			writel(readl(db->emac_vbase + EMAC_TX_CTL0_REG) | 1, db->emac_vbase + EMAC_TX_CTL0_REG);
	//
	//			dev->trans_start = jiffies;
	//		}
	netif_wake_queue(dev);
}

struct wemac_rxhdr {
	__s16	RxLen;
	__u16	RxStatus;
} __attribute__((__packed__));

/*
 *  Received a packet and pass to upper layer
 */
static void wemac_rx(struct net_device *dev)
{
	wemac_board_info_t *db = netdev_priv(dev);
	struct wemac_rxhdr rxhdr;
	struct sk_buff *skb;
	static struct sk_buff *skb_last=NULL;	//todo: change static variate to member of wemac_board_info_t. bingge
	u8 *rdptr;
	bool GoodPacket;
	int RxLen;
	static int RxLen_last=0;
	unsigned int RxStatus;
	unsigned int reg_val, Rxcount, ret;

	/* Check packet ready or not */
	do {
		Rxcount = readl(db->emac_vbase + EMAC_RX_FBC_REG);

		//add by penggang 20110621
		if(!Rxcount) {
			udelay(5);
			Rxcount = readl(db->emac_vbase + EMAC_RX_FBC_REG);
		}

		if (netif_msg_rx_status(db))
			dev_dbg(db->dev, "RXCount: %x\n", Rxcount);

		if((skb_last!=NULL)&&(RxLen_last>0)){
			dev->stats.rx_bytes += RxLen_last;

			/* Pass to upper layer */
			skb_last->protocol = eth_type_trans(skb_last, dev);
			netif_rx(skb_last);
			dev->stats.rx_packets++;
			skb_last=NULL;
			RxLen_last=0;

			reg_val = readl(db->emac_vbase + EMAC_RX_CTL_REG);
			reg_val &= (~(0x1<<2));
			writel(reg_val, db->emac_vbase + EMAC_RX_CTL_REG);
		}
		if(!Rxcount) {
			emacrx_completed_flag = 1;
			reg_val = readl(db->emac_vbase + EMAC_INT_CTL_REG);
			reg_val |= (0xf<<0) | (0x01<<8);
			writel(reg_val, db->emac_vbase + EMAC_INT_CTL_REG);
			return;
		}

		reg_val = readl(db->emac_vbase + EMAC_RX_IO_DATA_REG);
		if(netif_msg_rx_status(db))
			dev_dbg(db->dev, "receive header: %x\n", reg_val);
		if(reg_val!=0x0143414d) {
			//disable RX
			reg_val = readl(db->emac_vbase + EMAC_CTL_REG);
			writel(reg_val & (~(1<<2)), db->emac_vbase + EMAC_CTL_REG);

			//Flush RX FIFO
			reg_val = readl(db->emac_vbase + EMAC_RX_CTL_REG);
			writel(reg_val | (1<<3), db->emac_vbase + EMAC_RX_CTL_REG);

			while(readl(db->emac_vbase + EMAC_RX_CTL_REG)&(0x1<<3));

			//enable RX
			reg_val = readl(db->emac_vbase + EMAC_CTL_REG);
			writel(reg_val |(1<<2), db->emac_vbase + EMAC_CTL_REG);
			reg_val = readl(db->emac_vbase + EMAC_INT_CTL_REG);
			reg_val |= (0xf<<0) | (0x01<<8);
			writel(reg_val, db->emac_vbase + EMAC_INT_CTL_REG);

			emacrx_completed_flag = 1;

			return;
		}

		/* A packet ready now  & Get status/length */
		GoodPacket = true;

		(db->inblk)(db->emac_vbase + EMAC_RX_IO_DATA_REG, &rxhdr, sizeof(rxhdr));

		if (netif_msg_rx_status(db))
			dev_dbg(db->dev, "rxhdr: %x\n", *((int*)(&rxhdr)));

		RxLen = rxhdr.RxLen;
		RxStatus = rxhdr.RxStatus;

		if (netif_msg_rx_status(db))
			dev_dbg(db->dev, "RX: status %02x, length %04x\n",
					RxStatus, RxLen);

		/* Packet Status check */
		if (RxLen < 0x40) {
			GoodPacket = false;
			if (netif_msg_rx_err(db))
				dev_dbg(db->dev, "RX: Bad Packet (runt)\n");
		}

		/* RxStatus is identical to RSR register. */
		if (0 & RxStatus & (EMAC_CRCERR | EMAC_LENERR)) {
			GoodPacket = false;
			if (RxStatus & EMAC_CRCERR) {
				if (netif_msg_rx_err(db))
					dev_dbg(db->dev, "crc error\n");
				dev->stats.rx_crc_errors++;
			}
			if (RxStatus & EMAC_LENERR) {
				if (netif_msg_rx_err(db))
					dev_dbg(db->dev, "length error\n");
				dev->stats.rx_length_errors++;
			}
		}

		/* Move data from WEMAC */
		if (GoodPacket
				&& ((skb = dev_alloc_skb(RxLen + 4)) != NULL)) {
			skb_reserve(skb, 2);
			rdptr = (u8 *) skb_put(skb, RxLen - 4);

			/* Read received packet from RX SRAM */
			if (netif_msg_rx_status(db))
				dev_dbg(db->dev, "RxLen %x\n", RxLen);

			if (RxLen > DMA_CPU_TRRESHOLD) {
				reg_val = readl(db->emac_vbase + EMAC_RX_CTL_REG);
				reg_val |= (0x1<<2);
				writel(reg_val, db->emac_vbase + EMAC_RX_CTL_REG);
				ret = wemac_inblk_dma(db->emac_vbase + EMAC_RX_IO_DATA_REG, rdptr, RxLen);
				if (ret !=0) {
					printk("[emac]wemac_inblk_dma failed,ret=%d, using cpu to read fifo!\n", ret);
					reg_val = readl(db->emac_vbase + EMAC_RX_CTL_REG);
					reg_val &= (~(0x1<<2));
					writel(reg_val, db->emac_vbase + EMAC_RX_CTL_REG);

					(db->inblk)(db->emac_vbase + EMAC_RX_IO_DATA_REG, rdptr, RxLen);

					dev->stats.rx_bytes += RxLen;

					/* Pass to upper layer */
					skb->protocol = eth_type_trans(skb, dev);
					netif_rx(skb);
					dev->stats.rx_packets++;
				} else {
					RxLen_last = RxLen;
					skb_last = skb;
					break;
				}
			}else{
				(db->inblk)(db->emac_vbase + EMAC_RX_IO_DATA_REG, rdptr, RxLen);

				dev->stats.rx_bytes += RxLen;

				/* Pass to upper layer */
				skb->protocol = eth_type_trans(skb, dev);
				netif_rx(skb);
				dev->stats.rx_packets++;
			}
		} else {
			/* need to dump the packet's data */
			(db->dumpblk)(db->emac_vbase + EMAC_RX_IO_DATA_REG, RxLen);
		}
	} while (1);
}

static irqreturn_t wemac_interrupt(int irq, void *dev_id)
{
	struct net_device *dev = dev_id;
	wemac_board_info_t *db = netdev_priv(dev);
	int int_status;
	unsigned long flags;
	unsigned int reg_val;

	/* A real interrupt coming */
	/* holders of db->lock must always block IRQs */
	spin_lock_irqsave(&db->lock, flags);

	writel(0, db->emac_vbase + EMAC_INT_CTL_REG);	/* Disable all interrupt */

	/* Got WEMAC interrupt status */
	int_status = readl(db->emac_vbase + EMAC_INT_STA_REG);	/* Got ISR */
	writel(int_status, db->emac_vbase + EMAC_INT_STA_REG);	/* Clear ISR status */

	if (netif_msg_intr(db))
		dev_dbg(db->dev, "emac interrupt %02x\n", int_status);

	/* RX Flow Control High Level */
	//	if (int_status & 0x20000)
	//		printk("f\n");

	/* Received the coming packet */
	if ((int_status & 0x100) && (emacrx_completed_flag == 1)) {	 // carrier lost
		emacrx_completed_flag = 0;
		wemac_rx(dev);
	}
	/* Trnasmit Interrupt check */
	if (int_status & (0x01 | 0x02)){
		wemac_tx_done(dev, db, int_status);
	}

	if (int_status & (0x04 | 0x08)){
		printk(" ab : %x \n", int_status);
	}

	/* Re-enable interrupt mask */
	if (emacrx_completed_flag == 1) {
		reg_val = readl(db->emac_vbase + EMAC_INT_CTL_REG);
		reg_val |= (0xf<<0) | (0x01<<8);
		writel(reg_val, db->emac_vbase + EMAC_INT_CTL_REG);
	}

	spin_unlock_irqrestore(&db->lock, flags);

	return IRQ_HANDLED;
}

#ifdef CONFIG_NET_POLL_CONTROLLER
/*
 *Used by netconsole
 */
static void wemac_poll_controller(struct net_device *dev)
{
	disable_irq(dev->irq);
	wemac_interrupt(dev->irq, dev);
	enable_irq(dev->irq);
}
#endif

static void phy_reg_dump(wemac_board_info_t *db)
{
}

static void emac_reg_dump(wemac_board_info_t *db)
{
	int i=0;
	void __iomem *vbase = db->emac_vbase;

	wemac_dbg(db, 6, "\n============[Regsiter dump]============\n");
	for(i=0; i<=0xE0; i+=4){
		wemac_dbg(db, 6, "(OFFSET: 0x%02x) -- 0x%08x\n", i, readl(vbase + i));
	}
	wemac_dbg(db, 6, "\n============[Regsiter dump]============\n");
}

/*
 *  Open the interface.
 *  The interface is opened whenever "ifconfig" actives it.
 */
static int wemac_open(struct net_device *dev)
{
	wemac_board_info_t *db = netdev_priv(dev);

	if (netif_msg_ifup(db))
		dev_dbg(db->dev, "enabling %s\n", dev->name);

	/* Initialize WEMAC board */
	emac_sys_setup(db);
	wemac_powerup(dev);

	wemac_phy_init(db);
	wemac_phy_check(db);

	//wemac_reset(db);
	wemac_init_wemac(dev);

	/* If there is no IRQ type specified, default to something that
	 * may work, and tell the user that this is a problem */

	if (request_irq(dev->irq, &wemac_interrupt, IRQF_SHARED, dev->name, dev)){
		printk(KERN_ERR "Request irq(%u) is failed\n", dev->irq);
		return -EAGAIN;
	}

	wemac_schedule_poll(db);

	netif_start_queue(dev);

	return 0;
}

/*
 *   Read a word from phyxcer
 */
static int wemac_phy_read(struct net_device *dev, int phyaddr, int reg)
{
	wemac_board_info_t *db = netdev_priv(dev);
	unsigned long flags;
	int ret = 0;

	mutex_lock(&db->addr_lock);

	spin_lock_irqsave(&db->lock,flags);
	/* issue the phy address and reg */
	writel((phyaddr << 8) | reg, db->emac_vbase + EMAC_MAC_MADR_REG);
	/* pull up the phy io line */
	writel(0x1, db->emac_vbase + EMAC_MAC_MCMD_REG);
	spin_unlock_irqrestore(&db->lock,flags);

	while(readl(db->emac_vbase + EMAC_MAC_MIND_REG) & 0x01);

	/* push down the phy io line and read data */
	spin_lock_irqsave(&db->lock,flags);
	/* push down the phy io line */
	writel(0x0, db->emac_vbase + EMAC_MAC_MCMD_REG);
	/* and write data */
	ret = readl(db->emac_vbase + EMAC_MAC_MRDD_REG);
	spin_unlock_irqrestore(&db->lock,flags);

	mutex_unlock(&db->addr_lock);

	return ret;
}

/*
 *   Write a word to phyxcer
 */
static void wemac_phy_write(struct net_device *dev,
							int phyaddr, int reg, int value)
{
	wemac_board_info_t *db = netdev_priv(dev);
	unsigned long flags;

	mutex_lock(&db->addr_lock);

	spin_lock_irqsave(&db->lock,flags);
	/* issue the phy address and reg */
	writel((phyaddr << 8) | reg, db->emac_vbase + EMAC_MAC_MADR_REG);
	writel(0x1, db->emac_vbase + EMAC_MAC_MCMD_REG);
	spin_unlock_irqrestore(&db->lock, flags);

	while(readl(db->emac_vbase + EMAC_MAC_MIND_REG) & 0x01);

	spin_lock_irqsave(&db->lock,flags);
	/* push down the phy io line */
	writel(0x0, db->emac_vbase + EMAC_MAC_MCMD_REG);
	/* and write data */
	writel(value, db->emac_vbase + EMAC_MAC_MWTD_REG);
	spin_unlock_irqrestore(&db->lock, flags);

	mutex_unlock(&db->addr_lock);
}

static void wemac_shutdown(struct net_device *dev)
{
	unsigned int reg_val;
	wemac_board_info_t *db = netdev_priv(dev);

#ifdef EMAC_ON_FPGA
	wemac_phy_write(dev, db->mii.phy_id, MII_BMCR, BMCR_RESET);
	wemac_phy_write(dev, db->mii.phy_id, MII_BMCR, BMCR_PDOWN);	/* PHY POWER DOWN */
#endif

#ifdef PHY_POWER
	if(db->mos_pin_handler){
		db->mos_gpio->data = 0;
		gpio_set_one_pin_status(db->mos_pin_handler, db->mos_gpio, "emac_power", 1);
	}
#endif

	db->mii.advertising = 0;
	db->mii.full_duplex = 0;
	db->link_up = 0;

	writel(readl(db->emac_vbase + EMAC_CTL_REG) & (~(0x6))
			,db->emac_vbase + EMAC_CTL_REG);	/* Disable RX TX Ctl */

	writel(0, db->emac_vbase + EMAC_INT_CTL_REG);	/* Disable all interrupt */

	writel(readl(db->emac_vbase + EMAC_INT_STA_REG)
			,db->emac_vbase + EMAC_INT_STA_REG);	/* clear interupt status */

	writel(readl(db->emac_vbase + EMAC_CTL_REG) & (~(0x1))
			,db->emac_vbase + EMAC_CTL_REG);	/* Disable RX TX Ctl */

	reg_val = readl(db->sram_vbase + SRAMC_CFG_REG);
	reg_val &= (~(0x5<<2));
	writel(reg_val, db->sram_vbase + SRAMC_CFG_REG);

	emac_reg_dump(db);

#ifdef SYSCONFIG_CCMU
	/*  set up clock gating  */
	if(db->emac_clk != NULL)
		clk_disable(db->emac_clk);
#else
	reg_val = readl(db->ccmu_vbase + CCM_AHB_GATING_REG0);
	reg_val |= ~(0x1<<17);			//EMAC
	writel(reg_val, db->ccmu_vbase + CCM_AHB_GATING_REG0);
#endif

}

/*
 * Stop the interface.
 * The interface is stopped when it is brought.
 */
static int wemac_stop(struct net_device *ndev)
{
	wemac_board_info_t *db = netdev_priv(ndev);

	if (netif_msg_ifdown(db))
		dev_dbg(db->dev, "shutting down %s\n", ndev->name);

	cancel_delayed_work_sync(&db->phy_poll);
	netif_stop_queue(ndev);
	netif_carrier_off(ndev);

	wemac_shutdown(ndev);

	/* free interrupt */
	free_irq(ndev->irq, ndev);

	return 0;
}

static const struct net_device_ops wemac_netdev_ops = {
	.ndo_open		= wemac_open,
	.ndo_stop		= wemac_stop,
	.ndo_start_xmit		= wemac_start_xmit,
	.ndo_tx_timeout		= wemac_timeout,
	.ndo_set_rx_mode	= wemac_set_rx_mode,
	.ndo_do_ioctl		= wemac_ioctl,
	.ndo_change_mtu		= eth_change_mtu,
	.ndo_validate_addr	= eth_validate_addr,
	.ndo_set_mac_address	= wemac_set_mac_address,
#ifdef CONFIG_NET_POLL_CONTROLLER
	.ndo_poll_controller	= wemac_poll_controller,
#endif
};

/*
 * Search WEMAC board, allocate space and register it
 */
static int __devinit wemac_probe(struct platform_device *pdev)
{
	struct wemac_plat_data *pdata = pdev->dev.platform_data;
	struct wemac_board_info *db;	/* Point a board information structure */
	struct net_device *ndev;
	struct resource *iomem_emac, *iomem_gpio, *iomem_sram, *iomem_ccmu;
	int ret = 0;
	int iosize;

	/* Init network device */
	ndev = alloc_etherdev(sizeof(struct wemac_board_info));
	if (!ndev) {
		dev_err(&pdev->dev, "could not allocate device.\n");
		return -ENOMEM;
	}

	SET_NETDEV_DEV(ndev, &pdev->dev);

	/* setup board info structure */
	db = netdev_priv(ndev);
	memset(db, 0, sizeof(*db));

	ch_rx = sw_dma_request(DMACH_DEMACR, &emacrx_dma_client, ndev);
	if(ch_rx < 0){
		printk("error when request dma for emac rx\n");
		return ch_rx;
	}

	sw_dma_set_opfn(ch_rx, emacrx_dma_opfn);
	sw_dma_set_buffdone_fn(ch_rx, emacrx_dma_buffdone);

	ch_tx = sw_dma_request(DMACH_DEMACT, &emactx_dma_client, ndev);
	if(ch_tx < 0){
		printk("error when request dma for emac tx\n");
		sw_dma_free(DMACH_DEMACR, &emacrx_dma_client);
		return ch_tx;
	}

	sw_dma_set_opfn(ch_tx, emactx_dma_opfn);
	sw_dma_set_buffdone_fn(ch_tx, emactx_dma_buffdone);

	db->dev = &pdev->dev;
	db->ndev = ndev;

	spin_lock_init(&db->lock);
	mutex_init(&db->addr_lock);

	INIT_DELAYED_WORK(&db->phy_poll, wemac_poll_work);

	iomem_emac = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	iomem_sram = platform_get_resource(pdev, IORESOURCE_MEM, 1);

#ifndef SYSCONFIG_CCMU
	iomem_ccmu = platform_get_resource(pdev, IORESOURCE_MEM, 3);
#endif

#ifndef SYSCONFIG_GPIO
	iomem_gpio = platform_get_resource(pdev, IORESOURCE_MEM, 2);
#endif
	ndev->irq  = platform_get_irq(pdev, 0);

	if (iomem_emac == NULL
			|| iomem_sram == NULL
#ifndef SYSCONFIG_GPIO
			|| iomem_gpio == NULL
#endif
#ifndef SYSCONFIG_CCMU
			|| iomem_ccmu == NULL
#endif
			|| ndev->irq < 0 ) {
		dev_err(db->dev, "insufficient resources\n");
		ret = -ENOENT;
		goto out;
	}

	/* emac address remap */
	iosize = resource_size(iomem_emac);
	if(!request_mem_region(iomem_emac->start, iosize, pdev->name)){
		dev_err(db->dev, "cannot claim emac address reg area\n");
		ret = -EBUSY;
		goto out;
	}
	if(!(db->emac_vbase = ioremap(iomem_emac->start, iosize))){
		dev_err(db->dev, "failed to ioremap emac address reg\n");
		ret = -EINVAL;
		goto err_emac_map;
	}

	/* sram address remap */
	iosize = resource_size(iomem_sram);
	if(!request_mem_region(iomem_sram->start, iosize, pdev->name)){
		dev_err(db->dev, "cannot claim sram address reg area\n");
		ret = -EBUSY;
		goto err_sram_region;
	}
	if(!(db->sram_vbase = ioremap(iomem_sram->start, iosize))){
		dev_err(db->dev, "failed to ioremap sram address reg\n");
		ret = -EINVAL;
		goto err_sram_map;
	}

#ifndef SYSCONFIG_GPIO
	/* gpio address remap */
	iosize = resource_size(iomem_gpio);
	if(!request_mem_region(iomem_gpio->start, iosize, pdev->name)){
		dev_err(db->dev, "cannot claim sram address reg area\n");
		ret = -EBUSY;
		goto err_gpio_region;
	}
	if(!(db->gpio_vbase = ioremap(iomem_gpio->start, iosize))){
		dev_err(db->dev, "failed to ioremap sram address reg\n");
		ret = -EINVAL;
		goto err_gpio_map;
	}
#endif

#ifdef PHY_POWER
	db->mos_gpio = kmalloc(sizeof(user_gpio_set_t), GFP_KERNEL);
	db->mos_pin_handler = 0;
	if(NULL == db->mos_gpio){
		printk(KERN_ERR "can't request memory for mos_gpio\n");
	}else{
		if(SCRIPT_PARSER_OK != script_parser_fetch("emac_para", "emac_power",
					(int *)(db->mos_gpio), sizeof(user_gpio_set_t)/sizeof(int))){
			printk(KERN_ERR "can't get information emac_power gpio\n");
			kfree(db->mos_gpio);
		}else{
			db->mos_pin_handler = gpio_request(db->mos_gpio, 1);
			if(0 == db->mos_pin_handler){
				printk(KERN_ERR "can't request gpio_port %d, port_num %d\n",
						db->mos_gpio->port, db->mos_gpio->port_num);
				kfree(db->mos_gpio);
			}
		}
	}
#endif

#ifndef SYSCONFIG_CCMU
	/* ccmu address remap */
	iosize = resource_size(iomem_ccmu);
	if(!request_mem_region(iomem_ccmu->start, iosize, pdev->name)){
		dev_err(db->dev, "cannot claim sram address reg area\n");
		ret = -EBUSY;
		goto err_ccmu_region;
	}
	if(!(db->ccmu_vbase = ioremap(iomem_ccmu->start, iosize))){
		dev_err(db->dev, "failed to ioremap sram address reg\n");
		ret = -EINVAL;
		goto err_ccmu_map;
	}
#endif

	wemac_dbg(db, 3, "emac_vbase: %p,"
					"sram_vbase: %p,"
#ifndef SYSCONFIG_GPIO
					"gpio_vbase: %p,"
#endif
#ifndef SYSCONFIG_CCMU
					"ccmu_vbase: %p\n"
#endif
					,db->emac_vbase
					,db->sram_vbase
#ifndef SYSCONFIG_GPIO
					,db->gpio_vbase
#endif
#ifndef SYSCONFIG_CCMU
					,db->ccmu_vbase
#endif
			);

	/* fill in parameters for net-dev structure */
	ndev->base_addr = (unsigned long)db->emac_vbase;

	/* ensure at least we have a default set of IO routines */
	db->dumpblk = wemac_dumpblk_32bit;
	db->outblk  = wemac_outblk_32bit;
	db->inblk   = wemac_inblk_32bit;

	/* check to see if anything is being over-ridden */
	if (pdata != NULL) {
		/* check to see if the driver wants to over-ride the
		 * default IO width */
		if (pdata->inblk != NULL)
			db->inblk = pdata->inblk;

		if (pdata->outblk != NULL)
			db->outblk = pdata->outblk;

		if (pdata->dumpblk != NULL)
			db->dumpblk = pdata->dumpblk;
	}

	/* driver system function */
	ether_setup(ndev);

	ndev->netdev_ops	= &wemac_netdev_ops;
	ndev->watchdog_timeo	= msecs_to_jiffies(watchdog);
	ndev->ethtool_ops	= &wemac_ethtool_ops;

#ifdef CONFIG_NET_POLL_CONTROLLER
	ndev->poll_controller	 = &wemac_poll_controller;
#endif

	db->msg_enable       = 0xffffffff & (~NETIF_MSG_TX_DONE)
							& (~NETIF_MSG_INTR) & (~NETIF_MSG_RX_STATUS);
	db->mii.phy_id		 = phy_addr;
	db->mii.phy_id_mask  = 0x1f;
	db->mii.reg_num_mask = 0x1f;
	db->mii.force_media  = 0; // change force_media value to 0 to force check link status
	db->mii.full_duplex  = 0; // change full_duplex value to 0 to set initial duplex as half
	db->mii.dev	     = ndev;
	db->mii.mdio_read    = wemac_phy_read;
	db->mii.mdio_write   = wemac_phy_write;

	wemac_get_macaddr(db);

	platform_set_drvdata(pdev, ndev);
	ret = register_netdev(ndev);

	if (ret == 0)
		wemac_dbg(db, 3, "%s: at %p, IRQ %d MAC: %p\n",
						ndev->name, db->emac_vbase,
						ndev->irq, ndev->dev_addr);

	return 0;

#ifndef SYSCONFIG_CCMU
err_ccmu_map:
	release_mem_region(iomem_ccmu->start, resource_size(iomem_ccmu));
err_ccmu_region:
#endif

#ifndef SYSCONFIG_GPIO
	iounmap(db->gpio_vbase);
err_gpio_map:
	release_mem_region(iomem_gpio->start, resource_size(iomem_gpio));
err_gpio_region:
#endif
	iounmap(db->sram_vbase);
err_sram_map:
	release_mem_region(iomem_sram->start, resource_size(iomem_sram));
err_sram_region:
	iounmap(db->emac_vbase);
err_emac_map:
	release_mem_region(iomem_emac->start, resource_size(iomem_emac));
out:
	dev_err(db->dev, "Not found (%d).\n", ret);
	free_netdev(ndev);
	return ret;
}

static void wemac_get_macaddr(wemac_board_info_t *db)
{
	struct net_device *ndev = db->ndev;
	int i;
	char *p = mac_str;

	for(i=0;i<6;i++,p++)
		ndev->dev_addr[i] = simple_strtoul(p, &p, 16);

#ifdef DYNAMIC_MAC_SYSCONFIG
	char emac_mac[13]={'\0'};
	/* set mac_address to chip */
	if(SCRIPT_PARSER_OK != script_parser_fetch("dynamic", "MAC", (int *)emac_mac, 3)){
		printk(KERN_WARNING "In sysconfig.fex emac MAC_ADDRESS isn't valid!\n");
	}else if(!is_valid_ether_addr(ndev->dev_addr)){
		emac_mac[12]='\0';
		for(i=0; i<6; i++){
			char emac_tmp[3]=":::";
			memcpy(emac_tmp, (char *)(emac_mac+i*2), 2);
			emac_tmp[2]=':';
			ndev->dev_addr[i] = simple_strtoul(emac_tmp, NULL, 16);
		}
	}
#endif

	if (!is_valid_ether_addr(ndev->dev_addr)){
		dev_hw_addr_random(ndev, ndev->dev_addr);
	}

	if (!is_valid_ether_addr(ndev->dev_addr))
		printk(KERN_ERR "Invalid MAC address. Please set it using ifconfig\n");
};

static int wemac_drv_suspend(struct platform_device *dev, pm_message_t state)
{
	struct net_device *ndev = platform_get_drvdata(dev);
	wemac_board_info_t *db;

	if (!ndev)
		return 0;
	db = netdev_priv(ndev);

	if(mii_link_ok(&db->mii))
		netif_carrier_off(ndev);
	netif_device_detach(ndev);

	wemac_stop(ndev);
	return 0;
}

static int wemac_drv_resume(struct platform_device *dev)
{
	struct net_device *ndev = platform_get_drvdata(dev);
	wemac_board_info_t *db = netdev_priv(ndev);

	if (!ndev)
		return 0;

	wemac_open(ndev);

	netif_device_attach(ndev);
	if(mii_link_ok(&db->mii))
		netif_carrier_on(ndev);

	return 0;
}

static int __devexit wemac_drv_remove(struct platform_device *pdev)
{
	struct net_device *ndev = platform_get_drvdata(pdev);

	platform_set_drvdata(pdev, NULL);

	unregister_netdev(ndev);
	if(ch_rx > 0)
		sw_dma_free(ch_rx, &emacrx_dma_client);
	if(ch_tx > 0)
		sw_dma_free(ch_tx, &emactx_dma_client);
	wemac_release_board(pdev, (wemac_board_info_t *) netdev_priv(ndev));
	free_netdev(ndev);		/* free device structure */

	return 0;
}

static void wemac_device_release(struct device *dev)
{
}

static struct resource wemac_resources[] = {
	[0] = {
		.name	= "EMAC IOMEM",
		.start	= EMAC_BASE,
		.end	= EMAC_BASE+0xE0,
		.flags	= IORESOURCE_MEM,
	},
	[1] = {
		.name	= "SRAM CFG",
		.start	= SRAMC_BASE,
		.end	= SRAMC_BASE + SRAMC_CFG_REG,
		.flags	= IORESOURCE_MEM,
	},
	[2] = {
		.name	= "PIOA CFG",
		.start	= PIO_BASE,
		.end	= PIO_BASE + CCM_AHB_GATING_REG0,
		.flags	= IORESOURCE_MEM,
	},
	[3] = {
		.name	= "CCM AHB",
		.start	= CCM_BASE,
		.end	= CCM_BASE+1024,
		.flags	= IORESOURCE_MEM,
	},
	[4] = {
		.name	= "EMAC IRQ",
		.start	= AW_IRQ_EMAC,
		.end	= AW_IRQ_EMAC,
		.flags	= IORESOURCE_IRQ,
	}
};

static struct wemac_plat_data wemac_platdata = {
};

static struct platform_device wemac_device = {
	.name		= "wemac",
	.id		= 0,
	.num_resources	= ARRAY_SIZE(wemac_resources),
	.resource	= wemac_resources,
	.dev		= {
		.release =  wemac_device_release,
		.platform_data = &wemac_platdata,
	}
};

static struct platform_driver wemac_driver = {
	.driver	= {
		.name    = "wemac",
		.owner	 = THIS_MODULE,
	},
	.probe   = wemac_probe,
	.remove  = __devexit_p(wemac_drv_remove),
	.suspend = wemac_drv_suspend,
	.resume  = wemac_drv_resume,
};

static int __init set_mac_addr(char *str)
{
	char* p = str;

	memcpy(mac_str, p, 18);

#if 0
	int i;
	for(i=0;i<6;i++,p++)
		mac_addr[i] = simple_strtoul(p, &p, 16);
#endif

	return 0;
}
__setup("mac_addr=", set_mac_addr);

static int __init wemac_init(void)
{
#if 0
	int emac_used = 0;

	if(SCRIPT_PARSER_OK != script_parser_fetch("emac_para","emac_used", &emac_used, 1))
		printk(KERN_WARNING "emac_init fetch emac using configuration failed\n");

	if(!emac_used){
		printk(KERN_INFO "emac driver is disabled \n");
		return 0;
	}
#endif

	printk(KERN_INFO "[sunxi_emac]: Init %s Ethernet Driver, V%s\n", CARDNAME, DRV_VERSION);

	platform_device_register(&wemac_device);
	return platform_driver_register(&wemac_driver);
}

static void __exit wemac_cleanup(void)
{
#if 0
	int emac_used = 0;

	if(SCRIPT_PARSER_OK != script_parser_fetch("emac_para","emac_used", &emac_used, 1))
		printk(KERN_WARNING "emac_init fetch emac using configuration failed\n");

	if(!emac_used){
		printk(KERN_INFO "emac driver is disabled \n");
		return ;
	}
#endif

	platform_driver_unregister(&wemac_driver);
	platform_device_unregister(&wemac_device);

	printk(KERN_INFO "[sunxi_emac]: Remove %s Ethernet Driver, V%s\n", CARDNAME, DRV_VERSION);
}

module_init(wemac_init);
module_exit(wemac_cleanup);

MODULE_AUTHOR("Chenlm & Shuge");
MODULE_DESCRIPTION("Wemac network driver");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:wemac");
