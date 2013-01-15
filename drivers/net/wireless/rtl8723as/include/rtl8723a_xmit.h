/******************************************************************************
 *
 * Copyright(c) 2007 - 2011 Realtek Corporation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110, USA
 *
 *
 ******************************************************************************/
#ifndef __RTL8723A_XMIT_H__
#define __RTL8723A_XMIT_H__

#include <rtl8192c_xmit.h>

//
//defined for TX DESC Operation
//

#define MAX_TID (15)

//OFFSET 0
#define OFFSET_SZ	0
#define OFFSET_SHT	16
#define BMC		BIT(24)
#define LSG		BIT(26)
#define FSG		BIT(27)
#define OWN 		BIT(31)


//OFFSET 4
#define PKT_OFFSET_SZ	0
#define BK		BIT(6)
#define QSEL_SHT	8
#define Rate_ID_SHT	16
#define NAVUSEHDR	BIT(20)
#define PKT_OFFSET_SHT	26
#define HWPC		BIT(31)

//OFFSET 8
#define AGG_EN		BIT(29)

//OFFSET 12
#define SEQ_SHT		16

//OFFSET 16
#define QoS		BIT(6)
#define HW_SEQ_EN	BIT(7)
#define USERATE		BIT(8)
#define DISDATAFB	BIT(10)
#define DATA_SHORT	BIT(24)
#define DATA_BW		BIT(25)

//OFFSET 20
#define SGI		BIT(6)

struct txrpt_ccx_8723a {
	/* offset 0 */
	u8 tag1:1;
	u8 rsv:4;
	u8 int_bt:1;
	u8 int_tri:1;
	u8 int_ccx:1;

	/* offset 1 */
	u8 mac_id:5;
	u8 pkt_drop:1;
	u8 pkt_ok:1;
	u8 bmc:1;

	/* offset 2 */
	u8 retry_count:6;
	u8 lifetime_over:1;
	u8 retry_over:1;

	/* offset 3 */
	u8 ccx_qtime_0;
	u8 ccx_qtime_1;

	/* offset 5 */
	u8 final_data_rate;

	/* offset 6 */
	u8 sw1:4;
	u8 qsel:4;

	/* offset 7 */
	u8 sw0;
};

#define txrpt_ccx_8723a_sw(txrpt_ccx) ((txrpt_ccx)->sw0 + ((txrpt_ccx)->sw1<<8))

#ifdef CONFIG_XMIT_ACK
void dump_txrpt_ccx_8723a(void *buf);
void handle_txrpt_ccx_8723a(_adapter *adapter, void *buf);
#else
#define dump_txrpt_ccx_8723a(buf) do {} while(0)
#define handle_txrpt_ccx_8723a(adapter, buf) do {} while(0)
#endif //CONFIG_XMIT_ACK

void rtl8723a_update_txdesc(struct xmit_frame *pxmitframe, u8 *pmem);
void rtl8723a_fill_fake_txdesc(PADAPTER padapter, u8 *pDesc, u32 BufferLen, u8 IsPsPoll, u8 IsBTQosNull);

#if defined(CONFIG_SDIO_HCI) || defined(CONFIG_GSPI_HCI)
s32 rtl8723as_init_xmit_priv(PADAPTER padapter);
void rtl8723as_free_xmit_priv(PADAPTER padapter);
s32 rtl8723as_hal_xmit(PADAPTER padapter, struct xmit_frame *pxmitframe);
s32 rtl8723as_mgnt_xmit(PADAPTER padapter, struct xmit_frame *pmgntframe);
s32 rtl8723as_xmit_buf_handler(PADAPTER padapter);
thread_return rtl8723as_xmit_thread(thread_context context);
#define hal_xmit_handler rtl8723as_xmit_buf_handler
#endif

#ifdef CONFIG_USB_HCI
s32 rtl8723au_xmit_buf_handler(PADAPTER padapter);
#define hal_xmit_handler rtl8723au_xmit_buf_handler
#endif
#endif

