/**
 * @file app.h
 * @author Bernd Giesecke (bernd@giesecke.tk)
 * @brief Includes and defines
 * @version 0.2
 * @date 2024-11-18
 *
 * @copyright Copyright (c) 2024
 *
 */
#ifndef _APP_H_
#define _APP_H_
#include <Arduino.h>
// ATC+PCKG=02685B0367011E05650001237D02CB017400D6306601

/** Set _RAK19026_ to have correct display orientation */
#define _RAK19026_

// Debug
// Debug output set to 0 to disable app debug output
// #ifndef MY_DEBUG
// #define MY_DEBUG 0
// #endif

#if MY_DEBUG > 0
#if defined(_VARIANT_RAK3172_) || defined(_VARIANT_RAK3172_SIP_)
#define MYLOG(tag, ...)                  \
	do                                   \
	{                                    \
		if (tag)                         \
			Serial.printf("[%s] ", tag); \
		Serial.printf(__VA_ARGS__);      \
		Serial.printf("\n");             \
		Serial.flush();                  \
	} while (0);                         \
	delay(100)
#else // RAK4630 || RAK11720
#define MYLOG(tag, ...)                  \
	do                                   \
	{                                    \
		if (tag)                         \
			Serial.printf("[%s] ", tag); \
		Serial.printf(__VA_ARGS__);      \
		Serial.printf("\r\n");           \
		Serial.flush();                  \
		Serial6.printf(__VA_ARGS__);     \
		Serial6.printf("\r\n");          \
	} while (0);                         \
	delay(100)
#endif
#else
#define MYLOG(...)
#endif

// Set firmware version (done in arduino.json when using VSC + Arduino extension)
#ifndef SW_VERSION_0
#define SW_VERSION_0 2
#define SW_VERSION_1 0
#define SW_VERSION_2 11
#endif
/** Custom flash parameters structure */
struct custom_param_s
{
	uint32_t settings_crc = 0;
	uint32_t send_interval = 30000;
	uint8_t valid_flag = 0xAA;
	uint8_t test_mode = 0;
	bool display_saver = true;
	bool location_on = false;
	uint8_t custom_packet[129] = {0x01, 0x02, 0x03, 0x04};
	uint16_t custom_packet_len = 4;
	bool dr_sweep_on = false;
	int8_t timezone = 8;
	uint32_t mesh_check_node = 0;
};
// Structure size without CRC
#define custom_params_len sizeof(custom_param_s)

typedef enum test_mode_num
{
	MODE_LINKCHECK = 0,
	MODE_P2P = 1,
	MODE_FIELDTESTER = 2,
	MODE_FIELDTESTER_V2 = 3,
	MODE_MESHTASTIC = 4,
	INVALID_MODE = 5
} test_mode_num_t;

/** Custom flash parameters */
extern custom_param_s g_custom_parameters;

// Forward declarations
bool init_status_at(void);
bool init_interval_at(void);
bool init_test_mode_at(void);
bool init_custom_pckg_at(void);
bool init_dump_logs_at(void);
bool init_rtc_at(void);
bool init_app_ver_at(void);
bool init_product_info_at(void);
bool init_config_modules_at(void);
bool init_timezone_at(void);
bool init_settings_at(void);
bool init_mesh_node_at(void);
bool get_at_setting(void);
bool save_at_setting(void);
void set_linkcheck(void);
void set_p2p(void);
void set_field_tester(void);
void send_packet(void *data);
uint8_t get_min_dr(uint16_t region, uint16_t payload_size);
bool check_dr(uint16_t packet_len);
extern uint32_t g_send_repeat_time;
extern bool lorawan_mode;
extern volatile bool tx_active;
extern volatile bool forced_tx;
extern volatile bool dr_sweep_active;
extern uint8_t sync_time_status;
extern uint16_t *region_map[];
extern volatile bool ready_to_dump;
extern volatile int32_t packet_num;
extern bool g_settings_active;
extern uint8_t fPort;

// LoRaWAN stuff
#include "wisblock_cayenne.h"
extern WisCayenne g_solution_data;

// OLED
bool init_oled(void);
void oled_add_line(char *line);
void oled_show(void);
void oled_write_header(char *header_line, bool show_error = true);
void oled_clear(void);
void oled_write_line(int16_t line, int16_t y_pos, String text);
void oled_display(void);
void oled_power(bool on_off);
void display_show_menu(char *menu[], uint8_t menu_len, uint8_t sel_menu, uint8_t sel_item, bool display_saver = false, bool location_on = false);
void oled_saver(void *);
void prepare_oled_header(void);
extern custom_param_s g_last_settings;
extern char line_str[];
extern char *g_regions_list[];
extern char *p_bw_menu[];
extern bool has_oled;

// UI
typedef enum disp_mode_num
{
	T_TOP_MENU = 0,
	T_INFO_MENU,
	T_SETT_MENU,
	T_MODE_MENU,
	T_LORAWAN_MENU,
	T_LORAP2P_MENU,
	S_SEND_INT,
	S_LPW_BAND,
	S_LPW_ADR,
	S_LPW_DR,
	S_LPW_TX,
	S_P2P_FREQ,
	S_P2P_SF,
	S_P2P_BW,
	S_P2P_CR,
	S_P2P_PPL,
	S_P2P_TX,
	S_SUB_NONE = 255
};
extern bool g_settings_ui;

// Button
#include "MillisTaskManager.h"

#define BUTTON_INT_PIN WB_IO5

/*
 * @brief button state.
 */
typedef enum
{
	SINGLE_CLICK = 0,
	DOUBLE_CLICK,
	LONG_PRESS,
	TRIPPLE_CLICK,
	QUAD_CLICK,
	FIVE_CLICK,
	SIX_CLICK,
	SEVEN_CLICK,
	BUTTONSTATE_NONE,
} buttonState_t;

bool buttonInit(void);
uint8_t getButtonStatus(void);
void handle_button(void);
void buttonIntHandle(void);
extern MillisTaskManager mtmMain;
extern volatile uint8_t pressCount;
extern volatile bool display_power;

// ACC
#include <SparkFunLIS3DH.h>
#define ACC_INT_PIN WB_IO1
bool init_acc(bool active = false);
void clear_acc_int(void);
void read_acc(void);

// GNSS
#include <SparkFun_u-blox_GNSS_Arduino_Library.h>
bool init_gnss(bool active = false);
bool poll_gnss(void);
void gnss_handler(void *);
extern bool gnss_active;
extern uint16_t check_gnss_counter;
extern uint16_t check_gnss_max_try;
extern uint8_t max_sat;
extern uint8_t max_sat_unchanged;
extern volatile float g_last_lat;
extern volatile float g_last_long;
extern volatile float g_last_accuracy;
extern volatile uint32_t g_last_altitude;
extern volatile uint8_t g_last_satellites;
extern volatile bool has_gnss_location;

// SD Card
/** Log file info structure */
struct result_s
{
	uint16_t year = 24;
	uint8_t month = 9;
	uint8_t day = 27;
	uint8_t hour = 12;
	uint8_t min = 0;
	uint8_t sec = 0;
	uint8_t mode = 0;
	uint8_t gw = 0;
	float lat = 14.421536;
	float lng = 121.006819;
	int8_t min_rssi = 0;
	int8_t max_rssi = 0;
	int8_t max_snr = 0;
	int8_t rx_rssi = 0;
	int8_t rx_snr = 0;
	int16_t min_dst = 0;
	int16_t max_dst = 0;
	int16_t demod = 0;
	int16_t lost = 0;
	int8_t tx_dr = 0;
};
bool init_sd(void);
bool create_sd_file(void);
void write_sd_entry(void);
void dump_all_sd_files(void);
void dump_sd_file(const char *path);
void clear_sd_file(void);
extern volatile result_s result;
extern volatile char file_name[];
extern bool has_sd;
extern volatile bool sd_card_error;

// RAK12002 RTC
bool init_rak12002(void);
void set_rak12002(uint16_t year, uint8_t month, uint8_t date, uint8_t hour, uint8_t minute);
void set_rak12002(uint16_t year, uint8_t month, uint8_t date, uint8_t hour, uint8_t minute, uint8_t seconds);
void read_rak12002(void);
uint32_t get_unixtime_rak12002(void);
void get_mcu_time(void);
extern bool has_rtc;
/** RTC date/time structure */
struct date_time_s
{
	uint16_t year;
	uint8_t month;
	uint8_t weekday;
	uint8_t date;
	uint8_t hour;
	uint8_t minute;
	uint8_t second;
};
extern volatile date_time_s g_date_time;

#include "meshtastic.h"

#endif // _APP_H_
