#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_wps.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_event_loop.h"
#include "nvs_flash.h"
#include "driver/gpio.h"

#define WPS_TIMEOUT 120

#define CONNECTED_BIT BIT0
#define WPS_ENABLED_BIT BIT15

static EventGroupHandle_t wifi_event_group;

esp_err_t event_handler(void *ctx, system_event_t *event)
{
    switch(event->event_id) {
    case SYSTEM_EVENT_STA_GOT_IP:
        xEventGroupSetBits(wifi_event_group, CONNECTED_BIT);
        break;
    case SYSTEM_EVENT_STA_START:
        esp_wifi_connect();
        break;
    case SYSTEM_EVENT_STA_WPS_ER_SUCCESS:
    case SYSTEM_EVENT_STA_WPS_ER_FAILED:
    case SYSTEM_EVENT_STA_WPS_ER_TIMEOUT:
        ESP_ERROR_CHECK( esp_wifi_wps_disable());
        xEventGroupClearBits(wifi_event_group, WPS_ENABLED_BIT);
        esp_wifi_connect();
        break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
        xEventGroupClearBits(wifi_event_group, CONNECTED_BIT);
        if ( ! ( xEventGroupGetBits(wifi_event_group) & WPS_ENABLED_BIT ) ) {
            esp_wifi_connect();
        }
        break;
    default:
        break;
    }
    return ESP_OK;
}

void start_wps()
{
    xEventGroupSetBits(wifi_event_group, WPS_ENABLED_BIT);
    ESP_ERROR_CHECK( esp_wifi_disconnect());
    ESP_ERROR_CHECK( esp_wifi_wps_enable(WPS_TYPE_PBC));
    ESP_ERROR_CHECK( esp_wifi_wps_start(WPS_TIMEOUT));
}

void app_main(void)
{
    wifi_event_group = xEventGroupCreate();

    nvs_flash_init();
    tcpip_adapter_init();
    ESP_ERROR_CHECK( esp_event_loop_init(event_handler, NULL) );
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK( esp_wifi_init(&cfg) );
    ESP_ERROR_CHECK( esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK( esp_wifi_start() );

    gpio_set_pull_mode(GPIO_NUM_0, GPIO_PULLUP_ONLY);
    gpio_set_direction(GPIO_NUM_0, GPIO_MODE_INPUT);

    gpio_set_direction(GPIO_NUM_4, GPIO_MODE_OUTPUT);
    int level = 0;
    while (true) {
        gpio_set_level(GPIO_NUM_4, level);
        level = !level;
        vTaskDelay(300 / portTICK_PERIOD_MS);

        if ( gpio_get_level(GPIO_NUM_0) == 0 ) {
            start_wps();
        }
    }
}

