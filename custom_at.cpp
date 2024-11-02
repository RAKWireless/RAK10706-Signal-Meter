/**
 * @file custom_at.cpp
 * @author Bernd Giesecke (bernd@giesecke.tk)
 * @brief Custom AT commands for the application
 * @version 0.1
 * @date 2023-12-29
 *
 * @copyright Copyright (c) 2023
 *
 */
#include "app.h"
// CRC algo
#include <utilities.h>

#if defined(_VARIANT_RAK3172_) || defined(_VARIANT_RAK3172_SIP_)
#define AT_PRINTF(...)              \
	do                              \
	{                               \
		Serial.printf(__VA_ARGS__); \
		Serial.printf("\r\n");      \
	} while (0);                    \
	delay(100)
#else // RAK4630 || RAK11720
#define AT_PRINTF(...)               \
	do                               \
	{                                \
		Serial.printf(__VA_ARGS__);  \
		Serial.printf("\r\n");       \
		Serial6.printf(__VA_ARGS__); \
		Serial6.printf("\r\n");      \
	} while (0);                     \
	delay(100)
#endif

/** Custom flash parameters */
custom_param_s g_custom_parameters;
custom_param_s temp_params;

/** Flag if CRC API needs initialization */
bool crc_initialized = false;

/** Flag if log dump is possible */
volatile bool ready_to_dump = true;

/** Flag if ATC+PRD_INFO was used */
bool prd_info_requested = false;

// Forward declarations
int interval_send_handler(SERIAL_PORT port, char *cmd, stParam *param);
int status_handler(SERIAL_PORT port, char *cmd, stParam *param);
int test_mode_handler(SERIAL_PORT port, char *cmd, stParam *param);
int custom_pckg_handler(SERIAL_PORT port, char *cmd, stParam *param);
int dump_logs_handler(SERIAL_PORT port, char *cmd, stParam *param);
int rtc_command_handler(SERIAL_PORT port, char *cmd, stParam *param);
int app_ver_handler(SERIAL_PORT port, char *cmd, stParam *param);
int product_info_handler(SERIAL_PORT port, char *cmd, stParam *param);
/**
 * @brief Add send interval AT command
 *
 * @return true if success
 * @return false if failed
 */
bool init_interval_at(void)
{
	return api.system.atMode.add((char *)"SENDINT",
								 (char *)"Set/Get the interval sending time values in seconds 0 = off, max 7200 seconds (2 hours)",
								 (char *)"SENDINT", interval_send_handler,
								 RAK_ATCMD_PERM_WRITE | RAK_ATCMD_PERM_READ);
}

/**
 * @brief Handler for send interval AT command
 *
 * @param port Serial port used
 * @param cmd char array with the received AT command
 * @param param char array with the received AT command parameters
 * @return int result of command parsing
 * 			AT_OK AT command & parameters valid
 * 			AT_PARAM_ERROR command or parameters invalid
 */
int interval_send_handler(SERIAL_PORT port, char *cmd, stParam *param)
{
	if (param->argc == 1 && !strcmp(param->argv[0], "?"))
	{
		AT_PRINTF("%s=%ld", cmd, g_custom_parameters.send_interval / 1000);
	}
	else if (param->argc == 1)
	{
		MYLOG("AT_CMD", "param->argv[0] >> %s", param->argv[0]);
		for (int i = 0; i < strlen(param->argv[0]); i++)
		{
			if (!isdigit(*(param->argv[0] + i)))
			{
				MYLOG("AT_CMD", "%d is no digit", i);
				return AT_PARAM_ERROR;
			}
		}

		uint32_t new_send_freq = strtoul(param->argv[0], NULL, 10);

		MYLOG("AT_CMD", "Requested interval %ld", new_send_freq);

		if (new_send_freq >= 7201L)
		{
			return AT_PARAM_ERROR;
		}

		g_custom_parameters.send_interval = new_send_freq * 1000;

		MYLOG("AT_CMD", "New interval %ld", g_custom_parameters.send_interval);
		// Stop the timer
		api.system.timer.stop(RAK_TIMER_0);
		if (g_custom_parameters.send_interval != 0)
		{
			// Restart the timer
			api.system.timer.start(RAK_TIMER_0, g_custom_parameters.send_interval, NULL);
		}
		MYLOG("AT_CMD", "Timer restarted with %ld", g_custom_parameters.send_interval);
		// Save custom settings
		save_at_setting();
	}
	else
	{
		return AT_PARAM_ERROR;
	}

	return AT_OK;
}

/**
 * @brief Add test mode AT command
 *
 * @return true if success
 * @return false if failed
 */
bool init_test_mode_at(void)
{
	return api.system.atMode.add((char *)"MODE",
								 (char *)"Set/Get the test mode. 0 = LPWAN LinkCheck, 1 = LoRa P2P, 2 = FieldTester, 3 = FieldTester V2",
								 (char *)"MODE", test_mode_handler,
								 RAK_ATCMD_PERM_WRITE | RAK_ATCMD_PERM_READ);
}

/**
 * @brief Handler for test mode AT command
 *
 * @param port Serial port used
 * @param cmd char array with the received AT command
 * @param param char array with the received AT command parameters
 * @return int result of command parsing
 * 			AT_OK AT command & parameters valid
 * 			AT_PARAM_ERROR command or parameters invalid
 */
int test_mode_handler(SERIAL_PORT port, char *cmd, stParam *param)
{
	if (param->argc == 1 && !strcmp(param->argv[0], "?"))
	{
		AT_PRINTF("%s=%d", cmd, g_custom_parameters.test_mode);
	}
	else if (param->argc == 1)
	{
		MYLOG("AT_CMD", "param->argv[0] >> %s", param->argv[0]);
		for (int i = 0; i < strlen(param->argv[0]); i++)
		{
			if (!isdigit(*(param->argv[0] + i)))
			{
				MYLOG("AT_CMD", "%d is no digit", i);
				return AT_PARAM_ERROR;
			}
		}

		uint32_t new_mode = strtoul(param->argv[0], NULL, 10);
		uint8_t old_mode = g_custom_parameters.test_mode;

		MYLOG("AT_CMD", "Requested mode %ld", new_mode);

		if (new_mode >= INVALID_MODE)
		{
			return AT_PARAM_ERROR;
		}

		if (new_mode != old_mode)
		{
			g_custom_parameters.test_mode = new_mode;
			MYLOG("AT_CMD", "New test mode %ld", g_custom_parameters.test_mode);

			// Save custom settings
			save_at_setting();

			// Switch mode
			switch (g_custom_parameters.test_mode)
			{
			case MODE_LINKCHECK:
				set_linkcheck();
				break;
			case MODE_P2P:
				set_p2p();
				break;
			case MODE_FIELDTESTER:
			case MODE_FIELDTESTER_V2:
				set_field_tester();
				break;
			}

			// On mode change, always restart to refresh log file appearance
			AT_PRINTF("+EVT:RESTART_FOR_MODE_CHANGE");
			delay(5000);
			api.system.reboot();
		}
	}
	else
	{
		return AT_PARAM_ERROR;
	}

	return AT_OK;
}

/**
 * @brief Add custom packet AT command
 *
 * @return true if success
 * @return false if failed
 */
bool init_custom_pckg_at(void)
{
	return api.system.atMode.add((char *)"PCKG",
								 (char *)"Set/Get a custom packet (max 64 bytes)",
								 (char *)"PCKG", custom_pckg_handler,
								 RAK_ATCMD_PERM_WRITE | RAK_ATCMD_PERM_READ);
}

/**
 * @brief Handler for custom packet AT command
 *
 * @param port Serial port used
 * @param cmd char array with the received AT command
 * @param param char array with the received AT command parameters
 * @return int result of command parsing
 * 			AT_OK AT command & parameters valid
 * 			AT_PARAM_ERROR command or parameters invalid
 */
int custom_pckg_handler(SERIAL_PORT port, char *cmd, stParam *param)
{
	char temp_str[257];
	if (param->argc == 1 && !strcmp(param->argv[0], "?"))
	{
		if (g_custom_parameters.custom_packet_len == 0)
		{
			AT_PRINTF("%s=01020304", cmd);
		}
		else
		{
			atcmd_printf("%s=", cmd);
			for (uint8_t i = 0; i < g_custom_parameters.custom_packet_len; i++)
			{
				atcmd_printf("%02X", g_custom_parameters.custom_packet[i]);
			}
			atcmd_printf("\r\n");
		}
	}
	else if (param->argc == 1)
	{
		uint32_t len = strlen(param->argv[0]);
		MYLOG("AT_CMD", "param->argv[0] >> %s", param->argv[0]);
		if (0 != at_check_hex_param(param->argv[0], len, g_custom_parameters.custom_packet))
		{
			MYLOG("AT_CMD", "Invalid HEX ASCII string");
			return AT_PARAM_ERROR;
		}

		g_custom_parameters.custom_packet_len = len / 2;

		// Save custom settings
		save_at_setting();
	}
	else
	{
		return AT_PARAM_ERROR;
	}

	return AT_OK;
}

/**
 * @brief Add send interval AT command
 *
 * @return true if success
 * @return false if failed
 */
bool init_dump_logs_at(void)
{
	return api.system.atMode.add((char *)"LOGS",
								 (char *)"Get logs from SD card",
								 (char *)"LOGS", dump_logs_handler,
								 RAK_ATCMD_PERM_WRITE | RAK_ATCMD_PERM_READ);
}

/**
 * @brief Handler for send interval AT command
 *
 * @param port Serial port used
 * @param cmd char array with the received AT command
 * @param param char array with the received AT command parameters
 * @return int result of command parsing
 * 			AT_OK AT command & parameters valid
 * 			AT_PARAM_ERROR command or parameters invalid
 */
int dump_logs_handler(SERIAL_PORT port, char *cmd, stParam *param)
{
	if (!has_sd)
	{
		MYLOG("AT_CMD", "No SD card detected");
		return AT_PARAM_ERROR;
	}
	if (prd_info_requested)
	{
		return AT_OK;
	}

	if (param->argc == 1 && !strcmp(param->argv[0], "?"))
	{
		g_settings_ui = true;
		AT_PRINTF("\r\n");
		api.system.timer.stop(RAK_TIMER_0);
		api.system.timer.stop(RAK_TIMER_1);
		api.system.timer.stop(RAK_TIMER_2);
		oled_clear();
		oled_write_header("REBOOT", false);
		oled_add_line((char *)"Dumping SD card");
		oled_add_line((char *)"Do not power off");
		oled_display();

		time_t start_wait = millis();
		while (!ready_to_dump)
		{
			delay(1000);
			if ((millis() - start_wait) > 10000)
			{
				MYLOG("ATC", "Timeout waiting for TX finished");
				return AT_BUSY_ERROR;
			}
		}
		dump_all_sd_files();
		AT_PRINTF("\r\n");
		// reboot
		api.system.reboot();
	}
	else if (param->argc == 1 && !strcmp(param->argv[0], "e"))
	{
		g_settings_ui = true;
		api.system.timer.stop(RAK_TIMER_0);
		api.system.timer.stop(RAK_TIMER_1);
		api.system.timer.stop(RAK_TIMER_2);
		oled_clear();
		oled_write_header("REBOOT", false);
		oled_add_line((char *)"Erasing SD card");
		oled_add_line((char *)"Do not power off");
		oled_display();

		time_t start_wait = millis();
		while (!ready_to_dump)
		{
			delay(1000);
			if ((millis() - start_wait) > 20000)
			{
				MYLOG("ATC", "Timeout waiting for TX finished");
				return AT_BUSY_ERROR;
			}
		}

		clear_sd_file();

		// reboot
		api.system.reboot();
	}

	// else if (param->argc == 1)
	// {
	// 	return AT_PARAM_ERROR;
	// }
	else
	{
		return AT_PARAM_ERROR;
	}

	return AT_OK;
}

/**
 * @brief Add custom RTC AT commands
 *
 * @return true AT commands were added
 * @return false AT commands couldn't be added
 */
bool init_rtc_at(void)
{
	return api.system.atMode.add((char *)"RTC",
								 (char *)"Set/Get time of RTC [yyyy:mm:dd:hh:MM]",
								 (char *)"RTC", rtc_command_handler);
}

/**
 * @brief Handler for custom AT command for the RTC module
 *
 * @param port Serial port used
 * @param cmd char array with the received AT command
 * @param param char array with the received AT command parameters
 * @return int result of command parsing
 * 			AT_OK AT command & parameters valid
 * 			AT_PARAM_ERROR command or parameters invalid
 */
int rtc_command_handler(SERIAL_PORT port, char *cmd, stParam *param)
{
	// MYLOG("AT_CMD", "Param size %d", param->argc);
	if (param->argc == 1 && !strcmp(param->argv[0], "?"))
	{
		read_rak12002();
		Serial.print(cmd);
		Serial.print("=");
		Serial.printf("%d.%02d.%02d %d:%02d:%02d\n", g_date_time.year, g_date_time.month,
					  g_date_time.date, g_date_time.hour,
					  g_date_time.minute, g_date_time.second);
	}
	else if (param->argc == 5)
	{
		for (int j = 0; j < param->argc; j++)
		{
			for (int i = 0; i < strlen(param->argv[j]); i++)
			{
				if (!isdigit(*(param->argv[j] + i)))
				{
					// MYLOG("AT_CMD", "%d is no digit in param %d", i, j);
					return AT_PARAM_ERROR;
				}
			}
		}
		uint32_t year;
		uint32_t month;
		uint32_t date;
		uint32_t hour;
		uint32_t minute;

		/* Check year */
		year = strtoul(param->argv[0], NULL, 0);

		if (year > 3000)
		{
			// MYLOG("AT_CMD", "Year error %d", year);
			return AT_PARAM_ERROR;
		}

		if (year < 2022)
		{
			// MYLOG("AT_CMD", "Year error %d", year);
			return AT_PARAM_ERROR;
		}

		/* Check month */
		month = strtoul(param->argv[1], NULL, 0);

		if ((month < 1) || (month > 12))
		{
			// MYLOG("AT_CMD", "Month error %d", month);
			return AT_PARAM_ERROR;
		}

		// Check day
		date = strtoul(param->argv[2], NULL, 0);

		if ((date < 1) || (date > 31))
		{
			// MYLOG("AT_CMD", "Day error %d", date);
			return AT_PARAM_ERROR;
		}

		// Check hour
		hour = strtoul(param->argv[3], NULL, 0);

		if (hour > 24)
		{
			// MYLOG("AT_CMD", "Hour error %d", hour);
			return AT_PARAM_ERROR;
		}

		// Check minute
		minute = strtoul(param->argv[4], NULL, 0);

		if (minute > 59)
		{
			// MYLOG("AT_CMD", "Minute error %d", minute);
			return AT_PARAM_ERROR;
		}

		set_rak12002((uint16_t)year, (uint8_t)month, (uint8_t)date, (uint8_t)hour, (uint8_t)minute);

		return AT_OK;
	}
	else
	{
		return AT_PARAM_ERROR;
	}

	return AT_OK;
}

/**
 * @brief Add custom Status AT command
 *
 * @return true AT command were added
 * @return false AT command couldn't be added
 */
bool init_status_at(void)
{
	return api.system.atMode.add((char *)"STATUS",
								 (char *)"Get device information",
								 (char *)"STATUS", status_handler,
								 RAK_ATCMD_PERM_READ);
}

/** Regions as text array */
char *g_regions_list[] = {"EU433", "CN470", "RU864", "IN865", "EU868", "US915", "AU915", "KR920", "AS923", "AS923-2", "AS923-3", "AS923-4", "LA915"};
/** Network modes as text array*/
char *nwm_list[] = {"P2P", "LoRaWAN", "FSK"};
/** Available test modes as text array */
char *test_mode_list[] = {"LinkCheck", "LoRa P2P", "FieldTester"};
/**
 * @brief Print device status over Serial
 *
 * @param port Serial port used
 * @param cmd char array with the received AT command
 * @param param char array with the received AT command parameters
 * @return int result of command parsing
 * 			AT_OK AT command & parameters valid
 * 			AT_PARAM_ERROR command or parameters invalid
 */
int status_handler(SERIAL_PORT port, char *cmd, stParam *param)
{
	String value_str = "";
	int nw_mode = 0;
	int region_set = 0;
	uint8_t key_eui[16] = {0}; // efadff29c77b4829acf71e1a6e76f713

	if ((param->argc == 1 && !strcmp(param->argv[0], "?")) || (param->argc == 0))
	{
		AT_PRINTF("Device Status:");
		AT_PRINTF("Test Mode: %s", test_mode_list[g_custom_parameters.test_mode]);
		value_str = api.system.hwModel.get();
		value_str.toUpperCase();
		AT_PRINTF("Module: %s", value_str.c_str());
		AT_PRINTF("Version: %s", api.system.firmwareVer.get().c_str());
		AT_PRINTF("Send time: %d s", g_custom_parameters.send_interval / 1000);
		/// \todo
		nw_mode = api.lorawan.nwm.get();
		AT_PRINTF("Network mode %s", nwm_list[nw_mode]);
		if (nw_mode == 1)
		{
			AT_PRINTF("Network %s", api.lorawan.njs.get() ? "joined" : "not joined");
			region_set = api.lorawan.band.get();
			AT_PRINTF("Region: %d", region_set);
			AT_PRINTF("Region: %s", g_regions_list[region_set]);
			if (api.lorawan.njm.get())
			{
				AT_PRINTF("OTAA mode");
				api.lorawan.deui.get(key_eui, 8);
				AT_PRINTF("DevEUI=%02X%02X%02X%02X%02X%02X%02X%02X",
						  key_eui[0], key_eui[1], key_eui[2], key_eui[3],
						  key_eui[4], key_eui[5], key_eui[6], key_eui[7]);
				api.lorawan.appeui.get(key_eui, 8);
				AT_PRINTF("AppEUI=%02X%02X%02X%02X%02X%02X%02X%02X",
						  key_eui[0], key_eui[1], key_eui[2], key_eui[3],
						  key_eui[4], key_eui[5], key_eui[6], key_eui[7]);
				api.lorawan.appkey.get(key_eui, 16);
				AT_PRINTF("AppKey=%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X",
						  key_eui[0], key_eui[1], key_eui[2], key_eui[3],
						  key_eui[4], key_eui[5], key_eui[6], key_eui[7],
						  key_eui[8], key_eui[9], key_eui[10], key_eui[11],
						  key_eui[12], key_eui[13], key_eui[14], key_eui[15]);
			}
			else
			{
				AT_PRINTF("ABP mode");
				api.lorawan.appskey.get(key_eui, 16);
				AT_PRINTF("AppsKey=%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X",
						  key_eui[0], key_eui[1], key_eui[2], key_eui[3],
						  key_eui[4], key_eui[5], key_eui[6], key_eui[7],
						  key_eui[8], key_eui[9], key_eui[10], key_eui[11],
						  key_eui[12], key_eui[13], key_eui[14], key_eui[15]);
				api.lorawan.nwkskey.get(key_eui, 16);
				AT_PRINTF("NwksKey=%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X",
						  key_eui[0], key_eui[1], key_eui[2], key_eui[3],
						  key_eui[4], key_eui[5], key_eui[6], key_eui[7],
						  key_eui[8], key_eui[9], key_eui[10], key_eui[11],
						  key_eui[12], key_eui[13], key_eui[14], key_eui[15]);
				api.lorawan.daddr.get(key_eui, 4);
				AT_PRINTF("DevAddr=%02X%02X%02X%02X",
						  key_eui[0], key_eui[1], key_eui[2], key_eui[3]);
			}
		}
		else if (nw_mode == 0)
		{
			AT_PRINTF("Frequency = %d", api.lora.pfreq.get());
			AT_PRINTF("SF = %d", api.lora.psf.get());
			AT_PRINTF("BW = %d", api.lora.pbw.get());
			AT_PRINTF("CR = %d", api.lora.pcr.get());
			AT_PRINTF("Preamble length = %d", api.lora.ppl.get());
			AT_PRINTF("TX power = %d", api.lora.ptp.get());
		}
		else
		{
			AT_PRINTF("Frequency = %d", api.lora.pfreq.get());
			AT_PRINTF("Bitrate = %d", api.lora.pbr.get());
			AT_PRINTF("Deviaton = %d", api.lora.pfdev.get());
		}
		AT_PRINTF("Custom settings");
		AT_PRINTF("Testmode = %d", g_custom_parameters.test_mode);
		AT_PRINTF("Display saver %s", g_custom_parameters.display_saver ? "On" : "off");
		atcmd_printf("Custom Packet = ");
		for (uint8_t i = 0; i < g_custom_parameters.custom_packet_len; i++)
		{
			atcmd_printf("%02X", g_custom_parameters.custom_packet[i]);
		}
		atcmd_printf("\r\n");
	}
	else
	{
		return AT_PARAM_ERROR;
	}
	return AT_OK;
}

/**
 * @brief Add send interval AT command
 *
 * @return true if success
 * @return false if failed
 */
bool init_app_ver_at(void)
{
	return api.system.atMode.add((char *)"APPVER",
								 (char *)"Get application version",
								 (char *)"APPVER", app_ver_handler,
								 RAK_ATCMD_PERM_READ);
}

/**
 * @brief Handler for send interval AT command
 *
 * @param port Serial port used
 * @param cmd char array with the received AT command
 * @param param char array with the received AT command parameters
 * @return int result of command parsing
 * 			AT_OK AT command & parameters valid
 * 			AT_PARAM_ERROR command or parameters invalid
 */
int app_ver_handler(SERIAL_PORT port, char *cmd, stParam *param)
{
	if (param->argc == 1 && !strcmp(param->argv[0], "?"))
	{
		AT_PRINTF("%s=RUI3_Tester_V%d.%d.%d", cmd, SW_VERSION_0, SW_VERSION_1, SW_VERSION_2);
		return AT_OK;
	}
	return AT_ERROR;
}

bool init_product_info_at(void)
{
	/*****************************************************************/
	/* Add when device is available in WisToolBox                    */
	/*****************************************************************/

	// Change HW model to RAK10706
	// api.system.hwModel.set("RAK10706");
	api.system.hwModel.set("RAK4630");

	return api.system.atMode.add((char *)"PRD_INFO",
								 (char *)"Get device Information of RAK10706",
								 (char *)"PRD_INFO", product_info_handler,
								 RAK_ATCMD_PERM_READ);
}

int product_info_handler(SERIAL_PORT port, char *cmd, stParam *param)
{
	prd_info_requested = true;
	return AT_COMMAND_NOT_FOUND;

	/*****************************************************************/
	/* Add when device is available in WisToolBox                    */
	/*****************************************************************/

	// Return 
	uint8_t key_eui[16] = {0};
	if (param->argc == 1 && !strcmp(param->argv[0], "?"))
	{
		api.lorawan.deui.get(key_eui, 8);
		AT_PRINTF("%s=VC:V%d.%d.%d:%02X%02X%02X%02X%02X%02X%02X%02X:RAK10706:%02X%02X%02X%02X%02X%02X%02X%02X", cmd, SW_VERSION_0, SW_VERSION_1, SW_VERSION_2,
				  key_eui[0], key_eui[1], key_eui[2], key_eui[3],
				  key_eui[4], key_eui[5], key_eui[6], key_eui[7],
				  key_eui[0], key_eui[1], key_eui[2], key_eui[3],
				  key_eui[4], key_eui[5], key_eui[6], key_eui[7]);
		return AT_OK;
	}
	return AT_ERROR;
}
/**
 * @brief Get setting from flash
 *
 * @return false read from flash failed or invalid settings type
 */
bool get_at_setting(void)
{
	bool found_problem = false;

	MYLOG("AT_CMD", "Size of custom parameters %d", sizeof(custom_param_s));

	// // For testing, erase all flash settings
	// uint8_t erase_flash[255] = {0xff};
	// memset(erase_flash, 0xff, 255);
	// bool wr_result = api.system.flash.set(0, erase_flash, sizeof(custom_param_s));
	// if (!wr_result)
	// {
	// 	// First write attempt failed
	// 	MYLOG("AT_CMD", "Failed to erase custom parameters from Flash");
	// 	// Retry
	// 	wr_result = api.system.flash.set(0, erase_flash, sizeof(custom_param_s));
	// }
	// if (wr_result)
	// {
	// 	MYLOG("AT_CMD", "Erased custom parameters from Flash");
	// }

	uint8_t *flash_value = (uint8_t *)&temp_params.settings_crc;
	if (!api.system.flash.get(0, flash_value, sizeof(custom_param_s)))
	{
		MYLOG("AT_CMD", "Failed to read custom parameters from Flash");
		return false;
	}
	MYLOG("AT_CMD", "Got CRC: %08X", temp_params.settings_crc);

	uint32_t crc_expected = 0x00;
	// Check validity flag
	if (temp_params.valid_flag == 0xaa)
	{
		// Check CRC
		MYLOG("AT_CMD", "Create CRC");
		uint8_t *p_data = (uint8_t *)&temp_params.send_interval;
		crc_expected = Crc32(p_data, custom_params_len - (sizeof(uint32_t)));
		MYLOG("AT_CMD", "Calculated CRC %08X", crc_expected);
	}

	if (temp_params.settings_crc != crc_expected)
	{
		MYLOG("AT_CMD", "CRC error, expected %08X, got %08X", crc_expected, temp_params.settings_crc);

		// MYLOG("AT_CMD", "No valid settings found, set to default, read 0X%08X", temp_params.send_interval);
		g_custom_parameters.valid_flag = 0xaa;
		g_custom_parameters.send_interval = 30000;
		g_custom_parameters.test_mode = 0;
		g_custom_parameters.display_saver = false;
		g_custom_parameters.location_on = false;
		g_custom_parameters.custom_packet[0] = 0x01;
		g_custom_parameters.custom_packet[1] = 0x02;
		g_custom_parameters.custom_packet[2] = 0x03;
		g_custom_parameters.custom_packet[3] = 0x04;
		g_custom_parameters.custom_packet_len = 4;
		save_at_setting();
		return false;
	}
	g_custom_parameters.send_interval = temp_params.send_interval;

	if (temp_params.test_mode >= INVALID_MODE)
	{
		MYLOG("AT_CMD", "Invalid test mode found %d", temp_params.test_mode);
		g_custom_parameters.test_mode = 0;
		found_problem = true;
	}
	else
	{
		g_custom_parameters.test_mode = temp_params.test_mode;
	}

	if (temp_params.display_saver > 1)
	{
		MYLOG("AT_CMD", "Invalid display mode found %d", temp_params.display_saver);
		g_custom_parameters.display_saver = false;
		found_problem = true;
	}
	else
	{
		g_custom_parameters.display_saver = temp_params.display_saver;
	}

	if (temp_params.location_on > 1)
	{
		MYLOG("AT_CMD", "Invalid location mode found %d", temp_params.location_on);
		g_custom_parameters.location_on = false;
		found_problem = true;
	}
	else
	{
		g_custom_parameters.location_on = temp_params.location_on;
	}

	if (temp_params.custom_packet_len > 128)
	{
		MYLOG("AT_CMD", "Invalid packet_len found %d", temp_params.custom_packet_len);
		g_custom_parameters.custom_packet[0] = 0x00;
		g_custom_parameters.custom_packet_len = 0;
		found_problem = true;
	}
	else
	{
		g_custom_parameters.custom_packet_len = temp_params.custom_packet_len;
		memcpy(g_custom_parameters.custom_packet, temp_params.custom_packet, g_custom_parameters.custom_packet_len);
	}

	if (found_problem)
	{
		save_at_setting();
	}
	MYLOG("AT_CMD", "Send interval found %ld", g_custom_parameters.send_interval);
	MYLOG("AT_CMD", "Test mode found %d", g_custom_parameters.test_mode);
	MYLOG("AT_CMD", "Display mode found %s", g_custom_parameters.display_saver ? "On" : "Off");
	MYLOG("AT_CMD", "Location mode found %s", g_custom_parameters.location_on ? "On" : "Off");

	char temp[258] = {0x00};
	for (uint8_t i = 0; i < g_custom_parameters.custom_packet_len; i++)
	{
		sprintf(&temp[i * 2], "%02X", g_custom_parameters.custom_packet[i]);
	}
	MYLOG("AT_CMD", "Custom packet %s", temp);
	MYLOG("AT_CMD", "Custom packet len %d", g_custom_parameters.custom_packet_len);
	return true;
}

/**
 * @brief Save setting to flash
 *
 * @return true write to flash was successful
 * @return false write to flash failed or invalid settings type
 */
bool save_at_setting(void)
{
	MYLOG("AT_CMD", "Create CRC");
	// Create CRC
	memcpy(&temp_params.send_interval, &g_custom_parameters.send_interval, custom_params_len - (sizeof(uint32_t)));
	uint8_t *p_data = (uint8_t *)&temp_params.send_interval;
	uint32_t crc_calculated = Crc32(p_data, custom_params_len - (sizeof(uint32_t)));
	MYLOG("AT_CMD", "Calculated CRC %04X", crc_calculated);

	uint8_t *flash_value = (uint8_t *)&temp_params.settings_crc;
	temp_params.settings_crc = crc_calculated;
	// temp_params.send_interval = g_custom_parameters.send_interval;
	// temp_params.valid_flag = g_custom_parameters.valid_flag;
	// temp_params.test_mode = g_custom_parameters.test_mode;
	// temp_params.display_saver = g_custom_parameters.display_saver;
	// temp_params.location_on = g_custom_parameters.location_on;
	// memcpy(temp_params.custom_packet, g_custom_parameters.custom_packet, g_custom_parameters.custom_packet_len);
	// temp_params.custom_packet_len = g_custom_parameters.custom_packet_len;

	bool wr_result = false;
	MYLOG("AT_CMD", "Writing send interval 0X%08X ", temp_params.send_interval);
	MYLOG("AT_CMD", "Writing test mode %d ", temp_params.test_mode);
	MYLOG("AT_CMD", "Writing display mode %s ", temp_params.display_saver ? "On" : "Off");
	MYLOG("AT_CMD", "Writing location mode %s ", temp_params.location_on ? "On" : "Off");
	char temp[258] = {0x00};
	for (uint8_t i = 0; i < temp_params.custom_packet_len; i++)
	{
		sprintf(&temp[i * 2], "%02X", temp_params.custom_packet[i]);
	}
	MYLOG("AT_CMD", "Writing custom packet %s", temp);
	MYLOG("AT_CMD", "Writing custom packet len %d", temp_params.custom_packet_len);

	wr_result = api.system.flash.set(0, flash_value, sizeof(custom_param_s));
	if (!wr_result)
	{
		// Retry
		wr_result = api.system.flash.set(0, flash_value, sizeof(custom_param_s));
	}
	return wr_result;
}
