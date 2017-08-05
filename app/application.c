#include <application.h>
#include <radio.h>

#define TEMPERATURE_TAG_PUB_NO_CHANGE_INTEVAL (5 * 60 * 1000)
#define TEMPERATURE_TAG_PUB_VALUE_CHANGE 0.1f
#define TEMPERATURE_TAG_UPDATE_INTERVAL (1 * 1000)

#define HUMIDITY_TAG_PUB_NO_CHANGE_INTEVAL (5 * 60 * 1000)
#define HUMIDITY_TAG_PUB_VALUE_CHANGE 1.0f
#define HUMIDITY_TAG_UPDATE_INTERVAL (1 * 1000)

#define LUX_METER_TAG_PUB_NO_CHANGE_INTEVAL (5 * 60 * 1000)
#define LUX_METER_TAG_PUB_VALUE_CHANGE 5.0f
#define LUX_METER_TAG_UPDATE_INTERVAL (1 * 1000)

#define BAROMETER_TAG_PUB_NO_CHANGE_INTEVAL (5 * 60 * 1000)
#define BAROMETER_TAG_PUB_VALUE_CHANGE 10.0f
#define BAROMETER_TAG_UPDATE_INTERVAL (1 * 1000)

#if MODULE_POWER
#define MAX_PAGE_INDEX 3
#define CO2_UPDATE_INTERVAL (15 * 1000)
#else
#define MAX_PAGE_INDEX 2
#define CO2_UPDATE_INTERVAL (1 * 60 * 1000)
#endif

bc_led_t led;
bool led_state = false;

static struct
{
    float_t temperature;
    float_t humidity;
    float_t illuminance;
    float_t pressure;
    float_t altitude;
    float_t co2_concentation;

} values;

static const struct
{
    char *name0;
    char *format0;
    float_t *value0;
    char *unit0;
    char *name1;
    char *format1;
    float_t *value1;
    char *unit1;

} pages[] = {
    {"Temperature   ", "%.1f", &values.temperature, "\xb0" "C",
     "Humidity      ", "%.1f", &values.humidity, "%"},
    {"CO2           ", "%.0f", &values.co2_concentation, "ppm",
     "Illuminance   ", "%.1f", &values.illuminance, "lux"},
    {"Pressure      ", "%.0f", &values.pressure, "hPa",
     "Altitude      ", "%.1f", &values.altitude, "m"},
};

static int page_index = 0;
static int menu_item = 0;

#if MODULE_POWER
static uint64_t my_device_address;

static char *menu_items[] = {
        "Page 0",
        "LED Test",
        "Effect Rainbow",
        "Effect Rainbow cycle",
        "Effect Theater chase rainbow"
};

static uint32_t _bc_module_power_led_strip_dma_buffer[LED_STRIP_COUNT * LED_STRIP_TYPE * 2];
const bc_led_strip_buffer_t led_strip_buffer =
{
    .type = LED_STRIP_TYPE,
    .count = LED_STRIP_COUNT,
    .buffer = _bc_module_power_led_strip_dma_buffer
};

typedef enum
{
    LED_STRIP_SHOW_NONE = 0,
    LED_STRIP_SHOW_COLOR = 1,
    LED_STRIP_SHOW_COMPOUND = 2,
    LED_STRIP_SHOW_EFFECT = 3,
    LED_STRIP_SHOW_THERMOMETER = 4

} led_strip_show_t;

static struct
{
    led_strip_show_t show;
    led_strip_show_t last;
    bc_led_strip_t self;
    uint32_t color;
    struct
    {
        uint8_t data[5*20];
        int length;
    } compound;
    struct
    {
        float temperature;
        int8_t min;
        int8_t max;

    } thermometer;

    bc_scheduler_task_id_t update_task_id;

} led_strip = { .show = LED_STRIP_SHOW_NONE };

static bc_module_relay_t relay_0_0;
static bc_module_relay_t relay_0_1;

static void led_strip_update_task(void *param);
static void radio_event_handler(bc_radio_event_t event, void *event_param);
static void _radio_pub_state(uint8_t type, bool state);
#endif
static void temperature_tag_init(bc_i2c_channel_t i2c_channel, bc_tag_temperature_i2c_address_t i2c_address, temperature_tag_t *tag);
static void humidity_tag_init(bc_tag_humidity_revision_t revision, bc_i2c_channel_t i2c_channel, humidity_tag_t *tag);
static void lux_meter_tag_init(bc_i2c_channel_t i2c_channel, bc_tag_lux_meter_i2c_address_t i2c_address, lux_meter_tag_t *tag);
static void barometer_tag_init(bc_i2c_channel_t i2c_channel, barometer_tag_t *tag);

void button_event_handler(bc_button_t *self, bc_button_event_t event, void *event_param);
void lcd_button_event_handler(bc_button_t *self, bc_button_event_t event, void *event_param);
void temperature_tag_event_handler(bc_tag_temperature_t *self, bc_tag_temperature_event_t event, void *event_param);
void humidity_tag_event_handler(bc_tag_humidity_t *self, bc_tag_humidity_event_t event, void *event_param);
void lux_meter_event_handler(bc_tag_lux_meter_t *self, bc_tag_lux_meter_event_t event, void *event_param);
void barometer_tag_event_handler(bc_tag_barometer_t *self, bc_tag_barometer_event_t event, void *event_param);
void co2_event_handler(bc_module_co2_event_t event, void *event_param);
void flood_detector_event_handler(bc_flood_detector_t *self, bc_flood_detector_event_t event, void *event_param);
void pir_event_handler(bc_module_pir_t *self, bc_module_pir_event_t event, void*event_param);
void encoder_event_handler(bc_module_encoder_event_t event, void *event_param);
static void _radio_pub_u16(uint8_t type, uint16_t value);

void application_init(void)
{
    bc_led_init(&led, BC_GPIO_LED, false, false);

    bc_radio_init();

    static bc_button_t button;
    bc_button_init(&button, BC_GPIO_BUTTON, BC_GPIO_PULL_DOWN, false);
    bc_button_set_event_handler(&button, button_event_handler, NULL);

    //----------------------------

    static temperature_tag_t temperature_tag_0_0;
    temperature_tag_init(BC_I2C_I2C0, BC_TAG_TEMPERATURE_I2C_ADDRESS_DEFAULT, &temperature_tag_0_0);

    static temperature_tag_t temperature_tag_0_1;
    temperature_tag_init(BC_I2C_I2C0, BC_TAG_TEMPERATURE_I2C_ADDRESS_ALTERNATE, &temperature_tag_0_1);

    static temperature_tag_t temperature_tag_1_0;
    temperature_tag_init(BC_I2C_I2C1, BC_TAG_TEMPERATURE_I2C_ADDRESS_DEFAULT, &temperature_tag_1_0);

    static temperature_tag_t temperature_tag_1_1;
    temperature_tag_init(BC_I2C_I2C1, BC_TAG_TEMPERATURE_I2C_ADDRESS_ALTERNATE, &temperature_tag_1_1);

    //----------------------------

    static humidity_tag_t humidity_tag_0_0;
    humidity_tag_init(BC_TAG_HUMIDITY_REVISION_R1, BC_I2C_I2C0, &humidity_tag_0_0);

    static humidity_tag_t humidity_tag_0_2;
    humidity_tag_init(BC_TAG_HUMIDITY_REVISION_R2, BC_I2C_I2C0, &humidity_tag_0_2);

    static humidity_tag_t humidity_tag_0_4;
    humidity_tag_init(BC_TAG_HUMIDITY_REVISION_R3, BC_I2C_I2C0, &humidity_tag_0_4);

    static humidity_tag_t humidity_tag_1_0;
    humidity_tag_init(BC_TAG_HUMIDITY_REVISION_R1, BC_I2C_I2C1, &humidity_tag_1_0);

    static humidity_tag_t humidity_tag_1_2;
    humidity_tag_init(BC_TAG_HUMIDITY_REVISION_R2, BC_I2C_I2C1, &humidity_tag_1_2);

    static humidity_tag_t humidity_tag_1_4;
    humidity_tag_init(BC_TAG_HUMIDITY_REVISION_R3, BC_I2C_I2C1, &humidity_tag_1_4);

    //----------------------------

    static lux_meter_tag_t lux_meter_0_0;
    lux_meter_tag_init(BC_I2C_I2C0, BC_TAG_LUX_METER_I2C_ADDRESS_DEFAULT, &lux_meter_0_0);

    static lux_meter_tag_t lux_meter_0_1;
    lux_meter_tag_init(BC_I2C_I2C0, BC_TAG_LUX_METER_I2C_ADDRESS_ALTERNATE, &lux_meter_0_1);

    static lux_meter_tag_t lux_meter_1_0;
    lux_meter_tag_init(BC_I2C_I2C1, BC_TAG_LUX_METER_I2C_ADDRESS_DEFAULT, &lux_meter_1_0);

    static lux_meter_tag_t lux_meter_1_1;
    lux_meter_tag_init(BC_I2C_I2C1, BC_TAG_LUX_METER_I2C_ADDRESS_ALTERNATE, &lux_meter_1_1);

    //----------------------------

    static barometer_tag_t barometer_tag_0_0;
    barometer_tag_init(BC_I2C_I2C0, &barometer_tag_0_0);

    static barometer_tag_t barometer_tag_1_0;
    barometer_tag_init(BC_I2C_I2C1, &barometer_tag_1_0);

    //----------------------------

    bc_module_co2_init();
    bc_module_co2_set_update_interval(CO2_UPDATE_INTERVAL);
    bc_module_co2_set_event_handler(co2_event_handler, NULL);

    //----------------------------

    memset(&values, 0xff, sizeof(values));
    bc_module_lcd_init(&_bc_module_lcd_framebuffer);

    static bc_button_t lcd_left;
    bc_button_init_virtual(&lcd_left, BC_MODULE_LCD_BUTTON_LEFT, bc_module_lcd_get_button_driver(), false);
    bc_button_set_event_handler(&lcd_left, lcd_button_event_handler, NULL);

    static bc_button_t lcd_right;
    bc_button_init_virtual(&lcd_right, BC_MODULE_LCD_BUTTON_RIGHT, bc_module_lcd_get_button_driver(), false);
    bc_button_set_event_handler(&lcd_right, lcd_button_event_handler, NULL);

    static bc_flood_detector_t flood_detector_a;
    static event_param_t flood_detector_a_event_param = {.number = 'a', .value = -1};
    bc_flood_detector_init(&flood_detector_a, BC_FLOOD_DETECTOR_TYPE_LD_81_SENSOR_MODULE_CHANNEL_A);
    bc_flood_detector_set_event_handler(&flood_detector_a, flood_detector_event_handler, &flood_detector_a_event_param);
    bc_flood_detector_set_update_interval(&flood_detector_a, 1000);

    static bc_flood_detector_t flood_detector_b;
    static event_param_t flood_detector_b_event_param = {.number = 'b', .value = -1};
    bc_flood_detector_init(&flood_detector_b, BC_FLOOD_DETECTOR_TYPE_LD_81_SENSOR_MODULE_CHANNEL_B);
    bc_flood_detector_set_event_handler(&flood_detector_b, flood_detector_event_handler, &flood_detector_b_event_param);
    bc_flood_detector_set_update_interval(&flood_detector_b, 1000);

    static bc_module_pir_t pir;
    bc_module_pir_init(&pir);
    bc_module_pir_set_event_handler(&pir, pir_event_handler, NULL);

#if MODULE_POWER
    bc_radio_listen();
    bc_radio_set_event_handler(radio_event_handler, NULL);

    bc_module_power_init();
    bc_led_strip_init(&led_strip.self, bc_module_power_get_led_strip_driver(), &led_strip_buffer);

    bc_module_relay_init(&relay_0_0, BC_MODULE_RELAY_I2C_ADDRESS_DEFAULT);
    bc_module_relay_init(&relay_0_1, BC_MODULE_RELAY_I2C_ADDRESS_ALTERNATE);

    led_strip.update_task_id = bc_scheduler_register(led_strip_update_task, NULL, BC_TICK_INFINITY);

#else
    bc_module_battery_init(BC_MODULE_BATTERY_FORMAT_STANDARD);
#endif
}

void application_task(void)
{
    if (!bc_module_lcd_is_ready())
    {
        return;
    }

    bc_module_core_pll_enable();

    int w;
    char str[32];

    bc_module_lcd_clear();

    if (page_index < 3)
    {
        bc_module_lcd_set_font(&bc_font_ubuntu_15);
        bc_module_lcd_draw_string(10, 5, pages[page_index].name0, true);

        bc_module_lcd_set_font(&bc_font_ubuntu_28);
        snprintf(str, sizeof(str), pages[page_index].format0, *pages[page_index].value0);
        w = bc_module_lcd_draw_string(25, 25, str, true);
        bc_module_lcd_set_font(&bc_font_ubuntu_15);
        w = bc_module_lcd_draw_string(w, 35, pages[page_index].unit0, true);

        bc_module_lcd_set_font(&bc_font_ubuntu_15);
        bc_module_lcd_draw_string(10, 55, pages[page_index].name1, true);

        bc_module_lcd_set_font(&bc_font_ubuntu_28);
        snprintf(str, sizeof(str), pages[page_index].format1, *pages[page_index].value1);
        w = bc_module_lcd_draw_string(25, 75, str, true);
        bc_module_lcd_set_font(&bc_font_ubuntu_15);
        bc_module_lcd_draw_string(w, 85, pages[page_index].unit1, true);
    }
#if MODULE_POWER
    else
    {
        bc_module_lcd_set_font(&bc_font_ubuntu_13);

        for (int i = 0; i < 5; i++)
        {
            bc_module_lcd_draw_string(5, 5 + (i * 15), menu_items[i], i != menu_item);
        }

        bc_module_lcd_set_font(&bc_font_ubuntu_13);
        bc_module_lcd_draw_string(5, 115, "Down", true);
        bc_module_lcd_draw_string(90, 115, "Enter", true);
    }
#endif

    snprintf(str, sizeof(str), "%d/%d", page_index + 1, MAX_PAGE_INDEX + 1);
    bc_module_lcd_set_font(&bc_font_ubuntu_13);
    bc_module_lcd_draw_string(55, 115, str, true);

    bc_module_lcd_update();

    bc_module_core_pll_disable();
}

static void temperature_tag_init(bc_i2c_channel_t i2c_channel, bc_tag_temperature_i2c_address_t i2c_address, temperature_tag_t *tag)
{
    memset(tag, 0, sizeof(*tag));

    tag->param.number = (i2c_channel << 7) | i2c_address;

    bc_tag_temperature_init(&tag->self, i2c_channel, i2c_address);

    bc_tag_temperature_set_update_interval(&tag->self, TEMPERATURE_TAG_UPDATE_INTERVAL);

    bc_tag_temperature_set_event_handler(&tag->self, temperature_tag_event_handler, &tag->param);
}

static void humidity_tag_init(bc_tag_humidity_revision_t revision, bc_i2c_channel_t i2c_channel, humidity_tag_t *tag)
{
    uint8_t address;

    memset(tag, 0, sizeof(*tag));

    if (revision == BC_TAG_HUMIDITY_REVISION_R1)
    {
        address = 0x5f;
    }
    else if (revision == BC_TAG_HUMIDITY_REVISION_R2)
    {
        address = 0x40;
    }
    else if (revision == BC_TAG_HUMIDITY_REVISION_R3)
    {
        address = 0x40 | 0x0f; // 0x0f - hack
    }
    else
    {
        return;
    }

    tag->param.number = (i2c_channel << 7) | address;

    bc_tag_humidity_init(&tag->self, revision, i2c_channel, BC_TAG_HUMIDITY_I2C_ADDRESS_DEFAULT);

    bc_tag_humidity_set_update_interval(&tag->self, HUMIDITY_TAG_UPDATE_INTERVAL);

    bc_tag_humidity_set_event_handler(&tag->self, humidity_tag_event_handler, &tag->param);
}

static void lux_meter_tag_init(bc_i2c_channel_t i2c_channel, bc_tag_lux_meter_i2c_address_t i2c_address, lux_meter_tag_t *tag)
{
    memset(tag, 0, sizeof(*tag));

    tag->param.number = (i2c_channel << 7) | i2c_address;

    bc_tag_lux_meter_init(&tag->self, i2c_channel, i2c_address);

    bc_tag_lux_meter_set_update_interval(&tag->self, LUX_METER_TAG_UPDATE_INTERVAL);

    bc_tag_lux_meter_set_event_handler(&tag->self, lux_meter_event_handler, &tag->param);
}

static void barometer_tag_init(bc_i2c_channel_t i2c_channel, barometer_tag_t *tag)
{
    memset(tag, 0, sizeof(*tag));

    tag->param.number = (i2c_channel << 7) | 0x60;

    bc_tag_barometer_init(&tag->self, i2c_channel);

    bc_tag_barometer_set_update_interval(&tag->self, BAROMETER_TAG_UPDATE_INTERVAL);

    bc_tag_barometer_set_event_handler(&tag->self, barometer_tag_event_handler, &tag->param);
}

void button_event_handler(bc_button_t *self, bc_button_event_t event, void *event_param)
{
    (void) self;
    (void) event_param;

    if (event == BC_BUTTON_EVENT_PRESS)
    {
        bc_led_pulse(&led, 100);

        static uint16_t event_count = 0;

        bc_radio_pub_push_button(&event_count);

        event_count++;
    }
    else if (event == BC_BUTTON_EVENT_HOLD)
    {
        bc_radio_enroll_to_gateway();

        bc_led_set_mode(&led, BC_LED_MODE_OFF);

        bc_led_pulse(&led, 1000);
    }
}

void lcd_button_event_handler(bc_button_t *self, bc_button_event_t event, void *event_param)
{
    (void) event_param;

    if (event != BC_BUTTON_EVENT_CLICK)
    {
        return;
    }

    if (self->_channel.virtual_channel == BC_MODULE_LCD_BUTTON_LEFT)
    {
        if ((page_index != 3))
        {
            // Key prew page
            page_index--;
            if (page_index < 0)
            {
                page_index = MAX_PAGE_INDEX;
                menu_item = 0;
            }
        }
        else
        {
            // Key menu down
            menu_item++;
            if (menu_item == 5)
            {
                menu_item = 0;
            }
        }

        static uint16_t left_event_count = 0;
        _radio_pub_u16(RADIO_LCD_BUTTON_LEFT, left_event_count++);
    }
    else
    {
        if ((page_index != 3) || (menu_item == 0))
        {
            // Key next page
            page_index++;
            if (page_index > MAX_PAGE_INDEX)
            {
                page_index = 0;
            }
            if (page_index == 3)
            {
                menu_item = 0;
            }
        }
#if MODULE_POWER
        else if (page_index == 3)
        {
            // Key enter
            if (menu_item == 1)
            {
                bc_led_strip_effect_test(&led_strip.self);
            }
            else if (menu_item == 2)
            {
                bc_led_strip_effect_rainbow(&led_strip.self, 50);
            }
            else if (menu_item == 3)
            {
                bc_led_strip_effect_rainbow_cycle(&led_strip.self, 50);
            }
            else if (menu_item == 4)
            {
                bc_led_strip_effect_theater_chase_rainbow(&led_strip.self, 50);
            }
            led_strip.show = LED_STRIP_SHOW_EFFECT;
        }
#endif

        static uint16_t right_event_count = 0;
        _radio_pub_u16(RADIO_LCD_BUTTON_RIGHT, right_event_count++);
    }

    bc_scheduler_plan_now(0);
}

void temperature_tag_event_handler(bc_tag_temperature_t *self, bc_tag_temperature_event_t event, void *event_param)
{
    float value;
    event_param_t *param = (event_param_t *)event_param;

    if (event != BC_TAG_TEMPERATURE_EVENT_UPDATE)
    {
        return;
    }

    if (bc_tag_temperature_get_temperature_celsius(self, &value))
    {
        if ((fabs(value - param->value) >= TEMPERATURE_TAG_PUB_VALUE_CHANGE) || (param->next_pub < bc_scheduler_get_spin_tick()))
        {
            bc_radio_pub_thermometer(param->number, &value);
            param->value = value;
            param->next_pub = bc_scheduler_get_spin_tick() + TEMPERATURE_TAG_PUB_NO_CHANGE_INTEVAL;

            values.temperature = value;
            bc_scheduler_plan_now(0);
        }
    }
}

void humidity_tag_event_handler(bc_tag_humidity_t *self, bc_tag_humidity_event_t event, void *event_param)
{
    float value;
    event_param_t *param = (event_param_t *)event_param;

    if (event != BC_TAG_HUMIDITY_EVENT_UPDATE)
    {
        return;
    }

    if (bc_tag_humidity_get_humidity_percentage(self, &value))
    {
        if ((fabs(value - param->value) >= HUMIDITY_TAG_PUB_VALUE_CHANGE) || (param->next_pub < bc_scheduler_get_spin_tick()))
        {
            bc_radio_pub_humidity(param->number, &value);
            param->value = value;
            param->next_pub = bc_scheduler_get_spin_tick() + HUMIDITY_TAG_PUB_NO_CHANGE_INTEVAL;

            values.humidity = value;
            bc_scheduler_plan_now(0);
        }
    }
}

void lux_meter_event_handler(bc_tag_lux_meter_t *self, bc_tag_lux_meter_event_t event, void *event_param)
{
    float value;
    event_param_t *param = (event_param_t *)event_param;

    if (event != BC_TAG_LUX_METER_EVENT_UPDATE)
    {
        return;
    }

    if (bc_tag_lux_meter_get_illuminance_lux(self, &value))
    {
        if ((fabs(value - param->value) >= LUX_METER_TAG_PUB_VALUE_CHANGE) || (param->next_pub < bc_scheduler_get_spin_tick()))
        {
            bc_radio_pub_luminosity(param->number, &value);
            param->value = value;
            param->next_pub = bc_scheduler_get_spin_tick() + LUX_METER_TAG_PUB_NO_CHANGE_INTEVAL;

            values.illuminance = value;
            bc_scheduler_plan_now(0);
        }
    }
}

void barometer_tag_event_handler(bc_tag_barometer_t *self, bc_tag_barometer_event_t event, void *event_param)
{
    float pascal;
    float meter;
    event_param_t *param = (event_param_t *)event_param;

    if (event != BC_TAG_BAROMETER_EVENT_UPDATE)
    {
        return;
    }

    if (!bc_tag_barometer_get_pressure_pascal(self, &pascal))
    {
        return;
    }

    if ((fabs(pascal - param->value) >= BAROMETER_TAG_PUB_VALUE_CHANGE) || (param->next_pub < bc_scheduler_get_spin_tick()))
    {

        if (!bc_tag_barometer_get_altitude_meter(self, &meter))
        {
            return;
        }

        bc_radio_pub_barometer(param->number, &pascal, &meter);
        param->value = pascal;
        param->next_pub = bc_scheduler_get_spin_tick() + BAROMETER_TAG_PUB_NO_CHANGE_INTEVAL;

        values.pressure = pascal / 100.0;
        values.altitude = meter;
        bc_scheduler_plan_now(0);
    }
}

void co2_event_handler(bc_module_co2_event_t event, void *event_param)
{
    (void) event_param;
    float value;

    if (event == BC_MODULE_CO2_EVENT_UPDATE)
    {
        if (bc_module_co2_get_concentration(&value))
        {
            bc_radio_pub_co2(&value);
            values.co2_concentation = value;
            bc_scheduler_plan_now(0);
        }
    }
}

void flood_detector_event_handler(bc_flood_detector_t *self, bc_flood_detector_event_t event, void *event_param)
{
    if (event == BC_FLOOD_DETECTOR_EVENT_UPDATE)
    {
        if (bc_flood_detector_is_alarm(self) != ((event_param_t *) event_param)->value)
        {
            ((event_param_t *) event_param)->value = bc_flood_detector_is_alarm(self);

            uint8_t buffer[3];
            buffer[0] = RADIO_FLOOD_DETECTOR;
            buffer[1] = ((event_param_t *) event_param)->number;
            buffer[2] = ((event_param_t *) event_param)->value;
            bc_radio_pub_buffer(buffer, sizeof(buffer));
        }
    }
}

void pir_event_handler(bc_module_pir_t *self, bc_module_pir_event_t event, void *event_param)
{
    (void) self;
    (void) event_param;

    if (event == BC_MODULE_PIR_EVENT_MOTION)
    {
        static uint16_t event_count = 0;
        event_count++;

        uint8_t buffer[1 + sizeof(event_count)];

        buffer[0] = RADIO_PIR;

        memcpy(buffer + 1, &event_count, sizeof(event_count));

        bc_radio_pub_buffer(buffer, sizeof(buffer));
    }
}

#if MODULE_POWER
static void radio_event_handler(bc_radio_event_t event, void *event_param)
{
    (void) event_param;

    bc_led_set_mode(&led, BC_LED_MODE_OFF);

    if (event == BC_RADIO_EVENT_ATTACH)
    {
        bc_led_pulse(&led, 1000);
    }
    else if (event == BC_RADIO_EVENT_DETACH)
    {
        bc_led_pulse(&led, 1000);
    }
    else if (event == BC_RADIO_EVENT_INIT_DONE)
    {
        my_device_address = bc_radio_get_device_address();
    }
}

static void led_strip_update_task(void *param)
{
    (void) param;

    if (!bc_led_strip_is_ready(&led_strip.self))
    {
        bc_scheduler_plan_current_now();
        return;
    }

    switch (led_strip.show) {
        case LED_STRIP_SHOW_COLOR:
        {
            bc_led_strip_effect_stop(&led_strip.self);

            bc_led_strip_fill(&led_strip.self, led_strip.color);

            led_strip.show = LED_STRIP_SHOW_NONE;
            led_strip.last = LED_STRIP_SHOW_COLOR;
            break;
        }
        case LED_STRIP_SHOW_COMPOUND:
        {
            bc_led_strip_effect_stop(&led_strip.self);

            int from = 0;
            int to;
            uint8_t *color;

            for (int i = 0; i < led_strip.compound.length; i += 5)
            {
                color = led_strip.compound.data + i + 1;
                to = from + led_strip.compound.data[i];

                for (;(from < to) && (from < LED_STRIP_COUNT); from++)
                {
                    bc_led_strip_set_pixel_rgbw(&led_strip.self, from, color[0], color[1], color[2], color[3]);
                }

                from = to;
            }

            led_strip.show = LED_STRIP_SHOW_NONE;
            led_strip.last = LED_STRIP_SHOW_COMPOUND;
            break;
        }
        case LED_STRIP_SHOW_EFFECT:
        {
            led_strip.last = LED_STRIP_SHOW_EFFECT;
            return;
        }
        case LED_STRIP_SHOW_THERMOMETER:
        {
            uint8_t white = values.illuminance < 50 ? 1 : 0;
            bc_led_strip_effect_stop(&led_strip.self);

            bc_led_strip_thermometer(&led_strip.self, led_strip.thermometer.temperature,
                    led_strip.thermometer.min, led_strip.thermometer.max, white);

            led_strip.last = LED_STRIP_SHOW_THERMOMETER;
            break;
        }
        case LED_STRIP_SHOW_NONE:
        default:
        {
            break;
        }
    }

    bc_led_strip_write(&led_strip.self);

    bc_scheduler_plan_current_relative(250);
}

void bc_radio_on_buffer(uint64_t *peer_device_address, uint8_t *buffer, size_t *length)
{
    (void) peer_device_address;
    if (*length < (1 + sizeof(uint64_t)))
    {
        return;
    }

    uint64_t device_address;
    uint8_t *pointer = buffer + sizeof(uint64_t) + 1;

    memcpy(&device_address, buffer + 1, sizeof(device_address));

    if (device_address != my_device_address)
    {
        return;
    }

    switch (buffer[0]) {
        case RADIO_LED_SET:
        {
            if (*length != (1 + sizeof(uint64_t) + 1))
            {
                return;
            }
            led_state = buffer[sizeof(uint64_t) + 1];
            bc_led_set_mode(&led, led_state ? BC_LED_MODE_ON : BC_LED_MODE_OFF);
            _radio_pub_state(RADIO_LED, led_state);
            break;
        }
        case RADIO_RELAY_0_SET:
        case RADIO_RELAY_1_SET:
        {
            if (*length != (1 + sizeof(uint64_t) + 1))
            {
                return;
            }
            bc_module_relay_set_state(buffer[0] == RADIO_RELAY_0_SET ? &relay_0_0 : &relay_0_1, buffer[sizeof(uint64_t) + 1]);
            _radio_pub_state(buffer[0] == RADIO_RELAY_0_SET ? RADIO_RELAY_0 : RADIO_RELAY_1, buffer[sizeof(uint64_t) + 1]);
            break;
        }
        case RADIO_RELAY_POWER_SET:
        {
            if (*length != (1 + sizeof(uint64_t) + 1))
            {
                return;
            }
            bc_module_power_relay_set_state(buffer[sizeof(uint64_t) + 1]);
            break;
        }
        case RADIO_LED_STRIP_COLOR_SET:
        {    // HEAD(1B); ADDRESS(8B); COLOR(4B)
            if (*length != (1 + sizeof(uint64_t) + 4))
            {
                return;
            }

            led_strip.color = 0;

            led_strip.color |= ((uint32_t) *pointer++) << 24;
            led_strip.color |= ((uint32_t) *pointer++) << 16;
            led_strip.color |= ((uint32_t) *pointer++) << 8;
            led_strip.color |= ((uint32_t) *pointer);

            led_strip.show = LED_STRIP_SHOW_COLOR;
            bc_scheduler_plan_now(led_strip.update_task_id);
            break;
        }
        case RADIO_LED_STRIP_BRIGHTNESS_SET:
        {
            // HEAD(1B); ADDRESS(8B); BRIGHTNESS(1B)
            if (*length != (1 + sizeof(uint64_t) + 1))
            {
                return;
            }

            uint8_t brightness = (uint16_t)buffer[sizeof(uint64_t) + 1] * 255 / 100;

            bc_led_strip_set_brightness(&led_strip.self, brightness);

            led_strip.show = led_strip.last;
            bc_scheduler_plan_now(led_strip.update_task_id);
            break;
        }
        case RADIO_LED_STRIP_COMPOUND_SET:
        {
            // HEAD(1B); ADDRESS(8B); OFFSET(1B), COUNT(1B), COLOR(4B), COUNT(1B), COLOR(4B), ...
            if (*length < (1 + sizeof(uint64_t) + 1))
            {
                return;
            }

            bc_led_strip_effect_stop(&led_strip.self);

            int offset = buffer[sizeof(uint64_t) + 1];

            memcpy(led_strip.compound.data + offset, buffer + sizeof(uint64_t) + 2, sizeof(led_strip.compound.data) - offset);

            led_strip.compound.length = offset + (int) *length;

            led_strip.show = LED_STRIP_SHOW_COMPOUND;

            bc_scheduler_plan_now(led_strip.update_task_id);
            break;
        }
        case RADIO_LED_STRIP_EFFECT_SET:
        {
            //TYPE(1B); WAIT(2B); COLOR(4B)
            if (*length < (1 + sizeof(uint64_t) + 1 + sizeof(uint16_t) + sizeof(uint32_t)))
            {
                return;
            }

            uint16_t wait;
            memcpy(&wait, buffer + 1 + sizeof(uint64_t) + 1, sizeof(wait));
            uint32_t color;
            memcpy(&color, buffer + 1 + sizeof(uint64_t) + 1 + sizeof(wait), sizeof(color));

            switch (buffer[sizeof(uint64_t) + 1]) {
                case RADIO_LED_STRIP_EFFECT_TYPE_TEST:
                {
                    bc_led_strip_effect_test(&led_strip.self);
                    break;
                }
                case RADIO_LED_STRIP_EFFECT_TYPE_RAINBOW:
                {
                    bc_led_strip_effect_rainbow(&led_strip.self, wait);
                    break;
                }
                case RADIO_LED_STRIP_EFFECT_TYPE_RAINBOW_CYCLE:
                {
                    bc_led_strip_effect_rainbow_cycle(&led_strip.self, wait);
                    break;
                }
                case RADIO_LED_STRIP_EFFECT_TYPE_THEATER_CHASE_RAINBOW:
                {
                    bc_led_strip_effect_theater_chase_rainbow(&led_strip.self, wait);
                    break;
                }
                case RADIO_LED_STRIP_EFFECT_TYPE_COLOR_WIPE:
                {
                    bc_led_strip_effect_color_wipe(&led_strip.self, color, wait);
                    break;
                }
                case RADIO_LED_STRIP_EFFECT_TYPE_THEATER_CHASE:
                {
                    bc_led_strip_effect_theater_chase(&led_strip.self, color, wait);
                    break;
                }
                default:
                    return;
            }
            led_strip.show = LED_STRIP_SHOW_EFFECT;
            break;
        }
        case RADIO_LED_STRIP_THERMOMETER_SET:
        {
            if (*length < (1 + sizeof(uint64_t) + sizeof(float) + 1 + 1))
            {
                return;
            }
            bc_led_strip_effect_stop(&led_strip.self);
            memcpy(&led_strip.thermometer.temperature, buffer + 1 + sizeof(uint64_t), sizeof(float));
            led_strip.thermometer.min = buffer[1 + sizeof(uint64_t) + sizeof(float)];
            led_strip.thermometer.max = buffer[1 + sizeof(uint64_t) + sizeof(float) + 1];
            led_strip.show = LED_STRIP_SHOW_THERMOMETER;
            bc_scheduler_plan_now(led_strip.update_task_id);
            break;
        }
        default:
        {
            break;
        }
    }
}

static void _radio_pub_state(uint8_t type, bool state)
{
    uint8_t buffer[2];
    buffer[0] = type;
    buffer[1] = state;
    bc_radio_pub_buffer(buffer, sizeof(buffer));
}
#endif

static void _radio_pub_u16(uint8_t type, uint16_t value)
{
    uint8_t buffer[1 + sizeof(value)];
    buffer[0] = type;
    memcpy(buffer + 1, &value, sizeof(value));
    bc_radio_pub_buffer(buffer, sizeof(buffer));
}

