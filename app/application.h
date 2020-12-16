#ifndef _APPLICATION_H
#define _APPLICATION_H

#ifndef FIRMWARE
#define FIRMWARE "generic-node"
#endif

#ifndef VERSION
#define VERSION "vdev"
#endif

#include <bcl.h>
#include <twr.h>

typedef struct
{
    uint8_t channel;
    float value;
    bc_tick_t next_pub;

} event_param_t;

#endif
