/**
 *@file ble_ips.h
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
#ifndef BLE_IPS_H__
#define BLE_IPS_H__

#include <stdint.h>
#include <stdbool.h>

#include "sdk_config.h"
#include "ble.h"
#include "ble_srv_common.h"
#include "nrf_sdh_ble.h"
#include "ble_link_ctx_manager.h"

#ifdef __cplusplus
extern "C" {
#endif

// <o> BLE_IPS_BLE_OBSERVER_PRIO  
// <i> Priority with which BLE events are dispatched to the UART Service.

#ifndef BLE_IPS_BLE_OBSERVER_PRIO
#define BLE_IPS_BLE_OBSERVER_PRIO 2
#endif

/**@brief   Macro for defining a ble_ips instance.
 *
 * @param     _name            Name of the instance.
 * @param[in] _ips_max_clients Maximum number of IPS clients connected at a time.
 * @hideinitializer
 */
#define BLE_IPS_DEF(_name, _ips_max_clients)                      \
    BLE_LINK_CTX_MANAGER_DEF(CONCAT_2(_name, _link_ctx_storage),  \
                             (_ips_max_clients),                  \
                             sizeof(ble_ips_client_context_t));   \
    static ble_ips_t _name =                                      \
    {                                                             \
        .p_link_ctx_storage = &CONCAT_2(_name, _link_ctx_storage) \
    };                                                            \
    NRF_SDH_BLE_OBSERVER(_name ## _obs,                           \
                         BLE_IPS_BLE_OBSERVER_PRIO,               \
                         ble_ips_on_ble_evt,                      \
                         &_name)

#define BLE_UUID_IPS_SERVICE 0x733B /**< The UUID of the Nordic UART Service. */

#define OPCODE_LENGTH        1
#define HANDLE_LENGTH        2

/**@brief   Maximum length of data (in bytes) that can be transmitted to the peer by the Nordic UART service module. */
#if defined(NRF_SDH_BLE_GATT_MAX_MTU_SIZE) && (NRF_SDH_BLE_GATT_MAX_MTU_SIZE != 0)
    #define BLE_IPS_MAX_DATA_LEN (NRF_SDH_BLE_GATT_MAX_MTU_SIZE - OPCODE_LENGTH - HANDLE_LENGTH)
#else
    #define BLE_IPS_MAX_DATA_LEN (BLE_GATT_MTU_SIZE_DEFAULT - OPCODE_LENGTH - HANDLE_LENGTH)
    #warning NRF_SDH_BLE_GATT_MAX_MTU_SIZE is not defined.
#endif

#define BLE_IPS_MAX_DATA_CHAR_LEN    	(150 > BLE_IPS_MAX_DATA_LEN ? BLE_IPS_MAX_DATA_LEN : 150)/**< Maximum length of the RX Characteristic (in bytes). */
#define BLE_IPS_MAX_RES_CNT_CHAR_LEN    (1 > BLE_IPS_MAX_DATA_LEN ? BLE_IPS_MAX_DATA_LEN : 1) 	 /**< Maximum length of the RX Characteristic (in bytes). */
#define BLE_IPS_MAX_TMR_TICK_CHAR_LEN   (1 > BLE_IPS_MAX_DATA_LEN ? BLE_IPS_MAX_DATA_LEN : 1)	 /**< Maximum length of the RX Characteristic (in bytes). */
#define BLE_IPS_MAX_CUS_NAME_CHAR_LEN   (30 > BLE_IPS_MAX_DATA_LEN ? BLE_IPS_MAX_DATA_LEN : 30)	 /**< Maximum length of the RX Characteristic (in bytes). */
#define BLE_IPS_MAX_LED_MODE_CHAR_LEN   (1 > BLE_IPS_MAX_DATA_LEN ? BLE_IPS_MAX_DATA_LEN : 1)	 /**< Maximum length of the RX Characteristic (in bytes). */

/**@brief   Nordic UART Service event types. */
typedef enum
{
    BLE_IPS_EVT_DATA_RX,      					/**< Data received. */
	BLE_IPS_EVT_CUS_NAME_RX,					/**< Data received. */
	BLE_IPS_EVT_LED_MODE_RX,					/**< Data received. */
} ble_ips_evt_type_t;


/* Forward declaration of the ble_ips_t type. */
typedef struct ble_ips_s ble_ips_t;


/**@brief   Nordic UART Service @ref BLE_IPS_EVT_RX_DATA event data.
 *
 * @details This structure is passed to an event when @ref BLE_IPS_EVT_RX_DATA occurs.
 */
typedef struct
{
    uint8_t const * p_data; /**< A pointer to the buffer with received data. */
    uint16_t        length; /**< Length of received data. */
} ble_ips_evt_rx_data_t;


/**@brief Nordic UART Service client context structure.
 *
 * @details This structure contains state context related to hosts.
 */
typedef struct
{
    bool is_res_cnt_notification_enabled; 	/**< Variable to indicate if the peer has enabled notification of the RX characteristic.*/
    bool is_tmr_tick_notification_enabled; 	/**< Variable to indicate if the peer has enabled notification of the RX characteristic.*/
} ble_ips_client_context_t;


/**@brief   Nordic UART Service event structure.
 *
 * @details This structure is passed to an event coming from service.
 */
typedef struct
{
    ble_ips_evt_type_t         type;        /**< Event type. */
    ble_ips_t                * p_ips;       /**< A pointer to the instance. */
    uint16_t                   conn_handle; /**< Connection handle. */
    ble_ips_client_context_t * p_link_ctx;  /**< A pointer to the link context. */
    union
    {
        ble_ips_evt_rx_data_t rx_data; /**< @ref BLE_IPS_EVT_RX_DATA event data. */
    } params;
} ble_ips_evt_t;


/**@brief Nordic UART Service event handler type. */
typedef void (* ble_ips_data_handler_t) (ble_ips_evt_t * p_evt);


/**@brief   Nordic UART Service initialization structure.
 *
 * @details This structure contains the initialization information for the service. The application
 * must fill this structure and pass it to the service using the @ref ble_ips_init
 *          function.
 */
typedef struct
{
    ble_ips_data_handler_t data_handler; /**< Event handler to be called for handling received data. */
} ble_ips_init_t;


/**@brief   Nordic UART Service structure.
 *
 * @details This structure contains status information related to the service.
 */
struct ble_ips_s
{
    uint8_t                         uuid_type;          	/**< UUID type for Nordic UART Service Base UUID. */
    uint16_t                        service_handle;     	/**< Handle of Nordic UART Service (as provided by the SoftDevice). */
    uint16_t                     	conn_handle;            /**< Handle of the current connection (as provided by the BLE stack, is BLE_CONN_HANDLE_INVALID if not in a connection). */
    uint8_t 						response_count;
    uint8_t					 		timer_tick;
    ble_gatts_char_handles_t        data_char_handles;      /**< Handles related to the data characteristic (as provided by the SoftDevice). */
    ble_gatts_char_handles_t        res_cnt_char_handles;   /**< Handles related to the response count characteristic (as provided by the SoftDevice). */
    ble_gatts_char_handles_t        tmr_tick_char_handles;  /**< Handles related to the TX characteristic (as provided by the SoftDevice). */
    ble_gatts_char_handles_t        cus_name_char_handles;  /**< Handles related to the TX characteristic (as provided by the SoftDevice). */
    ble_gatts_char_handles_t        fw_ver_char_handles;    /**< Handles related to the RX characteristic (as provided by the SoftDevice). */
    ble_gatts_char_handles_t        led_mode_char_handles;  /**< Handles related to the TX characteristic (as provided by the SoftDevice). */
    blcm_link_ctx_storage_t * const p_link_ctx_storage; 	/**< Pointer to link context storage with handles of all current connections and its context. */
    ble_ips_data_handler_t          data_handler;       	/**< Event handler to be called for handling received data. */
};


/**@brief   Function for initializing the Nordic UART Service.
 *
 * @param[out] p_ips      Nordic UART Service structure. This structure must be supplied
 *                        by the application. It is initialized by this function and will
 *                        later be used to identify this particular service instance.
 * @param[in] p_ips_init  Information needed to initialize the service.
 *
 * @retval NRF_SUCCESS If the service was successfully initialized. Otherwise, an error code is returned.
 * @retval NRF_ERROR_NULL If either of the pointers p_ips or p_ips_init is NULL.
 */
uint32_t ble_ips_init(ble_ips_t * p_ips, ble_ips_init_t const * p_ips_init, uint8_t* p_cus_name, uint8_t cus_name_len);


/**@brief   Function for handling the Nordic UART Service's BLE events.
 *
 * @details The Nordic UART Service expects the application to call this function each time an
 * event is received from the SoftDevice. This function processes the event if it
 * is relevant and calls the Nordic UART Service event handler of the
 * application if necessary.
 *
 * @param[in] p_ble_evt     Event received from the SoftDevice.
 * @param[in] p_context     Nordic UART Service structure.
 */
void ble_ips_on_ble_evt(ble_evt_t const * p_ble_evt, void * p_context);


/**@brief   Function for sending a data to the peer.
 *
 * @details This function sends the input string as an RX characteristic notification to the
 *          peer.
 *
 * @param[in]     p_ips       Pointer to the Nordic UART Service structure.
 * @param[in]     p_data      String to be sent.
 * @param[in,out] p_length    Pointer Length of the string. Amount of sent bytes.
 * @param[in]     conn_handle Connection Handle of the destination client.
 *
 * @retval NRF_SUCCESS If the string was sent successfully. Otherwise, an error code is returned.
 */
uint32_t ble_ips_data_send(uint8_t *buf, int count, ble_ips_t *p_ips);

uint32_t ble_ips_response_cnt_notify(ble_ips_t *p_ips);

#ifdef __cplusplus
}
#endif

#endif // BLE_IPS_H__

/** @} */
