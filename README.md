nRF9160: Asset Tracker v2 For Actinius Icarus 
=========================
This is a modified version of of the Asset Tracker V2 software adapted for use on the Actinius Icarus with sleep support for ESP wifi devices and Accelerometer motion support for passive mode.

# Building
Requires nRF Connect SDK v2.3.0 to build, I used VSCode with the nRF Connect Extension suites for it. This repo assumes you already know how to build nrf firmware using the board configuration files and overlays.

# Wifi Support
This modified firmware supports putting the esp device to light sleep for significant power savings should wifi be used.
## Configuration 
`overlay-esp-wifi.conf` contains the configuration overlays for using an esp-at based device for wifi scanning. The uart pins are required to be defined by the hardware board overlays found in the boards folder. For the Actinius_icarus_ns.overlay the uart pins for the esp wifi are defined as 23 and 24 respectively.

The file `overlay-esp-wifi-sleep.conf` contains the extra configurations used to enable the sleeping features for the esp device. Only light sleep support is available for the esp device. `CONFIG_WIFI_ESP_AT_WAKEUP_PIN` is the GPIO pin on the ESP device that is configured as the input to control when to wakeup the device. It is passed to the ESP device via the command 

`overlay-hologram-pdn.conf` is added in this repo for convenient configuration for usage with hologram SIMs.

`overlay-debug.conf` enables debugging to the serial output, useful for getting diagnostics.

`CONFIG_WIFI_ESP_AT_SLEEP_MODE` is the sleep mode passed to the ESP device via the AT+SLEEP command. It should be set to 2 for light sleep activity.
Additionally the GPIO pin needs to be defined on the host nrf9160 to pair with the ESP wakeup pin, it's defined in the overlay file for this firmware. 
# LIS2DH Support for Passive mode
This version of the Asset Tracker firmware supports usage of the LIS2DH Accelerometer for Passive mode, temperature sensing is not supported on the due to the fact the temperature sensor on the accelerometer is relative to some unknown value and not absolute.
## Configuration
`CONFIG_LIS2DH`, and `CONFIG_EXTERNAL_SENSORS`, `CONFIG_LIS2DH_ACCEL_RANGE_XG` and `CONFIG_LIS2DH_ODR_X` need to be enabled and set accordingly for enabling passive mode support. 
## Usage
Because the LIS2DH is not one-to-one identical the ADXL362 some device configuration values are treated differently

`accThreshAct` is the threshold value in m/s<sup>2</sup> the measured movement must exceed before the accelerometer can trigger the device to upload location data. This value configures the ACT_THS register in the accelerometer.

`accTimeoutInact` is the time value (in seconds) that, once the measured motion exceeds `accThresAct`, the motion must maintain passing the threshold for this time value before the a trigger occurs. This value configures the ACT_DUR register in the accelerometer.

`accThresInact` is a tunable time value (in seconds) that the is used to prevent the device from being flooded with interrupts after the initial accelerometer trigger - the trigger is disabled by this value in seconds after the initial trigger before it is renabled again.

