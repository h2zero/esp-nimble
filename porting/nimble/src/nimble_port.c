/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#include <stddef.h>
#include "os/os.h"
#include "sysinit/sysinit.h"
#include "host/ble_hs.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "nimble/transport.h"
#if NIMBLE_CFG_CONTROLLER
#include "controller/ble_ll.h"
#include "os/os_cputime.h"
#endif

#include "nimble/nimble_port_freertos.h"
#if CONFIG_BT_CONTROLLER_ENABLED
#include "esp_bt.h"
#endif
#if !SOC_ESP_NIMBLE_CONTROLLER && CONFIG_BT_CONTROLLER_ENABLED
#include "esp_nimble_hci.h"
#endif

#include "esp_log.h"
#define NIMBLE_PORT_LOG_TAG          "BLE_INIT"

static struct ble_npl_eventq g_eventq_dflt;

extern void os_msys_init(void);
extern void os_mempool_module_init(void);
#if NIMBLE_CFG_CONTROLLER
extern void ble_ll_init(void);
#endif
extern void ble_hs_deinit(void);

/**
 * @brief esp_nimble_init - Initialize the NimBLE host stack
 * 
 * @return esp_err_t 
 */
esp_err_t esp_nimble_init(void)
{
#if CONFIG_BT_CONTROLLER_DISABLED
    esp_err_t ret;
#endif
#if !SOC_ESP_NIMBLE_CONTROLLER || !CONFIG_BT_CONTROLLER_ENABLED
    /* Initialize the function pointers for OS porting */
    npl_freertos_funcs_init();

    npl_freertos_mempool_init();

#if CONFIG_BT_CONTROLLER_ENABLED
    if(esp_nimble_hci_init() != ESP_OK) {
        ESP_LOGE(NIMBLE_PORT_LOG_TAG, "hci inits failed\n");
        return ESP_FAIL;
    }
#else
    ret = ble_buf_alloc();
    if (ret != ESP_OK) {
        ble_buf_free();
        return ESP_FAIL;
    }
    ble_transport_init();
#if MYNEWT_VAL(BLE_QUEUE_CONG_CHECK)
    ble_adv_list_init();
#endif
#endif

    /* Initialize default event queue */
    ble_npl_eventq_init(&g_eventq_dflt);
    /* Initialize the global memory pool */
    os_mempool_module_init();
    os_msys_init();

#endif

    /* Initialize the host */
    ble_transport_hs_init();

    ble_transport_ll_init();

    return ESP_OK;
}

esp_err_t
nimble_port_init(void)
{
    esp_err_t ret;

#if CONFIG_IDF_TARGET_ESP32 && CONFIG_BT_CONTROLLER_ENABLED
    esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT);
#endif
#if CONFIG_BT_CONTROLLER_ENABLED
    esp_bt_controller_config_t config_opts = BT_CONTROLLER_INIT_CONFIG_DEFAULT();

    ret = esp_bt_controller_init(&config_opts);
    if (ret != ESP_OK) {
        ESP_LOGE(NIMBLE_PORT_LOG_TAG, "controller init failed\n");
        return ret;
    }

    ret = esp_bt_controller_enable(ESP_BT_MODE_BLE);
    if (ret != ESP_OK) {
        // Deinit to free any memory the controller is using.
        if(esp_bt_controller_deinit() != ESP_OK) {
            ESP_LOGE(NIMBLE_PORT_LOG_TAG, "controller deinit failed\n");
        }

        ESP_LOGE(NIMBLE_PORT_LOG_TAG, "controller enable failed\n");
        return ret;
    }
#endif

    ret = esp_nimble_init();
    if (ret != ESP_OK) {

#if CONFIG_BT_CONTROLLER_ENABLED
	    // Disable and deinit controller to free memory
        if(esp_bt_controller_disable() != ESP_OK) {
            ESP_LOGE(NIMBLE_PORT_LOG_TAG, "controller disable failed\n");
        }

	    if(esp_bt_controller_deinit() != ESP_OK) {
            ESP_LOGE(NIMBLE_PORT_LOG_TAG, "controller deinit failed\n");
        }
#endif

	ESP_LOGE(NIMBLE_PORT_LOG_TAG, "nimble host init failed\n");
        return ret;
    }

#if MYNEWT_VAL(BT_HCI_LOG_INCLUDED)
    bt_hci_log_init();
#endif // (BT_HCI_LOG_INCLUDED == TRUE)

    return ESP_OK;
}

void
nimble_port_run(void)
{
    struct ble_npl_event *ev;

    while (1) {
        ev = ble_npl_eventq_get(&g_eventq_dflt, BLE_NPL_TIME_FOREVER);
        ble_npl_event_run(ev);
    }
}

struct ble_npl_eventq *
nimble_port_get_dflt_eventq(void)
{
    return &g_eventq_dflt;
}

#if NIMBLE_CFG_CONTROLLER
void
nimble_port_ll_task_func(void *arg)
{
    extern void ble_ll_task(void *);

    ble_ll_task(arg);
}
#endif
