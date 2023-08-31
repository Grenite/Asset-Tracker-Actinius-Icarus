#include <zephyr/kernel.h>
#include <stdio.h>
#include <string.h>
#include <zephyr/drivers/sensor.h>
#include <stdlib.h>
#include <math.h>

#include "ext_sensors.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(ext_sensors, CONFIG_EXTERNAL_SENSORS_LOG_LEVEL);

/*Refer to zephyr/drivers/sensor/lis2dh for further information on sensor attribute usage*/
#if IS_ENABLED(CONFIG_LIS2DH_ACCEL_RANGE_2G)
#define ACCEL_RANGE_MAX_M_S2 19.6133
#elif IS_ENABLED(CONFIG_LIS2DH_ACCEL_RANGE_4G)
#define ACCEL_RANGE_MAX_M_S2 39.2266
#elif IS_ENABLED(cONFIG_LIS2DH_ACCEL_RANGE_8G)
#define ACCEL_RANGE_MAX_M_S2 78.4532
#elif IS_ENABLED(cONFIG_LIS2DH_ACCEL_RANGE_16G)
#define ACCEL_RANGE_MAX_M_S2 156.9064
#endif

#define LIS2DH_ACTIVITY_MAX 127
#if IS_ENABLED(CONFIG_LIS2DH_ODR_1)
#define LIS2DH_ACTIVITY_MAX_S 127
#define LIS2DH_ODR 1
#elif IS_ENABLED(CONFIG_LIS2DH_ODR_2)
#define LIS2DH_ACTIVITY_MAX_S 12.7
#define LIS2DH_ODR 10
#elif IS_ENABLED(CONFIG_LIS2DH_ODR_3)
#define LIS2DH_ACTIVITY_MAX_S 5.08
#define LIS2DH_ODR 25
#elif IS_ENABLED(CONFIG_LIS2DH_ODR_4)
#define LIS2DH_ACTIVITY_MAX_S 2.54
#define LIS2DH_ODR 50
#elif IS_ENABLED(CONFIG_LIS2DH_ODR_5)
#define LIS2DH_ACTIVITY_MAX_S 1.27
#define LIS2DH_ODR 100
#elif IS_ENABLED(CONFIG_LIS2DH_ODR_6)
#define LIS2DH_ACTIVITY_MAX_S 0.635
#define LIS2DH_ODR 200
#elif IS_ENABLED(CONFIG_LIS2DH_ODR_7)
#define LIS2DH_ACTIVITY_MAX_S 0.3175
#define LIS2DH_ODR 400
#elif IS_ENABLED(CONFIG_LIS2DH_ODR_8)
#define LIS2DH_ACTIVITY_MAX_S 0.079375
#define LIS2DH_ODR 1600
#elif IS_ENABLED(CONFIG_LIS2DH_ODR_9_LOW)
#define LIS2DH_ACTIVITY_MAX_S 0.1016
#define LIS2DH_ODR 1250
#elif IS_ENABLED(CONFIG_LIS2DH_ODR_9_NORMAL)
#define LIS2DH_ACTIVITY_MAX_S 0.0254
#define LIS2DH_ODR 5000
#endif

struct env_sensor {
	enum sensor_channel channel;
	const struct device *dev;
	struct k_spinlock lock;
};

/** Sensor struct for the low-power accelerometer */
static struct env_sensor accel_sensor = {
	.channel = SENSOR_CHAN_ACCEL_XYZ,
	.dev = DEVICE_DT_GET_ANY(st_lis2dh),
};

static k_timeout_t trigger_wait_time; 
static ext_sensor_handler_t evt_handler;
static struct k_timer restart_timer;
static struct k_work restart_trigger_work_data;

int ext_sensors_accelerometer_trigger_callback_set(bool enable);

static void accelerometer_trigger_handler(const struct device *dev,
					  const struct sensor_trigger *trig)
{
	int err = 0;
	struct sensor_value data[ACCELEROMETER_CHANNELS];
	struct ext_sensor_evt evt = {0};

	switch (trig->type) {
	case SENSOR_TRIG_DELTA:

		if (sensor_sample_fetch(dev) < 0) {
			LOG_ERR("Sample fetch error");
			return;
		}

		err = sensor_channel_get(dev, SENSOR_CHAN_ACCEL_XYZ, &data[0]);
		if (err) {
			LOG_ERR("sensor_channel_get, error: %d", err);
			return;
		}

		evt.value_array[0] = sensor_value_to_double(&data[0]);
		evt.value_array[1] = sensor_value_to_double(&data[1]);
		evt.value_array[2] = sensor_value_to_double(&data[2]);

		LOG_DBG("Activity detected - Acceleration Values: x %lf y %lf z %lf", evt.value_array[0], evt.value_array[1], evt.value_array[2]);
        ext_sensors_accelerometer_trigger_callback_set(false);
        k_timer_start(&restart_timer, trigger_wait_time, K_NO_WAIT);

		evt_handler(&evt);
		break;
	default:
		LOG_ERR("Unknown trigger");
	}
}

void restart_acceleromter_trigger_work_handler(struct k_work *work){
	int rc = ext_sensors_accelerometer_trigger_callback_set(true);
	if (rc != 0) {
		printf("Failed to reset trigger: %d\n", rc);
		return;
	}
}

void restart_accelerometer_trigger_ISR(struct k_timer *timer_id){
	k_work_submit(&restart_trigger_work_data);
}

int ext_sensors_init(ext_sensor_handler_t handler)
{
	struct ext_sensor_evt evt = {0};

	if (handler == NULL) {
		LOG_ERR("External sensor handler NULL!");
		return -EINVAL;
	}
	trigger_wait_time = K_MSEC(1000/LIS2DH_ODR);
	evt_handler = handler;
    k_work_init(&restart_trigger_work_data, restart_acceleromter_trigger_work_handler);
    k_timer_init(&restart_timer, restart_accelerometer_trigger_ISR, NULL);
	if (!device_is_ready(accel_sensor.dev)) {
		LOG_ERR("Low-power accelerometer device is not ready");
		evt.type = EXT_SENSOR_EVT_ACCELEROMETER_ERROR;
		evt_handler(&evt);
	}
	return 0;
}
/* Temperature readings from LIS2DH are changes to some reference, not sure what that is */
int ext_sensors_temperature_get(double *ext_temp)
{
	int err;
	struct sensor_value data = {0};
	struct ext_sensor_evt evt = {0};


	err = sensor_sample_fetch(accel_sensor.dev);
	if (err) {
		LOG_ERR("Failed to fetch data from %s, error: %d",
			accel_sensor.dev->name, err);
		evt.type = EXT_SENSOR_EVT_TEMPERATURE_ERROR;
		evt_handler(&evt);
		return -ENODATA;
	}

	err = sensor_channel_get(accel_sensor.dev, SENSOR_CHAN_DIE_TEMP, &data);
	if (err) {
		LOG_ERR("Failed to fetch data from %s, error: %d",
			accel_sensor.dev->name, err);
		evt.type = EXT_SENSOR_EVT_TEMPERATURE_ERROR;
		evt_handler(&evt);
		return -ENODATA;
	}

	k_spinlock_key_t key = k_spin_lock(&(accel_sensor.lock));
	*ext_temp = sensor_value_to_double(&data);
	k_spin_unlock(&(accel_sensor.lock), key);

	return 0;
}

int ext_sensors_humidity_get(double *ext_hum)
{
    LOG_ERR("Humidity Sensor Not Available");
	return -ENOTSUP;
}

int ext_sensors_pressure_get(double *ext_press)
{
    LOG_ERR("Pressure Sensor Not Available");
    return -ENOTSUP;
}

int ext_sensors_air_quality_get(uint16_t *ext_bsec_air_quality)
{
	LOG_ERR("Air Quality Sensor Not Available");
	return -ENOTSUP;
}

/* Inactivty threshold will be used to set the wait time before trigger can be re-enabled after firing*/
int ext_sensors_accelerometer_threshold_set(double threshold, bool upper)
{
	if (!upper){
		trigger_wait_time = K_MSEC(1000*threshold);
		return 0;
	}
	int err;
	if (threshold > ACCEL_RANGE_MAX_M_S2){
		LOG_WRN("Threshold %lf exceeds maximum %lf, capping it", threshold, ACCEL_RANGE_MAX_M_S2);
		threshold = ACCEL_RANGE_MAX_M_S2;
	}
	if (threshold < 0){
		LOG_WRN("Threshold %lf cannot be less than 0, limiting it to 0", threshold);
		threshold = 0;
	}
	threshold *= 1000000;
	const struct sensor_value data = {
		.val1 = (unsigned long)threshold/1000000,
		.val2 = (unsigned long)threshold % 1000000
	};


	err = sensor_attr_set(accel_sensor.dev, SENSOR_CHAN_ACCEL_XYZ, SENSOR_ATTR_SLOPE_TH, &data);
	if (err) {
		LOG_ERR("Failed to set accelerometer threshold value");
		LOG_ERR("Device: %s, error: %d",
			accel_sensor.dev->name, err);
		return err;
	}
	return 0;
}

/* LIS2DH doesn't have an inactivity value, but instead has an ACT_DUR register
*  ACT_DUR determines how long above threshold before an interrupt is triggered
*  Duration in seconds dependent on ODR, 7-bit balue
*/
int ext_sensors_inactivity_timeout_set(double duration)
{
	int err, inact_time_decimal;

	if (duration > LIS2DH_ACTIVITY_MAX_S || duration < 0) {
		LOG_ERR("Invalid timeout value");
		return -ENOTSUP;
	}

	duration = LIS2DH_ODR*duration;
	inact_time_decimal = (int) (duration + 0.5);
	inact_time_decimal = MIN(inact_time_decimal, LIS2DH_ACTIVITY_MAX);
	inact_time_decimal = MAX(inact_time_decimal, 0);
	LOG_DBG("Decimal Value for act duration %d", inact_time_decimal);
	const struct sensor_value data = {
		.val1 = inact_time_decimal
	};

	err = sensor_attr_set(accel_sensor.dev, SENSOR_CHAN_ACCEL_XYZ, SENSOR_ATTR_SLOPE_DUR, &data);
	if (err) {
		LOG_ERR("Failed to set accelerometer inactivity timeout value");
		LOG_ERR("Device: %s, error: %d", accel_sensor.dev->name, err);
		return err;
	}
	return 0;
}


int ext_sensors_accelerometer_trigger_callback_set(bool enable)
{
	int err;
	struct sensor_trigger trig = {
		.chan = SENSOR_CHAN_ACCEL_XYZ,
		.type = SENSOR_TRIG_DELTA
	};
	struct ext_sensor_evt evt = {0};

	sensor_trigger_handler_t handler = enable ? accelerometer_trigger_handler : NULL;

	err = sensor_trigger_set(accel_sensor.dev, &trig, handler);
	if (err) {
        LOG_ERR("Could not set trigger for device %s, error: %d",
            accel_sensor.dev->name, err);
        evt.type = EXT_SENSOR_EVT_ACCELEROMETER_ERROR;
        evt_handler(&evt);
        return err;
	}
    return 0;
}
