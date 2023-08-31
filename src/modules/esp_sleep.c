#include <stdio.h>
#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/pm/pm.h>
#include <zephyr/pm/device.h>
#include <../../../zephyr/drivers/modem/modem_context.h>
#include <../../../zephyr/drivers/modem/modem_cmd_handler.h>
#include <../../../zephyr/drivers/wifi/esp_at/esp.h>

#include "esp_sleep.h"

#define GPIO_NODE 			DT_NODELABEL(gpio0)
#define SLEEPOUT_PIN DT_ALIAS(esp_at_sleep)
#define UART_NODE DT_NODELABEL(uart1)

LOG_MODULE_DECLARE(location_module, CONFIG_LOCATION_LOG_LEVEL);

const static struct device *gpio_dev;
static struct gpio_dt_spec sleepout_pin = GPIO_DT_SPEC_GET(SLEEPOUT_PIN, gpios);

static struct esp_data *data;
const static struct device *wifi_dev;
const static struct device *uart_dev;

static bool ready = false;
//Priority 79 is one less than the CONFIG_WIFI_INIT_PRIORITY which is 80
SYS_INIT(esp_sleep_init_pins, POST_KERNEL, 79);
int esp_sleep_init_pins(const struct device *dev){
    gpio_dev = DEVICE_DT_GET(GPIO_NODE);
	if (!gpio_dev) {
		LOG_ERR("Error getting GPIO device binding");
		return -ENODEV;
	}
    int ret = gpio_pin_configure_dt(&sleepout_pin, GPIO_OUTPUT_HIGH);
    if (ret != 0){
        LOG_ERR("Failed to configure output sleep pin: %d", ret);
        return -EIO;
    }
    k_sleep(K_MSEC(100));
    return 0;  
}
int esp_sleep_init_device(void){
    uart_dev = DEVICE_DT_GET(UART_NODE);
    if (!uart_dev) {
 		LOG_ERR("Error getting UART device binding");
		return -ENODEV;       
    }

    wifi_dev = DEVICE_DT_GET(DT_CHOSEN(ncs_location_wifi));
    if (!device_is_ready(wifi_dev)){
        LOG_ERR("Wi-Fi device %s not ready for sleep activation", wifi_dev->name);
        return -ENODEV;
    }
    data = wifi_dev->data;

    char cmd[sizeof("AT+SLEEPWKCFG=2,x,1") + 1];
    snprintk(cmd, sizeof(cmd), "AT+SLEEPWKCFG=2,%d,1", CONFIG_WIFI_ESP_AT_WAKEUP_PIN);
    int ret = esp_cmd_send(data, NULL, 0, cmd, ESP_CMD_TIMEOUT); 
    if (ret != 0){
        LOG_ERR("Failed to set sleep wakeup configuration pin");
        return ret;
    }
    // char cmd[sizeof("AT+SLEEP=x") + 1];
    snprintk(cmd, sizeof(cmd), "AT+SLEEP=%d", CONFIG_WIFI_ESP_AT_SLEEP_MODE);
    ret = esp_cmd_send(data, NULL, 0, cmd, ESP_CMD_TIMEOUT);
    if (ret != 0){
        LOG_ERR("Failed to configure esp sleep mode");
        return ret;
    }
    ready = true;
    return 0;
}
void esp_sleep_wakeup(void){
    if (!ready) return;
    enum pm_device_state state;
    pm_device_state_get(uart_dev, &state);
    if (state != PM_DEVICE_STATE_ACTIVE) {
        LOG_DBG("Resuming UART Device....");
        pm_device_action_run(uart_dev, PM_DEVICE_ACTION_RESUME);
    }
    LOG_DBG("Waking up esp device...");
    gpio_pin_set_dt(&sleepout_pin, 1);
    k_sleep(K_MSEC(100));
}

void esp_sleep_sleep(void){
    if (!ready) return;
    LOG_DBG("Putting esp device to sleep...");
    pm_device_action_run(uart_dev, PM_DEVICE_ACTION_SUSPEND);
    gpio_pin_set_dt(&sleepout_pin, 0);
}