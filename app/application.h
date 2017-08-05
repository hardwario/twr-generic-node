#ifndef _APPLICATION_H
#define _APPLICATION_H

#include <bc_common.h>
#include <bcl.h>

typedef struct
{
    uint8_t number;
    float value;
    bc_tick_t next_pub;

} event_param_t;

typedef struct
{
    bc_tag_temperature_t self;
    event_param_t param;

} temperature_tag_t;

typedef struct
{
    bc_tag_humidity_t self;
    event_param_t param;

} humidity_tag_t;

typedef struct
{
    bc_tag_lux_meter_t self;
    event_param_t param;

} lux_meter_tag_t;


#endif
