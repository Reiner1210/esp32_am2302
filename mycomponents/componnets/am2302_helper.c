#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"
#include "esp_clk.h"
#include "driver/gpio.h"

#include "am2302_helper.h"

// Timings from datasheet
// https://akizukidenshi.com/download/ds/aosong/AM2302.pdf

#define INIT_DELAY						(1000)		// 1msec
#define PULSE_TIMEOUT					(120)		// no pulse is longer than 120µsec
#define INIT_RESPONSE_PULSE_WIDTH_MIN	(65)		// Init response puls min width
#define INIT_RESPONSE_PULSE_WIDTH_MAX	(85)		// Init response puls max width
#define READ_INTERVAL					(2000)		// 2sec

const static char* log_tag = "Am2302";

static portMUX_TYPE am2303_mutex = portMUX_INITIALIZER_UNLOCKED;

void am2302_init_data(am2302_data_t* data, gpio_num_t pin)
{
	memset(data, 0, sizeof(*data));
	data->pin = pin;
}

static uint32_t read_pulse(int pin, int level, uint32_t timeout)
{
	int64_t start = esp_timer_get_time();
	esp_timer_get_time();
	while(gpio_get_level(pin) != level)
	{
		if (esp_timer_get_time() - start > timeout)
			return 0;
	}
	start = esp_timer_get_time();
	while(gpio_get_level(pin) == level)
	{
		if (esp_timer_get_time() - start > timeout)
			return 0;
	}
	return esp_timer_get_time() - start;
}

bool am2302_read_sensor(am2302_data_t* config_data, float* temperature, float* humidity)
{
	bool flag = false;

	if (config_data->pin < GPIO_NUM_34)
	{
		if (!config_data->last_read || (xTaskGetTickCount() - config_data->last_read) >= pdMS_TO_TICKS(READ_INTERVAL))
		{
			gpio_pad_select_gpio(config_data->pin);
			gpio_set_direction(config_data->pin, GPIO_MODE_INPUT);
			gpio_set_pull_mode(config_data->pin, GPIO_PULLUP_ONLY);
			
			gpio_set_direction(config_data->pin, GPIO_MODE_OUTPUT);
			
			gpio_set_level(config_data->pin, 0);
			ets_delay_us(INIT_DELAY);

			uint16_t hum = 0;
			uint16_t temp = 0;
			uint8_t check = 0;
			uint32_t data[82];
			int anzahl = sizeof(data)/sizeof(data[0]), i;
			
			flag = true;
			
			taskENTER_CRITICAL(&am2303_mutex); // no interrupts!
			
			gpio_set_direction(config_data->pin, GPIO_MODE_INPUT);
			gpio_set_pull_mode(config_data->pin, GPIO_PULLUP_ONLY);
			
			if (config_data->init_set_active_high)
				gpio_set_level(config_data->pin, 1); // normally not needed when using the MPSFET level shifter
			// ets_delay_us(30);
			
			for (i=0; i<anzahl; i++)
			{
				if ((data[i] = read_pulse(config_data->pin, i%2, PULSE_TIMEOUT)) == 0)
				{
					flag = false;
					break;
				}
			}					
			
			taskEXIT_CRITICAL(&am2303_mutex); // reenable interrupts
			
			if (flag)
			{
				for (i=0; i<anzahl; i+=2)
				{
					if (i<2)
					{
						ESP_LOGV(log_tag, "Init --- Low: %u, High: %u", data[i], data[i+1]);
//						if (data[i] < (i || !config_data->init_set_active_high ? INIT_RESPONSE_PULSE_WIDTH_MIN : INIT_RESPONSE_PULSE_WIDTH_MIN - 20) || data[i] > INIT_RESPONSE_PULSE_WIDTH_MAX)
						if (data[i] < INIT_RESPONSE_PULSE_WIDTH_MIN || data[i] > INIT_RESPONSE_PULSE_WIDTH_MAX)
						{
							ESP_LOGD(log_tag, "Init %d out of range", i);
							flag = false;
						}
					}
					else
					{
						ESP_LOGV(log_tag, "Data %d --- Low: %u, High: %u", i/2, data[i], data[i+1]);
						if (i<34)
						{
							hum <<= 1;
							if (data[i+1] >= 35)
								hum |= 1;
						}
						else if (i<66)
						{
							temp <<= 1;
							if (data[i+1] >= 35)
								temp |= 1;
						}
						else
						{
							check <<= 1;
							if (data[i+1] >= 35)
								check |= 1;
						}
					}
				}
				if (flag)
				{
					if (( (((temp >>8 ) & 0xFF) + ((temp & 0xFF)) + ((hum >>8 ) & 0xFF) + (hum & 0xFF)) & 0xFF) != check)
					{
						ESP_LOGD(log_tag, "Invalid checksum");
						flag = false;
					}
					else
					{
						if (temperature)
							*temperature = temp/10.0f;
						if (humidity)
							*humidity = hum/10.0f;
						
						config_data->last_temperature = temp;
						config_data->last_humidity = hum;
						config_data->last_data_valid = true;
					}
				}
			}
			else
				{ESP_LOGD(log_tag, "Read error");}
				
			config_data->last_read = xTaskGetTickCount();
		}
		else
		{
			// Too short read interval - use old values
			if (config_data->last_data_valid)
			{
				if (temperature)
					*temperature = config_data->last_temperature/10.0f;
				if (humidity)
					*humidity = config_data->last_humidity/10.0f;
				flag = true;
			}
			ESP_LOGW(log_tag, "Too short time interval between two reads !!!");
		}
	}
	else
		{ESP_LOGE(log_tag, "The pin %u is an input pin only !!!", config_data->pin);}
	
	return flag;
}
