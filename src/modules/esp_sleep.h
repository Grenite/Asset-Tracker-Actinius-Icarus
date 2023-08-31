#ifndef ESP_SLEEP_H
#define ESP_SLEEP_H

int esp_sleep_init_pins(const struct device *dev);
int esp_sleep_init_device(void);
void esp_sleep_wakeup(void);
void esp_sleep_sleep(void);

#endif