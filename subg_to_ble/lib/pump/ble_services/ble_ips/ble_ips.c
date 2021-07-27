/**
 *@file ble_ips.c
 *@author Ribin Huang (you@domain.com)
 *@brief 
 *@version 1.0
 *@date 2021-01-05
 *
 *Copyright (c) 2019 - 2020 Fractal Auto Technology Co.,Ltd.
 *All right reserved.
 *
 *This program is free software; you can redistribute it and/or modify
 *it under the terms of the GNU General Public License version 2 as
 *published by the Free Software Foundation.
 *
 */
#include "sdk_common.h"
#include "ble.h"
#include "ble_ips.h"
#include "ble_srv_common.h"
#include "app_timer.h"
#include "kit_log.h"

#define BLE_TMR_TICK_ONE_MIN    60000

#define BLE_UUID_DATA_CHAR 		0xE849               /**< The UUID of the Characteristic. */
#define BLE_UUID_RES_CNT_CHAR 	0x7910               /**< The UUID of the Characteristic. */
#define BLE_UUID_TMR_TICK_CHAR 	0x7910               /**< The UUID of the Characteristic. */
#define BLE_UUID_CUS_NAME_CHAR 	0x2AF0               /**< The UUID of the Characteristic. */
#define BLE_UUID_FW_VER_CHAR 	0x9DC9               /**< The UUID of the Characteristic. */
#define BLE_UUID_LED_MODE_CHAR 	0x4241               /**< The UUID of the Characteristic. */

#define IPS_BASE_UUID 			{{0x45, 0x38, 0x2A, 0x9C, 0x21, 0x69, 0x56, 0xB8, 0x97, 0x41, 0xc5, 0x99, 0x3B, 0x73, 0x35, 0x02}}//0235733b-99c5-4197-b856-69219c2a3845
#define IPS_CHAR_DATA_UUID 		{{0x55, 0x91, 0xDA, 0xDA, 0x6A, 0x01, 0x7C, 0x86, 0xE2, 0x42, 0x28, 0x50, 0x49, 0xE8, 0x42, 0xC8}}//c842e849-5028-42e2-867c-016adada9155
#define IPS_CHAR_RES_CNT_UUID 	{{0x4A, 0x1F, 0xB8, 0xE2, 0xC5, 0x50, 0xFE, 0xA0, 0xA5, 0x43, 0x9E, 0xB8, 0x10, 0x79, 0x6C, 0x6E}}//6e6c7910-b89e-43a5-a0fe-50c5e2b81f4a
#define IPS_CHAR_TMR_TICK_UUID 	{{0x7E, 0x6F, 0xB8, 0xE2, 0xC5, 0x50, 0xAF, 0x78, 0xA5, 0x43, 0x9E, 0xB8, 0x10, 0x79, 0x6C, 0x6E}}//6e6c7910-b89e-43a5-78af-50c5e2b86f7e
#define IPS_CHAR_CUS_NAME_UUID 	{{0x66, 0x9A, 0x0C, 0x20, 0x00, 0x08, 0x21, 0x8C, 0xE4, 0x11, 0x28, 0x1E, 0xF0, 0x2A, 0x3B, 0xD9}}//d93b2af0-1e28-11e4-8c21-0800200c9a66
#define IPS_CHAR_FW_VER_UUID 	{{0xF2, 0x8C, 0x23, 0x4D, 0x10, 0x0A, 0x51, 0xA0, 0x95, 0x42, 0x91, 0x7C, 0xC9, 0x9D, 0xD9, 0x30}}//30d99dc9-7c91-4295-a051-0a104d238cf2
#define IPS_CHAR_LED_MODE_UUID 	{{0x4E, 0xF1, 0x32, 0x67, 0xE1, 0xFC, 0x5F, 0xA2, 0x9C, 0x4F, 0xA7, 0xF1, 0x41, 0x42, 0xD8, 0xC6}}//c6d84241-f1a7-4f9c-a25f-fce16732f14e

#define DATA_CHAR_DESC_NAME 		"Data"
#define RES_CNT_CHAR_DESC_NAME 		"Response Count"
#define TMR_TICK_CHAR_DESC_NAME		"Timer Tick"
#define CUS_NAME_CHAR_DESC_NAME 	"Custom Name"
#define FW_VER_CHAR_DESC_NAME		"Version"
#define LED_MODE_CHAR_DESC_NAME		"LED Mode"

#define FW_VER_CHAR_VALUE		  	"ble_rfspy 2.0"

#define TAG "IPS"

APP_TIMER_DEF(timerTickTimer);

/**@brief Function for setting security requirements of a characteristic.
 *
 * @param[in]  level   required security level.
 * @param[out] p_perm  Characteristic security requirements.
 *
 * @return     encoded security level and security mode.
 */
static void user_set_security_req(security_req_t level, ble_gap_conn_sec_mode_t * p_perm)
{
    BLE_GAP_CONN_SEC_MODE_SET_NO_ACCESS(p_perm);
    switch (level)
    {
        case SEC_NO_ACCESS:
            BLE_GAP_CONN_SEC_MODE_SET_NO_ACCESS(p_perm);
        break;
        case SEC_OPEN:
            BLE_GAP_CONN_SEC_MODE_SET_OPEN(p_perm);
        break;
        case SEC_JUST_WORKS:
            BLE_GAP_CONN_SEC_MODE_SET_ENC_NO_MITM(p_perm);
        break;
        case SEC_MITM:
            BLE_GAP_CONN_SEC_MODE_SET_ENC_WITH_MITM(p_perm);
        break;
        case SEC_SIGNED:
            BLE_GAP_CONN_SEC_MODE_SET_SIGNED_NO_MITM(p_perm);
        break;
        case SEC_SIGNED_MITM:
            BLE_GAP_CONN_SEC_MODE_SET_SIGNED_WITH_MITM(p_perm);
        break;
    }
    return;
}

static uint32_t user_128bit_uuid_characteristic_add(uint16_t service_handle,
                            ble_add_char_params_t *p_char_props,
                            ble_gatts_char_handles_t *p_char_handle,
                            ble_uuid128_t *user_128bit_uuid
                            )
{
    ble_gatts_char_md_t char_md;
    ble_gatts_attr_t    attr_char_value;
    ble_uuid_t          char_uuid;
    ble_gatts_attr_md_t attr_md;
    ble_gatts_attr_md_t user_descr_attr_md;
    ble_gatts_attr_md_t cccd_md;
    ret_code_t          err_code;
	
    err_code = sd_ble_uuid_vs_add(user_128bit_uuid, &char_uuid.type);
    VERIFY_SUCCESS(err_code);
    char_uuid.uuid = p_char_props->uuid;

    memset(&attr_md, 0, sizeof(ble_gatts_attr_md_t));
    user_set_security_req(p_char_props->read_access, &attr_md.read_perm);
    user_set_security_req(p_char_props->write_access, & attr_md.write_perm);
    attr_md.rd_auth    = (p_char_props->is_defered_read ? 1 : 0);
    attr_md.wr_auth    = (p_char_props->is_defered_write ? 1 : 0);
    attr_md.vlen       = (p_char_props->is_var_len ? 1 : 0);
    attr_md.vloc       = (p_char_props->is_value_user ? BLE_GATTS_VLOC_USER : BLE_GATTS_VLOC_STACK);

    memset(&char_md, 0, sizeof(ble_gatts_char_md_t));
    if ((p_char_props->char_props.notify == 1)||(p_char_props->char_props.indicate == 1))
    {
        memset(&cccd_md, 0, sizeof(cccd_md));
        user_set_security_req(p_char_props->cccd_write_access, &cccd_md.write_perm);
        BLE_GAP_CONN_SEC_MODE_SET_OPEN(&cccd_md.read_perm);
        cccd_md.vloc       = BLE_GATTS_VLOC_STACK;
        char_md.p_cccd_md  = &cccd_md;
    }
    char_md.char_props     = p_char_props->char_props;
    char_md.char_ext_props = p_char_props->char_ext_props;

    memset(&attr_char_value, 0, sizeof(ble_gatts_attr_t));
    attr_char_value.p_uuid    = &char_uuid;
    attr_char_value.p_attr_md = &attr_md;
    attr_char_value.max_len   = p_char_props->max_len;
    if (p_char_props->p_init_value != NULL)
    {
        attr_char_value.init_len  = p_char_props->init_len;
        attr_char_value.p_value   = p_char_props->p_init_value;
    }
		
    if (p_char_props->p_user_descr != NULL)
    {
        memset(&user_descr_attr_md, 0, sizeof(ble_gatts_attr_md_t));
        char_md.char_user_desc_max_size = p_char_props->p_user_descr->max_size;
        char_md.char_user_desc_size     = p_char_props->p_user_descr->size;
        char_md.p_char_user_desc        = p_char_props->p_user_descr->p_char_user_desc;
        char_md.p_user_desc_md          = &user_descr_attr_md;
        user_set_security_req(p_char_props->p_user_descr->read_access, &user_descr_attr_md.read_perm);
        user_set_security_req(p_char_props->p_user_descr->write_access, &user_descr_attr_md.write_perm);
        user_descr_attr_md.rd_auth      = (p_char_props->p_user_descr->is_defered_read ? 1 : 0);
        user_descr_attr_md.wr_auth      = (p_char_props->p_user_descr->is_defered_write ? 1 : 0);
        user_descr_attr_md.vlen         = (p_char_props->p_user_descr->is_var_len ? 1 : 0);
        user_descr_attr_md.vloc         = (p_char_props->p_user_descr->is_value_user ? BLE_GATTS_VLOC_USER : BLE_GATTS_VLOC_STACK);
    }
	
    if (p_char_props->p_presentation_format != NULL)
    {
        char_md.p_char_pf = p_char_props->p_presentation_format;
    }
	
    return sd_ble_gatts_characteristic_add(service_handle,
                                           &char_md,
                                           &attr_char_value,
                                           p_char_handle);
}

/**@brief Function for handling the @ref BLE_GAP_EVT_CONNECTED event from the SoftDevice.
 *
 * @param[in] p_ips     Nordic UART Service structure.
 * @param[in] p_ble_evt Pointer to the event received from BLE stack.
 */
static void on_connect(ble_ips_t * p_ips, ble_evt_t const * p_ble_evt)
{
    ret_code_t                 err_code;
    ble_gatts_value_t          gatts_val;
    uint8_t                    cccd_value[2];
    ble_ips_client_context_t * p_client = NULL;
	
    p_ips->conn_handle = p_ble_evt->evt.gatts_evt.conn_handle;

    err_code = blcm_link_ctx_get(p_ips->p_link_ctx_storage,
                                 p_ble_evt->evt.gap_evt.conn_handle,
                                 (void *) &p_client);
    if (err_code != NRF_SUCCESS)
    {
        KIT_LOG(TAG, "Link context for 0x%02X connection handle could not be fetched.",
                      p_ble_evt->evt.gap_evt.conn_handle);
    }

    /* Check the hosts CCCD value to inform of readiness to send data using the RX characteristic */
    memset(&gatts_val, 0, sizeof(ble_gatts_value_t));
    gatts_val.p_value = cccd_value;
    gatts_val.len     = sizeof(cccd_value);
    gatts_val.offset  = 0;

    err_code = sd_ble_gatts_value_get(p_ble_evt->evt.gap_evt.conn_handle,
                                      p_ips->res_cnt_char_handles.cccd_handle,
                                      &gatts_val);
    if ((err_code == NRF_SUCCESS)     &&
        (p_ips->data_handler != NULL) &&
        ble_srv_is_notification_enabled(gatts_val.p_value))
    {
        if (p_client != NULL)
        {
            p_client->is_res_cnt_notification_enabled = true;
			KIT_LOG(TAG, "Connect: subscribe for resp cnt.");
        }
    }
	
    err_code = sd_ble_gatts_value_get(p_ble_evt->evt.gap_evt.conn_handle,
                                      p_ips->tmr_tick_char_handles.cccd_handle,
                                      &gatts_val);
    if ((err_code == NRF_SUCCESS)     &&
        (p_ips->data_handler != NULL) &&
        ble_srv_is_notification_enabled(gatts_val.p_value))
    {
        if (p_client != NULL)
        {
            p_client->is_tmr_tick_notification_enabled = true;
			KIT_LOG(TAG, "Connect: subscribe for timer tick.");
        }
    }
}

/**@brief Function for handling the Disconnect event.
 *
 * @param[in]   p_bps       Blood Pressure Service structure.
 * @param[in]   p_ble_evt   Event received from the BLE stack.
 */
static void on_disconnect(ble_ips_t * p_ips, ble_evt_t const * p_ble_evt)
{
    UNUSED_PARAMETER(p_ble_evt);
    p_ips->conn_handle = BLE_CONN_HANDLE_INVALID;
}

/**@brief Function for handling the @ref BLE_GATTS_EVT_WRITE event from the SoftDevice.
 *
 * @param[in] p_ips     Nordic UART Service structure.
 * @param[in] p_ble_evt Pointer to the event received from BLE stack.
 */
static void on_write(ble_ips_t * p_ips, ble_evt_t const * p_ble_evt)
{
    ret_code_t                    err_code;
    ble_ips_evt_t                 evt;
    ble_ips_client_context_t    * p_client;
    ble_gatts_evt_write_t const * p_evt_write = &p_ble_evt->evt.gatts_evt.params.write;

    err_code = blcm_link_ctx_get(p_ips->p_link_ctx_storage,
                                 p_ble_evt->evt.gatts_evt.conn_handle,
                                 (void *) &p_client);
    if (err_code != NRF_SUCCESS)
    {
        KIT_LOG(TAG, "Link context for 0x%02X connection handle could not be fetched.",
                      p_ble_evt->evt.gatts_evt.conn_handle);
    }

    memset(&evt, 0, sizeof(ble_ips_evt_t));
    evt.p_ips       = p_ips;
    evt.conn_handle = p_ble_evt->evt.gatts_evt.conn_handle;
    evt.p_link_ctx  = p_client;

    if ((p_evt_write->handle == p_ips->res_cnt_char_handles.cccd_handle) &&
        (p_evt_write->len == 2))
    {
        if (p_client != NULL)
        {
            if (ble_srv_is_notification_enabled(p_evt_write->data))
            {
                p_client->is_res_cnt_notification_enabled = true;
				KIT_LOG(TAG, "Write: subscribe for resp cnt.");
            }
            else
            {
                p_client->is_res_cnt_notification_enabled = false;
				KIT_LOG(TAG, "Write: unsubscribe for resp cnt.");
            }
        }
    }
    else if ((p_evt_write->handle == p_ips->tmr_tick_char_handles.cccd_handle) &&
        (p_evt_write->len == 2))
    {
        if (p_client != NULL)
        {
            if (ble_srv_is_notification_enabled(p_evt_write->data))
            {
                p_client->is_tmr_tick_notification_enabled = true;
				KIT_LOG(TAG, "Write: subscribe for timer tick.");
            }
            else
            {
                p_client->is_tmr_tick_notification_enabled = false;
				KIT_LOG(TAG, "Write: unsubscribe for timer tick.");
            }
        }
    }
    else if ((p_evt_write->handle == p_ips->data_char_handles.value_handle) &&
             (p_ips->data_handler != NULL))
    {
        evt.type                  = BLE_IPS_EVT_DATA_RX;
        evt.params.rx_data.p_data = p_evt_write->data;
        evt.params.rx_data.length = p_evt_write->len;
        p_ips->data_handler(&evt);
    }
    else if ((p_evt_write->handle == p_ips->cus_name_char_handles.value_handle) &&
             (p_ips->data_handler != NULL))
    {
        evt.type                  = BLE_IPS_EVT_CUS_NAME_RX;
        evt.params.rx_data.p_data = p_evt_write->data;
        evt.params.rx_data.length = p_evt_write->len;
        p_ips->data_handler(&evt);
    }   
	else if ((p_evt_write->handle == p_ips->led_mode_char_handles.value_handle) &&
             (p_ips->data_handler != NULL))
    {
        evt.type                  = BLE_IPS_EVT_LED_MODE_RX;
        evt.params.rx_data.p_data = p_evt_write->data;
        evt.params.rx_data.length = p_evt_write->len;
        p_ips->data_handler(&evt);
    }
    else
    {
        // Do Nothing. This event is not relevant for this service.
    }
}


/**@brief Function for handling the @ref BLE_GATTS_EVT_HVN_TX_COMPLETE event from the SoftDevice.
 *
 * @param[in] p_ips     Nordic UART Service structure.
 * @param[in] p_ble_evt Pointer to the event received from BLE stack.
 */
static void on_hvx_tx_complete(ble_ips_t * p_ips, ble_evt_t const * p_ble_evt)
{
    /*ret_code_t                 err_code;
    ble_ips_evt_t              evt;
    ble_ips_client_context_t * p_client;

    err_code = blcm_link_ctx_get(p_ips->p_link_ctx_storage,
                                 p_ble_evt->evt.gatts_evt.conn_handle,
                                 (void *) &p_client);
    if (err_code != NRF_SUCCESS)
    {
        NRF_LOG_ERROR("Link context for 0x%02X connection handle could not be fetched.",
                      p_ble_evt->evt.gatts_evt.conn_handle);
        return;
    }

    if (p_client->is_notification_enabled)
    {
        memset(&evt, 0, sizeof(ble_ips_evt_t));
        evt.type        = BLE_IPS_EVT_TX_RDY;
        evt.p_ips       = p_ips;
        evt.conn_handle = p_ble_evt->evt.gatts_evt.conn_handle;
        evt.p_link_ctx  = p_client;

        p_ips->data_handler(&evt);
    }*/
}

void ble_ips_on_ble_evt(ble_evt_t const * p_ble_evt, void * p_context)
{
    if ((p_context == NULL) || (p_ble_evt == NULL))
    {
        return;
    }

    ble_ips_t * p_ips = (ble_ips_t *)p_context;

    switch (p_ble_evt->header.evt_id)
    {
        case BLE_GAP_EVT_CONNECTED:
            on_connect(p_ips, p_ble_evt);
            break;
			
		case BLE_GAP_EVT_DISCONNECTED:
			on_disconnect(p_ips, p_ble_evt);
			break;

        case BLE_GATTS_EVT_WRITE:
            on_write(p_ips, p_ble_evt);
            break;

        case BLE_GATTS_EVT_HVN_TX_COMPLETE:
            on_hvx_tx_complete(p_ips, p_ble_evt);
            break;

        default:
            // No implementation needed.
            break;
    }
}

static void ble_ips_timer_tick_handle(void *pContext)
{
    ret_code_t 					err_code = NRF_SUCCESS;
    ble_gatts_hvx_params_t 		hvx_params;
    ble_ips_client_context_t 	*p_client;
    ble_ips_t 					*p_ips = (ble_ips_t *)pContext;
    ble_gatts_value_t           gatts_value;
	uint16_t 					hvx_len;
	
	p_ips->timer_tick = p_ips->timer_tick + 1;
	if(p_ips->timer_tick >= 0xff)
	{
		p_ips->timer_tick = 0;
	}
    blcm_link_ctx_get(p_ips->p_link_ctx_storage, p_ips->conn_handle, (void *) &p_client);
	
    if ((p_ips->conn_handle == BLE_CONN_HANDLE_INVALID) || (p_client == NULL))
    {
		KIT_LOG(TAG, "Timer tick, not connected!");
        return;
    }

    if (!p_client->is_tmr_tick_notification_enabled)
    {
		KIT_LOG(TAG, "Timer tick, not subscribed!");
        return;
    }

	//set timer tick gatt value for reading by client
	// Initialize value struct.
	memset(&gatts_value, 0, sizeof(gatts_value));
	gatts_value.len 	= sizeof(p_ips->timer_tick);
	gatts_value.offset	= 0;
	gatts_value.p_value = &p_ips->timer_tick;
	// Update database.
	err_code = sd_ble_gatts_value_set(BLE_CONN_HANDLE_INVALID,
									  p_ips->tmr_tick_char_handles.value_handle,
									  &gatts_value);
	if (err_code != NRF_SUCCESS)
	{
		KIT_LOG(TAG, "Timer tick, set value err: 0x%02x!", err_code);
        return;
	}

    memset(&hvx_params, 0, sizeof(hvx_params));
	
	hvx_len 		  = sizeof(p_ips->timer_tick);
    hvx_params.handle = p_ips->tmr_tick_char_handles.value_handle;
    hvx_params.p_data = &p_ips->timer_tick;
    hvx_params.p_len  = &hvx_len;
    hvx_params.type   = BLE_GATT_HVX_NOTIFICATION;

    err_code = sd_ble_gatts_hvx(p_ips->conn_handle, &hvx_params);
	
	if (err_code != NRF_SUCCESS)
	{
		KIT_LOG(TAG, "Timer tick, notify failed: 0x%02x!", err_code);
		return;
	}
	KIT_LOG(TAG, "Timer tick, notify OK: 0x%02x!", p_ips->timer_tick);
}

uint32_t ble_ips_response_cnt_notify(ble_ips_t *p_ips) 
{
    ret_code_t         			err_code = NRF_SUCCESS;
	ble_gatts_hvx_params_t	   	hvx_params;
	ble_ips_client_context_t * 	p_client;
    ble_gatts_value_t           gatts_value;
	uint16_t 					hvx_len;
	
	p_ips->response_count = p_ips->response_count + 1;
	if(p_ips->response_count >= 0xff)
	{
		p_ips->response_count = 0;
	}
	blcm_link_ctx_get(p_ips->p_link_ctx_storage, p_ips->conn_handle, (void *) &p_client);
	
    if ((p_ips->conn_handle == BLE_CONN_HANDLE_INVALID) || (p_client == NULL))
    {
		KIT_LOG(TAG, "Resp cnt, not connected!");
        return NRF_ERROR_NOT_FOUND;
    }

    if (!p_client->is_res_cnt_notification_enabled)
    {
		KIT_LOG(TAG, "Resp cnt, not subscribed!");
        return NRF_ERROR_INVALID_STATE;
    }

	//set response count gatt value for reading by client
	// Initialize value struct.
	memset(&gatts_value, 0, sizeof(gatts_value));
	gatts_value.len 	= sizeof(p_ips->response_count);
	gatts_value.offset	= 0;
	gatts_value.p_value = &p_ips->response_count;
	// Update database.
	err_code = sd_ble_gatts_value_set(BLE_CONN_HANDLE_INVALID,
									  p_ips->res_cnt_char_handles.value_handle,
									  &gatts_value);
	if (err_code != NRF_SUCCESS)
	{
		KIT_LOG(TAG, "Resp cnt, set value err: 0x%02x!", err_code);
        return err_code;
	}

	memset(&hvx_params, 0, sizeof(hvx_params));
	
	hvx_len = sizeof(p_ips->response_count);
	hvx_params.handle = p_ips->res_cnt_char_handles.value_handle;
	hvx_params.p_data = &p_ips->response_count;
	hvx_params.p_len  = &hvx_len;
	hvx_params.type   = BLE_GATT_HVX_NOTIFICATION;

	err_code = sd_ble_gatts_hvx(p_ips->conn_handle, &hvx_params);
	
	if (err_code != NRF_SUCCESS)
	{
		KIT_LOG(TAG, "Resp cnt, notify failed: 0x%02x!", err_code);
		return err_code;
	}
	KIT_LOG(TAG, "Respo cnt, notify OK: 0x%02x!", p_ips->response_count);

	return err_code;
}

uint32_t ble_ips_data_send(uint8_t *buf, int count, ble_ips_t *p_ips) 
{
    ret_code_t         err_code = NRF_SUCCESS;
    ble_gatts_value_t  gatts_value;
	
    if (p_ips->conn_handle == BLE_CONN_HANDLE_INVALID)
    {
		KIT_LOG(TAG, "Data access, not connected!");
        return NRF_ERROR_NOT_FOUND;
    }

	// Initialize value struct.
	memset(&gatts_value, 0, sizeof(gatts_value));
	gatts_value.len 	= count;
	gatts_value.offset	= 0;
	gatts_value.p_value = buf;
	// Update database.
	err_code = sd_ble_gatts_value_set(p_ips->conn_handle,
									  p_ips->data_char_handles.value_handle,
									  &gatts_value);

    uint8_t  value_buf[BLE_IPS_MAX_DATA_CHAR_LEN];
    ble_gatts_value_t gatts_value1;

    // Initialize value struct.
    memset(&gatts_value1, 0, sizeof(gatts_value1));

    gatts_value1.len     = count;
    gatts_value1.offset  = 0;
    gatts_value1.p_value = value_buf;

    err_code = sd_ble_gatts_value_get(p_ips->conn_handle,
									  p_ips->data_char_handles.value_handle,
									  &gatts_value1);
	
	if (err_code != NRF_SUCCESS)
	{
		KIT_LOG(TAG, "Data access, notify failed: 0x%02x!", err_code);
		return err_code;
	}
	
	Kit_PrintBytes(TAG, "Data access, notify OK: ", (const uint8_t*)value_buf, count);
	
	return err_code;
}

uint32_t ble_ips_init(ble_ips_t * p_ips, ble_ips_init_t const * p_ips_init, uint8_t* p_cus_name, uint8_t cus_name_len)
{
    ret_code_t            err_code;
    ble_uuid_t            ble_uuid;
    ble_uuid128_t         ips_base_uuid 			= IPS_BASE_UUID;
    ble_uuid128_t         ips_char_data_uuid 		= IPS_CHAR_DATA_UUID;
    ble_uuid128_t         ips_char_res_cnt_uuid 	= IPS_CHAR_RES_CNT_UUID;
    ble_uuid128_t         ips_char_tmr_tick_uuid 	= IPS_CHAR_TMR_TICK_UUID;
    ble_uuid128_t         ips_char_cus_name_uuid 	= IPS_CHAR_CUS_NAME_UUID;
    ble_uuid128_t         ips_char_fw_ver_uuid 		= IPS_CHAR_FW_VER_UUID;
    ble_uuid128_t         ips_char_led_mode_uuid 	= IPS_CHAR_LED_MODE_UUID;
    ble_add_char_params_t add_char_params;
	ble_add_char_user_desc_t add_char_user_desc;

    VERIFY_PARAM_NOT_NULL(p_ips);
    VERIFY_PARAM_NOT_NULL(p_ips_init);
	
    // Initialize the service structure.
    p_ips->data_handler = p_ips_init->data_handler;
    p_ips->conn_handle = BLE_CONN_HANDLE_INVALID;
    p_ips->response_count = 0;
    p_ips->timer_tick = 0;

    /**@snippet [Adding proprietary Service to the SoftDevice] */
    // Add a custom base UUID.
    err_code = sd_ble_uuid_vs_add(&ips_base_uuid, &p_ips->uuid_type);
    VERIFY_SUCCESS(err_code);

    ble_uuid.type = p_ips->uuid_type;
    ble_uuid.uuid = BLE_UUID_IPS_SERVICE;

    // Add the service.
    err_code = sd_ble_gatts_service_add(BLE_GATTS_SRVC_TYPE_PRIMARY,
                                        &ble_uuid,
                                        &p_ips->service_handle);
    /**@snippet [Adding proprietary Service to the SoftDevice] */
    VERIFY_SUCCESS(err_code);

    // Add the Data Characteristic.
    char data_desc[] = DATA_CHAR_DESC_NAME;
	uint8_t data_init_value = 0;
	
    memset(&add_char_params, 0, sizeof(add_char_params));
    memset(&add_char_user_desc, 0, sizeof(add_char_user_desc));
	add_char_params.p_user_descr = &add_char_user_desc;
    add_char_params.uuid                 = BLE_UUID_DATA_CHAR;
    add_char_params.max_len              = BLE_IPS_MAX_DATA_CHAR_LEN;
    add_char_params.init_len             = sizeof(uint8_t);
    add_char_params.p_init_value         = &data_init_value;
    add_char_params.is_var_len           = true;
    add_char_params.char_props.write     = 1;
    add_char_params.char_props.read      = 1;
    add_char_params.read_access          = SEC_OPEN;
    add_char_params.write_access         = SEC_OPEN;
	add_char_user_desc.p_char_user_desc  = (uint8_t *)data_desc;
	add_char_user_desc.size 			 = strlen(data_desc);
	add_char_user_desc.max_size 		 = strlen(data_desc);
	add_char_user_desc.read_access       = SEC_OPEN;

    err_code = user_128bit_uuid_characteristic_add(p_ips->service_handle, &add_char_params, &p_ips->data_char_handles, &ips_char_data_uuid);
    if (err_code != NRF_SUCCESS)
    {
        return err_code;
    }

    // Add the Response Count Characteristic.
    char res_cnt_desc[] = RES_CNT_CHAR_DESC_NAME;
	uint8_t res_cnt_init_value = 0;
	
    memset(&add_char_params, 0, sizeof(add_char_params));
    memset(&add_char_user_desc, 0, sizeof(add_char_user_desc));
	add_char_params.p_user_descr = &add_char_user_desc;
    add_char_params.uuid                 = BLE_UUID_RES_CNT_CHAR;
    add_char_params.max_len              = BLE_IPS_MAX_RES_CNT_CHAR_LEN;
    add_char_params.init_len             = BLE_IPS_MAX_RES_CNT_CHAR_LEN;
    add_char_params.p_init_value         = &res_cnt_init_value;
    add_char_params.char_props.notify    = 1;
    add_char_params.char_props.read      = 1;
	add_char_params.read_access          = SEC_OPEN;
    add_char_params.cccd_write_access    = SEC_OPEN;
	add_char_user_desc.p_char_user_desc  = (uint8_t *)res_cnt_desc;
	add_char_user_desc.size 			 = strlen(res_cnt_desc);
	add_char_user_desc.max_size 		 = strlen(res_cnt_desc);
	add_char_user_desc.read_access       = SEC_OPEN;

    err_code = user_128bit_uuid_characteristic_add(p_ips->service_handle, &add_char_params, &p_ips->res_cnt_char_handles, &ips_char_res_cnt_uuid);
    if (err_code != NRF_SUCCESS)
    {
        return err_code;
    }

    // Add the Timer Tick Characteristic.
    char tmr_tick_desc[] = TMR_TICK_CHAR_DESC_NAME;
	uint8_t tmr_tick_init_value = 0;
	
    memset(&add_char_params, 0, sizeof(add_char_params));
    memset(&add_char_user_desc, 0, sizeof(add_char_user_desc));
	add_char_params.p_user_descr = &add_char_user_desc;
    add_char_params.uuid                 = BLE_UUID_TMR_TICK_CHAR;
    add_char_params.max_len              = BLE_IPS_MAX_TMR_TICK_CHAR_LEN;
    add_char_params.init_len             = BLE_IPS_MAX_TMR_TICK_CHAR_LEN;
    add_char_params.p_init_value         = &tmr_tick_init_value;
    add_char_params.char_props.notify    = 1;
    add_char_params.char_props.read      = 1;
	add_char_params.read_access          = SEC_OPEN;
    add_char_params.cccd_write_access    = SEC_OPEN;
	add_char_user_desc.p_char_user_desc  = (uint8_t *)tmr_tick_desc;
	add_char_user_desc.size 			 = strlen(tmr_tick_desc);
	add_char_user_desc.max_size 		 = strlen(tmr_tick_desc);
	add_char_user_desc.read_access       = SEC_OPEN;

    err_code = user_128bit_uuid_characteristic_add(p_ips->service_handle, &add_char_params, &p_ips->tmr_tick_char_handles, &ips_char_tmr_tick_uuid);
    if (err_code != NRF_SUCCESS)
    {
        return err_code;
    }

    // Add the Custom Name Characteristic.
    char cus_name_desc[] = CUS_NAME_CHAR_DESC_NAME;
	
    memset(&add_char_params, 0, sizeof(add_char_params));
    memset(&add_char_user_desc, 0, sizeof(add_char_user_desc));
	add_char_params.p_user_descr = &add_char_user_desc;
    add_char_params.uuid                 = BLE_UUID_CUS_NAME_CHAR;
    add_char_params.max_len              = BLE_IPS_MAX_CUS_NAME_CHAR_LEN;
    add_char_params.init_len             = cus_name_len;
    add_char_params.p_init_value         = p_cus_name;
    add_char_params.is_var_len           = true;
    add_char_params.char_props.write     = 1;
    add_char_params.char_props.read      = 1;
	add_char_params.read_access          = SEC_OPEN;
    add_char_params.write_access         = SEC_OPEN;
	add_char_user_desc.p_char_user_desc  = (uint8_t *)cus_name_desc;
	add_char_user_desc.size 			 = strlen(cus_name_desc);
	add_char_user_desc.max_size 		 = strlen(cus_name_desc);
	add_char_user_desc.read_access       = SEC_OPEN;

    err_code = user_128bit_uuid_characteristic_add(p_ips->service_handle, &add_char_params, &p_ips->cus_name_char_handles, &ips_char_cus_name_uuid);
    if (err_code != NRF_SUCCESS)
    {
        return err_code;
    }

	// Add the Version Characteristic.
    char fw_ver_desc[] = FW_VER_CHAR_DESC_NAME;
	
    memset(&add_char_params, 0, sizeof(add_char_params));
    memset(&add_char_user_desc, 0, sizeof(add_char_user_desc));
	add_char_params.p_user_descr = &add_char_user_desc;
    add_char_params.uuid                 = BLE_UUID_FW_VER_CHAR;
    add_char_params.max_len              = strlen(FW_VER_CHAR_VALUE);
    add_char_params.init_len             = strlen(FW_VER_CHAR_VALUE);
    add_char_params.p_init_value         = (uint8_t*)FW_VER_CHAR_VALUE;
    add_char_params.char_props.read      = 1;
	add_char_params.read_access          = SEC_OPEN;
	add_char_user_desc.p_char_user_desc  = (uint8_t *)fw_ver_desc;
	add_char_user_desc.size 			 = strlen(fw_ver_desc);
	add_char_user_desc.max_size 		 = strlen(fw_ver_desc);
	add_char_user_desc.read_access       = SEC_OPEN;

    err_code = user_128bit_uuid_characteristic_add(p_ips->service_handle, &add_char_params, &p_ips->fw_ver_char_handles, &ips_char_fw_ver_uuid);
    if (err_code != NRF_SUCCESS)
    {
        return err_code;
    }
	
	// Add the Led Mode Characteristic.
    char led_mode_desc[] = LED_MODE_CHAR_DESC_NAME;
	uint8_t led_init_value = 0;
	
    memset(&add_char_params, 0, sizeof(add_char_params));
    memset(&add_char_user_desc, 0, sizeof(add_char_user_desc));
	add_char_params.p_user_descr = &add_char_user_desc;
    add_char_params.uuid                 = BLE_UUID_LED_MODE_CHAR;
    add_char_params.max_len              = BLE_IPS_MAX_LED_MODE_CHAR_LEN;
    add_char_params.init_len             = BLE_IPS_MAX_LED_MODE_CHAR_LEN;
    add_char_params.p_init_value         = &led_init_value;
    add_char_params.char_props.read      = 1;
    add_char_params.char_props.write     = 1;
	add_char_params.read_access  	     = SEC_OPEN;
	add_char_params.write_access		 = SEC_OPEN;
	add_char_user_desc.p_char_user_desc  = (uint8_t *)led_mode_desc;
	add_char_user_desc.size 			 = strlen(led_mode_desc);
	add_char_user_desc.max_size 		 = strlen(led_mode_desc);
	add_char_user_desc.read_access       = SEC_OPEN;

    err_code = user_128bit_uuid_characteristic_add(p_ips->service_handle, &add_char_params, &p_ips->led_mode_char_handles, &ips_char_led_mode_uuid);
    if (err_code != NRF_SUCCESS)
    {
        return err_code;
    }

	app_timer_create(&timerTickTimer, APP_TIMER_MODE_REPEATED, ble_ips_timer_tick_handle);
	app_timer_start(timerTickTimer, APP_TIMER_TICKS(BLE_TMR_TICK_ONE_MIN), p_ips);

	return NRF_SUCCESS;
}

