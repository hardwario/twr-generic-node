#ifndef _SENSOR_H
#define _SENSOR_H

#include <twr.h>

#ifndef SENSOR_POOL_LENGTH
#define SENSOR_POOL_LENGTH 10
#endif

#define TEMPERATURE_PUB_NO_CHANGE_INTERVAL (15 * 60 * 1000)
#define TEMPERATURE_PUB_VALUE_CHANGE 2.f
#define TEMPERATURE_UPDATE_INTERVAL (5 * 1000)

#define HYGROMETER_PUB_NO_CHANGE_INTERVAL (15 * 60 * 1000)
#define HYGROMETER_PUB_VALUE_CHANGE 5.0f
#define HYGROMETER_UPDATE_INTERVAL (5 * 1000)

#define LUX_METER_PUB_NO_CHANGE_INTERVAL (15 * 60 * 1000)
#define LUX_METER_PUB_VALUE_CHANGE 50.0f
#define LUX_METER_UPDATE_INTERVAL (10 * 1000)

#define BAROMETER_PUB_NO_CHANGE_INTERVAL (15 * 60 * 1000)
#define BAROMETER_PUB_VALUE_CHANGE 50.0f
#define BAROMETER_UPDATE_INTERVAL (1 * 60 * 1000)

#define VOC_PUB_NO_CHANGE_INTERVAL (15 * 60 * 1000)
#define VOC_PUB_VALUE_CHANGE 50.0f
#define VOC_UPDATE_INTERVAL (30 * 1000)


typedef enum
{
    SENSOR_TYPE_TMP112,
    SENSOR_TYPE_OPT3001,
    SENSOR_TYPE_MPL3115A2,
    SENSOR_TYPE_HTS221,
    SENSOR_TYPE_HDC2080,
    SENSOR_TYPE_SHT20,
    SENSOR_TYPE_SHT30,
    SENSOR_TYPE_SGP30,
    SENSOR_TYPE_SGPC3

} sensor_type_t;

typedef struct
{
    const sensor_type_t type;
    const twr_i2c_channel_t i2c;
    const uint8_t address;
    const uint8_t channel;

} sensor_attr_t;

typedef struct sensor_t {
    const sensor_attr_t *attr;
    float value;
    twr_tick_t next_pub;
    union
    {
        twr_tmp112_t tmp112; // thermomether
        twr_opt3001_t opt3001; // lux_meter
        twr_mpl3115a2_t mpl3115a2; // barometer
        twr_hts221_t hts221;   // hygrometer
        twr_hdc2080_t hdc2080; // hygrometer
        twr_sht20_t sht20;     // hygrometer
        twr_sht30_t sht30;     // hygrometer
        twr_sgp30_t sgp30; // VOC
        twr_sgpc3_t sgpc3; // VOC LP
    } instance;

} sensor_t;

typedef enum
{
    SENSORS_EVENT_SENSOR_PUB

} sensors_event_t;

void sensors_init(void);
void sensors_scan(void);
void sensors_set_event_handler(void (*event_handler)(sensors_event_t, sensor_t *, void *), void *event_param);
#endif
