#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"

#include "am2302_helper.h"

#include "sdkconfig.h"


static const char* log_tag = "sensor_test";

#define AM2302_PIN		CONFIG_AM3202_GPIO_PIN

void app_main()
{
	float temperature, humidity;
	am2302_data_t data;
	am2302_init_data(&data, AM2302_PIN);
	// data.init_set_active_high = true; // without MOSFET Level-shifter you will perhaps need this
	
	while(1)
	{
		vTaskDelay(pdMS_TO_TICKS(5000));
		printf("Start reading ...\n");
		
		int64_t start = esp_timer_get_time();
		if (am2302_read_sensor(&data, &temperature, &humidity))
			printf("Temperature: %.1f °C, Humidity: %.1f %% Time: %lld µsec\n", temperature, humidity, esp_timer_get_time() - start);
		else
			{ESP_LOGE(log_tag, "read failed");}
	}
}
