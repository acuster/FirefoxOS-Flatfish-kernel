/*
 * arch/arm/mach-sun7i/include/mach/dma.h
 * (C) Copyright 2010-2015
 * Reuuimlla Technology Co., Ltd. <www.reuuimllatech.com>
 * liugang <liugang@reuuimllatech.com>
 *
 * sun7i dma driver header file
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 */

#ifndef __SW_DMA_H
#define __SW_DMA_H

#if 0
#include <linux/spinlock.h>

#define USE_SPINLOCK_20120802	/* use spin_lock_irqsave instead of local_irq_save */

#define MAX_DMA_TRANSFER_SIZE   0x100000 /* Data Unit is half word  */

#define DMA_CH_VALID		(1<<31)
#define DMA_CH_NEVER		(1<<30)

/* We use `virtual` dma channels to hide the fact we have only a limited
 * number of DMA channels, and not of all of them (dependant on the device)
 * can be attached to any DMA source. We therefore let the DMA core handle
 * the allocation of hardware channels to clients.
*/
enum sw_dma_ch {
	/*NDMA*/
	DMACH_NSPI0_RX,
	DMACH_NSPI0_TX,
	DMACH_NSPI1_RX,
	DMACH_NSPI1_TX,
	DMACH_NSPI2_RX,
	DMACH_NSPI2_TX,
	DMACH_NSPI3_RX,
	DMACH_NSPI3_TX,
	DMACH_NUART0,
	DMACH_NUART1,
	DMACH_NUART2,
	DMACH_NUART3,
	DMACH_NUART4,
	DMACH_NUART5,
	DMACH_NUART6,
	DMACH_NUART7,
	DMACH_NSRAM,
	DMACH_NSDRAM,
	DMACH_NTPAD,
	DMACH_NADDA_PLAY,//audio play
	DMACH_NADDA_CAPTURE,//audio capture
	DMACH_NIIS_PLAY,
	DMACH_NIIS_CAPTURE,
	DMACH_NIR0,
	DMACH_NIR1,
	DMACH_NSPDIF,
	DMACH_NAC97,
	DMACH_NHDMI,//HDMI
	/*DDMA*/
	DMACH_DSRAM,
	DMACH_DSDRAM,
	DMACH_DPATA,
	DMACH_DNAND,
	DMACH_DUSB0,
	DMACH_DEMACR,
	DMACH_DEMACT,
	DMACH_DSPI1_RX,
	DMACH_DSPI1_TX,
	DMACH_DSSR,
	DMACH_DSST,
	DMACH_TCON0,
	DMACH_TCON1,
	DMACH_HDMIAUDIO,//HDMIAUDIO
	DMACH_DMS,
	DMACH_DSPI0_RX,
	DMACH_DSPI0_TX,
	DMACH_DSPI2_RX,
	DMACH_DSPI2_TX,
	DMACH_DSPI3_RX,
	DMACH_DSPI3_TX,
	DMACH_MAX,/* 8 NDMAs, 8 DDMAs */
};


#define N_DRQSRC_SHIFT		0
#define N_DRQDST_SHIFT		16
#define D_DRQSRC_SHIFT		0
#define D_DRQDST_SHIFT		16
#define DRQ_INVALID			0xff

/*normal DMA Source*/
#define N_DRQSRC_IR0RX		0b00000
#define N_DRQSRC_IR1RX 		0b00001
#define N_DRQSRC_SPDIFRX	0b00010
#define N_DRQSRC_IISRX		0b00011
#define N_DRQSRC_AC97RX		0b00101
#define N_DRQSRC_UART0RX	0b01000
#define N_DRQSRC_UART1RX 	0b01001
#define N_DRQSRC_UART2RX	0b01010
#define N_DRQSRC_UART3RX	0b01011
#define N_DRQSRC_UART4RX	0b01100
#define N_DRQSRC_UART5RX	0b01101
#define N_DRQSRC_UART6RX	0b01110
#define N_DRQSRC_UART7RX	0b01111
#define N_DRQSRC_HDMIDDCRX	0b10000
#define N_DRQSRC_AUDIOCDAD	0b10011	//Audio Codec D/A
#define N_DRQSRC_SRAM		0b10101
#define N_DRQSRC_SDRAM		0b10110
#define N_DRQSRC_TPAD		0b10111	//TP A/D
#define N_DRQSRC_SPI0RX		0b11000
#define N_DRQSRC_SPI1RX		0b11001
#define N_DRQSRC_SPI2RX		0b11010
#define N_DRQSRC_SPI3RX		0b11011

/*normal DMA destination*/
#define N_DRQDST_IR0TX		0b00000
#define N_DRQDST_IR1TX 		0b00001
#define N_DRQDST_SPDIFTX	0b00010
#define N_DRQDST_IISTX		0b00011
#define N_DRQDST_AC97TX		0b00101
#define N_DRQDST_UART0TX	0b01000
#define N_DRQDST_UART1TX 	0b01001
#define N_DRQDST_UART2TX	0b01010
#define N_DRQDST_UART3TX	0b01011
#define N_DRQDST_UART4TX	0b01100
#define N_DRQDST_UART5TX	0b01101
#define N_DRQDST_UART6TX	0b01110
#define N_DRQDST_UART7TX	0b01111
#define N_DRQDST_HDMIDDCTX	0b10000	//HDMI DDC TX
#define N_DRQDST_AUDIOCDAD	0b10011	//Audio Codec D/A
#define N_DRQDST_SRAM		0b10101
#define N_DRQDST_SDRAM		0b10110
#define N_DRQDST_SPI0TX		0b11000
#define N_DRQDST_SPI1TX		0b11001
#define N_DRQDST_SPI2TX		0b11010
#define N_DRQDST_SPI3TX		0b11011

/*Dedicated DMA Source*/
#define D_DRQSRC_SRAM		0b00000//0x0 SRAM memory
#define D_DRQSRC_SDRAM		0b00001//0x1 SDRAM memory
#define D_DRQSRC_PATA		0b00010//0x2 PATA
#define D_DRQSRC_NAND 		0b00011//0x3 NAND Flash Controller(NFC)
#define D_DRQSRC_USB0 		0b00100//0x4 USB0
#define D_DRQSRC_EMACRX		0b00111//0x7 Ethernet MAC Rx
#define D_DRQSRC_SPI1RX		0b01001//0x9 SPI1 RX
#define D_DRQSRC_SECRX 		0b01011//0xB Security System Rx
#define D_DRQSRC_MS 		0b10111//0x17 Memory Stick Controller(MSC)
#define D_DRQSRC_SPI0RX		0b11011//0x1B SPI0 RX
#define D_DRQSRC_SPI2RX		0b11101//0x1D SPI2 RX
#define D_DRQSRC_SPI3RX		0b11111//0x1F SPI3 RX


/*Dedicated DMA Destination*/
#define D_DRQDST_SRAM		0b00000//0x0 SRAM memory
#define D_DRQDST_SDRAM		0b00001//0x1 SDRAM memory
#define D_DRQDST_PATA		0b00010//0x2 PATA
#define D_DRQDST_NAND 		0b00011//0x3 NAND Flash Controller(NFC)
#define D_DRQDST_USB0 		0b00100//0x4 USB0
#define D_DRQDST_EMACTX		0b00110//0x6 Ethernet MAC Rx
#define D_DRQDST_SPI1TX		0b01000//0x8 SPI1 RX
#define D_DRQDST_SECTX 		0b01010//0xA Security System Rx
#define D_DRQDST_TCON0 		0b01110//0xE TCON0
#define D_DRQDST_TCON1 		0b01111//0xF TCON1
#define D_DRQDST_MS			0b10111//0x17 Memory Stick Controller(MSC)
#define D_DRQDST_HDMIAUDIO	0b11000//0x18 HDMI Audio
#define D_DRQDST_SPI0TX		0b11010//0x1A SPI0 TX
#define D_DRQDST_SPI2TX		0b11100//0x1C SPI2 TX
#define D_DRQDST_SPI3TX		0b11110//0x1E SPI3 TX


enum drq_type {
		DRQ_TYPE_SRAM,
		DRQ_TYPE_SDRAM,
		DRQ_TYPE_PATA,
		DRQ_TYPE_NAND,
		DRQ_TYPE_USB0,
		DRQ_TYPE_EMAC,
		DRQ_TYPE_SPI1,
		DRQ_TYPE_SS,//Security System
		DRQ_TYPE_MS,//Memory Stick Control
		DRQ_TYPE_SPI0,
		DRQ_TYPE_SPI2,
		DRQ_TYPE_SPI3,
		DRQ_TYPE_TCON0,
		DRQ_TYPE_TCON1,
		DRQ_TYPE_HDMI,

		DRQ_TYPE_HDMIAUDIO,
		DRQ_TYPE_IR0,
		DRQ_TYPE_IR1,
		DRQ_TYPE_SPDIF,
		DRQ_TYPE_IIS,
		DRQ_TYPE_AC97,
		DRQ_TYPE_UART0,
		DRQ_TYPE_UART1,
		DRQ_TYPE_UART2,
		DRQ_TYPE_UART3,
		DRQ_TYPE_UART4,
		DRQ_TYPE_UART5,
		DRQ_TYPE_UART6,
		DRQ_TYPE_UART7,
		DRQ_TYPE_AUDIO,
		DRQ_TYPE_TPAD,
		DRQ_TYPE_MAX,
};


/* DMAXFER_(dist)_(sigle/burst/tippl)_(byte/half/word)_(src)_(sigle/burst/tippl)_(byte/half/word) */
#define X_SIGLE   0
#define X_BURST   1
#define X_TIPPL	  2
#define X_BYTE    0
#define X_HALF    1
#define X_WORD    2

/*data length and burst length combination in DDMA and NDMA */
enum xferunit {
	/*des:X_SIGLE  src:X_SIGLE*/
	DMAXFER_D_SBYTE_S_SBYTE,
	DMAXFER_D_SBYTE_S_SHALF,
	DMAXFER_D_SBYTE_S_SWORD,
	DMAXFER_D_SHALF_S_SBYTE,
	DMAXFER_D_SHALF_S_SHALF,
	DMAXFER_D_SHALF_S_SWORD,
	DMAXFER_D_SWORD_S_SBYTE,
	DMAXFER_D_SWORD_S_SHALF,
	DMAXFER_D_SWORD_S_SWORD,

	/*des:X_SIGLE  src:X_BURST*/
	DMAXFER_D_SBYTE_S_BBYTE,
	DMAXFER_D_SBYTE_S_BHALF,
	DMAXFER_D_SBYTE_S_BWORD,
	DMAXFER_D_SHALF_S_BBYTE,
	DMAXFER_D_SHALF_S_BHALF,
	DMAXFER_D_SHALF_S_BWORD,
	DMAXFER_D_SWORD_S_BBYTE,
	DMAXFER_D_SWORD_S_BHALF,
	DMAXFER_D_SWORD_S_BWORD,

	/*des:X_SIGLE   src:X_TIPPL*/
	DMAXFER_D_SBYTE_S_TBYTE,
	DMAXFER_D_SBYTE_S_THALF,
	DMAXFER_D_SBYTE_S_TWORD,
	DMAXFER_D_SHALF_S_TBYTE,
	DMAXFER_D_SHALF_S_THALF,
	DMAXFER_D_SHALF_S_TWORD,
	DMAXFER_D_SWORD_S_TBYTE,
	DMAXFER_D_SWORD_S_THALF,
	DMAXFER_D_SWORD_S_TWORD,

	/*des:X_BURST  src:X_BURST*/
	DMAXFER_D_BBYTE_S_BBYTE,
	DMAXFER_D_BBYTE_S_BHALF,
	DMAXFER_D_BBYTE_S_BWORD,
	DMAXFER_D_BHALF_S_BBYTE,
	DMAXFER_D_BHALF_S_BHALF,
	DMAXFER_D_BHALF_S_BWORD,
	DMAXFER_D_BWORD_S_BBYTE,
	DMAXFER_D_BWORD_S_BHALF,
	DMAXFER_D_BWORD_S_BWORD,

	/*des:X_BURST   src:X_SIGLE*/
	DMAXFER_D_BBYTE_S_SBYTE,
	DMAXFER_D_BBYTE_S_SHALF,
	DMAXFER_D_BBYTE_S_SWORD,
	DMAXFER_D_BHALF_S_SBYTE,
	DMAXFER_D_BHALF_S_SHALF,
	DMAXFER_D_BHALF_S_SWORD,
	DMAXFER_D_BWORD_S_SBYTE,
	DMAXFER_D_BWORD_S_SHALF,
	DMAXFER_D_BWORD_S_SWORD,

	/*des:X_BURST   src:X_TIPPL*/
	DMAXFER_D_BBYTE_S_TBYTE,
	DMAXFER_D_BBYTE_S_THALF,
	DMAXFER_D_BBYTE_S_TWORD,
	DMAXFER_D_BHALF_S_TBYTE,
	DMAXFER_D_BHALF_S_THALF,
	DMAXFER_D_BHALF_S_TWORD,
	DMAXFER_D_BWORD_S_TBYTE,
	DMAXFER_D_BWORD_S_THALF,
	DMAXFER_D_BWORD_S_TWORD,

	/*des:X_TIPPL   src:X_TIPPL*/
	DMAXFER_D_TBYTE_S_TBYTE,
	DMAXFER_D_TBYTE_S_THALF,
	DMAXFER_D_TBYTE_S_TWORD,
	DMAXFER_D_THALF_S_TBYTE,
	DMAXFER_D_THALF_S_THALF,
	DMAXFER_D_THALF_S_TWORD,
	DMAXFER_D_TWORD_S_TBYTE,
	DMAXFER_D_TWORD_S_THALF,
	DMAXFER_D_TWORD_S_TWORD,

	/*des:X_TIPPL   src:X_SIGLE*/
	DMAXFER_D_TBYTE_S_SBYTE,
	DMAXFER_D_TBYTE_S_SHALF,
	DMAXFER_D_TBYTE_S_SWORD,
	DMAXFER_D_THALF_S_SBYTE,
	DMAXFER_D_THALF_S_SHALF,
	DMAXFER_D_THALF_S_SWORD,
	DMAXFER_D_TWORD_S_SBYTE,
	DMAXFER_D_TWORD_S_SHALF,
	DMAXFER_D_TWORD_S_SWORD,

	/*des:X_TIPPL   src:X_BURST*/
	DMAXFER_D_TBYTE_S_BBYTE,
	DMAXFER_D_TBYTE_S_BHALF,
	DMAXFER_D_TBYTE_S_BWORD,
	DMAXFER_D_THALF_S_BBYTE,
	DMAXFER_D_THALF_S_BHALF,
	DMAXFER_D_THALF_S_BWORD,
	DMAXFER_D_TWORD_S_BBYTE,
	DMAXFER_D_TWORD_S_BHALF,
	DMAXFER_D_TWORD_S_BWORD,
	DMAXFER_MAX
};

/* DMAADDRT_(dist)_(increase/fix)_(src)_(increase/fix) */
#define A_INC     0x0
#define A_FIX     0x1
#define A_LN      0x0
#define A_IO      0x1
#define A_PH      0x2
#define A_PV      0x3

enum addrt {
	/*NDMA address type*/
	DMAADDRT_D_INC_S_INC,
	DMAADDRT_D_INC_S_FIX,
	DMAADDRT_D_FIX_S_INC,
	DMAADDRT_D_FIX_S_FIX,

	/*DDMA address type*/
	DMAADDRT_D_LN_S_LN,
	DMAADDRT_D_LN_S_IO,
	DMAADDRT_D_LN_S_PH,
	DMAADDRT_D_LN_S_PV,

	DMAADDRT_D_IO_S_LN,
	DMAADDRT_D_IO_S_IO,
	DMAADDRT_D_IO_S_PH,
	DMAADDRT_D_IO_S_PV,

	DMAADDRT_D_PH_S_LN,
	DMAADDRT_D_PH_S_IO,
	DMAADDRT_D_PH_S_PH,
	DMAADDRT_D_PH_S_PV,

	DMAADDRT_D_PV_S_LN,
	DMAADDRT_D_PV_S_IO,
	DMAADDRT_D_PV_S_PH,
	DMAADDRT_D_PV_S_PV,

	DMAADDRT_MAX
};

/* use this to specifiy hardware channel number */
#define DMACH_LOW_LEVEL	(1<<28)

/* we have 16 dma channels */
#define SW_DMA_CHANNELS		(16)

/* types */
enum sw_dma_state {
	SW_DMA_IDLE,
	SW_DMA_RUNNING,
	SW_DMA_PAUSED
};


/* enum sw_dma_loadst
 *
 * This represents the state of the DMA engine, wrt to the loaded / running
 * transfers. Since we don't have any way of knowing exactly the state of
 * the DMA transfers, we need to know the state to make decisions on wether
 * we can
 *
 * SW_DMA_NONE
 *
 * There are no buffers loaded (the channel should be inactive)
 *
 * SW_DMA_1LOADED
 *
 * There is one buffer loaded, however it has not been confirmed to be
 * loaded by the DMA engine. This may be because the channel is not
 * yet running, or the DMA driver decided that it was too costly to
 * sit and wait for it to happen.
 *
 * SW_DMA_1RUNNING
 *
 * The buffer has been confirmed running, and not finisged
 *
 * SW_DMA_1LOADED_1RUNNING
 *
 * There is a buffer waiting to be loaded by the DMA engine, and one
 * currently running.
*/

enum sw_dma_loadst {
	SW_DMALOAD_NONE,
	SW_DMALOAD_1LOADED,
	SW_DMALOAD_1RUNNING,
	SW_DMALOAD_1LOADED_1RUNNING,
};

enum sw_dma_buffresult {
	SW_RES_OK,
	SW_RES_ERR,
	SW_RES_ABORT
};

enum sw_dmadir {
	SW_DMA_RWNULL,
	SW_DMA_RDEV,		/* read from dev */
	SW_DMA_WDEV,		/* write to dev */
	SW_DMA_M2M,
//	SW_DMA_RWDEV		/* can r/w dev */
};

enum dma_hf_irq {
	SW_DMA_IRQ_NO,
	SW_DMA_IRQ_HALF,
	SW_DMA_IRQ_FULL
};
/* enum sw_chan_op
 *
 * operation codes passed to the DMA code by the user, and also used
 * to inform the current channel owner of any changes to the system state
*/

enum sw_chan_op {
	SW_DMAOP_START,
	SW_DMAOP_STOP,
	SW_DMAOP_PAUSE,
	SW_DMAOP_RESUME,
	SW_DMAOP_FLUSH,
	SW_DMAOP_TIMEOUT,		/* internal signal to handler */
	SW_DMAOP_STARTED,		/* indicate channel started */
};

/* flags */

#define SW_DMAF_SLOW         (1<<0)   /* slow, so don't worry about
					    * waiting for reloads */
#define SW_DMAF_AUTOSTART    (1<<1)   /* auto-start if buffer queued */

/* dma buffer */

struct sw_dma_client {
	char                *name;
};

/* sw_dma_buf_s
 *
 * internally used buffer structure to describe a queued or running
 * buffer.
*/

struct sw_dma_buf;
struct sw_dma_buf {
	struct sw_dma_buf	*next;
	int			 magic;		/* magic */
	int			 size;		/* buffer size in bytes */
	dma_addr_t		 data;		/* start of DMA data */
	dma_addr_t		 ptr;		/* where the DMA got to [1] */
	void			*id;		/* client's id */
};

/* [1] is this updated for both recv/send modes? */

struct sw_dma_chan;

/* sw_dma_cbfn_t
 *
 * buffer callback routine type
*/

typedef void (*sw_dma_cbfn_t)(struct sw_dma_chan *,
				void *buf, int size,
				enum sw_dma_buffresult result);

typedef int  (*sw_dma_opfn_t)(struct sw_dma_chan *,
				enum sw_chan_op );

struct sw_dma_stats {
	unsigned long		loads;
	unsigned long		timeout_longest;
	unsigned long		timeout_shortest;
	unsigned long		timeout_avg;
	unsigned long		timeout_failed;
};

struct sw_dma_map;

/* struct sw_dma_chan
 *
 * full state information for each DMA channel
*/

struct sw_dma_chan {
	/* channel state flags and information */
	unsigned char		 number;      /* number of this dma channel */
	unsigned char		 in_use;      /* channel allocated */
	unsigned char		 irq_claimed; /* irq claimed for channel */
	unsigned char		 irq_enabled; /* irq enabled for channel */

	/* channel state */

	enum sw_dma_state	 state;
	enum sw_dma_loadst	 load_state;
	struct sw_dma_client *client;

	/* channel configuration */
	unsigned long		 dev_addr;
	unsigned long		 load_timeout;
	unsigned int		 flags;		/* channel flags */
	unsigned int		 hw_cfg;	/* last hw config */

	struct sw_dma_map	*map;		/* channel hw maps */

	/* channel's hardware position and configuration */
	void __iomem		*regs;		/* channels registers */
	void __iomem		*addr_reg;	/* data address register */
	//unsigned int		 irq;		/* channel irq */
	unsigned long		 dcon;		/* default value of DCON */

	/* driver handles */
	sw_dma_cbfn_t	 callback_fn;	/* buffer done callback */
	sw_dma_cbfn_t	 callback_hd;	/* buffer half done callback */
	sw_dma_opfn_t	 op_fn;		/* channel op callback */

	/* stats gathering */
	struct sw_dma_stats *stats;
	struct sw_dma_stats  stats_store;

	/* buffer list and information */
	struct sw_dma_buf	*curr;		/* current dma buffer */
	struct sw_dma_buf	*next;		/* next buffer to load */
	struct sw_dma_buf	*end;		/* end of queue */

	/* system device */
#if 0	/* no sys_device in linux3.3 */
	struct sys_device	dev;
#endif
	void * dev_id;

#ifdef USE_SPINLOCK_20120802
	/*
	 * dma channel lock, in smp enviroment, use spin_lock_irqsave
	 * instead of local_irq_save.
	 */
	spinlock_t 		lock;
#endif /* USE_SPINLOCK_20120802 */
};

/*the channel number of above 8 is DDMA channel.*/
#define IS_DADECATE_DMA(ch) (ch->number >= 8)

struct dma_hw_conf{
	unsigned char		drqsrc_type;
	unsigned char		drqdst_type;

	unsigned char		xfer_type;
	unsigned char		address_type;
	unsigned char           dir;
	unsigned char		hf_irq;
	unsigned char		reload;

	unsigned long		from;
	unsigned long		to;
	unsigned long		cmbk;
};

extern inline void DMA_COPY_HW_CONF(struct dma_hw_conf *to, struct dma_hw_conf *from);

/* struct sw_dma_map
 *
 * this holds the mapping information for the channel selected
 * to be connected to the specified device
*/
struct sw_dma_map {
	const char		*name;
	struct dma_hw_conf  user_hw_conf;
	const struct dma_hw_conf*  default_hw_conf;
	struct dma_hw_conf* conf_ptr;
	unsigned long channels[SW_DMA_CHANNELS];
};

struct sw_dma_selection {
	struct sw_dma_map	*map;
	unsigned long		 map_size;
	unsigned long		 dcon_mask;
};

/* struct sw_dma_order_ch
 *
 * channel map for one of the `enum dma_ch` dma channels. the list
 * entry contains a set of low-level channel numbers, orred with
 * DMA_CH_VALID, which are checked in the order in the array.
*/

struct sw_dma_order_ch {
	unsigned int	list[SW_DMA_CHANNELS];	/* list of channels */
	unsigned int	flags;				/* flags */
};

/* struct s3c24xx_dma_order
 *
 * information provided by either the core or the board to give the
 * dma system a hint on how to allocate channels
*/

struct sw_dma_order {
	struct sw_dma_order_ch	channels[DMACH_MAX];
};

/* the currently allocated channel information */
extern struct sw_dma_chan sw_chans[];

/* note, we don't really use dma_device_t at the moment */
typedef unsigned long dma_device_t;

/* functions --------------------------------------------------------------- */

/* sw_dma_request
 *
 * request a dma channel exclusivley
*/

extern int sw_dma_request(unsigned int channel,
			struct sw_dma_client *, void *dev);


/* sw_dma_ctrl
 *
 * change the state of the dma channel
*/

extern int sw_dma_ctrl(unsigned int channel, enum sw_chan_op op);

/* sw_dma_setflags
 *
 * set the channel's flags to a given state
*/

extern int sw_dma_setflags(unsigned int channel,
				unsigned int flags);

/* sw_dma_free
 *
 * free the dma channel (will also abort any outstanding operations)
*/

extern int sw_dma_free(unsigned int channel, struct sw_dma_client *);

/* sw_dma_enqueue
 *
 * place the given buffer onto the queue of operations for the channel.
 * The buffer must be allocated from dma coherent memory, or the Dcache/WB
 * drained before the buffer is given to the DMA system.
*/

extern int sw_dma_enqueue(unsigned int channel, void *id,
			dma_addr_t data, int size);

/* sw_dma_config
 *
 * configure the dma channel
*/
extern void poll_dma_pending(int chan_nr);

extern int sw_dma_config(unsigned int channel, struct dma_hw_conf* user_conf);

extern int sw15_dma_init(void);

extern int sw_dma_order_set(struct sw_dma_order *ord);

extern int sw_dma_init_map(struct sw_dma_selection *sel);

/* sw_dma_getposition
 *
 * get the position that the dma transfer is currently at
*/

extern int sw_dma_getposition(unsigned int channel,
				dma_addr_t *src, dma_addr_t *dest);

extern int sw_dma_set_opfn(unsigned int, sw_dma_opfn_t rtn);
extern int sw_dma_set_buffdone_fn(unsigned int, sw_dma_cbfn_t rtn);
extern int sw_dma_set_halfdone_fn(unsigned int, sw_dma_cbfn_t rtn);
extern int sw_dma_getcurposition(unsigned int channel,
				dma_addr_t *src, dma_addr_t *dest);
#elif 0
#include <mach/hardware.h>

/* burst length */
#define X_SIGLE   0
#define X_BURST   1
#define X_TIPPL	  2

/* data width */
#define X_BYTE    0
#define X_HALF    1
#define X_WORD    2

/* address mode */
#define A_LN      0x0
#define A_IO      0x1

/*
 * data width and burst length combination
 * index for xfer_arr[]
 */
enum xferunit_e {
	/* des:X_SIGLE  src:X_SIGLE */
	DMAXFER_D_SBYTE_S_SBYTE,
	DMAXFER_D_SBYTE_S_SHALF,
	DMAXFER_D_SBYTE_S_SWORD,
	DMAXFER_D_SHALF_S_SBYTE,
	DMAXFER_D_SHALF_S_SHALF,
	DMAXFER_D_SHALF_S_SWORD,
	DMAXFER_D_SWORD_S_SBYTE,
	DMAXFER_D_SWORD_S_SHALF,
	DMAXFER_D_SWORD_S_SWORD,

	/* des:X_SIGLE  src:X_BURST */
	DMAXFER_D_SBYTE_S_BBYTE,
	DMAXFER_D_SBYTE_S_BHALF,
	DMAXFER_D_SBYTE_S_BWORD,
	DMAXFER_D_SHALF_S_BBYTE,
	DMAXFER_D_SHALF_S_BHALF,
	DMAXFER_D_SHALF_S_BWORD,
	DMAXFER_D_SWORD_S_BBYTE,
	DMAXFER_D_SWORD_S_BHALF,
	DMAXFER_D_SWORD_S_BWORD,

	/* des:X_SIGLE   src:X_TIPPL */
	DMAXFER_D_SBYTE_S_TBYTE,
	DMAXFER_D_SBYTE_S_THALF,
	DMAXFER_D_SBYTE_S_TWORD,
	DMAXFER_D_SHALF_S_TBYTE,
	DMAXFER_D_SHALF_S_THALF,
	DMAXFER_D_SHALF_S_TWORD,
	DMAXFER_D_SWORD_S_TBYTE,
	DMAXFER_D_SWORD_S_THALF,
	DMAXFER_D_SWORD_S_TWORD,

	/* des:X_BURST  src:X_BURST */
	DMAXFER_D_BBYTE_S_BBYTE,
	DMAXFER_D_BBYTE_S_BHALF,
	DMAXFER_D_BBYTE_S_BWORD,
	DMAXFER_D_BHALF_S_BBYTE,
	DMAXFER_D_BHALF_S_BHALF,
	DMAXFER_D_BHALF_S_BWORD,
	DMAXFER_D_BWORD_S_BBYTE,
	DMAXFER_D_BWORD_S_BHALF,
	DMAXFER_D_BWORD_S_BWORD,

	/* des:X_BURST   src:X_SIGLE */
	DMAXFER_D_BBYTE_S_SBYTE,
	DMAXFER_D_BBYTE_S_SHALF,
	DMAXFER_D_BBYTE_S_SWORD,
	DMAXFER_D_BHALF_S_SBYTE,
	DMAXFER_D_BHALF_S_SHALF,
	DMAXFER_D_BHALF_S_SWORD,
	DMAXFER_D_BWORD_S_SBYTE,
	DMAXFER_D_BWORD_S_SHALF,
	DMAXFER_D_BWORD_S_SWORD,

	/* des:X_BURST   src:X_TIPPL */
	DMAXFER_D_BBYTE_S_TBYTE,
	DMAXFER_D_BBYTE_S_THALF,
	DMAXFER_D_BBYTE_S_TWORD,
	DMAXFER_D_BHALF_S_TBYTE,
	DMAXFER_D_BHALF_S_THALF,
	DMAXFER_D_BHALF_S_TWORD,
	DMAXFER_D_BWORD_S_TBYTE,
	DMAXFER_D_BWORD_S_THALF,
	DMAXFER_D_BWORD_S_TWORD,

	/* des:X_TIPPL   src:X_TIPPL */
	DMAXFER_D_TBYTE_S_TBYTE,
	DMAXFER_D_TBYTE_S_THALF,
	DMAXFER_D_TBYTE_S_TWORD,
	DMAXFER_D_THALF_S_TBYTE,
	DMAXFER_D_THALF_S_THALF,
	DMAXFER_D_THALF_S_TWORD,
	DMAXFER_D_TWORD_S_TBYTE,
	DMAXFER_D_TWORD_S_THALF,
	DMAXFER_D_TWORD_S_TWORD,

	/* des:X_TIPPL   src:X_SIGLE */
	DMAXFER_D_TBYTE_S_SBYTE,
	DMAXFER_D_TBYTE_S_SHALF,
	DMAXFER_D_TBYTE_S_SWORD,
	DMAXFER_D_THALF_S_SBYTE,
	DMAXFER_D_THALF_S_SHALF,
	DMAXFER_D_THALF_S_SWORD,
	DMAXFER_D_TWORD_S_SBYTE,
	DMAXFER_D_TWORD_S_SHALF,
	DMAXFER_D_TWORD_S_SWORD,

	/* des:X_TIPPL   src:X_BURST */
	DMAXFER_D_TBYTE_S_BBYTE,
	DMAXFER_D_TBYTE_S_BHALF,
	DMAXFER_D_TBYTE_S_BWORD,
	DMAXFER_D_THALF_S_BBYTE,
	DMAXFER_D_THALF_S_BHALF,
	DMAXFER_D_THALF_S_BWORD,
	DMAXFER_D_TWORD_S_BBYTE,
	DMAXFER_D_TWORD_S_BHALF,
	DMAXFER_D_TWORD_S_BWORD,
	DMAXFER_MAX
};

/*
 * src/dst address type
 * index for addrtype_arr[]
 */
enum addrt_e {
	DMAADDRT_D_LN_S_LN,
	DMAADDRT_D_LN_S_IO,
	DMAADDRT_D_IO_S_LN,
	DMAADDRT_D_IO_S_IO,
	DMAADDRT_MAX
};

/* dma channel irq type */
enum dma_chan_irq_type {
	CHAN_IRQ_NO 	= 0,			/* none */
	CHAN_IRQ_HD	= (0b001	),	/* package half done irq */
	CHAN_IRQ_FD	= (0b010	),	/* package full done irq */
	CHAN_IRQ_QD	= (0b100	)	/* queue end irq */
};

/*
 * dma config information
 */
typedef struct tag_dma_config {
	/*
	 * data length and burst length combination in DDMA and NDMA
	 * eg: DMAXFER_D_SWORD_S_SWORD, DMAXFER_D_SBYTE_S_BBYTE
	 */
	enum xferunit_e	xfer_type;
	/*
	 * NDMA/DDMA src/dst address type
	 * eg: DMAADDRT_D_INC_S_INC(NDMA addr type),
	 *     DMAADDRT_D_LN_S_LN / DMAADDRT_D_LN_S_IO(DDMA addr type)
	 */
	enum addrt_e	address_type;
	u32		para;		/* dma para reg */
	u32 		irq_spt;	/* channel irq supported, eg: CHAN_IRQ_HD | CHAN_IRQ_FD */
//	u32		src_addr;	/* src phys addr */
	//u32		dst_addr;	/* dst phys addr */
	//u32		byte_cnt;	/* byte cnt for src_addr/dst_addr transfer */
	bool		bconti_mode;	/* continue mode */
	u8		src_drq_type;	/* src drq type */
	u8		dst_drq_type;	/* dst drq type */
}dma_config_t;

/* normal channel src drq type */
enum n_drqsrc_e {
	N_SRC_IR0_RX		= 0b00000,
	N_SRC_IR1_RX		= 0b00001,
	N_SRC_SPDIF_RX		= 0b00010,
	N_SRC_IIS0_RX		= 0b00011,
	N_SRC_IIS1_RX		= 0b00100,
	N_SRC_AC97_RX		= 0b00101,
	N_SRC_IIS2_RX		= 0b00110,
	N_SRC_RSV0		= 0b00111,
	N_SRC_UART0_RX		= 0b01000,
	N_SRC_UART1_RX		= 0b01001,
	N_SRC_UART2_RX		= 0b01010,
	N_SRC_UART3_RX		= 0b01011,
	N_SRC_UART4_RX		= 0b01100,
	N_SRC_UART5_RX		= 0b01101,
	N_SRC_UART6_RX		= 0b01110,
	N_SRC_UART7_RX		= 0b01111,
	N_SRC_HDMI_DDC_RX	= 0b10000,
	N_SRC_USB_EP1		= 0b10001,
	N_SRC_RSV1		= 0b10010,
	N_SRC_AUDIO_CODEC_AD	= 0b10011,
	N_SRC_RSV2		= 0b10100,
	N_SRC_SRAM		= 0b10101,
	N_SRC_SDRAM		= 0b10110,
	N_SRC_TP_AD		= 0b10111,
	N_SRC_SPI0_RX		= 0b11000,
	N_SRC_SPI1_RX		= 0b11001,
	N_SRC_SPI2_RX		= 0b11010,
	N_SRC_SPI3_RX		= 0b11011,
	N_SRC_USB_EP2		= 0b11100,
	N_SRC_USB_EP3		= 0b11101,
	N_SRC_USB_EP4		= 0b11110,
	N_SRC_USB_EP5		= 0b11111,
};

/* normal channel dst drq type */
enum n_drqdst_e {
	N_DST_IR0_TX		= 0b00000,
	N_DST_IR1_TX		= 0b00001,
	N_DST_SPDIF_TX		= 0b00010,
	N_DST_IIS0_TX		= 0b00011,
	N_DST_IIS1_TX		= 0b00100,
	N_DST_AC97_TX		= 0b00101,
	N_DST_IIS2_TX		= 0b00110,
	N_DST_RSV0		= 0b00111,
	N_DST_UART0_TX		= 0b01000,
	N_DST_UART1_TX		= 0b01001,
	N_DST_UART2_TX		= 0b01010,
	N_DST_UART3_TX		= 0b01011,
	N_DST_UART4_TX		= 0b01100,
	N_DST_UART5_TX		= 0b01101,
	N_DST_UART6_TX		= 0b01110,
	N_DST_UART7_TX		= 0b01111,
	N_DST_HDMI_DDC_TX	= 0b10000,
	N_DST_USB_EP1		= 0b10001,
	N_DST_RSV1		= 0b10010,
	N_DST_AUDIO_CODEC_DA	= 0b10011,
	N_DST_RSV2		= 0b10100,
	N_DST_SRAM		= 0b10101,
	N_DST_SDRAM		= 0b10110,
	N_DST_TP_AD		= 0b10111,
	N_DST_SPI0_TX		= 0b11000,
	N_DST_SPI1_TX		= 0b11001,
	N_DST_SPI2_TX		= 0b11010,
	N_DST_SPI3_TX		= 0b11011,
	N_DST_USB_EP2		= 0b11100,
	N_DST_USB_EP3		= 0b11101,
	N_DST_USB_EP4		= 0b11110,
	N_DST_USB_EP5		= 0b11111,
};

/* dedicate channel src drq type */
enum d_drqsrc_e {
	D_SRC_SRAM		= 0b00000,
	D_SRC_SDRAM		= 0b00001,
	D_SRC_RSV		= 0b00010,
	D_SRC_NAND		= 0b00011,
	D_SRC_USB0		= 0b00100,
	D_SRC_RSV		= 0b00101,
	D_SRC_RSV		= 0b00110,
	D_SRC_EMAC_RX		= 0b00111,
	D_SRC_RSV		= 0b01000,
	D_SRC_SPI1_RX		= 0b01001,
	D_SRC_RSV		= 0b01010,
	D_SRC_SS_RX		= 0b01011, /* security system rx */
	D_SRC_RSV		= 0b01100,
	D_SRC_RSV		= 0b01101,
	D_SRC_RSV		= 0b01110,
	D_SRC_RSV		= 0b01111,
	D_SRC_RSV		= 0b10000,
	D_SRC_RSV		= 0b10001,
	D_SRC_RSV		= 0b10010,
	D_SRC_RSV		= 0b10011,
	D_SRC_RSV		= 0b10100,
	D_SRC_RSV		= 0b10101,
	D_SRC_RSV		= 0b10110,
	D_SRC_MSC		= 0b10111,
	D_SRC_RSV		= 0b11000,
	D_SRC_RSV		= 0b11001,
	D_SRC_RSV		= 0b11010,
	D_SRC_SPI0_RX		= 0b11011,
	D_SRC_RSV		= 0b11100,
	D_SRC_SPI2_RX		= 0b11101,
	D_SRC_RSV		= 0b11110,
	D_SRC_SPI3_RX		= 0b11111,
};

/* dedicate channel dst drq type */
enum d_drqdst_e {
	D_DST_SRAM		= 0b00000,
	D_DST_SDRAM		= 0b00001,
	D_DST_RSV		= 0b00010,
	D_DST_NAND		= 0b00011,
	D_DST_USB0		= 0b00100,
	D_DST_RSV		= 0b00101,
	D_DST_EMAC_TX		= 0b00110,
	D_DST_RSV		= 0b00111,
	D_DST_SPI1_TX		= 0b01000,
	D_DST_RSV		= 0b01001,
	D_DST_SS_TX		= 0b01010, /* security system tx */
	D_DST_RSV		= 0b01011,
	D_DST_RSV		= 0b01100,
	D_DST_RSV		= 0b01101,
	D_DST_TCON0		= 0b01110,
	D_DST_TCON1		= 0b01111,
	D_DST_RSV		= 0b10000,
	D_DST_RSV		= 0b10001,
	D_DST_RSV		= 0b10010,
	D_DST_RSV		= 0b10011,
	D_DST_RSV		= 0b10100,
	D_DST_RSV		= 0b10101,
	D_DST_RSV		= 0b10110,
	D_DST_MSC		= 0b10111,
	D_DST_HDMI_AUD		= 0b11000,
	D_DST_RSV		= 0b11001,
	D_DST_SPI0_TX		= 0b11010,
	D_DST_RSV		= 0b11011,
	D_DST_SPI2_TX		= 0b11100,
	D_DST_RSV		= 0b11101,
	D_DST_SPI3_TX		= 0b11110,
	D_DST_RSV		= 0b11111,
};
#endif /* 0 */

/* dma operation type */
typedef enum {
	DMA_OP_START,  			/* start dma */
	DMA_OP_STOP,  			/* stop dma */

	DMA_OP_GET_STATUS,  		/* get channel status: idle/busy */
	DMA_OP_GET_CUR_SRC_ADDR,  	/* get current src address */
	DMA_OP_GET_CUR_DST_ADDR,  	/* get current dst address */
	DMA_OP_GET_BYTECNT_LEFT,  	/* get byte cnt left */

	DMA_OP_SET_HD_CB,		/* set half done callback */
	DMA_OP_SET_FD_CB,		/* set full done callback */
}dma_op_type_e;

/* dma handle type defination */
typedef void * dma_hdl_t;

/* dma callback func */
typedef void (* dma_cb)(dma_hdl_t dma_hdl, void *parg);
typedef void (* dma_op_cb)(dma_hdl_t dma_hdl, void *parg, enum dma_op_type_e op);

/* dma callback struct */
typedef struct {
	dma_cb 		func;	/* dma callback fuction */
	void 		*parg;	/* args of func */
}dma_cb_t;

/* dma operation callback struct */
typedef struct {
	dma_op_cb 	func;	/* dma operation callback fuction */
	void 		*parg;	/* args of func */
}dma_op_cb_t;

typedef enum {
	CHAN_NORAML,
	CHAN_DEDICATE,
}dma_chan_type_e;

enum xferunit_e {
	/* des:X_SIGLE  src:X_SIGLE */
	DMAXFER_D_SBYTE_S_SBYTE,
	DMAXFER_D_SBYTE_S_SHALF,
	DMAXFER_D_SBYTE_S_SWORD,
	DMAXFER_D_SHALF_S_SBYTE,
	DMAXFER_D_SHALF_S_SHALF,
	DMAXFER_D_SHALF_S_SWORD,
	DMAXFER_D_SWORD_S_SBYTE,
	DMAXFER_D_SWORD_S_SHALF,
	DMAXFER_D_SWORD_S_SWORD,

	/* des:X_SIGLE  src:X_BURST */
	DMAXFER_D_SBYTE_S_BBYTE,
	DMAXFER_D_SBYTE_S_BHALF,
	DMAXFER_D_SBYTE_S_BWORD,
	DMAXFER_D_SHALF_S_BBYTE,
	DMAXFER_D_SHALF_S_BHALF,
	DMAXFER_D_SHALF_S_BWORD,
	DMAXFER_D_SWORD_S_BBYTE,
	DMAXFER_D_SWORD_S_BHALF,
	DMAXFER_D_SWORD_S_BWORD,

	/* des:X_SIGLE   src:X_TIPPL */
	DMAXFER_D_SBYTE_S_TBYTE,
	DMAXFER_D_SBYTE_S_THALF,
	DMAXFER_D_SBYTE_S_TWORD,
	DMAXFER_D_SHALF_S_TBYTE,
	DMAXFER_D_SHALF_S_THALF,
	DMAXFER_D_SHALF_S_TWORD,
	DMAXFER_D_SWORD_S_TBYTE,
	DMAXFER_D_SWORD_S_THALF,
	DMAXFER_D_SWORD_S_TWORD,

	/* des:X_BURST  src:X_BURST */
	DMAXFER_D_BBYTE_S_BBYTE,
	DMAXFER_D_BBYTE_S_BHALF,
	DMAXFER_D_BBYTE_S_BWORD,
	DMAXFER_D_BHALF_S_BBYTE,
	DMAXFER_D_BHALF_S_BHALF,
	DMAXFER_D_BHALF_S_BWORD,
	DMAXFER_D_BWORD_S_BBYTE,
	DMAXFER_D_BWORD_S_BHALF,
	DMAXFER_D_BWORD_S_BWORD,

	/* des:X_BURST   src:X_SIGLE */
	DMAXFER_D_BBYTE_S_SBYTE,
	DMAXFER_D_BBYTE_S_SHALF,
	DMAXFER_D_BBYTE_S_SWORD,
	DMAXFER_D_BHALF_S_SBYTE,
	DMAXFER_D_BHALF_S_SHALF,
	DMAXFER_D_BHALF_S_SWORD,
	DMAXFER_D_BWORD_S_SBYTE,
	DMAXFER_D_BWORD_S_SHALF,
	DMAXFER_D_BWORD_S_SWORD,

	/* des:X_BURST   src:X_TIPPL */
	DMAXFER_D_BBYTE_S_TBYTE,
	DMAXFER_D_BBYTE_S_THALF,
	DMAXFER_D_BBYTE_S_TWORD,
	DMAXFER_D_BHALF_S_TBYTE,
	DMAXFER_D_BHALF_S_THALF,
	DMAXFER_D_BHALF_S_TWORD,
	DMAXFER_D_BWORD_S_TBYTE,
	DMAXFER_D_BWORD_S_THALF,
	DMAXFER_D_BWORD_S_TWORD,

	/* des:X_TIPPL   src:X_TIPPL */
	DMAXFER_D_TBYTE_S_TBYTE,
	DMAXFER_D_TBYTE_S_THALF,
	DMAXFER_D_TBYTE_S_TWORD,
	DMAXFER_D_THALF_S_TBYTE,
	DMAXFER_D_THALF_S_THALF,
	DMAXFER_D_THALF_S_TWORD,
	DMAXFER_D_TWORD_S_TBYTE,
	DMAXFER_D_TWORD_S_THALF,
	DMAXFER_D_TWORD_S_TWORD,

	/* des:X_TIPPL   src:X_SIGLE */
	DMAXFER_D_TBYTE_S_SBYTE,
	DMAXFER_D_TBYTE_S_SHALF,
	DMAXFER_D_TBYTE_S_SWORD,
	DMAXFER_D_THALF_S_SBYTE,
	DMAXFER_D_THALF_S_SHALF,
	DMAXFER_D_THALF_S_SWORD,
	DMAXFER_D_TWORD_S_SBYTE,
	DMAXFER_D_TWORD_S_SHALF,
	DMAXFER_D_TWORD_S_SWORD,

	/* des:X_TIPPL   src:X_BURST */
	DMAXFER_D_TBYTE_S_BBYTE,
	DMAXFER_D_TBYTE_S_BHALF,
	DMAXFER_D_TBYTE_S_BWORD,
	DMAXFER_D_THALF_S_BBYTE,
	DMAXFER_D_THALF_S_BHALF,
	DMAXFER_D_THALF_S_BWORD,
	DMAXFER_D_TWORD_S_BBYTE,
	DMAXFER_D_TWORD_S_BHALF,
	DMAXFER_D_TWORD_S_BWORD,
	DMAXFER_MAX
};

/*
 * src/dst address type
 * index for addrtype_arr[]
 */
enum addrt_e {
	DMAADDRT_D_LN_S_LN,
	DMAADDRT_D_LN_S_IO,
	DMAADDRT_D_IO_S_LN,
	DMAADDRT_D_IO_S_IO,
	DMAADDRT_MAX
};

typedef struct tag_dma_config {
	/*
	 * data length and burst length combination in DDMA and NDMA
	 * eg: DMAXFER_D_SWORD_S_SWORD, DMAXFER_D_SBYTE_S_BBYTE
	 */
	enum xferunit_e	xfer_type;
	/*
	 * NDMA/DDMA src/dst address type
	 * eg: DMAADDRT_D_INC_S_INC(NDMA addr type),
	 *     DMAADDRT_D_LN_S_LN / DMAADDRT_D_LN_S_IO(DDMA addr type)
	 */
	enum addrt_e	address_type;
	u32		para;		/* dma para reg */
	u32 		irq_spt;	/* channel irq supported, eg: CHAN_IRQ_HD | CHAN_IRQ_FD */
	bool		bconti_mode;	/* continue mode */
	u8		src_drq_type;	/* src drq type */
	u8		dst_drq_type;	/* dst drq type */
}dma_config_t;

/* dma export symbol */
dma_hdl_t sw_dma_request(char * name, dma_chan_type_e type);
u32 sw_dma_release(dma_hdl_t dma_hdl);
u32 sw_dma_enqueue(dma_hdl_t dma_hdl, u32 src_addr, u32 dst_addr, u32 byte_cnt);
u32 sw_dma_config(dma_hdl_t dma_hdl, dma_config_t *pcfg);
u32 sw_dma_ctl(dma_hdl_t dma_hdl, dma_op_type_e op, void *parg);
int sw_dma_getposition(dma_hdl_t dma_hdl, u32 *pSrc, u32 *pDst);
void sw_dma_dump_chan(dma_hdl_t dma_hdl);

#endif /* __SW_DMA_H */
