/**
 * @file rtc.cpp
 * @author Bernd Giesecke (bernd.giesecke@rakwireless.com)
 * @brief Initialization and usage of RAK12002 RTC module
 * @version 0.1
 * @date 2022-02-18
 *
 * @copyright Copyright (c) 2022
 *
 */
#include "app.h"
#include <Melopero_RV3028.h>

/** Instance of the RTC class */
Melopero_RV3028 rtc;

date_time_s g_date_time;

bool has_rtc = false;

/**
 * @brief Initialize the RTC
 *
 * @return true if success
 * @return false if failed
 */
bool init_rak12002(void)
{
	Wire.begin();

	// Scan the I2C interfaces for devices
	byte error;
	bool found_rtc = false;

	for (byte address = 1; address < 127; address++)
	{
		Wire.beginTransmission(address);
		error = Wire.endTransmission();
		if (error == 0)
		{
			MYLOG("SCAN", "Found sensor on I2C1 0x%02X\n", address);
			if (address == 0x52)
			{
				found_rtc = true;
				break;
			}
		}
	}

	if (!found_rtc)
	{
		MYLOG("RTC", "No RTC present");
		return false;
	}

	rtc.initI2C(Wire);

	rtc.useEEPROM(false);

	rtc.writeToRegister(0x35, 0x00);
	rtc.writeToRegister(0x37, 0xB4); // Direct Switching Mode (DSM): when VDD < VBACKUP, switchover occurs from VDD to VBACKUP

	rtc.set24HourMode(); // Set the device to use the 24hour format (default) instead of the 12 hour format

	g_date_time.year = rtc.getYear();
	g_date_time.month = rtc.getMonth();
	g_date_time.weekday = rtc.getWeekday();
	g_date_time.date = rtc.getDate();
	g_date_time.hour = rtc.getHour();
	g_date_time.minute = rtc.getMinute();
	g_date_time.second = rtc.getSecond();

	if (g_date_time.year > 2060)
	{
		// MYLOG("RTC", "Invalid year, no RTC present");
		return false;
	}
	MYLOG("RTC", "%d.%02d.%02d %d:%02d:%02d", g_date_time.year, g_date_time.month, g_date_time.date, g_date_time.hour, g_date_time.minute, g_date_time.second);
	return true;
}

/**
 * @brief Get time as UNIX timestamp
 *
 * @return uint32_t Unix timestamp
 */
uint32_t get_unixtime_rak12002(void)
{
	return rtc.getUnixTime();
}

/**
 * @brief Set the RAK12002 date and time
 *
 * @param year in 4 digit format, e.g. 2020
 * @param month 1 to 12
 * @param date 1 to 31
 * @param hour 0 to 23
 * @param minute 0 to 59
 */
void set_rak12002(uint16_t year, uint8_t month, uint8_t date, uint8_t hour, uint8_t minute)
{
	uint8_t weekday = (date + (uint16_t)((2.6 * month) - 0.2) - (2 * (year / 100)) + year + (uint16_t)(year / 4) + (uint16_t)(year / 400)) % 7;
	// MYLOG("RTC", "Calculated weekday is %d", weekday);
	rtc.setTime(year, month, weekday, date, hour, minute, 0);
}

void set_rak12002(uint16_t year, uint8_t month, uint8_t date, uint8_t hour, uint8_t minute, uint8_t seconds)
{
	uint8_t weekday = (date + (uint16_t)((2.6 * month) - 0.2) - (2 * (year / 100)) + year + (uint16_t)(year / 4) + (uint16_t)(year / 400)) % 7;
	MYLOG("RTC", "Calculated weekday is %d", weekday);
	rtc.setTime(year, month, weekday, date, hour, minute, seconds);
}

/**
 * @brief Update g_data_time structure with current the date
 *        and time from the RTC
 *
 */
void read_rak12002(void)
{
	g_date_time.year = rtc.getYear();
	g_date_time.month = rtc.getMonth();
	g_date_time.weekday = rtc.getWeekday();
	g_date_time.date = rtc.getDate();
	g_date_time.hour = rtc.getHour();
	g_date_time.minute = rtc.getMinute();
	g_date_time.second = rtc.getSecond();
}

/**
 * @brief Get the internal MCU time stamp
 *      Saves MCU time in global structure g_date_time
 *
 */
void get_mcu_time(void)
{
	char local_time[30] = {0};
	struct tm localtime;
	SysTime_t UnixEpoch = SysTimeGet();
	UnixEpoch.Seconds -= 18;		  /*removing leap seconds*/
	UnixEpoch.Seconds += 8 * 60 * 60; // Make it GMT+8
	SysTimeLocalTime(UnixEpoch.Seconds, &localtime);
	sprintf(local_time, "%02dh%02dm%02ds on %02d/%02d/%04d", localtime.tm_hour, localtime.tm_min, localtime.tm_sec,
			localtime.tm_mon + 1, localtime.tm_mday, localtime.tm_year + 1900);
	MYLOG("RTC", " MCU time %s", local_time);

	g_date_time.year = localtime.tm_year + 1900;
	g_date_time.month = localtime.tm_mon + 1;
	g_date_time.date = localtime.tm_mday;
	g_date_time.hour = localtime.tm_hour;
	g_date_time.minute = localtime.tm_min;
	g_date_time.second = localtime.tm_sec;
}