/**
 * @file dr_calculator.cpp
 * @author Bernd Giesecke (bernd@giesecke.tk)
 * @brief Calculate the required datarate from the payload size
 * @version 0.2
 * @date 2024-11-18
 *
 * @copyright Copyright (c) 2024
 *
 */
#include "app.h"

uint16_t in865_eu433_ru864_eu868_ps[16] = {51, 51, 51, 115, 242, 242, 242, 242, 0, 0, 0, 0, 0, 0, 0, 0};
uint16_t au915_ps[16] = {51, 51, 51, 115, 242, 242, 242, 0, 53, 129, 242, 242, 242, 242, 0, 0};
uint16_t cn470_kr920_ps[16] = {51, 51, 51, 115, 242, 242, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
uint16_t us915_ps[16] = {11, 53, 125, 242, 242, 0, 0, 0, 53, 129, 242, 242, 242, 242, 0, 0};
uint16_t as923_ps[16] = {0, 0, 19, 61, 133, 250, 250, 250, 0, 0, 0, 0, 0, 0, 0, 0};

uint16_t *region_map[12] = {in865_eu433_ru864_eu868_ps, cn470_kr920_ps, in865_eu433_ru864_eu868_ps, in865_eu433_ru864_eu868_ps, in865_eu433_ru864_eu868_ps,
							us915_ps, au915_ps, cn470_kr920_ps, as923_ps, as923_ps, as923_ps, as923_ps};

/**
 * @brief Get the minimum datarate based on region and required payload size
 *
 * @param region LoRaWAN region
 *               0 = EU433, 1 = CN470, 2 = RU864, 3 = IN865, 4 = EU868, 5 = US915,
 *               6 = AU915, 7 = KR920, 8 = AS923-1 , 9 = AS923-2 , 10 = AS923-3 , 11 = AS923-4)
 * @param payload_size required payload size
 * @return uint8_t datarate 0 to 15 or 16 if no matching DR could be found
 */
uint8_t get_min_dr(uint16_t region, uint16_t payload_size)
{
	// MYLOG("DR_CALC", "Got region %d and payload size %d", region, payload_size);
	// Get the datarate - payload size map
	uint16_t *region_ps = region_map[region];
	// MYLOG("DR_CALC", "Available packet sizes:");
	// for (int idx = 0; idx < 15; idx++)
	// {
	// 	Serial.printf("DR %d max size %d\n", idx, region_ps[idx]);
	// }
	// Go through all payload sizes
	for (uint8_t idx = 0; idx < 16; idx++)
	{
		// Check if dr payload size is larger than requested payload size
		if (payload_size < region_ps[idx])
		{
			// Found a datarate that can carry the payload size
			return idx;
		}
	}
	// No matching datarate for the payload size found
	return 16;
}

/**
 * @brief Check if packet fits with current DR
 * 
 * @return true Packet size ok
 * @return false Packet is too large
 */
bool check_dr(uint16_t packet_len)
{
	// Check DR and packet size
	uint8_t new_dr = get_min_dr(api.lorawan.band.get(), packet_len);
	MYLOG("UPLINK", "Get DR for packet len %d returned %d, current is %d", packet_len, new_dr, api.lorawan.dr.get());
	if (new_dr <= api.lorawan.dr.get())
	{
		MYLOG("UPLINK", "Possible Datarate is ok or smaller than current");
		return true;
	}
	else
	{
		MYLOG("UPLINK", "Datarate doesn't allow packet size");
		if (has_oled && !g_settings_ui)
		{
			oled_add_line((char *)"Packet too large!");
			sprintf(line_str, "Requires DR%d", new_dr);
			oled_add_line(line_str);
			tx_active = false;
			// Do not send packet
			return false;
		}
	}
}