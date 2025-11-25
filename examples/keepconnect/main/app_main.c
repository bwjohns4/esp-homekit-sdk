/*
 * ESPRESSIF MIT License
 *
 * Copyright (c) 2018 <ESPRESSIF SYSTEMS (SHANGHAI) PTE LTD>
 *
 * Permission is hereby granted for use on ESPRESSIF SYSTEMS products only, in which case,
 * it is free of charge, to any person obtaining a copy of this software and associated
 * documentation files (the "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the Software is furnished
 * to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or
 * substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */
/* HomeKit Smart Outlet Example
*/
#include <stdio.h>
#include <string.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <esp_log.h>
#include <driver/gpio.h>

#include <hap.h>

#include <hap_apple_servs.h>
#include <hap_apple_chars.h>

#include <app_wifi.h>
#include <app_hap_setup_payload.h>

#include "esp_ota_ops.h"
#include "esp_http_client.h"
#include "esp_https_ota.h"


static const char *TAG = "HAP outlet";

#define SMART_OUTLET_TASK_PRIORITY  1
#define SMART_OUTLET_TASK_STACKSIZE 4 * 1024
#define SMART_OUTLET_TASK_NAME      "hap_outlet"

#define OUTLET_IN_USE_GPIO GPIO_NUM_14

#define ESP_INTR_FLAG_DEFAULT 0

esp_err_t _http_event_handler(esp_http_client_event_t *evt)
{
    switch(evt->event_id) {
        case HTTP_EVENT_ERROR:
            ESP_LOGD(TAG, "HTTP_EVENT_ERROR");
            break;
        case HTTP_EVENT_ON_CONNECTED:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_CONNECTED");
            break;
        case HTTP_EVENT_HEADER_SENT:
            ESP_LOGD(TAG, "HTTP_EVENT_HEADER_SENT");
            break;
        case HTTP_EVENT_ON_HEADER:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
            break;
        case HTTP_EVENT_ON_DATA:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
            break;
        case HTTP_EVENT_ON_FINISH:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_FINISH");
            break;
        case HTTP_EVENT_DISCONNECTED:
            ESP_LOGD(TAG, "HTTP_EVENT_DISCONNECTED");
            break;
    }
    return ESP_OK;
}

void simple_ota_example_task(void * pvParameter)
{
    ESP_LOGI(TAG, "Starting OTA example...");
    while(1){
    esp_http_client_config_t config = {
        .url = "http://secureapi.johnson-creative.com/SmartPlugs/v2firmware.bin",
        .event_handler = _http_event_handler,
    };
    esp_err_t ret = esp_https_ota(&config);
    if (ret == ESP_OK) {
        esp_restart();
    } else {
        ESP_LOGD(TAG, "Firmware Upgrades Failed");
    }
    vTaskDelay(3600000 / portTICK_PERIOD_MS);
}
}

struct button{
    uint16_t debounceTime;
    uint16_t long_press_time;
    bool pressedValue;
    uint32_t last_press_time;
    uint32_t last_event_time;
} volatile myButton;

static QueueHandle_t s_esp_evt_queue = NULL;
/**
 * @brief the recover outlet in use gpio interrupt function
 */
static void IRAM_ATTR outlet_in_use_isr(void* arg)
{
    uint32_t gpio_num = (uint32_t) arg;
    //xQueueSendFromISR(s_esp_evt_queue, &gpio_num, NULL);
}

static void IRAM_ATTR toggleRelay(void* arg){
    //ESP_LOGI(TAG, "Outlet-In-Use trigger FAIL");
    uint32_t gpio_num = (uint32_t) 14;
    uint32_t now = xTaskGetTickCountFromISR();
    if((now - myButton.last_event_time)*portTICK_PERIOD_MS < myButton.debounceTime) return;
    myButton.last_press_time = now;
    if(!gpio_get_level(0)){
        myButton.last_press_time = now;
    } else{
        if(((now - myButton.last_press_time) * portTICK_PERIOD_MS >myButton.long_press_time)){
            hap_reset_to_factory();
            return;
        } else{
            gpio_set_level(14, !gpio_get_level(14));
            gpio_set_level(3, !gpio_get_level(3));
            xQueueSendFromISR(s_esp_evt_queue, &gpio_num, NULL);
        }
    }
}

/**
 * Enable a GPIO Pin for Outlet in Use Detection
 */
static void outlet_in_use_key_init(uint32_t key_gpio_pin)
{
    gpio_config_t io_conf;
    /* Interrupt for both the edges  */
    io_conf.intr_type = GPIO_INTR_ANYEDGE;
    /* Bit mask of the pins */
    io_conf.pin_bit_mask = ((1ULL<<14) | (1ULL<<3));
    /* Set as input mode */
    io_conf.mode = GPIO_MODE_OUTPUT;
    /* Enable internal pull-up */
    io_conf.pull_up_en = 0;
    /* Disable internal pull-down */
    io_conf.pull_down_en = 0;
    /* Set the GPIO configuration */
    gpio_config(&io_conf);

   
    /* Interrupt for both the edges  */
    io_conf.intr_type = GPIO_INTR_ANYEDGE;
    /* Bit mask of the pins */
    io_conf.pin_bit_mask = 1ULL << 0;
    /* Set as input mode */
    io_conf.mode = GPIO_MODE_INPUT;
    /* Enable internal pull-up */
    io_conf.pull_up_en = 1;
    /* Disable internal pull-down */
    io_conf.pull_down_en = 0;
    /* Set the GPIO configuration */
    gpio_config(&io_conf);


    /* Install gpio isr service */
    gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);
    /* Hook isr handler for specified gpio pin */
    gpio_isr_handler_add(key_gpio_pin, outlet_in_use_isr, (void*)key_gpio_pin);
    gpio_isr_handler_add(0, toggleRelay, (void*)0);

    gpio_set_level(14, 0);
    gpio_set_level(3, 1);

    myButton.debounceTime = 200;
    myButton.long_press_time = 15000;
}

/**
 * Initialize the Smart Outlet Hardware.Here, we just enebale the Outlet-In-Use detection.
 */
void smart_outlet_hardware_init(gpio_num_t gpio_num)
{
    s_esp_evt_queue = xQueueCreate(2, sizeof(uint32_t));
    if (s_esp_evt_queue != NULL) {
        outlet_in_use_key_init(gpio_num);
    }
}

/* Mandatory identify routine for the accessory.
 * In a real accessory, something like LED blink should be implemented
 * got visual identification
 */
static int outlet_identify(hap_acc_t *ha)
{
    ESP_LOGI(TAG, "Accessory identified");
    return HAP_SUCCESS;
}

/* A dummy callback for handling a write on the "On" characteristic of Outlet.
 * In an actual accessory, this should control the hardware
 */
static int outlet_write(hap_write_data_t write_data[], int count,
        void *serv_priv, void *write_priv)
{
    int i, ret = HAP_SUCCESS;
    hap_write_data_t *write;
    for (i = 0; i < count; i++) {
        write = &write_data[i];
        if (!strcmp(hap_char_get_type_uuid(write->hc), HAP_CHAR_UUID_ON)) {
            ESP_LOGI(TAG, "Received Write. Outlet %s", write->val.b ? "On" : "Off");
            /* TODO: Control Actual Hardware */
            //gpio16_output_set(0);
            gpio_set_level(14, write->val.b ? 1 : 0);
            gpio_set_level(3, write->val.b ? 0 : 1);
            hap_char_update_val(write->hc, &(write->val));
            *(write->status) = HAP_STATUS_SUCCESS;
        } else {
            *(write->status) = HAP_STATUS_RES_ABSENT;
        }
    }
    return ret;
}

/*The main thread for handling the Smart Outlet Accessory */
static void smart_outlet_thread_entry(void *p)
{
    hap_acc_t *accessory;
    hap_serv_t *service;

    /* Initialize the HAP core */
    hap_init(HAP_TRANSPORT_WIFI);

    /* Initialise the mandatory parameters for Accessory which will be added as
     * the mandatory services internally
     */
    hap_acc_cfg_t cfg = {
        .name = "Byron Loves You",
        .manufacturer = "Keep Connect",
        .model = "KC112",
        .serial_num = "001122334455",
        .fw_rev = "0.9.0",
        .hw_rev = NULL,
        .pv = "1.1.0",
        .identify_routine = outlet_identify,
        .cid = HAP_CID_OUTLET,

    };
    /* Create accessory object */
    accessory = hap_acc_create(&cfg);

    /* Add a dummy Product Data */
    uint8_t product_data[] = {'E','S','P','3','2','H','A','P'};
    hap_acc_add_product_data(accessory, product_data, sizeof(product_data));

    /* Add Wi-Fi Transport service required for HAP Spec R16 */
    hap_acc_add_wifi_transport_service(accessory, 0);

    /* Create the Outlet Service. Include the "name" since this is a user visible service  */
    service = hap_serv_outlet_create(false, false);
    hap_serv_add_char(service, hap_char_name_create("My Smart Outlet"));

    /* Get pointer to the outlet in use characteristic which we need to monitor for state changes */
    hap_char_t *outlet_in_use = hap_serv_get_char_by_uuid(service, HAP_CHAR_UUID_OUTLET_IN_USE);
    hap_char_t *outlet_state = hap_serv_get_char_by_uuid(service, HAP_CHAR_UUID_ON);

    /* Set the write callback for the service */
    hap_serv_set_write_cb(service, outlet_write);

    /* Add the Outlet Service to the Accessory Object */
    hap_acc_add_serv(accessory, service);

    /* Add the Accessory to the HomeKit Database */
    hap_add_accessory(accessory);

    /* Initialize the appliance specific hardware. This enables out-in-use detection */
    smart_outlet_hardware_init(OUTLET_IN_USE_GPIO);

    /* For production accessories, the setup code shouldn't be programmed on to
     * the device. Instead, the setup info, derived from the setup code must
     * be used. Use the factory_nvs_gen utility to generate this data and then
     * flash it into the factory NVS partition.
     *
     * By default, the setup ID and setup info will be read from the factory_nvs
     * Flash partition and so, is not required to set here explicitly.
     *
     * However, for testing purpose, this can be overridden by using hap_set_setup_code()
     * and hap_set_setup_id() APIs, as has been done here.
     */
#ifdef CONFIG_EXAMPLE_USE_HARDCODED_SETUP_CODE
    /* Unique Setup code of the format xxx-xx-xxx. Default: 111-22-333 */
    hap_set_setup_code(CONFIG_EXAMPLE_SETUP_CODE);
    /* Unique four character Setup Id. Default: ES32 */
    hap_set_setup_id(CONFIG_EXAMPLE_SETUP_ID);
#ifdef CONFIG_APP_WIFI_USE_WAC_PROVISIONING
    app_hap_setup_payload(CONFIG_EXAMPLE_SETUP_CODE, CONFIG_EXAMPLE_SETUP_ID, true, cfg.cid);
#else
    app_hap_setup_payload(CONFIG_EXAMPLE_SETUP_CODE, CONFIG_EXAMPLE_SETUP_ID, false, cfg.cid);
#endif
#endif

    /* Enable Hardware MFi authentication (applicable only for MFi variant of SDK) */
    hap_enable_mfi_auth(HAP_MFI_AUTH_HW);

    /* Initialize Wi-Fi */
    app_wifi_init();

    /* After all the initializations are done, start the HAP core */
    hap_start();
    /* Start Wi-Fi */
    app_wifi_start(portMAX_DELAY);

    uint32_t io_num = OUTLET_IN_USE_GPIO;
    hap_val_t appliance_value = {
        .b = false,
    };
    hap_val_t appliance_state = {
        .b = false,
    };

    xTaskCreate(&simple_ota_example_task, "ota_example_task", 8192, NULL, 5, NULL);

    /* Listen for Outlet-In-Use state change events. Other read/write functionality will be handled
     * by the HAP Core.
     * When the Outlet in Use GPIO goes low, it means Outlet is not in use.
     * When the Outlet in Use GPIO goes high, it means Outlet is in use.
     * Applications can define own logic as per their hardware.
     */
    while (1) {
        if (xQueueReceive(s_esp_evt_queue, &io_num, portMAX_DELAY) == pdFALSE) {
            ESP_LOGI(TAG, "Outlet-In-Use trigger FAIL");
        } else {
            appliance_value.b = gpio_get_level(io_num);
            /* If any state change is detected, update the Outlet In Use characteristic value */
            hap_char_update_val(outlet_in_use, &appliance_value);
            hap_char_update_val(outlet_state, &appliance_value);
            ESP_LOGI(TAG, "Outlet-In-Use triggered [%d]", appliance_value.b);
        }
    }
}

void app_main()
{
    //esp_set_cpu_freq(ESP_CPU_FREQ_160M);
    /* Create the application thread */
    xTaskCreate(smart_outlet_thread_entry, SMART_OUTLET_TASK_NAME, SMART_OUTLET_TASK_STACKSIZE,
                NULL, SMART_OUTLET_TASK_PRIORITY, NULL);
}

