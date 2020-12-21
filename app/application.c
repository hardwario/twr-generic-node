#include <application.h>
#include <radio.h>
#include <sensors.h>
#include <values.h>

#define BATTERY_UPDATE_INTERVAL (60 * 60 * 1000)

#define CO2_PUB_NO_CHANGE_INTERVAL (15 * 60 * 1000)
#define CO2_PUB_VALUE_CHANGE 50.0f

#define FLOOD_DETECTOR_NO_CHANGE_INTERVAL (15 * 60 * 1000)
#define FLOOD_DETECTOR_UPDATE_INTERVAL (1 * 1000)

#define MAX_PAGE_INDEX 3

#if MODULE_POWER
    #define PAGE_INDEX_MENU 3
    #define CO2_UPDATE_INTERVAL (15 * 1000)
    #define RADIO_MODE TWR_RADIO_MODE_NODE_LISTENING
    #ifndef LED_STRIP_TYPE
        #define LED_STRIP_TYPE 4
    #endif
    #ifndef LED_STRIP_COUNT
        #define LED_STRIP_COUNT 144
    #endif
#else
    #define PAGE_INDEX_MENU -1
    #define CO2_UPDATE_INTERVAL (1 * 60 * 1000)
    #define RADIO_MODE TWR_RADIO_MODE_NODE_SLEEPING
#endif // #if MODULE_POWER

twr_led_t led;
twr_led_t led_lcd_green;

bool led_state = false;

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
    {"Voc           ", "%.0f", &values.voc, "ppb",
     "Battery       ", "%.1f", &values.battery_voltage, "V"},
};

static int page_index = 0;
static int menu_item = 0;

static struct
{
    twr_tick_t next_update;
    bool mqtt;

} lcd;

#if MODULE_POWER
static char *menu_items[] = {
        "Page 0",
        "LED Test",
        "Effect Rainbow",
        "Effect Rainbow cycle",
        "Effect Theater chase rainbow"
};

static uint32_t _twr_module_power_led_strip_dma_buffer[LED_STRIP_COUNT * LED_STRIP_TYPE * 2];
const twr_led_strip_buffer_t led_strip_buffer =
{
    .type = LED_STRIP_TYPE,
    .count = LED_STRIP_COUNT,
    .buffer = _twr_module_power_led_strip_dma_buffer
};

static struct
{
    enum
    {
        LED_STRIP_SHOW_COLOR = 0,
        LED_STRIP_SHOW_COMPOUND = 1,
        LED_STRIP_SHOW_EFFECT = 2,
        LED_STRIP_SHOW_THERMOMETER = 3

    } show;
    twr_led_strip_t self;
    uint32_t color;
    struct
    {
        uint8_t data[TWR_RADIO_NODE_MAX_COMPOUND_BUFFER_SIZE];
        int length;
    } compound;
    struct
    {
        float temperature;
        int8_t min;
        int8_t max;
        uint8_t white_dots;
        float set_point;
        uint32_t color;

    } thermometer;

    twr_scheduler_task_id_t update_task_id;

} led_strip = { .show = LED_STRIP_SHOW_COLOR, .color = 0 };

static twr_module_relay_t relay_0_0;
static twr_module_relay_t relay_0_1;

static void led_strip_update_task(void *param);
static void radio_event_handler(twr_radio_event_t event, void *event_param);
#else
void battery_event_handler(twr_module_battery_event_t event, void *event_param);
#endif //MODULE_POWER

static void lcd_page_render();
static void app_sensor_init(void);

void button_event_handler(twr_button_t *self, twr_button_event_t event, void *event_param);
void lcd_event_handler(twr_module_lcd_event_t event, void *event_param);
void co2_event_handler(twr_module_co2_event_t event, void *event_param);
void flood_detector_event_handler(twr_flood_detector_t *self, twr_flood_detector_event_t event, void *event_param);
void pir_event_handler(twr_module_pir_t *self, twr_module_pir_event_t event, void*event_param);
void encoder_event_handler(twr_module_encoder_event_t event, void *event_param);
void sensors_event_handler(sensors_event_t event, sensor_t *sensor, void *event_param);

void application_init(void)
{
    memset(&values, 0xff, sizeof(values));

    twr_log_init(TWR_LOG_LEVEL_DUMP, TWR_LOG_TIMESTAMP_ABS);

    twr_led_init(&led, TWR_GPIO_LED, false, false);
    twr_led_set_mode(&led, TWR_LED_MODE_OFF);

    twr_radio_init(RADIO_MODE);
    twr_radio_pairing_request(FIRMWARE, VERSION);

    sensors_init();
    sensors_set_event_handler(sensors_event_handler, NULL);

    twr_led_pulse(&led, 2000);
    twr_scheduler_plan_from_now(0, 2000);
}

void application_task(void)
{
    static bool init = false;

    if (!init)
    {
        init = true;
        app_sensor_init();
        sensors_scan();
    }

    if (!twr_module_lcd_is_ready())
    {
        return;
    }

    if (!lcd.mqtt)
    {
        lcd_page_render();
    }
    else
    {
        twr_scheduler_plan_current_relative(500);
    }

    twr_module_lcd_update();
}

static void app_sensor_init(void)
{
    static twr_button_t button;
    twr_button_init(&button, TWR_GPIO_BUTTON, TWR_GPIO_PULL_DOWN, false);
    twr_button_set_event_handler(&button, button_event_handler, NULL);
    twr_button_set_hold_time(&button, 1000);

    static event_param_t co2_event_param = { .next_pub = 0 };
    twr_module_co2_init();
    twr_module_co2_set_update_interval(CO2_UPDATE_INTERVAL);
    twr_module_co2_set_event_handler(co2_event_handler, &co2_event_param);

    //----------------------------
    twr_module_lcd_init();
    twr_module_lcd_set_event_handler(lcd_event_handler, NULL);
    twr_module_lcd_set_button_hold_time(1000);
    const twr_led_driver_t* driver = twr_module_lcd_get_led_driver();
    twr_led_init_virtual(&led_lcd_green, 1, driver, 1);

    // static twr_flood_detector_t flood_detector;
    // static event_param_t flood_detector_event_param = {.next_pub = 0};
    // twr_flood_detector_init(&flood_detector, TWR_FLOOD_DETECTOR_TYPE_LD_81_SENSOR_MODULE_CHANNEL_A);
    // twr_flood_detector_set_event_handler(&flood_detector, flood_detector_event_handler, &flood_detector_event_param);
    // twr_flood_detector_set_update_interval(&flood_detector, FLOOD_DETECTOR_UPDATE_INTERVAL);

    static twr_module_pir_t pir;
    twr_module_pir_init(&pir);
    twr_module_pir_set_event_handler(&pir, pir_event_handler, NULL);

#if MODULE_POWER
    twr_radio_set_event_handler(radio_event_handler, NULL);

    twr_module_power_init();
    twr_led_strip_init(&led_strip.self, twr_module_power_get_led_strip_driver(), &led_strip_buffer);

    twr_module_relay_init(&relay_0_0, TWR_MODULE_RELAY_I2C_ADDRESS_DEFAULT);
    twr_module_relay_init(&relay_0_1, TWR_MODULE_RELAY_I2C_ADDRESS_ALTERNATE);

    led_strip.update_task_id = twr_scheduler_register(led_strip_update_task, NULL, TWR_TICK_INFINITY);
#else
    twr_module_battery_init();
    twr_module_battery_set_event_handler(battery_event_handler, NULL);
    twr_module_battery_set_update_interval(BATTERY_UPDATE_INTERVAL);
#endif
}

static void lcd_page_render()
{

    int w;
    char str[32];

    twr_system_pll_enable();

    twr_module_lcd_clear();

    if ((page_index <= MAX_PAGE_INDEX) && (page_index != PAGE_INDEX_MENU))
    {
        twr_module_lcd_set_font(&twr_font_ubuntu_15);
        twr_module_lcd_draw_string(10, 5, pages[page_index].name0, true);

        twr_module_lcd_set_font(&twr_font_ubuntu_28);
        snprintf(str, sizeof(str), pages[page_index].format0, *pages[page_index].value0);
        w = twr_module_lcd_draw_string(25, 25, str, true);
        twr_module_lcd_set_font(&twr_font_ubuntu_15);
        w = twr_module_lcd_draw_string(w, 35, pages[page_index].unit0, true);

        twr_module_lcd_set_font(&twr_font_ubuntu_15);
        twr_module_lcd_draw_string(10, 55, pages[page_index].name1, true);

        twr_module_lcd_set_font(&twr_font_ubuntu_28);
        snprintf(str, sizeof(str), pages[page_index].format1, *pages[page_index].value1);
        w = twr_module_lcd_draw_string(25, 75, str, true);
        twr_module_lcd_set_font(&twr_font_ubuntu_15);
        twr_module_lcd_draw_string(w, 85, pages[page_index].unit1, true);
    }
#if MODULE_POWER
    else
    {
        twr_module_lcd_set_font(&twr_font_ubuntu_13);

        for (int i = 0; i < 5; i++)
        {
            twr_module_lcd_draw_string(5, 5 + (i * 15), menu_items[i], i != menu_item);
        }

        twr_module_lcd_set_font(&twr_font_ubuntu_13);
        twr_module_lcd_draw_string(5, 115, "Down", true);
        twr_module_lcd_draw_string(90, 115, "Enter", true);
    }
#endif

    snprintf(str, sizeof(str), "%d/%d", page_index + 1, MAX_PAGE_INDEX + 1);
    twr_module_lcd_set_font(&twr_font_ubuntu_13);
    twr_module_lcd_draw_string(55, 115, str, true);

    twr_system_pll_disable();
}

void button_event_handler(twr_button_t *self, twr_button_event_t event, void *event_param)
{
    (void) self;
    static uint16_t event_count = 0;
    static uint16_t hold_count = 0;

    // Do not send button events when LCD Module is connected
    if (twr_module_lcd_is_ready())
    {
        return;
    }

    if (event == TWR_BUTTON_EVENT_CLICK)
    {
        twr_led_pulse(&led, 100);
        event_count++;
        twr_radio_pub_push_button(&event_count);
    }

    if (event == TWR_BUTTON_EVENT_HOLD)
    {
        hold_count++;
        twr_radio_pub_int("push-button/-/hold-count", (int*)&hold_count);
        twr_led_pulse(&led, 400);
        sensors_scan();
    }
}

void lcd_event_handler(twr_module_lcd_event_t event, void *event_param)
{
    (void) event_param;

    if (event == TWR_MODULE_LCD_EVENT_LEFT_CLICK)
    {
        if ((page_index != PAGE_INDEX_MENU))
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
            if (menu_item > 4)
            {
                menu_item = 0;
            }
        }

        static uint16_t left_event_count = 0;
        left_event_count++;
        //twr_radio_pub_event_count(TWR_RADIO_PUB_EVENT_LCD_BUTTON_LEFT, &left_event_count);
    }
    else if(event == TWR_MODULE_LCD_EVENT_RIGHT_CLICK)
    {
        if ((page_index != PAGE_INDEX_MENU) || (menu_item == 0))
        {
            // Key next page
            page_index++;
            if (page_index > MAX_PAGE_INDEX)
            {
                page_index = 0;
            }
            if (page_index == PAGE_INDEX_MENU)
            {
                menu_item = 0;
            }
        }
#if MODULE_POWER
        else if (page_index == PAGE_INDEX_MENU)
        {
            // Key enter
            if (menu_item == 1)
            {
                twr_led_strip_effect_test(&led_strip.self);
            }
            else if (menu_item == 2)
            {
                twr_led_strip_effect_rainbow(&led_strip.self, 50);
            }
            else if (menu_item == 3)
            {
                twr_led_strip_effect_rainbow_cycle(&led_strip.self, 50);
            }
            else if (menu_item == 4)
            {
                twr_led_strip_effect_theater_chase_rainbow(&led_strip.self, 50);
            }
            led_strip.show = LED_STRIP_SHOW_EFFECT;
        }
#endif

        static uint16_t right_event_count = 0;
        right_event_count++;
        //twr_radio_pub_event_count(TWR_RADIO_PUB_EVENT_LCD_BUTTON_RIGHT, &right_event_count);
    }
    else if(event == TWR_MODULE_LCD_EVENT_LEFT_HOLD)
    {
        static int left_hold_event_count = 0;
        left_hold_event_count++;
        twr_radio_pub_int("push-button/lcd:left-hold/event-count", &left_hold_event_count);

        twr_led_pulse(&led_lcd_green, 100);
    }
    else if(event == TWR_MODULE_LCD_EVENT_RIGHT_HOLD)
    {
        static int right_hold_event_count = 0;
        right_hold_event_count++;
        twr_radio_pub_int("push-button/lcd:right-hold/event-count", &right_hold_event_count);

        twr_led_pulse(&led_lcd_green, 100);

    }
    else if(event == TWR_MODULE_LCD_EVENT_BOTH_HOLD)
    {
        static int both_hold_event_count = 0;
        both_hold_event_count++;
        twr_radio_pub_int("push-button/lcd:both-hold/event-count", &both_hold_event_count);
        twr_led_pulse(&led_lcd_green, 400);
        sensors_scan();
    }

    twr_scheduler_plan_now(0);
}

void co2_event_handler(twr_module_co2_event_t event, void *event_param)
{
    event_param_t *param = (event_param_t *) event_param;
    float value;

    if (event == TWR_MODULE_CO2_EVENT_UPDATE)
    {
        if (twr_module_co2_get_concentration_ppm(&value))
        {
            if ((fabs(value - param->value) >= CO2_PUB_VALUE_CHANGE) || (param->next_pub < twr_scheduler_get_spin_tick()))
            {
                twr_radio_pub_co2(&value);
                param->value = value;
                param->next_pub = twr_scheduler_get_spin_tick() + CO2_PUB_NO_CHANGE_INTERVAL;

                values.co2_concentation = value;
                twr_scheduler_plan_now(0);
            }
        }
    }
}

void flood_detector_event_handler(twr_flood_detector_t *self, twr_flood_detector_event_t event, void *event_param)
{
    event_param_t *param = (event_param_t *)event_param;
    bool is_alarm;

    if (event == TWR_FLOOD_DETECTOR_EVENT_UPDATE)
    {
        is_alarm = twr_flood_detector_is_alarm(self);

        if ((is_alarm != param->value) || (param->next_pub < twr_scheduler_get_spin_tick()))
        {
           twr_radio_pub_bool("flood-detector/a/alarm", &is_alarm);

           param->value = is_alarm;
           param->next_pub = twr_scheduler_get_spin_tick() + FLOOD_DETECTOR_NO_CHANGE_INTERVAL;
        }
    }
}

void pir_event_handler(twr_module_pir_t *self, twr_module_pir_event_t event, void *event_param)
{
    (void) self;
    (void) event_param;

    if (event == TWR_MODULE_PIR_EVENT_MOTION)
    {
        static uint16_t event_count = 0;

        event_count++;

        twr_radio_pub_event_count(TWR_RADIO_PUB_EVENT_PIR_MOTION, &event_count);
    }
}

void sensors_event_handler(sensors_event_t event, sensor_t *sensor, void *event_param)
{
    if (event == SENSORS_EVENT_SENSOR_PUB) {
        twr_scheduler_plan_now(0);
    }
}

#if MODULE_POWER
static void radio_event_handler(twr_radio_event_t event, void *event_param)
{
    (void) event_param;

    if (event == TWR_RADIO_EVENT_ATTACH)
    {
        twr_led_pulse(&led, 1000);
    }
    else if (event == TWR_RADIO_EVENT_DETACH)
    {
        twr_led_pulse(&led, 1000);
    }
}

void twr_radio_node_on_state_set(uint64_t *id, uint8_t state_id, bool *state)
{
    (void) id;

    switch (state_id) {
        case TWR_RADIO_NODE_STATE_LED:
        {
            led_state = *state;

            twr_led_set_mode(&led, *state ? TWR_LED_MODE_ON : TWR_LED_MODE_OFF);

            twr_radio_pub_state(TWR_RADIO_PUB_STATE_LED, state);

            break;
        }
        case TWR_RADIO_NODE_STATE_RELAY_MODULE_0:
        {
            twr_module_relay_set_state(&relay_0_0, *state);

            twr_radio_pub_state(TWR_RADIO_PUB_STATE_RELAY_MODULE_0, state);

            break;
        }
        case TWR_RADIO_NODE_STATE_RELAY_MODULE_1:
        {
            twr_module_relay_set_state(&relay_0_1, *state);

            twr_radio_pub_state(TWR_RADIO_PUB_STATE_RELAY_MODULE_1, state);

            break;
        }
        case TWR_RADIO_NODE_STATE_POWER_MODULE_RELAY:
        {
            twr_module_power_relay_set_state(*state);

            twr_radio_pub_state(TWR_RADIO_PUB_STATE_POWER_MODULE_RELAY, state);

            break;
        }
        default:
        {
            return;
        }
    }
}

void twr_radio_node_on_state_get(uint64_t *id, uint8_t state_id)
{
    (void) id;

    switch (state_id) {
        case TWR_RADIO_NODE_STATE_LED:
        {
            twr_radio_pub_state(TWR_RADIO_PUB_STATE_LED, &led_state);

            break;
        }
        case TWR_RADIO_NODE_STATE_RELAY_MODULE_0:
        {

            twr_module_relay_state_t r_state = twr_module_relay_get_state(&relay_0_0);

            if (r_state != TWR_MODULE_RELAY_STATE_UNKNOWN)
            {
                bool state = r_state == TWR_MODULE_RELAY_STATE_TRUE ? true : false;

                twr_radio_pub_state(TWR_RADIO_PUB_STATE_RELAY_MODULE_0, &state);
            }

            break;
        }
        case TWR_RADIO_NODE_STATE_RELAY_MODULE_1:
        {
            twr_module_relay_state_t r_state = twr_module_relay_get_state(&relay_0_1);

            if (r_state != TWR_MODULE_RELAY_STATE_UNKNOWN)
            {
                bool state = r_state == TWR_MODULE_RELAY_STATE_TRUE ? true : false;

                twr_radio_pub_state(TWR_RADIO_PUB_STATE_RELAY_MODULE_1, &state);
            }

            break;
        }
        case TWR_RADIO_NODE_STATE_POWER_MODULE_RELAY:
        {
            bool state = twr_module_power_relay_get_state();

            twr_radio_pub_state(TWR_RADIO_PUB_STATE_POWER_MODULE_RELAY, &state);

            break;
        }
        default:
        {
            return;
        }
    }
}

void led_strip_update_task(void *param)
{
    (void) param;

    if (!twr_led_strip_is_ready(&led_strip.self))
    {
        twr_scheduler_plan_current_now();

        return;
    }

    twr_led_strip_write(&led_strip.self);

    twr_scheduler_plan_current_relative(250);
}

void led_strip_fill(void)
{
    if (led_strip.show == LED_STRIP_SHOW_COLOR)
    {
        twr_led_strip_fill(&led_strip.self, led_strip.color);
    }
    else if (led_strip.show == LED_STRIP_SHOW_COMPOUND)
    {
        int from = 0;
        int to;
        uint8_t *color;

        for (int i = 0; i < led_strip.compound.length; i += 5)
        {
            color = led_strip.compound.data + i + 1;
            to = from + led_strip.compound.data[i];

            for (;(from < to) && (from < LED_STRIP_COUNT); from++)
            {
                twr_led_strip_set_pixel_rgbw(&led_strip.self, from, color[3], color[2], color[1], color[0]);
            }

            from = to;
        }
    }
    else if (led_strip.show == LED_STRIP_SHOW_THERMOMETER)
    {
        twr_led_strip_thermometer(&led_strip.self, led_strip.thermometer.temperature, led_strip.thermometer.min, led_strip.thermometer.max, led_strip.thermometer.white_dots, led_strip.thermometer.set_point, led_strip.thermometer.color);
    }
}

void twr_radio_node_on_led_strip_color_set(uint64_t *id, uint32_t *color)
{
    (void) id;

    twr_led_strip_effect_stop(&led_strip.self);

    led_strip.color = *color;

    led_strip.show = LED_STRIP_SHOW_COLOR;

    led_strip_fill();

    twr_scheduler_plan_now(led_strip.update_task_id);
}

void twr_radio_node_on_led_strip_brightness_set(uint64_t *id, uint8_t *brightness)
{
    (void) id;

    twr_led_strip_set_brightness(&led_strip.self, *brightness);

    led_strip_fill();

    twr_scheduler_plan_now(led_strip.update_task_id);
}

void twr_radio_node_on_led_strip_compound_set(uint64_t *id, uint8_t *compound, size_t length)
{
    (void) id;

    twr_led_strip_effect_stop(&led_strip.self);

    memcpy(led_strip.compound.data, compound, length);

    led_strip.compound.length = length;

    led_strip.show = LED_STRIP_SHOW_COMPOUND;

    led_strip_fill();

    twr_scheduler_plan_now(led_strip.update_task_id);
}

void twr_radio_node_on_led_strip_effect_set(uint64_t *id, twr_radio_node_led_strip_effect_t type, uint16_t wait, uint32_t *color)
{
    (void) id;

    switch (type) {
        case TWR_RADIO_NODE_LED_STRIP_EFFECT_TEST:
        {
            twr_led_strip_effect_test(&led_strip.self);
            break;
        }
        case TWR_RADIO_NODE_LED_STRIP_EFFECT_RAINBOW:
        {
            twr_led_strip_effect_rainbow(&led_strip.self, wait);
            break;
        }
        case TWR_RADIO_NODE_LED_STRIP_EFFECT_RAINBOW_CYCLE:
        {
            twr_led_strip_effect_rainbow_cycle(&led_strip.self, wait);
            break;
        }
        case TWR_RADIO_NODE_LED_STRIP_EFFECT_THEATER_CHASE_RAINBOW:
        {
            twr_led_strip_effect_theater_chase_rainbow(&led_strip.self, wait);
            break;
        }
        case TWR_RADIO_NODE_LED_STRIP_EFFECT_COLOR_WIPE:
        {
            twr_led_strip_effect_color_wipe(&led_strip.self, *color, wait);
            break;
        }
        case TWR_RADIO_NODE_LED_STRIP_EFFECT_THEATER_CHASE:
        {
            twr_led_strip_effect_theater_chase(&led_strip.self, *color, wait);
            break;
        }
        case TWR_RADIO_NODE_LED_STRIP_EFFECT_STROBOSCOPE:
        {
            twr_led_strip_effect_stroboscope(&led_strip.self, *color, wait);
            break;
        }
        case TWR_RADIO_NODE_LED_STRIP_EFFECT_ICICLE:
        {
            twr_led_strip_effect_icicle(&led_strip.self, *color, wait);
            break;
        }
        case TWR_RADIO_NODE_LED_STRIP_EFFECT_PULSE_COLOR:
        {
            twr_led_strip_effect_pulse_color(&led_strip.self, *color, wait);
            break;
        }
        default:
            return;
    }

    led_strip.show = LED_STRIP_SHOW_EFFECT;
}

void twr_radio_node_on_led_strip_thermometer_set(uint64_t *id, float *temperature, int8_t *min, int8_t *max, uint8_t *white_dots, float *set_point, uint32_t *set_point_color)
{
    (void) id;

    twr_led_strip_effect_stop(&led_strip.self);

    led_strip.thermometer.temperature = *temperature;
    led_strip.thermometer.min = *min;
    led_strip.thermometer.max = *max;
    led_strip.thermometer.white_dots = *white_dots;

    if (set_point != NULL)
    {
        led_strip.thermometer.set_point = *set_point;
        led_strip.thermometer.color = *set_point_color;
    }
    else
    {
        led_strip.thermometer.set_point = *min - 1;
    }

    led_strip.show = LED_STRIP_SHOW_THERMOMETER;

    led_strip_fill();

    twr_scheduler_plan_now(led_strip.update_task_id);
}

void twr_radio_pub_on_buffer(uint64_t *peer_device_address, uint8_t *buffer, size_t length)
{
    (void) peer_device_address;
    if (length < (1 + sizeof(uint64_t)))
    {
        return;
    }

    uint64_t device_address;
    uint8_t *pointer = buffer + sizeof(uint64_t) + 1;

    memcpy(&device_address, buffer + 1, sizeof(device_address));

    if (device_address != twr_radio_get_my_id())
    {
        return;
    }

    switch (buffer[0]) {
        case RADIO_RELAY_0_PULSE_SET:
        case RADIO_RELAY_1_PULSE_SET:
        {
            if (length != (1 + sizeof(uint64_t) + 1 + 4))
            {
                return;
            }
            uint32_t duration; // Duration is 4 byte long in a radio packet, but 8 bytes as a twr_relay_pulse parameter.
            memcpy(&duration, &buffer[sizeof(uint64_t) + 2], sizeof(uint32_t));
            twr_module_relay_pulse(buffer[0] == RADIO_RELAY_0_PULSE_SET ? &relay_0_0 : &relay_0_1, buffer[sizeof(uint64_t) + 1], (twr_tick_t)duration);
            break;
        }
        case RADIO_LCD_TEXT_SET:
        {
            if (length < (1 + sizeof(uint64_t) + 4 + 2))
            {
                return;
            }

            int x = (int) *pointer++;
            int y = (int) *pointer++;
            int font_size = (int) *pointer++;
            bool color = (bool) *pointer++;
            size_t text_length = (size_t) *pointer++;
            char text[32];
            memcpy(text, pointer, text_length + 1);
            text[text_length] = 0;

            twr_system_pll_enable();

            if (!lcd.mqtt)
            {
                twr_module_lcd_clear();
                lcd.mqtt = true;
                twr_scheduler_plan_now(0);
            }

            switch (font_size)
            {
                case 11:
                {
                    twr_module_lcd_set_font(&twr_font_ubuntu_11);
                    break;
                }
                case 13:
                {
                    twr_module_lcd_set_font(&twr_font_ubuntu_13);
                    break;
                }
                case 15:
                {
                    twr_module_lcd_set_font(&twr_font_ubuntu_15);
                    break;
                }
                case 24:
                {
                    twr_module_lcd_set_font(&twr_font_ubuntu_24);
                    break;
                }
                case 28:
                {
                    twr_module_lcd_set_font(&twr_font_ubuntu_28);
                    break;
                }
                case 33:
                {
                    twr_module_lcd_set_font(&twr_font_ubuntu_33);
                    break;
                }
                default:
                {
                    twr_module_lcd_set_font(&twr_font_ubuntu_15);
                    break;
                }
            }

            twr_module_lcd_draw_string(x, y, text, color);

            twr_system_pll_disable();

            break;
        }
        case RADIO_LCD_SCREEN_CLEAR:
        {
            twr_system_pll_enable();

            if (!lcd.mqtt)
            {
                lcd.mqtt = true;
                twr_scheduler_plan_now(0);
            }

            twr_module_lcd_clear();

            twr_system_pll_disable();
            break;
        }
        default:
        {
            break;
        }
    }
}

#else

void battery_event_handler(twr_module_battery_event_t event, void *event_param)
{
    (void) event;
    (void) event_param;

    float voltage;
    int percentage;

    if (twr_module_battery_get_voltage(&voltage))
    {
        values.battery_voltage = voltage;
        twr_radio_pub_battery(&voltage);
    }

    if (twr_module_battery_get_charge_level(&percentage))
    {
        values.battery_pct = percentage;
    }
}

#endif // MODULE_POWER
