/**
 *@file app_ble.c
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
#include "app_error.h"
#include "app_timer.h"
#include "kit_log.h"
#include "app_battery.h"
#include "app_ble.h"
#include "ble.h"
#include "ble_gap.h"
#include "ble_hci.h"
#include "nrf_sdh.h"
#include "nrf_sdh_ble.h"
#include "nrf_sdh_soc.h"
#include "ble_dfu.h"
#include "ble_srv_common.h"
#include "ble_advdata.h"
#include "ble_advertising.h"
#include "ble_conn_params.h"
#include "ble_conn_state.h"
#include "nrf_svci_async_function.h"
#include "nrf_svci_async_handler.h"
#include "nrf_ble_gatt.h"
#include "nrf_ble_qwr.h"
#include "ble_ips.h"
#include "ble_bas.h"
#include "ble_nus.h"
#include "app_aps.h"
#include "app_subg.h"
#include "app_factory.h"
#include "ocp.h"
#include "app_indication.h"
#include "app_config.h"
#include "boards.h"

#define BLE_CONN_CFG_TAG        			1                            /**< Tag that refers to the BLE stack configuration set with @ref sd_ble_cfg_set. The default tag is @ref BLE_CONN_CFG_TAG_DEFAULT. */
#define BLE_EVT_OBSERVER_PRIO   			3                            /**< BLE observer priority of the application. There is no need to modify this value. */
#define BLE_SOC_OBSERVER_PRIO   			1                            /**< Applications' SoC observer priority. You shouldn't need to modify this value. */

#define BLE_IPS_SERVICE_UUID_TYPE       	BLE_UUID_TYPE_VENDOR_BEGIN                  /**< UUID type for the Nordic UART Service (vendor specific). */
#define BLE_ADV_INTERVAL                	480                                         /**< The advertising interval (in units of 0.625 ms. This value corresponds to 187.5 ms). */
#define BLE_ADV_DURATION                	0                                        	/**< The advertising duration (180 seconds) in units of 10 milliseconds. */

#define BLE_MIN_CONN_INTERVAL               MSEC_TO_UNITS(50, UNIT_1_25_MS)//80ms            /**< Minimum acceptable connection interval (0.1 seconds). */
#define BLE_MAX_CONN_INTERVAL               MSEC_TO_UNITS(100, UNIT_1_25_MS)//100ms           /**< Maximum acceptable connection interval (0.2 second). */
#define BLE_SLAVE_LATENCY                   0                                           /**< Slave latency. */
#define BLE_CONN_SUP_TIMEOUT                MSEC_TO_UNITS(5000, UNIT_10_MS)             /**< Connection supervisory timeout (4 seconds). */
#define BLE_FIRST_CONN_PARAMS_UPDATE_DELAY  APP_TIMER_TICKS(5000)                       /**< Time from initiating event (connect or start of notification) to first time sd_ble_gap_conn_param_update is called (5 seconds). */
#define BLE_NEXT_CONN_PARAMS_UPDATE_DELAY   APP_TIMER_TICKS(30000)                      /**< Time between each call to sd_ble_gap_conn_param_update after the first call (30 seconds). */
#define BLE_MAX_CONN_PARAMS_UPDATE_COUNT    3                                           /**< Number of attempts before giving up the connection parameter negotiation. */

#define BLE_BATT_UPDATE_INTERVAL     		APP_TIMER_TICKS(10000)                   	/**< Battery level measurement interval (ticks). */
#define BLE_TX_POWER_LEVEL                  (4)                                   		/**< TX Power Level value. This will be set both in the TX Power service, in the advertising data, and also used to set the radio transmit power. */

#define TAG 								"BLE"

#define DEAD_BEEF                       0xDEADBEEF                                  /**< Value used as error code on stack dump, can be used to identify stack location on stack unwind. */

BLE_NUS_DEF(m_nus, NRF_SDH_BLE_TOTAL_LINK_COUNT);                                   /**< BLE NUS service instance. */
BLE_IPS_DEF(m_ips, NRF_SDH_BLE_TOTAL_LINK_COUNT);		/**< BLE IPS service instance. */
NRF_BLE_GATT_DEF(m_gatt);							/**< GATT module instance. */
NRF_BLE_QWR_DEF(m_qwr);                             	/**< Context for the Queued Write module.*/
BLE_ADVERTISING_DEF(m_advertising);				/**< Advertising module instance. */
BLE_BAS_DEF(m_bas);									/**< Structure used to identify the battery service. */
APP_TIMER_DEF(m_battery_timer_id);                    /**< Battery timer. */

static uint16_t m_conn_handle = BLE_CONN_HANDLE_INVALID;	/**< Handle of the current connection. */
static ble_uuid_t m_adv_uuids[] =                           /**< Universally unique service identifier. */
{
    {BLE_UUID_IPS_SERVICE, BLE_IPS_SERVICE_UUID_TYPE}
};

static bool bleNameChangeFlg = false;
static eBleState_t bleState = BLE_STATE_ADV;

/**@brief Function for assert macro callback.
 *
 * @details This function will be called in case of an assert in the SoftDevice.
 *
 * @warning This handler is an example only and does not fit a final product. You need to analyse
 *          how your product is supposed to react in case of Assert.
 * @warning On assert from the SoftDevice, the system can only recover on reset.
 *
 * @param[in] line_num    Line number of the failing ASSERT call.
 * @param[in] p_file_name File name of the failing ASSERT call.
 */
void assert_nrf_callback(uint16_t line_num, const uint8_t * p_file_name)
{
    app_error_handler(DEAD_BEEF, line_num, p_file_name);
}

/**@brief Function for changing the tx power.
 */
static void tx_power_set(int8_t power)
{
    ret_code_t err = sd_ble_gap_tx_power_set(BLE_GAP_TX_POWER_ROLE_CONN, m_conn_handle, power);
    APP_ERROR_CHECK(err);
}

/**@brief Function for performing battery measurement and updating the Battery Level characteristic
 *        in Battery Service.
 */
static void battery_level_update_handle(void * p_context)
{
    ret_code_t err_code;

    err_code = ble_bas_battery_level_update(&m_bas, Batt_GetLevel(), BLE_CONN_HANDLE_ALL);
    if ((err_code != NRF_SUCCESS) &&
        (err_code != NRF_ERROR_INVALID_STATE) &&
        (err_code != NRF_ERROR_RESOURCES) &&
        (err_code != BLE_ERROR_GATTS_SYS_ATTR_MISSING)
       )
    {
		KIT_LOG(TAG, "Update batt err: 0x%02x!", err_code);
    }
}

/**@brief Function for the GAP initialization.
 *
 * @details This function sets up all the necessary GAP (Generic Access Profile) parameters of the
 *			device including the device name, appearance, and the preferred connection parameters.
 */
static void gap_params_init(void)
{
	uint32_t				err_code = NRF_SUCCESS;
	ble_gap_conn_params_t	gap_conn_params;
	ble_gap_conn_sec_mode_t sec_mode;

	BLE_GAP_CONN_SEC_MODE_SET_NO_ACCESS(&sec_mode);
	
	err_code = sd_ble_gap_device_name_set(&sec_mode, (const uint8_t *)Cfg_GetAdvName(), Cfg_GetAdvNameLen());
	APP_ERROR_CHECK(err_code);

	memset(&gap_conn_params, 0, sizeof(gap_conn_params));
	gap_conn_params.min_conn_interval = BLE_MIN_CONN_INTERVAL;
	gap_conn_params.max_conn_interval = BLE_MAX_CONN_INTERVAL;
	gap_conn_params.slave_latency	  = BLE_SLAVE_LATENCY;
	gap_conn_params.conn_sup_timeout  = BLE_CONN_SUP_TIMEOUT;

	err_code = sd_ble_gap_ppcp_set(&gap_conn_params);
	APP_ERROR_CHECK(err_code);
}

// YOUR_JOB: Update this code if you want to do anything given a DFU event (optional).
/**@brief Function for handling dfu events from the Buttonless Secure DFU service
 *
 * @param[in]   event   Event from the Buttonless Secure DFU service.
 */
static void ble_dfu_evt_handler(ble_dfu_buttonless_evt_type_t event)
{
    switch (event)
    {
        case BLE_DFU_EVT_BOOTLOADER_ENTER_PREPARE:
        {
            //KIT_LOG(TAG, "Device is preparing to enter bootloader mode.");
			/*
            // Prevent device from advertising on disconnect.
            ble_adv_modes_config_t config;
            advertising_config_get(&config);
            config.ble_adv_on_disconnect_disabled = true;
            ble_advertising_modes_config_set(&m_advertising, &config);

            // Disconnect all other bonded devices that currently are connected.
            // This is required to receive a service changed indication
            // on bootup after a successful (or aborted) Device Firmware Update.
            uint32_t conn_count = ble_conn_state_for_each_connected(disconnect, NULL);
            NRF_LOG_INFO("Disconnected %d links.", conn_count);
            */
            break;
        }

        case BLE_DFU_EVT_BOOTLOADER_ENTER:
            // YOUR_JOB: Write app-specific unwritten data to FLASH, control finalization of this
            //           by delaying reset by reporting false in app_shutdown_handler
            //KIT_LOG(TAG, "Device will enter bootloader mode.");
            break;

        case BLE_DFU_EVT_BOOTLOADER_ENTER_FAILED:
            //KIT_LOG(TAG, "Request to enter bootloader mode failed asynchroneously.");
            // YOUR_JOB: Take corrective measures to resolve the issue
            //           like calling APP_ERROR_CHECK to reset the device.
            break;

        case BLE_DFU_EVT_RESPONSE_SEND_ERROR:
            //KIT_LOG(TAG, "Request to send a response to client failed.");
            // YOUR_JOB: Take corrective measures to resolve the issue
            //           like calling APP_ERROR_CHECK to reset the device.
            //APP_ERROR_CHECK(false);
            break;

        default:
            //KIT_LOG(TAG, "Unknown event from ble_dfu_buttonless.");
            break;
    }
}

/**@brief Function for handling Queued Write Module errors.
 *
 * @details A pointer to this function will be passed to each service which may need to inform the
 *          application about an error.
 *
 * @param[in]   nrf_error   Error code containing information about what went wrong.
 */
static void nrf_qwr_error_handler(uint32_t nrf_error)
{
    APP_ERROR_HANDLER(nrf_error);
}

/**@brief Function for handling the data from the Nordic UART Service.
 *
 * @details This function will process the data received from the Nordic UART BLE Service and send
 *          it to the UART module.
 *
 * @param[in] p_evt       Nordic UART Service event.
 */
/**@snippet [Handling the data received over BLE] */
static void nus_data_handler(ble_nus_evt_t * p_evt)
{
	switch(p_evt->type )
	{
		case BLE_NUS_EVT_RX_DATA:	
			KIT_LOG(TAG, "Nus cmd header: 0x%2x!", p_evt->params.rx_data.p_data[0]);
			KIT_LOG(TAG, "Nus cmd len: %d", p_evt->params.rx_data.length);
			KIT_LOG(TAG, "Nus cmd 1: 0x%2x!", p_evt->params.rx_data.p_data[1]);
			//KIT_LOG(TAG, "Nus cmd 2: 0x%2x!", p_evt->params.rx_data.p_data[2]);
			if(p_evt->params.rx_data.p_data[0] == FCT_REQ_SATRT_TEST)
			{
				Fct_StartLoop();
			}
			else if(p_evt->params.rx_data.p_data[0] == FCT_REQ_HEADER)
			{
		 		Fct_PutReq(&p_evt->params.rx_data.p_data[1], p_evt->params.rx_data.length - 1);
			}
			else if(p_evt->params.rx_data.p_data[0] == FCT_REQ_STOP_TEST)
			{
				Fct_StopLoop();
			}
			else if(p_evt->params.rx_data.p_data[0] == CFG_REQ_HEADER)
			{
				Cfg_PutReq(&p_evt->params.rx_data.p_data[1], p_evt->params.rx_data.length - 1);
			}				
		  	break;
		  
		case BLE_NUS_EVT_TX_RDY: 
			break;
			
		default:
			break;
	}
}

static void ips_data_handler(ble_ips_evt_t * p_evt)
{
	int8_t rssi;
    uint8_t ch_index;

    switch (p_evt->type)
    {
        case BLE_IPS_EVT_DATA_RX:			
			sd_ble_gap_rssi_get(m_conn_handle, &rssi, &ch_index);
			Kit_PrintBytes(TAG, "Data access, write:", (const uint8_t*)p_evt->params.rx_data.p_data, p_evt->params.rx_data.length);
			Aps_PutCmd(p_evt->params.rx_data.p_data, p_evt->params.rx_data.length, (int)rssi);
            break;

        case BLE_IPS_EVT_CUS_NAME_RX:
			Kit_PrintBytes(TAG, "Custom name access, write:", (const uint8_t*)p_evt->params.rx_data.p_data, p_evt->params.rx_data.length);
			if(p_evt->params.rx_data.length > 0)
			{
				Cfg_SetAdvName(p_evt->params.rx_data.p_data, p_evt->params.rx_data.length);
				bleNameChangeFlg = true;
				sd_ble_gap_disconnect(m_conn_handle, BLE_HCI_CONN_INTERVAL_UNACCEPTABLE);
			}
            break;
						
		case BLE_IPS_EVT_LED_MODE_RX:
			//KIT_LOG(TAG, "Led mode access, write: set mode = %d.", p_evt->params.rx_data.p_data[0]);
			break;
			
        default:
            // No implementation needed.
            break;
    }    
}
/**@snippet [Handling the data received over BLE] */

/**@brief Function for handling the Connection Parameters Module.
 *
 * @details This function will be called for all events in the Connection Parameters Module which
 *          are passed to the application.
 *          @note All this function does is to disconnect. This could have been done by simply
 *                setting the disconnect_on_fail config parameter, but instead we use the event
 *                handler mechanism to demonstrate its use.
 *
 * @param[in] p_evt  Event received from the Connection Parameters Module.
 */
static void on_conn_params_evt(ble_conn_params_evt_t * p_evt)
{
    uint32_t err_code = NRF_SUCCESS;

    if (p_evt->evt_type == BLE_CONN_PARAMS_EVT_FAILED)
    {
		//KIT_LOG(TAG, "Connect params evt failed!");
        err_code = sd_ble_gap_disconnect(m_conn_handle, BLE_HCI_CONN_INTERVAL_UNACCEPTABLE);
        APP_ERROR_CHECK(err_code);
    }
}

/**@brief Function for handling a Connection Parameters error.
 *
 * @param[in] nrf_error  Error code containing information about what went wrong.
 */
static void conn_params_error_handler(uint32_t nrf_error)
{
    APP_ERROR_HANDLER(nrf_error);
}

/**@brief Function for initializing the Connection Parameters module.
 */
static void conn_params_init(void)
{
    uint32_t               err_code = NRF_SUCCESS;
    ble_conn_params_init_t cp_init;

    memset(&cp_init, 0, sizeof(cp_init));

    cp_init.p_conn_params                  = NULL;
    cp_init.first_conn_params_update_delay = BLE_FIRST_CONN_PARAMS_UPDATE_DELAY;
    cp_init.next_conn_params_update_delay  = BLE_NEXT_CONN_PARAMS_UPDATE_DELAY;
    cp_init.max_conn_params_update_count   = BLE_MAX_CONN_PARAMS_UPDATE_COUNT;
    cp_init.start_on_notify_cccd_handle    = BLE_GATT_HANDLE_INVALID;
    cp_init.disconnect_on_fail             = false;
    cp_init.evt_handler                    = on_conn_params_evt;
    cp_init.error_handler                  = conn_params_error_handler;

    err_code = ble_conn_params_init(&cp_init);
    APP_ERROR_CHECK(err_code);
}


/**@brief Function for handling advertising events.
 *
 * @details This function will be called for advertising events which are passed to the application.
 *
 * @param[in] ble_adv_evt  Advertising event.
 */
static void on_adv_evt(ble_adv_evt_t ble_adv_evt)
{
    switch (ble_adv_evt)
    {
        case BLE_ADV_EVT_FAST:
            break;

        case BLE_ADV_EVT_IDLE:
            break;

        default:
            break;
    }
}

/**@brief Function for initializing the Advertising functionality.
 */
static void advertising_init(void)
{
    uint32_t               err_code = NRF_SUCCESS;
    ble_advertising_init_t init;

    memset(&init, 0, sizeof(init));

    init.advdata.name_type               = BLE_ADVDATA_FULL_NAME;
    init.advdata.include_appearance      = false;
    init.advdata.flags                   = BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE;
    init.advdata.uuids_complete.uuid_cnt = sizeof(m_adv_uuids) / sizeof(m_adv_uuids[0]);
    init.advdata.uuids_complete.p_uuids  = m_adv_uuids;

    init.config.ble_adv_fast_enabled  = true;
    init.config.ble_adv_fast_interval = BLE_ADV_INTERVAL;
    init.config.ble_adv_fast_timeout  = BLE_ADV_DURATION;
    init.config.ble_adv_on_disconnect_disabled = false;
    init.evt_handler = on_adv_evt;
	
    init.srdata.name_type               = BLE_ADVDATA_FULL_NAME;

    err_code = ble_advertising_init(&m_advertising, &init);
    APP_ERROR_CHECK(err_code);

    ble_advertising_conn_cfg_tag_set(&m_advertising, BLE_CONN_CFG_TAG);
}

/**@brief Function for starting advertising.
 */
static void advertising_start(void)
{
    uint32_t err_code = NRF_SUCCESS;
	err_code = ble_advertising_start(&m_advertising, BLE_ADV_MODE_FAST);
    APP_ERROR_CHECK(err_code);

    KIT_LOG(TAG, "Adv start!");
}

/**@brief Function for handling BLE events.
 *
 * @param[in]   p_ble_evt   Bluetooth stack event.
 * @param[in]   p_context   Unused.
 */
static void ble_evt_handler(ble_evt_t const * p_ble_evt, void * p_context)
{
    uint32_t err_code = NRF_SUCCESS;
	ble_gap_conn_sec_mode_t sec_mode;

    switch (p_ble_evt->header.evt_id)
    {
        case BLE_GAP_EVT_DISCONNECTED:
            KIT_LOG(TAG, "Disconnected!");
			sd_ble_gap_rssi_stop(m_conn_handle);
            m_conn_handle = BLE_CONN_HANDLE_INVALID;
			Fct_StopLoop();
			Aps_StopLoop();
			Cfg_StopLoop();
			if(bleNameChangeFlg)
			{
				sd_ble_gap_adv_stop(m_advertising.adv_handle);
				BLE_GAP_CONN_SEC_MODE_SET_NO_ACCESS(&sec_mode);
				sd_ble_gap_device_name_set(&sec_mode, (const uint8_t *)Cfg_GetAdvName(), Cfg_GetAdvNameLen());
				advertising_init();
				advertising_start();
				bleNameChangeFlg = false;
				KIT_LOG(TAG, "Adv name change, re-init adv!");
			}
			else
			{
				Idc_SetType(INDICATE_DISCONNECTED);
			}
			bleState = BLE_STATE_ADV;
            break;

        case BLE_GAP_EVT_CONNECTED:
            KIT_LOG(TAG, "Connected!");
            m_conn_handle = p_ble_evt->evt.gap_evt.conn_handle;
            err_code = nrf_ble_qwr_conn_handle_assign(&m_qwr, m_conn_handle);
            APP_ERROR_CHECK(err_code);
			tx_power_set(BLE_TX_POWER_LEVEL);
            sd_ble_gap_rssi_start(m_conn_handle, BLE_GAP_RSSI_THRESHOLD_INVALID, 0);
			err_code = app_timer_start(m_battery_timer_id, BLE_BATT_UPDATE_INTERVAL, NULL);
			APP_ERROR_CHECK(err_code);
			battery_level_update_handle(NULL);
			Aps_StartLoop();
			Cfg_StartLoop();
			Idc_SetType(INDICATE_CONNECTED);
			bleState = BLE_STATE_CONNECTED;
            break;
			
		case BLE_GATTS_EVT_HVN_TX_COMPLETE:
			//KIT_LOG(TAG, "Ble tx comolete.");
			break;

        case BLE_GAP_EVT_PHY_UPDATE_REQUEST:
        {
            //KIT_LOG(TAG, "PHY update request.");
            ble_gap_phys_t const phys =
            {
                .rx_phys = BLE_GAP_PHY_AUTO,
                .tx_phys = BLE_GAP_PHY_AUTO,
            };
            err_code = sd_ble_gap_phy_update(p_ble_evt->evt.gap_evt.conn_handle, &phys);
            APP_ERROR_CHECK(err_code);
            break;
        }

        case BLE_GAP_EVT_SEC_PARAMS_REQUEST:
            // Pairing not supported
            //KIT_LOG(TAG, "Pairing not supported.");
            err_code = sd_ble_gap_sec_params_reply(m_conn_handle, BLE_GAP_SEC_STATUS_PAIRING_NOT_SUPP, NULL, NULL);
            APP_ERROR_CHECK(err_code);
            break;

        case BLE_GATTS_EVT_SYS_ATTR_MISSING:
            // No system attributes have been stored.
            //KIT_LOG(TAG, "No system attributes have been stored.");
            err_code = sd_ble_gatts_sys_attr_set(m_conn_handle, NULL, 0, 0);
            APP_ERROR_CHECK(err_code);
            break;

        case BLE_GATTC_EVT_TIMEOUT:
            // Disconnect on GATT Client timeout event.
            //KIT_LOG(TAG, "GATT Client Timeout.");
            err_code = sd_ble_gap_disconnect(p_ble_evt->evt.gattc_evt.conn_handle,
                                             BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
            APP_ERROR_CHECK(err_code);
            break;

        case BLE_GATTS_EVT_TIMEOUT:
            // Disconnect on GATT Server timeout event.
            //KIT_LOG(TAG, "GATT Server Timeout.");
            err_code = sd_ble_gap_disconnect(p_ble_evt->evt.gatts_evt.conn_handle,
                                             BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
            APP_ERROR_CHECK(err_code);
            break;

        default:
            // No implementation needed.
            break;
    }
}

/**@brief Function for handling events from the GATT library. */
static void gatt_evt_handler(nrf_ble_gatt_t * p_gatt, nrf_ble_gatt_evt_t const * p_evt)
{
}

/**@brief   Function for initializing the GATT module.
 * @details The GATT module handles ATT_MTU and Data Length update procedures automatically.
 */
static void gatt_init(void)
{
    ret_code_t err_code = NRF_SUCCESS;

    err_code = nrf_ble_gatt_init(&m_gatt, gatt_evt_handler);
    APP_ERROR_CHECK(err_code);

    err_code = nrf_ble_gatt_att_mtu_periph_set(&m_gatt, NRF_SDH_BLE_GATT_MAX_MTU_SIZE);
    APP_ERROR_CHECK(err_code);
}

/**
 * @brief SoftDevice SoC event handler.
 *
 * @param[in] evt_id    SoC event.
 * @param[in] p_context Context.
 */
static void soc_evt_handler(uint32_t evt_id, void * p_context)
{
    switch (evt_id)
    {
        case NRF_EVT_FLASH_OPERATION_SUCCESS:
            break;
        /* fall through */
        case NRF_EVT_FLASH_OPERATION_ERROR:
            break;

        default:
            // No implementation needed.
            break;
    }
}

/**@brief Function for initializing the BLE stack.
 *
 * @details Initializes the SoftDevice and the BLE event interrupt.
 */
static void ble_stack_init(void)
{
    ret_code_t err_code = NRF_SUCCESS;
	
    // Initialize the async SVCI interface to bootloader before any interrupts are enabled.
    err_code = ble_dfu_buttonless_async_svci_init();
    APP_ERROR_CHECK(err_code);
	
    err_code = nrf_sdh_enable_request();
    APP_ERROR_CHECK(err_code);

    // Fetch the start address of the application RAM.
    uint32_t ram_start = 0;
    err_code = nrf_sdh_ble_default_cfg_set(BLE_CONN_CFG_TAG, &ram_start);
    APP_ERROR_CHECK(err_code);

    // Enable BLE stack.
    err_code = nrf_sdh_ble_enable(&ram_start);
    APP_ERROR_CHECK(err_code);

    // Register a handler for BLE events.
    NRF_SDH_BLE_OBSERVER(m_ble_observer, BLE_EVT_OBSERVER_PRIO, ble_evt_handler, NULL);
    NRF_SDH_SOC_OBSERVER(m_soc_observer, BLE_SOC_OBSERVER_PRIO, soc_evt_handler, NULL);
}

/**@brief Function for initializing services that will be used by the application.
 */
static void services_init(void)
{
    uint32_t           err_code = NRF_SUCCESS;
    nrf_ble_qwr_init_t qwr_init  = {0};
    ble_bas_init_t     bas_init;
    ble_ips_init_t     ips_init;
    ble_nus_init_t     nus_init;
    ble_dfu_buttonless_init_t dfus_init = {0};

    // Initialize Queued Write Module.
    qwr_init.error_handler = nrf_qwr_error_handler;
    err_code = nrf_ble_qwr_init(&m_qwr, &qwr_init);
    APP_ERROR_CHECK(err_code);
    
    // Initialize Battery Service.
    memset(&bas_init, 0, sizeof(bas_init));
    // Here the sec level for the Battery Service can be changed/increased.
    bas_init.bl_rd_sec        = SEC_OPEN;
    bas_init.evt_handler          = NULL;
    bas_init.p_report_ref         = NULL;
    bas_init.initial_batt_level   = 80;
    err_code = ble_bas_init(&m_bas, &bas_init);
    APP_ERROR_CHECK(err_code);

    // Initialize Insulin Pump Service.
    memset(&ips_init, 0, sizeof(ips_init));
    ips_init.data_handler = ips_data_handler;
    err_code = ble_ips_init(&m_ips, &ips_init, Cfg_GetAdvName(), Cfg_GetAdvNameLen());
    APP_ERROR_CHECK(err_code);
	
	//init dfu must after init IPS, because advertise need to adverting IPS vendor uuid
    // Initialize DFU Service.
    dfus_init.evt_handler = ble_dfu_evt_handler;
    err_code = ble_dfu_buttonless_init(&dfus_init);
    APP_ERROR_CHECK(err_code);
	
    // Initialize NUS.
    memset(&nus_init, 0, sizeof(nus_init));
    nus_init.data_handler = nus_data_handler;
    err_code = ble_nus_init(&m_nus, &nus_init);
    APP_ERROR_CHECK(err_code);

}

void Ble_Init(void)
{
    ret_code_t err_code;
	
	ble_stack_init();
	Cfg_StorageLoad();
	gap_params_init();
	gatt_init();
	services_init();
	conn_params_init();
	advertising_init();
	advertising_start();

    err_code = app_timer_create(&m_battery_timer_id, APP_TIMER_MODE_REPEATED, battery_level_update_handle);
    APP_ERROR_CHECK(err_code);
			
	KIT_LOG(TAG, "Init OK!");
}

void Ble_NusSendData(uint8_t *data, uint8_t len) 
{
	uint32_t err_code = NRF_SUCCESS;
	
	err_code = ble_nus_data_send(&m_nus, data, (uint16_t *)(&len), m_conn_handle);
	
	if(err_code != NRF_SUCCESS)
	{
		KIT_LOG(TAG, "Nus send data error!");
	}
}

void Ble_IpsNotifyRespCntAndSendData(uint8_t *data, int len) 
{
	uint32_t err_code = NRF_SUCCESS;
	
	err_code = ble_ips_data_send(data, len, &m_ips);
	
	if(err_code == NRF_SUCCESS)
	{
		ble_ips_response_cnt_notify(&m_ips);
	}
	else
	{
		KIT_LOG(TAG, "Ips notify error!");
	}
}

eBleState_t Ble_GetState(void) 
{
	return bleState;
}


