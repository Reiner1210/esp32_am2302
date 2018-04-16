#ifndef _AM2302_HELPER_H
#define _AM2302_HELPER_H

/**
 * =====================================================================================================
 * This library allows read out a AM2302 (DHT22) sensor
 * Author:  Reiner Pröls, reiner.proels@gmail.com
 * License: LGPL
 * =====================================================================================================
 * 
 * ATTENTION: if you want to operate the sensor at the recommended 5V you need a "level shifer"
 * because ESP32 uses 3.3V and the inputs are not 5V tolerant.
 * I recommend the following wiring for protecting your ESP32:
 
 
 
                       +3.3V   +3.3V    +5.0V    +5.0V     GND
                         o       o        o        o        o
                         |       |        |        |        |
                         |       |        |        |        |
                         |       |        |        |        |
                        | |      |       | |       |        |
                        | | R2   |       | | R3    |        |
                        |_|      |       |_|       |        |
                         |  ____ |______  |        |        |
               ____      | |     G      | |        |        |
  GPIO o------|____|-----o-|S  2N7000  D|-o        |        |
 (ESP32)        R1         |____________| |        |        |
                                 T1       |        |---||---|
                                          |        |   C1   |
                                       ___|________|________|______
                                      |  SDA(2)   VDD(1)   GND(4) | 
                                      |                           |
                                      |                           |
                                      |          AM2302           |
                                      |          (DHT22)          |
                                      |                           |
                                      |___________________________|
                                        
                                        
Parts:
       T1:  n-channel MOSFET e.g. 2N7000, pay attention that you connect source (S) and drain (D) correctly !
       R1:  Resistor 330 Ohm
       R2:  Resistor 10 kOhm
       R3:  Resistor 10 kOhm
       C1:  Capacitor 100 nF

       The T1, R2, and R3 work as a bidirectional level shifter.
       
       R3 is an additional protection if e.g. GPIO is set accidentally to high and SDA is low.
       I only recommend it in experimental systems, in production systems you do not need it.
       
       C1 prevents the AM2302 from oscillating. Place it as near as possible to the sensor.
       
       As GPIO you have to take a pin wich can work as output AND input.
       
       A readout of the sensor will need ~ 5-6 msec
       
*/

#ifdef __cplusplus
extern "C" {
#endif
	
typedef struct
{
	gpio_num_t pin;
	bool init_set_active_high;
	bool last_data_valid;
	uint16_t last_temperature;
	uint16_t last_humidity;
	TickType_t last_read;
} am2302_data_t;

void am2302_init_data(am2302_data_t* data, gpio_num_t pin);
bool am2302_read_sensor(am2302_data_t* data, float* temperature, float* humidity);

#ifdef __cplusplus
}
#endif

#endif // _AM2302_HELPER_H
