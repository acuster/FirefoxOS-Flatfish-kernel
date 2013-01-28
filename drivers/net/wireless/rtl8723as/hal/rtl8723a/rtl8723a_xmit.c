/******************************************************************************
 *
 * Copyright(c) 2007 - 2012 Realtek Corporation. All rights reserved.
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
#define _RTL8723A_XMIT_C_

#include <drv_conf.h>
#include <osdep_service.h>
#include <drv_types.h>
#include <rtl8723a_hal.h>

#ifdef CONFIG_XMIT_ACK
void dump_txrpt_ccx_8723a(void *buf)
{
	struct txrpt_ccx_8723a *txrpt_ccx = buf;

	DBG_871X("tag1:%u, int_bt:%u, int_tri:%u, int_ccx:%u, mac_id:%u, pkt_drop:%u, pkt_ok:%u, bmc:%u\n"
		"retry_count:%u, lifetime_over:%u, retry_over:%u, final_data_rate:%u, qsel:%u, sw:%u\n",
		txrpt_ccx->tag1, txrpt_ccx->int_bt, txrpt_ccx->int_tri, txrpt_ccx->int_ccx, txrpt_ccx->mac_id, txrpt_ccx->pkt_drop, txrpt_ccx->pkt_ok, txrpt_ccx->bmc,
		txrpt_ccx->retry_count, txrpt_ccx->lifetime_over, txrpt_ccx->retry_over, txrpt_ccx->final_data_rate, txrpt_ccx->qsel, txrpt_ccx_8723a_sw(txrpt_ccx)
	);
}

void handle_txrpt_ccx_8723a(_adapter *adapter, void *buf)
{
	struct txrpt_ccx_8723a *txrpt_ccx = buf;

	//dump_txrpt_ccx_8723a(buf);

	if (txrpt_ccx->int_ccx) {
		if (txrpt_ccx->pkt_ok)
			rtw_ack_tx_done(&adapter->xmitpriv, RTW_SCTX_DONE_SUCCESS);
		else
			rtw_ack_tx_done(&adapter->xmitpriv, RTW_SCTX_DONE_CCX_PKT_FAIL);
	}
}
#endif //CONFIG_XMIT_ACK
