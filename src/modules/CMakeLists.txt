#
# Copyright (c) 2021 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

target_include_directories(app PRIVATE .)
target_sources(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/modules_common.c)
target_sources_ifdef(CONFIG_CLOUD_MODULE app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/cloud_module.c)
target_sources_ifdef(CONFIG_MODEM_MODULE app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/modem_module.c)
target_sources_ifdef(CONFIG_LOCATION_MODULE app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/location_module.c)
target_sources_ifdef(CONFIG_UI_MODULE app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/ui_module.c)
target_sources_ifdef(CONFIG_SENSOR_MODULE app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/sensor_module.c)
target_sources_ifdef(CONFIG_DATA_MODULE app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/data_module.c)
target_sources_ifdef(CONFIG_UTIL_MODULE app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/util_module.c)
target_sources_ifdef(CONFIG_LED_CONTROL app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/led_module.c)
target_sources_ifdef(CONFIG_DEBUG_MODULE app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/debug_module.c)
target_sources_ifdef(CONFIG_WIFI_SLEEP app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/esp_sleep.c)
target_sources_ifdef(CONFIG_ADC_BATTERY app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/battery.c)
zephyr_include_directories($ENV{ZEPHYR_BASE}/drivers/modem)