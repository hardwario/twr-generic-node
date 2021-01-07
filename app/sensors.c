#include <sensors.h>
#include <twr_log.h>
#include <values.h>

#define SENSORS_ALL_COUNT (sizeof(_sensors_all) / sizeof(sensor_attr_t))

static const sensor_attr_t _sensors_all[] = {
    // Tag Barometer
    {SENSOR_TYPE_MPL3115A2, TWR_I2C_I2C0, 0x60, TWR_RADIO_PUB_CHANNEL_R1_I2C0_ADDRESS_DEFAULT}, // Climate Module
    {SENSOR_TYPE_MPL3115A2, TWR_I2C_I2C1, 0x60, TWR_RADIO_PUB_CHANNEL_R1_I2C1_ADDRESS_DEFAULT},
    // Tag Temperature
    {SENSOR_TYPE_TMP112, TWR_I2C_I2C0, 0x48, TWR_RADIO_PUB_CHANNEL_R1_I2C0_ADDRESS_DEFAULT}, // Climate Module R1
    {SENSOR_TYPE_TMP112, TWR_I2C_I2C1, 0x48, TWR_RADIO_PUB_CHANNEL_R1_I2C1_ADDRESS_DEFAULT},
    {SENSOR_TYPE_TMP112, TWR_I2C_I2C0, 0x49, TWR_RADIO_PUB_CHANNEL_R1_I2C0_ADDRESS_ALTERNATE}, // Core Thermomether
    {SENSOR_TYPE_TMP112, TWR_I2C_I2C1, 0x49, TWR_RADIO_PUB_CHANNEL_R1_I2C1_ADDRESS_ALTERNATE},
    // Tag Lux Meter
    {SENSOR_TYPE_OPT3001, TWR_I2C_I2C0, 0x44, TWR_RADIO_PUB_CHANNEL_R1_I2C0_ADDRESS_DEFAULT}, // Climate Module
    {SENSOR_TYPE_OPT3001, TWR_I2C_I2C1, 0x44, TWR_RADIO_PUB_CHANNEL_R1_I2C1_ADDRESS_DEFAULT},
    {SENSOR_TYPE_OPT3001, TWR_I2C_I2C0, 0x45, TWR_RADIO_PUB_CHANNEL_R1_I2C0_ADDRESS_ALTERNATE},
    {SENSOR_TYPE_OPT3001, TWR_I2C_I2C1, 0x45, TWR_RADIO_PUB_CHANNEL_R1_I2C1_ADDRESS_ALTERNATE},
    // Tag Humidity R1
    {SENSOR_TYPE_HTS221, TWR_I2C_I2C0, 0x5f, TWR_RADIO_PUB_CHANNEL_R1_I2C0_ADDRESS_DEFAULT},
    {SENSOR_TYPE_HTS221, TWR_I2C_I2C1, 0x5f, TWR_RADIO_PUB_CHANNEL_R1_I2C1_ADDRESS_DEFAULT},
    // Tag Humidity R2
    {SENSOR_TYPE_HDC2080, TWR_I2C_I2C0, 0x40, TWR_RADIO_PUB_CHANNEL_R2_I2C0_ADDRESS_DEFAULT},
    {SENSOR_TYPE_HDC2080, TWR_I2C_I2C1, 0x40, TWR_RADIO_PUB_CHANNEL_R2_I2C1_ADDRESS_DEFAULT},
    {SENSOR_TYPE_HDC2080, TWR_I2C_I2C0, 0x41, TWR_RADIO_PUB_CHANNEL_R2_I2C0_ADDRESS_ALTERNATE},
    {SENSOR_TYPE_HDC2080, TWR_I2C_I2C1, 0x41, TWR_RADIO_PUB_CHANNEL_R2_I2C1_ADDRESS_ALTERNATE},
    // Tag Humidity R3
    {SENSOR_TYPE_SHT20, TWR_I2C_I2C0, 0x40, TWR_RADIO_PUB_CHANNEL_R3_I2C0_ADDRESS_DEFAULT}, // Climate Module R1
    {SENSOR_TYPE_SHT20, TWR_I2C_I2C1, 0x40, TWR_RADIO_PUB_CHANNEL_R3_I2C1_ADDRESS_DEFAULT},
    // Tag Humidity R4
    {SENSOR_TYPE_SHT30, TWR_I2C_I2C0, 0x44, TWR_RADIO_PUB_CHANNEL_R4_I2C0_ADDRESS_DEFAULT},
    {SENSOR_TYPE_SHT30, TWR_I2C_I2C1, 0x44, TWR_RADIO_PUB_CHANNEL_R4_I2C1_ADDRESS_DEFAULT},
    {SENSOR_TYPE_SHT30, TWR_I2C_I2C0, 0x45, TWR_RADIO_PUB_CHANNEL_R4_I2C0_ADDRESS_ALTERNATE}, // Climate Module R2
    {SENSOR_TYPE_SHT30, TWR_I2C_I2C1, 0x45, TWR_RADIO_PUB_CHANNEL_R4_I2C1_ADDRESS_ALTERNATE},
    // Tag VOC
    {SENSOR_TYPE_SGP30, TWR_I2C_I2C0, 0x58, TWR_RADIO_PUB_CHANNEL_R1_I2C0_ADDRESS_DEFAULT},
    {SENSOR_TYPE_SGP30, TWR_I2C_I2C1, 0x58, TWR_RADIO_PUB_CHANNEL_R1_I2C1_ADDRESS_DEFAULT},
    // Tag VOC LP
    {SENSOR_TYPE_SGPC3, TWR_I2C_I2C0, 0x58, TWR_RADIO_PUB_CHANNEL_R1_I2C0_ADDRESS_DEFAULT},
    {SENSOR_TYPE_SGPC3, TWR_I2C_I2C1, 0x58, TWR_RADIO_PUB_CHANNEL_R1_I2C1_ADDRESS_DEFAULT},
};

static struct
{
    sensor_t pool[SENSOR_POOL_LENGTH];
    uint8_t scan_index;

    void (*event_handler)(sensors_event_t, sensor_t *, void *);
    void *event_param;

} _sensors;

static bool _sensor_try_next(sensor_t *ctx);
static bool _sensor_alloc(const sensor_attr_t *attr);
static void _sensor_dealloc(sensor_t *ctx);
static void _sensor_measure(sensor_t *ctx);
static void _sensor_tmp112_event_handler(twr_tmp112_t *self, twr_tmp112_event_t event, void *event_param);
static void _sensor_hts221_event_handler(twr_hts221_t *self, twr_hts221_event_t event, void *event_param);
static void _sensor_hdc2080_event_handler(twr_hdc2080_t *child, twr_hdc2080_event_t event, void *event_param);
static void _sensor_sht20_event_handler(twr_sht20_t *self, twr_sht20_event_t event, void *event_param);
static void _sensor_sht30_event_handler(twr_sht30_t *self, twr_sht30_event_t event, void *event_param);
static void _sensor_opt3001_event_handler(twr_opt3001_t *self, twr_opt3001_event_t event, void *event_param);
static void _sensor_mpl3115a2_event_handler(twr_mpl3115a2_t *self, twr_mpl3115a2_event_t event, void *event_param);
static void _sensor_sgp30_event_handler(twr_sgp30_t *self, twr_sgp30_event_t event, void *event_param);
static void _sensor_sgpc3_event_handler(twr_sgpc3_t *self, twr_sgpc3_event_t event, void *event_param);

void sensors_init(void)
{
    memset(&_sensors, 0, sizeof(_sensors));
}

void sensors_scan(void)
{
    twr_log_debug("sensors_scan");

    memset(&values, 0xff, sizeof(values));

    sensors_measure();

    _sensors.scan_index = 0;
    for (uint8_t index = 0; index < SENSOR_POOL_LENGTH; index++)
    {
        if (!_sensor_try_next(NULL))
        {
            return;
        }
    }
}

void sensors_measure(void)
{
    for (uint8_t index = 0; index < SENSOR_POOL_LENGTH; index++)
    {
        _sensors.pool[index].next_pub = 0;
        _sensor_measure(&_sensors.pool[index]);
    }
}

void sensors_set_event_handler(void (*event_handler)(sensors_event_t, sensor_t *, void *), void *event_param)
{
    _sensors.event_handler = event_handler;
    _sensors.event_param = event_param;
}

static bool _sensor_try_next(sensor_t *ctx)
{
    _sensor_dealloc(ctx);

    // Find not allocated sensor attr
    const sensor_attr_t *attr = NULL;
    do
    {
        if (_sensors.scan_index >= SENSORS_ALL_COUNT)
            return false;

        attr = &_sensors_all[_sensors.scan_index++];

        for (uint8_t index = 0; index < SENSOR_POOL_LENGTH; index++)
        {
            if (_sensors.pool[index].attr == attr)
            {
                attr = NULL;
                break;
            }
        }

    } while (attr == NULL);

    return _sensor_alloc(attr);
}

static bool _sensor_alloc(const sensor_attr_t *attr)
{
    sensor_t *ctx = NULL;

    for (uint8_t index = 0; index < SENSOR_POOL_LENGTH; index++)
    {
        if (_sensors.pool[index].attr == NULL)
        {
            ctx = &_sensors.pool[index];
            break;
        }
    }

    if (ctx == NULL)
    {
        return false;
    }

    ctx->attr = attr;
    ctx->value = NAN;
    ctx->next_pub = 0;

    switch (attr->type)
    {
    case SENSOR_TYPE_TMP112:
        twr_tmp112_init(&ctx->instance.tmp112, attr->i2c, attr->address);
        twr_tmp112_set_event_handler(&ctx->instance.tmp112, _sensor_tmp112_event_handler, ctx);
        twr_tmp112_set_update_interval(&ctx->instance.tmp112, TEMPERATURE_UPDATE_INTERVAL);
        break;
    case SENSOR_TYPE_HTS221:
        twr_hts221_init(&ctx->instance.hts221, attr->i2c, attr->address);
        twr_hts221_set_event_handler(&ctx->instance.hts221, _sensor_hts221_event_handler, ctx);
        twr_hts221_set_update_interval(&ctx->instance.hts221, HYGROMETER_UPDATE_INTERVAL);
        break;
    case SENSOR_TYPE_HDC2080:
        twr_hdc2080_init(&ctx->instance.hdc2080, attr->i2c, attr->address);
        twr_hdc2080_set_event_handler(&ctx->instance.hdc2080, _sensor_hdc2080_event_handler, ctx);
        twr_hdc2080_set_update_interval(&ctx->instance.hdc2080, HYGROMETER_UPDATE_INTERVAL);
        break;
    case SENSOR_TYPE_SHT20:
        twr_sht20_init(&ctx->instance.sht20, attr->i2c, attr->address);
        twr_sht20_set_event_handler(&ctx->instance.sht20, _sensor_sht20_event_handler, ctx);
        twr_sht20_set_update_interval(&ctx->instance.sht20, HYGROMETER_UPDATE_INTERVAL);
        break;
    case SENSOR_TYPE_SHT30:
        twr_sht30_init(&ctx->instance.sht30, attr->i2c, attr->address);
        twr_sht30_set_event_handler(&ctx->instance.sht30, _sensor_sht30_event_handler, ctx);
        twr_sht30_set_update_interval(&ctx->instance.sht30, HYGROMETER_UPDATE_INTERVAL);
        break;
    case SENSOR_TYPE_OPT3001:
        twr_opt3001_init(&ctx->instance.opt3001, attr->i2c, attr->address);
        twr_opt3001_set_event_handler(&ctx->instance.opt3001, _sensor_opt3001_event_handler, ctx);
        twr_opt3001_set_update_interval(&ctx->instance.opt3001, LUX_METER_UPDATE_INTERVAL);
        break;
    case SENSOR_TYPE_MPL3115A2:
        twr_mpl3115a2_init(&ctx->instance.mpl3115a2, attr->i2c, attr->address);
        twr_mpl3115a2_set_event_handler(&ctx->instance.mpl3115a2, _sensor_mpl3115a2_event_handler, ctx);
        twr_mpl3115a2_set_update_interval(&ctx->instance.mpl3115a2, BAROMETER_UPDATE_INTERVAL);
        break;
    case SENSOR_TYPE_SGP30:
        twr_sgp30_init(&ctx->instance.sgp30, attr->i2c, attr->address);
        twr_sgp30_set_event_handler(&ctx->instance.sgp30, _sensor_sgp30_event_handler, ctx);
        twr_sgp30_set_update_interval(&ctx->instance.sgp30, VOC_UPDATE_INTERVAL);
        break;
    case SENSOR_TYPE_SGPC3:
        twr_sgpc3_init(&ctx->instance.sgpc3, attr->i2c, attr->address);
        twr_sgpc3_set_event_handler(&ctx->instance.sgpc3, _sensor_sgpc3_event_handler, ctx);
        twr_sgpc3_set_update_interval(&ctx->instance.sgpc3, VOC_UPDATE_INTERVAL);
        break;
    default:
        break;
    }

    return true;
}

static void _sensor_dealloc(sensor_t *ctx)
{
    if (ctx == NULL)
        return;

    switch (ctx->attr->type)
    {
    case SENSOR_TYPE_TMP112:
        twr_tmp112_deinit(&ctx->instance.tmp112);
        break;
    case SENSOR_TYPE_HTS221:
        twr_hts221_deinit(&ctx->instance.hts221);
        break;
    case SENSOR_TYPE_HDC2080:
        twr_hdc2080_deinit(&ctx->instance.hdc2080);
        break;
    case SENSOR_TYPE_SHT20:
        twr_sht20_deinit(&ctx->instance.sht20);
        break;
    case SENSOR_TYPE_SHT30:
        twr_sht30_deinit(&ctx->instance.sht30);
        break;
    case SENSOR_TYPE_OPT3001:
        twr_opt3001_deinit(&ctx->instance.opt3001);
        break;
    case SENSOR_TYPE_MPL3115A2:
        twr_mpl3115a2_deinit(&ctx->instance.mpl3115a2);
        break;
    case SENSOR_TYPE_SGP30:
        twr_sgp30_deinit(&ctx->instance.sgp30);
        break;
    case SENSOR_TYPE_SGPC3:
        twr_sgpc3_deinit(&ctx->instance.sgpc3);
        break;
    default:
        break;
    }
    ctx->attr = NULL;
}

static void _sensor_measure(sensor_t *ctx)
{
    if ((ctx == NULL) || (ctx->attr == NULL))
        return;

    switch (ctx->attr->type)
    {
    case SENSOR_TYPE_TMP112:
        twr_tmp112_measure(&ctx->instance.tmp112);
        break;
    case SENSOR_TYPE_HTS221:
        twr_hts221_measure(&ctx->instance.hts221);
        break;
    case SENSOR_TYPE_HDC2080:
        twr_hdc2080_measure(&ctx->instance.hdc2080);
        break;
    case SENSOR_TYPE_SHT20:
        twr_sht20_measure(&ctx->instance.sht20);
        break;
    case SENSOR_TYPE_SHT30:
        twr_sht30_measure(&ctx->instance.sht30);
        break;
    case SENSOR_TYPE_OPT3001:
        twr_opt3001_measure(&ctx->instance.opt3001);
        break;
    case SENSOR_TYPE_MPL3115A2:
        twr_mpl3115a2_measure(&ctx->instance.mpl3115a2);
        break;
    case SENSOR_TYPE_SGP30:
        twr_sgp30_measure(&ctx->instance.sgp30);
        break;
    case SENSOR_TYPE_SGPC3:
        twr_sgpc3_measure(&ctx->instance.sgpc3);
        break;
    default:
        break;
    }
}

static void _sensor_event_pub(sensor_t *sensor)
{
    if (_sensors.event_handler == NULL)
        return;

    _sensors.event_handler(SENSORS_EVENT_SENSOR_PUB, sensor, _sensors.event_param);
}

static void _sensor_tmp112_event_handler(twr_tmp112_t *self, twr_tmp112_event_t event, void *event_param)
{
    sensor_t *ctx = (sensor_t *)event_param;

    twr_log_debug("TMP112 channel %d event %d", ctx->attr->channel, event);
    float value;

    if ((event == TWR_TMP112_EVENT_UPDATE) && twr_tmp112_get_temperature_celsius(self, &value))
    {
        if ((fabs(value - ctx->value) >= TEMPERATURE_PUB_VALUE_CHANGE) || (ctx->next_pub < twr_tick_get()))
        {
            twr_radio_pub_temperature(ctx->attr->channel, &value);

            ctx->value = value;
            ctx->next_pub = twr_tick_get() + TEMPERATURE_PUB_NO_CHANGE_INTERVAL;

            values.temperature = value;

            _sensor_event_pub(ctx);
        }
    }
    else if (event == TWR_TMP112_EVENT_ERROR)
    {
        _sensor_try_next(ctx);
    }
}

static void _sensor_hts221_event_handler(twr_hts221_t *self, twr_hts221_event_t event, void *event_param)
{
    sensor_t *ctx = (sensor_t *)event_param;

    // twr_log_debug("HTS221 channel %d event %d", ctx->attr->channel, event);
    float value;

    if ((event == TWR_HTS221_EVENT_UPDATE) && twr_hts221_get_humidity_percentage(self, &value))
    {
        if ((fabs(value - ctx->value) >= HYGROMETER_PUB_VALUE_CHANGE) || (ctx->next_pub < twr_tick_get()))
        {
            twr_radio_pub_humidity(ctx->attr->channel, &value);
            ctx->value = value;
            ctx->next_pub = twr_tick_get() + HYGROMETER_PUB_NO_CHANGE_INTERVAL;

            values.humidity = value;
            _sensor_event_pub(ctx);
        }
    }
    else if (event == TWR_HTS221_EVENT_ERROR)
    {
        _sensor_try_next(ctx);
    }
}

static void _sensor_hdc2080_event_handler(twr_hdc2080_t *self, twr_hdc2080_event_t event, void *event_param)
{
    sensor_t *ctx = (sensor_t *)event_param;

    // twr_log_debug("HDC2080 channel %d event %d", ctx->attr->channel, event);
    float value;

    if ((event == TWR_HDC2080_EVENT_UPDATE) && twr_hdc2080_get_humidity_percentage(self, &value))
    {
        if ((fabs(value - ctx->value) >= HYGROMETER_PUB_VALUE_CHANGE) || (ctx->next_pub < twr_tick_get()))
        {
            twr_radio_pub_humidity(ctx->attr->channel, &value);
            ctx->value = value;
            ctx->next_pub = twr_tick_get() + HYGROMETER_PUB_NO_CHANGE_INTERVAL;

            values.humidity = value;
            _sensor_event_pub(ctx);
        }
    }
    else if (event == TWR_HDC2080_EVENT_ERROR)
    {
        _sensor_try_next(ctx);
    }
}

static void _sensor_sht20_event_handler(twr_sht20_t *self, twr_sht20_event_t event, void *event_param)
{
    sensor_t *ctx = (sensor_t *)event_param;

    // twr_log_debug("SHT20 channel %d event %d", ctx->attr->channel, event);
    float value;

    if ((event == TWR_SHT20_EVENT_UPDATE) && twr_sht20_get_humidity_percentage(self, &value))
    {
        if ((fabs(value - ctx->value) >= HYGROMETER_PUB_VALUE_CHANGE) || (ctx->next_pub < twr_tick_get()))
        {
            twr_radio_pub_humidity(ctx->attr->channel, &value);
            ctx->value = value;
            ctx->next_pub = twr_tick_get() + HYGROMETER_PUB_NO_CHANGE_INTERVAL;

            values.humidity = value;
            _sensor_event_pub(ctx);
        }
    }
    else if (event == TWR_SHT20_EVENT_ERROR)
    {
        _sensor_try_next(ctx);
    }
}

static void _sensor_sht30_event_handler(twr_sht30_t *self, twr_sht30_event_t event, void *event_param)
{
    sensor_t *ctx = (sensor_t *)event_param;

    // twr_log_debug("SHT30 channel %d event %d", ctx->attr->channel, event);
    float value;

    if ((event == TWR_SHT30_EVENT_UPDATE) && twr_sht30_get_humidity_percentage(self, &value))
    {
        if ((fabs(value - ctx->value) >= HYGROMETER_PUB_VALUE_CHANGE) || (ctx->next_pub < twr_tick_get()))
        {
            twr_radio_pub_humidity(ctx->attr->channel, &value);
            ctx->value = value;
            ctx->next_pub = twr_tick_get() + HYGROMETER_PUB_NO_CHANGE_INTERVAL;

            values.humidity = value;
            _sensor_event_pub(ctx);
        }
    }
    else if (event == TWR_SHT30_EVENT_ERROR)
    {
        _sensor_try_next(ctx);
    }
}

static void _sensor_opt3001_event_handler(twr_opt3001_t *self, twr_opt3001_event_t event, void *event_param)
{
    sensor_t *ctx = (sensor_t *)event_param;

    // twr_log_debug("OPT3001 channel %d event %d", ctx->attr->channel, event);
    float value;

    if ((event == TWR_OPT3001_EVENT_UPDATE) && twr_opt3001_get_illuminance_lux(self, &value))
    {
        if (value < 1.f)
            value = 0.f;

        if ((fabs(value - ctx->value) >= LUX_METER_PUB_VALUE_CHANGE) ||
            ((value < 1.f) && (ctx->value > 10.f)) ||
            ((value > 10.f) && (ctx->value < 1.f)) ||
            (ctx->next_pub < twr_tick_get()))
        {
            twr_radio_pub_luminosity(ctx->attr->channel, &value);
            ctx->value = value;
            ctx->next_pub = twr_tick_get() + LUX_METER_PUB_NO_CHANGE_INTERVAL;

            values.illuminance = value;
            _sensor_event_pub(ctx);
        }
    }
    else if (event == TWR_OPT3001_EVENT_ERROR)
    {
        _sensor_try_next(ctx);
    }
}

static void _sensor_mpl3115a2_event_handler(twr_mpl3115a2_t *self, twr_mpl3115a2_event_t event, void *event_param)
{
    sensor_t *ctx = (sensor_t *)event_param;

    // twr_log_debug("MPL3115A2 channel %d event %d", ctx->attr->channel, event);
    float pascal;
    float meter;

    if ((event == TWR_MPL3115A2_EVENT_UPDATE) &&
        twr_mpl3115a2_get_pressure_pascal(self, &pascal) && twr_mpl3115a2_get_altitude_meter(self, &meter))
    {
        if ((fabs(pascal - ctx->value) >= BAROMETER_PUB_VALUE_CHANGE) || (ctx->next_pub < twr_tick_get()))
        {

            twr_radio_pub_barometer(ctx->attr->channel, &pascal, &meter);
            ctx->value = pascal;
            ctx->next_pub = twr_tick_get() + BAROMETER_PUB_NO_CHANGE_INTERVAL;

            values.pressure = pascal / 100.0;
            values.altitude = meter;
            _sensor_event_pub(ctx);
        }
    }
    else if (event == TWR_MPL3115A2_EVENT_ERROR)
    {
        _sensor_try_next(ctx);
    }
}

static void _sensor_sgp30_event_handler(twr_sgp30_t *self, twr_sgp30_event_t event, void *event_param)
{
    sensor_t *ctx = (sensor_t *)event_param;

    // twr_log_debug("SGP30 channel %d event %d", ctx->attr->channel, event);
    uint16_t value;

    if ((event == TWR_SGP30_EVENT_UPDATE) && twr_sgp30_get_tvoc_ppb(self, &value))
    {
        if ((fabs(value - ctx->value) >= VOC_PUB_VALUE_CHANGE) || (ctx->next_pub < twr_tick_get()))
        {
            int tvoc = value;
            if (ctx->attr->channel == TWR_RADIO_PUB_CHANNEL_R1_I2C0_ADDRESS_DEFAULT)
            {
                twr_radio_pub_int("voc-sensor/0:0/tvoc", &tvoc);
            }
            else
            {
                twr_radio_pub_int("voc-sensor/1:0/tvoc", &tvoc);
            }

            ctx->value = value;
            ctx->next_pub = twr_tick_get() + VOC_PUB_NO_CHANGE_INTERVAL;

            values.voc = value;
            _sensor_event_pub(ctx);
        }
    }
    else if (event == TWR_SGP30_EVENT_ERROR)
    {
        _sensor_try_next(ctx);
    }
}

static void _sensor_sgpc3_event_handler(twr_sgpc3_t *self, twr_sgpc3_event_t event, void *event_param)
{
    sensor_t *ctx = (sensor_t *)event_param;

    // twr_log_debug("SGPC3 channel %d event %d", ctx->attr->channel, event);
    uint16_t value;

    if ((event == TWR_SGPC3_EVENT_UPDATE) && twr_sgpc3_get_tvoc_ppb(self, &value))
    {
        if ((fabs(value - ctx->value) >= VOC_PUB_VALUE_CHANGE) || (ctx->next_pub < twr_tick_get()))
        {
            int tvoc = value;
            if (ctx->attr->channel == TWR_RADIO_PUB_CHANNEL_R1_I2C0_ADDRESS_DEFAULT)
            {
                twr_radio_pub_int("voc-lp-sensor/0:0/tvoc", &tvoc);
            }
            else
            {
                twr_radio_pub_int("voc-lp-sensor/1:0/tvoc", &tvoc);
            }

            ctx->value = value;
            ctx->next_pub = twr_tick_get() + VOC_PUB_NO_CHANGE_INTERVAL;

            values.voc = value;
            _sensor_event_pub(ctx);
        }
    }
    else if (event == TWR_SGPC3_EVENT_ERROR)
    {
        _sensor_try_next(ctx);
    }
}
