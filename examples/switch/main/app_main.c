#include <string.h>
#include <inttypes.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_log.h>
#include <esp_event.h>
#include <nvs_flash.h>
#include <stdlib.h>  // rand(), srand()
#include <time.h>    // time()

#include <esp_rmaker_core.h>
#include <esp_rmaker_standard_types.h>
#include <esp_rmaker_standard_params.h>
#include <esp_rmaker_standard_devices.h>
#include <esp_rmaker_schedule.h>
#include <esp_rmaker_scenes.h>
#include <esp_rmaker_console.h>
#include <esp_rmaker_ota.h>

#include <esp_rmaker_common_events.h>

#include <app_wifi.h>
#include <app_insights.h>

#include "app_priv.h"

static const char *TAG = "app_main";
esp_rmaker_device_t *switch_device;
esp_rmaker_device_t *switch_device1;
esp_rmaker_device_t *switch_device2;
esp_rmaker_device_t *switch_device3;
esp_rmaker_device_t *dispense_device;

/* Callbacks */
static esp_err_t rgb_write_cb(const esp_rmaker_device_t *device, const esp_rmaker_param_t *param,
            const esp_rmaker_param_val_t val, void *priv_data, esp_rmaker_write_ctx_t *ctx)
{
    if (ctx) {
        ESP_LOGI(TAG, "Received write request for RGB device via : %s", esp_rmaker_device_cb_src_to_str(ctx->src));
    }
    // Generate a random 3-digit number
    int dispensed_value = rand() % 900 + 100; // Generates a random number between 100 and 999

    // Update the Dispense parameter with the generated value
    esp_rmaker_param_t *dispense_param = esp_rmaker_device_get_param_by_type(dispense_device, ESP_RMAKER_PARAM_OTHER);
    if (dispense_param) {
        esp_rmaker_param_update_and_report(dispense_param, esp_rmaker_int(dispensed_value));
    } else {
        ESP_LOGE(TAG, "Dispense parameter not found on device %s", esp_rmaker_device_get_name(dispense_device));
    }

    return ESP_OK;
}

static esp_err_t write_cb(const esp_rmaker_device_t *device, const esp_rmaker_param_t *param,
            const esp_rmaker_param_val_t val, void *priv_data, esp_rmaker_write_ctx_t *ctx)
{
    if (ctx) {
        ESP_LOGI(TAG, "Received write request via : %s", esp_rmaker_device_cb_src_to_str(ctx->src));
    }
    if (strcmp(esp_rmaker_param_get_name(param), ESP_RMAKER_DEF_POWER_NAME) == 0) {
        ESP_LOGI(TAG, "Received value = %s for %s - %s",
                val.val.b? "true" : "false", esp_rmaker_device_get_name(device),
                esp_rmaker_param_get_name(param));
        app_driver_set_state(val.val.b);
        esp_rmaker_param_update_and_report(param, val);
    }
    return ESP_OK;
}

/* Event handler */
static void event_handler(void* arg, esp_event_base_t event_base,
                          int32_t event_id, void* event_data)
{
    if (event_base == RMAKER_EVENT) {
        switch (event_id) {
            case RMAKER_EVENT_INIT_DONE:
                ESP_LOGI(TAG, "RainMaker Initialised.");
                break;
            case RMAKER_EVENT_CLAIM_STARTED:
                ESP_LOGI(TAG, "RainMaker Claim Started.");
                break;
            case RMAKER_EVENT_CLAIM_SUCCESSFUL:
                ESP_LOGI(TAG, "RainMaker Claim Successful.");
                break;
            case RMAKER_EVENT_CLAIM_FAILED:
                ESP_LOGI(TAG, "RainMaker Claim Failed.");
                break;
            case RMAKER_EVENT_LOCAL_CTRL_STARTED:
                ESP_LOGI(TAG, "Local Control Started.");
                break;
            case RMAKER_EVENT_LOCAL_CTRL_STOPPED:
                ESP_LOGI(TAG, "Local Control Stopped.");
                break;
            default:
                ESP_LOGW(TAG, "Unhandled RainMaker Event: %"PRIi32, event_id);
        }
    } else if (event_base == RMAKER_COMMON_EVENT) {
        switch (event_id) {
            case RMAKER_EVENT_REBOOT:
                ESP_LOGI(TAG, "Rebooting in %d seconds.", *((uint8_t *)event_data));
                break;
            case RMAKER_EVENT_WIFI_RESET:
                ESP_LOGI(TAG, "Wi-Fi credentials reset.");
                break;
            case RMAKER_EVENT_FACTORY_RESET:
                ESP_LOGI(TAG, "Node reset to factory defaults.");
                break;
            case RMAKER_MQTT_EVENT_CONNECTED:
                ESP_LOGI(TAG, "MQTT Connected.");
                break;
            case RMAKER_MQTT_EVENT_DISCONNECTED:
                ESP_LOGI(TAG, "MQTT Disconnected.");
                break;
            case RMAKER_MQTT_EVENT_PUBLISHED:
                ESP_LOGI(TAG, "MQTT Published. Msg id: %d.", *((int *)event_data));
                break;
            default:
                ESP_LOGW(TAG, "Unhandled RainMaker Common Event: %"PRIi32, event_id);
        }
    } else if (event_base == APP_WIFI_EVENT) {
        switch (event_id) {
            case APP_WIFI_EVENT_QR_DISPLAY:
                ESP_LOGI(TAG, "Provisioning QR : %s", (char *)event_data);
                break;
            case APP_WIFI_EVENT_PROV_TIMEOUT:
                ESP_LOGI(TAG, "Provisioning Timed Out. Please reboot.");
                break;
            case APP_WIFI_EVENT_PROV_RESTART:
                ESP_LOGI(TAG, "Provisioning has restarted due to failures.");
                break;
            default:
                ESP_LOGW(TAG, "Unhandled App Wi-Fi Event: %"PRIi32, event_id);
                break;
        }
    } else if (event_base == RMAKER_OTA_EVENT) {
         switch(event_id) {
            case RMAKER_OTA_EVENT_STARTING:
                ESP_LOGI(TAG, "Starting OTA.");
                break;
            case RMAKER_OTA_EVENT_IN_PROGRESS:
                ESP_LOGI(TAG, "OTA is in progress.");
                break;
            case RMAKER_OTA_EVENT_SUCCESSFUL:
                ESP_LOGI(TAG, "OTA successful.");
                break;
            case RMAKER_OTA_EVENT_FAILED:
                ESP_LOGI(TAG, "OTA Failed.");
                break;
            case RMAKER_OTA_EVENT_REJECTED:
                ESP_LOGI(TAG, "OTA Rejected.");
                break;
            case RMAKER_OTA_EVENT_DELAYED:
                ESP_LOGI(TAG, "OTA Delayed.");
                break;
            case RMAKER_OTA_EVENT_REQ_FOR_REBOOT:
                ESP_LOGI(TAG, "Firmware image downloaded. Please reboot your device to apply the upgrade.");
                break;
            default:
                ESP_LOGW(TAG, "Unhandled OTA Event: %"PRIi32, event_id);
                break;
        }
    } else {
        ESP_LOGW(TAG, "Invalid event received!");
    }
}

void app_main()
{
    // Initialize random number generator
    srand(time(NULL));

    // Initialize Application specific hardware drivers and set initial state
    esp_rmaker_console_init();
    app_driver_init();
    app_driver_set_state(DEFAULT_POWER);

    // Initialize NVS
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);

    // Initialize Wi-Fi
    app_wifi_init();

    // Register an event handler to catch RainMaker events
    ESP_ERROR_CHECK(esp_event_handler_register(RMAKER_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(RMAKER_COMMON_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(APP_WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(RMAKER_OTA_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));

    // Initialize the ESP RainMaker Agent
    esp_rmaker_config_t rainmaker_cfg = {
        .enable_time_sync = false,
    };
    esp_rmaker_node_t *node = esp_rmaker_node_init(&rainmaker_cfg, "ESP RainMaker Device", "Switch");
    if (!node) {
        ESP_LOGE(TAG, "Could not initialise node. Aborting!!!");
        vTaskDelay(5000/portTICK_PERIOD_MS);
        abort();
    }

    // Create the original Switch devices
    switch_device = esp_rmaker_device_create("Switch", ESP_RMAKER_DEVICE_SWITCH, NULL);
    switch_device1 = esp_rmaker_device_create("Switch1", ESP_RMAKER_DEVICE_SWITCH, NULL);
    switch_device2 = esp_rmaker_device_create("Switch2", ESP_RMAKER_DEVICE_SWITCH, NULL);
    switch_device3 = esp_rmaker_device_create("Switch3", ESP_RMAKER_DEVICE_SWITCH, NULL);

    // Add the write callback for the original devices
    esp_rmaker_device_add_cb(switch_device, write_cb, NULL);
    esp_rmaker_device_add_cb(switch_device1, write_cb, NULL);
    esp_rmaker_device_add_cb(switch_device2, write_cb, NULL);
    esp_rmaker_device_add_cb(switch_device3, write_cb, NULL);

    // Add standard parameters for the original switch devices
    esp_rmaker_device_add_param(switch_device, esp_rmaker_name_param_create(ESP_RMAKER_DEF_NAME_PARAM, "Switch"));
    esp_rmaker_device_add_param(switch_device1, esp_rmaker_name_param_create(ESP_RMAKER_DEF_NAME_PARAM, "Switch1"));
    esp_rmaker_device_add_param(switch_device2, esp_rmaker_name_param_create(ESP_RMAKER_DEF_NAME_PARAM, "Switch2"));
    esp_rmaker_device_add_param(switch_device3, esp_rmaker_name_param_create(ESP_RMAKER_DEF_NAME_PARAM, "Switch3"));

    esp_rmaker_param_t *power_param = esp_rmaker_power_param_create(ESP_RMAKER_DEF_POWER_NAME, DEFAULT_POWER);
    esp_rmaker_param_t *power_param1 = esp_rmaker_power_param_create(ESP_RMAKER_DEF_POWER_NAME, DEFAULT_POWER);
    esp_rmaker_param_t *power_param2 = esp_rmaker_power_param_create(ESP_RMAKER_DEF_POWER_NAME, DEFAULT_POWER);
    esp_rmaker_param_t *power_param3 = esp_rmaker_power_param_create(ESP_RMAKER_DEF_POWER_NAME, DEFAULT_POWER);

    esp_rmaker_device_add_param(switch_device, power_param);
    esp_rmaker_device_add_param(switch_device1, power_param1);
    esp_rmaker_device_add_param(switch_device2, power_param2);
    esp_rmaker_device_add_param(switch_device3, power_param3);

    esp_rmaker_device_assign_primary_param(switch_device, power_param);
    esp_rmaker_device_assign_primary_param(switch_device1, power_param1);
    esp_rmaker_device_assign_primary_param(switch_device2, power_param2);
    esp_rmaker_device_assign_primary_param(switch_device3, power_param3);

    // Create the Dispense device
    dispense_device = esp_rmaker_device_create("Dispense", ESP_RMAKER_DEVICE_OTHER, NULL);
    esp_rmaker_device_add_cb(dispense_device, rgb_write_cb, NULL);
    esp_rmaker_device_add_param(dispense_device, esp_rmaker_name_param_create(ESP_RMAKER_DEF_NAME_PARAM, "Dispense"));
    esp_rmaker_device_add_param(dispense_device, esp_rmaker_other_param_create("Dispense Value", esp_rmaker_int(0)));

    // Add all devices to the node
    esp_rmaker_node_add_device(node, switch_device);
    esp_rmaker_node_add_device(node, switch_device1);
    esp_rmaker_node_add_device(node, switch_device2);
    esp_rmaker_node_add_device(node, switch_device3);
    esp_rmaker_node_add_device(node, dispense_device);

    // Enable OTA
    esp_rmaker_ota_enable_default();

    // Enable timezone service
    esp_rmaker_timezone_service_enable();

    // Enable scheduling
    esp_rmaker_schedule_enable();

    // Enable Scenes
    esp_rmaker_scenes_enable();

    // Enable Insights
    app_insights_enable();

    // Start the ESP RainMaker Agent
    esp_rmaker_start();

    // Set custom manufacturer data
    err = app_wifi_set_custom_mfg_data(MGF_DATA_DEVICE_TYPE_SWITCH, MFG_DATA_DEVICE_SUBTYPE_SWITCH);

    // Start Wi-Fi
    err = app_wifi_start(POP_TYPE_RANDOM);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Could not start Wifi. Aborting!!!");
        vTaskDelay(5000/portTICK_PERIOD_MS);
        abort();
    }
}
