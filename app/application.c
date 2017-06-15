#include <application.h>
#include <radio.h>

#if MODULE_POWER
#define UPDATE_INTERVAL (5 * 1000)
#else
#define UPDATE_INTERVAL (5 * 60 * 1000)
#endif

#define UPDATE_INTERVAL_CO2 (UPDATE_INTERVAL < (15 * 1000) ? (15 * 1000) : UPDATE_INTERVAL)
#define LED_STRIP_COUNT 144
#define LED_STRIP_TYPE BC_LED_STRIP_TYPE_RGBW

bc_led_t led;
bool led_state = false;

static struct {
	float_t temperature;
	float_t humidity;
	float_t illuminance;
	float_t pressure;
	float_t altitude;
	float_t co2_concentation;

} values;

static const struct {
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

#if MODULE_POWER
static uint64_t my_device_address;
bc_led_strip_t led_strip;
static uint8_t pixels[LED_STRIP_COUNT * 4];
static uint32_t _bc_module_power_led_strip_dma_buffer[LED_STRIP_COUNT * 3 * 2];
const bc_led_strip_buffer_t bc_module_power_led_strip_buffer =
{
    .type = LED_STRIP_TYPE,
    .count = LED_STRIP_COUNT,
    .buffer = _bc_module_power_led_strip_dma_buffer
};
uint16_t led_strip_brightness = 255;

static bc_module_relay_t relay_0_0;
static bc_module_relay_t relay_0_1;

static void radio_event_handler(bc_radio_event_t event, void *event_param);
#endif
void button_event_handler(bc_button_t *self, bc_button_event_t event, void *event_param);
void lcd_button_event_handler(bc_button_t *self, bc_button_event_t event, void *event_param);
void temperature_tag_event_handler(bc_tag_temperature_t *self, bc_tag_temperature_event_t event, void *event_param);
void humidity_tag_event_handler(bc_tag_humidity_t *self, bc_tag_humidity_event_t event, void *event_param);
void lux_meter_event_handler(bc_tag_lux_meter_t *self, bc_tag_lux_meter_event_t event, void *event_param);
void barometer_tag_event_handler(bc_tag_barometer_t *self, bc_tag_barometer_event_t event, void *event_param);
void co2_event_handler(bc_module_co2_event_t event, void *event_param);
void flood_detector_event_handler(bc_flood_detector_t *self, bc_flood_detector_event_t event, void *event_param);
void pir_event_handler(bc_module_pir_t *self, bc_module_pir_event_t event, void*event_param);

void application_init(void)
{
    bc_led_init(&led, BC_GPIO_LED, false, false);

    bc_radio_init();

    static bc_button_t button;
    bc_button_init(&button, BC_GPIO_BUTTON, BC_GPIO_PULL_DOWN, false);
    bc_button_set_event_handler(&button, button_event_handler, NULL);

    static bc_tag_temperature_t temperature_tag_0_48;
    bc_tag_temperature_init(&temperature_tag_0_48, BC_I2C_I2C0, BC_TAG_TEMPERATURE_I2C_ADDRESS_DEFAULT);
    bc_tag_temperature_set_update_interval(&temperature_tag_0_48, UPDATE_INTERVAL);
    static uint8_t temperature_tag_0_48_i2c = (BC_I2C_I2C0 << 7) | BC_TAG_TEMPERATURE_I2C_ADDRESS_DEFAULT;
    bc_tag_temperature_set_event_handler(&temperature_tag_0_48, temperature_tag_event_handler, &temperature_tag_0_48_i2c);

    static bc_tag_temperature_t temperature_tag_0_49;
    bc_tag_temperature_init(&temperature_tag_0_49, BC_I2C_I2C0, BC_TAG_TEMPERATURE_I2C_ADDRESS_ALTERNATE);
    bc_tag_temperature_set_update_interval(&temperature_tag_0_49, UPDATE_INTERVAL);
    static uint8_t temperature_tag_0_49_i2c = (BC_I2C_I2C0 << 7) | BC_TAG_TEMPERATURE_I2C_ADDRESS_ALTERNATE;
    bc_tag_temperature_set_event_handler(&temperature_tag_0_49, temperature_tag_event_handler, &temperature_tag_0_49_i2c);

    static bc_tag_temperature_t temperature_tag_1_48;
    bc_tag_temperature_init(&temperature_tag_1_48, BC_I2C_I2C1, BC_TAG_TEMPERATURE_I2C_ADDRESS_DEFAULT);
    bc_tag_temperature_set_update_interval(&temperature_tag_1_48, UPDATE_INTERVAL);
    static uint8_t temperature_tag_1_48_i2c = (BC_I2C_I2C1 << 7) | BC_TAG_TEMPERATURE_I2C_ADDRESS_DEFAULT;
    bc_tag_temperature_set_event_handler(&temperature_tag_1_48, temperature_tag_event_handler,&temperature_tag_1_48_i2c);

    static bc_tag_temperature_t temperature_tag_1_49;
    bc_tag_temperature_init(&temperature_tag_1_49, BC_I2C_I2C1, BC_TAG_TEMPERATURE_I2C_ADDRESS_ALTERNATE);
    bc_tag_temperature_set_update_interval(&temperature_tag_1_49, UPDATE_INTERVAL);
    static uint8_t temperature_tag_1_49_i2c = (BC_I2C_I2C1 << 7) | BC_TAG_TEMPERATURE_I2C_ADDRESS_ALTERNATE;
    bc_tag_temperature_set_event_handler(&temperature_tag_1_49, temperature_tag_event_handler,&temperature_tag_1_49_i2c);

    //----------------------------

    static bc_tag_humidity_t humidity_tag_r2_0_40;
    bc_tag_humidity_init(&humidity_tag_r2_0_40, BC_TAG_HUMIDITY_REVISION_R2, BC_I2C_I2C0, BC_TAG_HUMIDITY_I2C_ADDRESS_DEFAULT);
    bc_tag_humidity_set_update_interval(&humidity_tag_r2_0_40, UPDATE_INTERVAL);
    static uint8_t humidity_tag_r2_0_40_i2c = (BC_I2C_I2C0 << 7) | 0x40;
    bc_tag_humidity_set_event_handler(&humidity_tag_r2_0_40, humidity_tag_event_handler, &humidity_tag_r2_0_40_i2c);

    static bc_tag_humidity_t humidity_tag_r2_0_41;
    bc_tag_humidity_init(&humidity_tag_r2_0_41, BC_TAG_HUMIDITY_REVISION_R2, BC_I2C_I2C0, BC_TAG_HUMIDITY_I2C_ADDRESS_ALTERNATE);
    bc_tag_humidity_set_update_interval(&humidity_tag_r2_0_41, UPDATE_INTERVAL);
    static uint8_t humidity_tag_r2_0_41_i2c = (BC_I2C_I2C0 << 7) | 0x41;
    bc_tag_humidity_set_event_handler(&humidity_tag_r2_0_41, humidity_tag_event_handler, &humidity_tag_r2_0_41_i2c);

    static bc_tag_humidity_t humidity_tag_r1_0_5f;
    bc_tag_humidity_init(&humidity_tag_r1_0_5f, BC_TAG_HUMIDITY_REVISION_R1, BC_I2C_I2C0, BC_TAG_HUMIDITY_I2C_ADDRESS_DEFAULT);
    bc_tag_humidity_set_update_interval(&humidity_tag_r1_0_5f, UPDATE_INTERVAL);
    static uint8_t humidity_tag_r1_0_5f_i2c = (BC_I2C_I2C0 << 7) | 0x5f;
    bc_tag_humidity_set_event_handler(&humidity_tag_r1_0_5f, humidity_tag_event_handler, &humidity_tag_r1_0_5f_i2c);

    static bc_tag_humidity_t humidity_tag_r2_1_40;
    bc_tag_humidity_init(&humidity_tag_r2_1_40, BC_TAG_HUMIDITY_REVISION_R2, BC_I2C_I2C1, BC_TAG_HUMIDITY_I2C_ADDRESS_DEFAULT);
    bc_tag_humidity_set_update_interval(&humidity_tag_r2_1_40, UPDATE_INTERVAL);
    static uint8_t humidity_tag_r2_1_40_i2c = (BC_I2C_I2C1 << 7) | 0x40;
    bc_tag_humidity_set_event_handler(&humidity_tag_r2_1_40, humidity_tag_event_handler, &humidity_tag_r2_1_40_i2c);

    static bc_tag_humidity_t humidity_tag_r2_1_41;
    bc_tag_humidity_init(&humidity_tag_r2_1_41, BC_TAG_HUMIDITY_REVISION_R2, BC_I2C_I2C1, BC_TAG_HUMIDITY_I2C_ADDRESS_ALTERNATE);
    bc_tag_humidity_set_update_interval(&humidity_tag_r2_1_41, UPDATE_INTERVAL);
    static uint8_t humidity_tag_r2_1_41_i2c = (BC_I2C_I2C1 << 7) | 0x41;
    bc_tag_humidity_set_event_handler(&humidity_tag_r2_1_41, humidity_tag_event_handler, &humidity_tag_r2_1_41_i2c);

    static bc_tag_humidity_t humidity_tag_r1_1_5f;
    bc_tag_humidity_init(&humidity_tag_r1_1_5f, BC_TAG_HUMIDITY_REVISION_R1, BC_I2C_I2C1, BC_TAG_HUMIDITY_I2C_ADDRESS_DEFAULT);
    bc_tag_humidity_set_update_interval(&humidity_tag_r1_1_5f, UPDATE_INTERVAL);
    static uint8_t humidity_tag_r1_1_5f_i2c = (BC_I2C_I2C1 << 7) | 0x5f;
    bc_tag_humidity_set_event_handler(&humidity_tag_r1_1_5f, humidity_tag_event_handler, &humidity_tag_r1_1_5f_i2c);

    //----------------------------

    static bc_tag_lux_meter_t lux_meter_0_44;
    bc_tag_lux_meter_init(&lux_meter_0_44, BC_I2C_I2C0, BC_TAG_LUX_METER_I2C_ADDRESS_DEFAULT);
    bc_tag_lux_meter_set_update_interval(&lux_meter_0_44, UPDATE_INTERVAL);
    static uint8_t lux_meter_0_44_i2c = (BC_I2C_I2C0 << 7) | BC_TAG_LUX_METER_I2C_ADDRESS_DEFAULT;
    bc_tag_lux_meter_set_event_handler(&lux_meter_0_44, lux_meter_event_handler, &lux_meter_0_44_i2c);

    static bc_tag_lux_meter_t lux_meter_0_45;
    bc_tag_lux_meter_init(&lux_meter_0_45, BC_I2C_I2C0, BC_TAG_LUX_METER_I2C_ADDRESS_ALTERNATE);
    bc_tag_lux_meter_set_update_interval(&lux_meter_0_45, UPDATE_INTERVAL);
    static uint8_t lux_meter_0_45_i2c = (BC_I2C_I2C0 << 7) | BC_TAG_LUX_METER_I2C_ADDRESS_ALTERNATE;
    bc_tag_lux_meter_set_event_handler(&lux_meter_0_45, lux_meter_event_handler, &lux_meter_0_45_i2c);

    static bc_tag_lux_meter_t lux_meter_1_44;
    bc_tag_lux_meter_init(&lux_meter_1_44, BC_I2C_I2C1, BC_TAG_LUX_METER_I2C_ADDRESS_DEFAULT);
    bc_tag_lux_meter_set_update_interval(&lux_meter_1_44, UPDATE_INTERVAL);
    static uint8_t lux_meter_1_44_i2c = (BC_I2C_I2C1 << 7) | BC_TAG_LUX_METER_I2C_ADDRESS_DEFAULT;
    bc_tag_lux_meter_set_event_handler(&lux_meter_1_44, lux_meter_event_handler, &lux_meter_1_44_i2c);

    static bc_tag_lux_meter_t lux_meter_1_45;
    bc_tag_lux_meter_init(&lux_meter_1_45, BC_I2C_I2C1, BC_TAG_LUX_METER_I2C_ADDRESS_ALTERNATE);
    bc_tag_lux_meter_set_update_interval(&lux_meter_1_45, UPDATE_INTERVAL);
    static uint8_t lux_meter_1_45_i2c = (BC_I2C_I2C1 << 7) | BC_TAG_LUX_METER_I2C_ADDRESS_ALTERNATE;
    bc_tag_lux_meter_set_event_handler(&lux_meter_1_45, lux_meter_event_handler, &lux_meter_1_45_i2c);

    //----------------------------

    static bc_tag_barometer_t barometer_tag_0;
    bc_tag_barometer_init(&barometer_tag_0, BC_I2C_I2C0);
    bc_tag_barometer_set_update_interval(&barometer_tag_0, UPDATE_INTERVAL);
    static uint8_t barometer_tag_0_i2c = (BC_I2C_I2C0 << 7) | 0x60;
    bc_tag_barometer_set_event_handler(&barometer_tag_0, barometer_tag_event_handler, &barometer_tag_0_i2c);

    static bc_tag_barometer_t barometer_tag_1;
    bc_tag_barometer_init(&barometer_tag_1, BC_I2C_I2C1);
    bc_tag_barometer_set_update_interval(&barometer_tag_1, UPDATE_INTERVAL);
    static uint8_t barometer_tag_1_i2c = (BC_I2C_I2C1 << 7) | 0x60;
    bc_tag_barometer_set_event_handler(&barometer_tag_1, barometer_tag_event_handler, &barometer_tag_1_i2c);

    //----------------------------

    bc_module_co2_init();
    bc_module_co2_set_update_interval(UPDATE_INTERVAL_CO2);
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
    static event_param_t flood_detector_a_event_param = {.number = 'a', .last_value = -1};
    bc_flood_detector_init(&flood_detector_a, BC_FLOOD_DETECTOR_TYPE_LD_81_SENSOR_MODULE_CHANNEL_A);
    bc_flood_detector_set_event_handler(&flood_detector_a, flood_detector_event_handler, &flood_detector_a_event_param);
    bc_flood_detector_set_update_interval(&flood_detector_a, 1000);

    static bc_flood_detector_t flood_detector_b;
    static event_param_t flood_detector_b_event_param = {.number = 'b', .last_value = -1};
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
    bc_led_strip_init(&led_strip, bc_module_power_get_led_strip_driver(), &bc_module_power_led_strip_buffer_rgbw_144);

    bc_module_relay_init(&relay_0_0, BC_MODULE_RELAY_I2C_ADDRESS_DEFAULT);
    bc_module_relay_init(&relay_0_1, BC_MODULE_RELAY_I2C_ADDRESS_ALTERNATE);

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

    bc_module_lcd_set_font(&bc_font_ubuntu_15);
    bc_module_lcd_draw_string(10, 5, pages[page_index].name0);

    bc_module_lcd_set_font(&bc_font_ubuntu_28);
    snprintf(str, sizeof(str), pages[page_index].format0, *pages[page_index].value0);
    w = bc_module_lcd_draw_string(25, 25, str);
    bc_module_lcd_set_font(&bc_font_ubuntu_15);
    w = bc_module_lcd_draw_string(w, 35, pages[page_index].unit0);

    bc_module_lcd_set_font(&bc_font_ubuntu_15);
    bc_module_lcd_draw_string(10, 55, pages[page_index].name1);

    bc_module_lcd_set_font(&bc_font_ubuntu_28);
    snprintf(str, sizeof(str), pages[page_index].format1, *pages[page_index].value1);
    w = bc_module_lcd_draw_string(25, 75, str);
    bc_module_lcd_set_font(&bc_font_ubuntu_15);
    bc_module_lcd_draw_string(w, 85, pages[page_index].unit1);

    snprintf(str, sizeof(str), "%d/3", page_index + 1);

    bc_module_lcd_draw_string(100, 108, str);

	bc_module_lcd_update();

	bc_module_core_pll_disable();
}

void button_event_handler(bc_button_t *self, bc_button_event_t event, void *event_param)
{
    (void) self;
    (void) event_param;

    if (event == BC_BUTTON_EVENT_PRESS)
    {
        bc_led_pulse(&led, 100);

        static uint16_t event_count = 0;

        event_count++;

        bc_radio_pub_push_button(&event_count);
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
		page_index--;
		if (page_index < 0)
		{
			page_index = 2;
		}
	}
	else
	{
		page_index++;
		if (page_index > 2)
		{
			page_index = 0;
		}
	}

	bc_scheduler_plan_now(0);
}

void temperature_tag_event_handler(bc_tag_temperature_t *self, bc_tag_temperature_event_t event, void *event_param)
{
    float value;

    if (event != BC_TAG_TEMPERATURE_EVENT_UPDATE)
    {
        return;
    }

    if (bc_tag_temperature_get_temperature_celsius(self, &value))
    {
        bc_radio_pub_thermometer(*(uint8_t *)event_param, &value);
        values.temperature = value;
        bc_scheduler_plan_now(0);
    }
}

void humidity_tag_event_handler(bc_tag_humidity_t *self, bc_tag_humidity_event_t event, void *event_param)
{
    float value;

    if (event != BC_TAG_HUMIDITY_EVENT_UPDATE)
    {
        return;
    }

    if (bc_tag_humidity_get_humidity_percentage(self, &value))
    {
        bc_radio_pub_humidity(*(uint8_t *)event_param, &value);
        values.humidity = value;
        bc_scheduler_plan_now(0);
    }
}

void lux_meter_event_handler(bc_tag_lux_meter_t *self, bc_tag_lux_meter_event_t event, void *event_param)
{
    float value;

    if (event != BC_TAG_LUX_METER_EVENT_UPDATE)
    {
        return;
    }

    if (bc_tag_lux_meter_get_illuminance_lux(self, &value))
    {
        bc_radio_pub_luminosity(*(uint8_t *)event_param, &value);
        values.illuminance = value;
        bc_scheduler_plan_now(0);
    }
}

void barometer_tag_event_handler(bc_tag_barometer_t *self, bc_tag_barometer_event_t event, void *event_param)
{
    float pascal;
    float meter;

    if (event != BC_TAG_BAROMETER_EVENT_UPDATE)
    {
        return;
    }

    if (!bc_tag_barometer_get_pressure_pascal(self, &pascal))
    {
        return;
    }

    if (!bc_tag_barometer_get_altitude_meter(self, &meter))
    {
        return;
    }

    bc_radio_pub_barometer(*(uint8_t *)event_param, &pascal, &meter);
    values.pressure = pascal / 100.0;
    values.altitude = meter;
    bc_scheduler_plan_now(0);

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
		if (bc_flood_detector_is_alarm(self) != ((event_param_t *) event_param)->last_value)
		{
			((event_param_t *) event_param)->last_value = bc_flood_detector_is_alarm(self);

			uint8_t buffer[3];
			buffer[0] = RADIO_FLOOD_DETECTOR;
			buffer[1] = ((event_param_t *) event_param)->number;
			buffer[2] = ((event_param_t *) event_param)->last_value;
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

static void _update_led_strip(void)
{
	size_t i = 0;
	uint8_t red;
	uint8_t green;
	uint8_t blue;
	uint8_t white;
	for (int position = 0; position < LED_STRIP_COUNT;position++)
	{
		red = ((uint16_t) pixels[i] * led_strip_brightness) >> 8;
		green = ((uint16_t) pixels[i + 1] * led_strip_brightness) >> 8;
		blue = ((uint16_t) pixels[i + 2] * led_strip_brightness) >> 8;
		white = ((uint16_t) pixels[i + 3] * led_strip_brightness) >> 8;
		bc_led_strip_set_pixel_rgbw(&led_strip, position, red, green, blue, white);
		i += 4;
	}
	bc_led_strip_write(&led_strip);
}

static void _fill_pixels(int from, int to, uint8_t *color)
{
	size_t i = (from * 4);
	for (;(from < to) && (from < LED_STRIP_COUNT); from++)
	{
		pixels[i] = color[0];
		pixels[i + 1] = color[1];
		pixels[i + 2] = color[2];
		pixels[i + 3] = color[3];
		i += 4;
	}
}

void bc_radio_on_buffer(uint64_t *peer_device_address, uint8_t *buffer, size_t *length)
{
    (void) peer_device_address;
    if (*length < (1 + sizeof(uint64_t)))
    {
    	return;
    }

    uint64_t device_address;

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
		{	// HEAD(1B); ADDRESS(8B); COLOR(4B)
			if (*length != (1 + sizeof(uint64_t) + 4))
			{
				return;
			}
			_fill_pixels(0, LED_STRIP_COUNT, buffer + sizeof(uint64_t) + 1);
			_update_led_strip();
			break;
		}
		case RADIO_LED_STRIP_BRIGHTNESS_SET:
		{
			// HEAD(1B); ADDRESS(8B); BRIGHTNESS(1B)
			if (*length != (1 + sizeof(uint64_t) + 1))
			{
				return;
			}
			led_strip_brightness = (uint16_t)buffer[sizeof(uint64_t) + 1] * 255 / 100;
			_update_led_strip();
			break;
		}
		case RADIO_LED_STRIP_COMPOUND_SET:
		{
			// HEAD(1B); ADDRESS(8B); OFFSET(1B), COUNT(1B), COLOR(4B), COUNT(1B), COLOR(4B), ...
			if (*length < (1 + sizeof(uint64_t) + 1))
			{
				return;
			}

			int offset = buffer[sizeof(uint64_t) + 1];

			for (size_t i = sizeof(uint64_t) + 2; i < *length; i += 5)
			{
				_fill_pixels(offset, offset + buffer[i], buffer + i + 1);
				offset += buffer[i];
			}
			_update_led_strip();
			break;
		}
		default:
			break;
	}
}
#endif
