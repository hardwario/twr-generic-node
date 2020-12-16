#ifndef _VALUES_H
#define _VALUES_H

#include <twr_common.h>

typedef struct
{
    float_t temperature;
    float_t humidity;
    float_t illuminance;
    float_t pressure;
    float_t altitude;
    float_t co2_concentation;
    float_t battery_voltage;
    float_t battery_pct;
    uint16_t voc;

} values_t;

extern values_t values;

#endif
