/**
 * @file RUI3-Signal-Meter-P2P-LPWAN.ino
 * @author Bernd Giesecke (bernd@giesecke.tk)
 * @brief Simple signal meter for LoRa P2P and LoRaWAN
 * @version 0.1
 * @date 2023-11-23
 *
 * @copyright Copyright (c) 2023
 *
 */
#include "app.h"

/** Last RX SNR level*/
volatile int8_t last_snr = 0;
/** Last RX RSSI level*/
volatile int16_t last_rssi = 0;
/** Link check result */
volatile uint8_t link_check_state;
/** Demodulation margin */
volatile uint8_t link_check_demod_margin;
/** Number of gateways */
volatile uint8_t link_check_gateways;
/** Sent packet counter */
volatile int32_t packet_num = 0;
/** Lost packet counter (only LPW mode)*/
volatile int32_t packet_lost = 0;
/** Last RX data rate */
volatile uint8_t last_dr = 0;
/** TX fail reason (only LPW mode)*/
volatile int32_t tx_fail_status;
/** Packet Loss Rate (only FieldTester V2 mode) */
volatile float plr;

/** TX active flag (used for manual sending in FieldTester Mode and P2P mode) */
volatile bool tx_active = false;
/** Flag if TX is manually triggered */
volatile bool forced_tx = false;
/** Flag if DR sweeping is active */
volatile bool dr_sweep_active = false;

/** LoRa mode */
bool lorawan_mode = true;

/** Flag if OLED was found */
bool has_oled = false;
/** Buffer for OLED output */
char line_str[256];
/** Flag for display handler */
uint8_t display_reason;

/** Task Manager for button press */
MillisTaskManager mtmMain;

/** LoRaWAN packet (used for FieldTester Mode only) */
WisCayenne g_solution_data(255);

/** Buffer for FieldTester downlink */
uint8_t field_tester_pckg[32];

/** Flag for GNSS readings active */
bool gnss_active = false;

/** Flag if SD card was found */
bool has_sd = false;

/** Flag if new file is created */
bool has_file = false;

/** Flag if GNSS is working */
bool has_gnss = false;

/** Name of current log file */
char volatile file_name[] = "0000-log.csv";

/** Structure for result data */
volatile result_s result;

/** Flag if TimeReq succeeded */
uint8_t sync_time_status = 0;

/**
 * @brief Send a LoRaWAN packet
 *
 * @param data unused
 */
void send_packet(void *data)
{
	tx_active = true;
	ready_to_dump = false;

	if ((g_custom_parameters.test_mode == MODE_FIELDTESTER) || (g_custom_parameters.test_mode == MODE_FIELDTESTER_V2))
	{
		// Clear payload
		g_solution_data.reset();

		if (g_custom_parameters.location_on)
		{
			if (!gnss_active)
			{
				if (has_oled && !g_settings_ui)
				{
					oled_clear();
					if (g_custom_parameters.test_mode == MODE_FIELDTESTER)
					{
						oled_write_header((char *)"RAK FieldTester");
					}
					else
					{
						oled_write_header((char *)"RAK FieldTest V2");
					}
					oled_add_line((char *)"Start location acquisition");
				}

				// Check if we already have a sufficient location fix
				if (poll_gnss())
				{
					// if (has_oled && !g_settings_ui)
					// {
					// oled_clear();
					// oled_add_line((char *)"Got Location Fix");
					// sprintf(line_str, "La %.4f Lo %.4f", g_last_lat / 10000000.0, g_last_long / 10000000.0);
					// oled_add_line(line_str);
					// sprintf(line_str, "HDOP %.2f Sat: %d", g_last_accuracy / 100.0, g_last_satellites);
					// oled_add_line(line_str);
					// }

					// Get gateway time
					if (sync_time_status == 0)
					{
						MYLOG("APP", "Request time");
						api.lorawan.timereq.set(1);
					}
					// Check if packet size fits DR
					if (!check_dr(g_custom_parameters.custom_packet_len))
					{
						return;
					}

					// Always send confirmed packet to make sure a reply is received
					if (!api.lorawan.send(g_solution_data.getSize(), g_solution_data.getBuffer(), 1, true, 1))
					{
						MYLOG("APP", "LoRaWAN send returned error");
						tx_active = false;
						ready_to_dump = true;
					}
					// Increase sent packet number
					packet_num++;
				}
				else
				{
					if (forced_tx || dr_sweep_active) // Send immediately, with or without location
					{
						forced_tx = false;
						if (has_oled && !g_settings_ui)
						{
							sprintf(line_str, "Force TX w/o location");
							oled_add_line(line_str);
						}
						// If forced TX, send whether we have location or not 143050416, 1206306357
						if (g_custom_parameters.test_mode == MODE_FIELDTESTER_V2)
						{
							g_solution_data.addGNSS_T2(0, 0, (int16_t)packet_num);
						}
						else
						{
							g_solution_data.addGNSS_T(0, 0, 0, 1, 0);
						}

						// Get gateway time
						if (sync_time_status == 0)
						{
							MYLOG("GNSS", "Request time");
							api.lorawan.timereq.set(1);
						}

						// Check if packet size fits DR
						if (!check_dr(g_custom_parameters.custom_packet_len))
						{
							return;
						}

						if (!api.lorawan.send(g_solution_data.getSize(), g_solution_data.getBuffer(), 1, true, 1))
						{
							tx_active = false;
							MYLOG("GNSS", "LoRaWAN send returned error");
							ready_to_dump = true;
						}
						else
						{
							tx_active = true;
						}
						// Increase sent packet number
						packet_num++;
					}
					else // Wait for location fix
					{
						// Start checking for valid location
						// Set flag for GNSS active to avoid retrigger */
						gnss_active = true;
						g_solution_data.reset();
						check_gnss_counter = 0;
						// Max location aquisition time is half of send frequency
						check_gnss_max_try = g_custom_parameters.send_interval / 2 / 2500;
						// Reset satellites check values
						max_sat = 0;
						max_sat_unchanged = 0;
						// Start the timer
						api.system.timer.start(RAK_TIMER_3, 2500, NULL);
					}
				}
			}
			else
			{
				if (has_oled && !g_settings_ui)
				{
					oled_clear();
					oled_write_header((char *)"RAK FieldTester");
					oled_add_line((char *)"Acquisition ongoing");
				}

				MYLOG("APP", "GNSS already active");
			}
		}
		else
		{
			if (has_oled && !g_settings_ui)
			{
				oled_clear();
				oled_write_header((char *)"RAK FieldTester");
				oled_add_line((char *)"Indoor test");
			}
			// Location is switched off, send indoor test packet
			if (g_custom_parameters.test_mode == MODE_FIELDTESTER_V2)
			{
				g_solution_data.addGNSS_T2(0, 0, (int16_t)packet_num);
			}
			else
			{
				g_solution_data.addGNSS_T(0, 0, 0, 1.0, 0);
			}

			// Get gateway time
			if (sync_time_status == 0)
			{
				api.lorawan.timereq.set(1);
				MYLOG("APP", "Request time");
			}

			// Check if packet size fits DR
			if (!check_dr(g_custom_parameters.custom_packet_len))
			{
				return;
			}

			// Always send confirmed packet to make sure a reply is received
			if (!api.lorawan.send(g_solution_data.getSize(), g_solution_data.getBuffer(), 1, true, 1))
			{
				MYLOG("APP", "LoRaWAN send returned error");
				tx_active = false;
			}
			// Increase sent packet number
			packet_num++;
		}
	}
	else // LinkCheck or LoRa P2P
	{
		if (has_oled && !g_settings_ui)
		{
			oled_clear();
			oled_write_header((char *)"RAK Signal Meter");
			forced_tx = false;
		}

		g_solution_data.reset();

		if (api.lorawan.nwm.get())
		{
			if (api.lorawan.njs.get())
			{
				digitalWrite(LED_BLUE, HIGH);
				MYLOG("APP", "Send packet");
				if (dr_sweep_active)
				{
					oled_add_line((char *)"DR sweep sending");
				}
				else if (forced_tx)
				{
					oled_add_line((char *)"Forced sending");
				}
				else
				{
					oled_add_line((char *)"Start sending");
				}

				// // Check DR ==> Not good to change DR automatically!!!
				// uint8_t new_dr = get_min_dr(api.lorawan.band.get(), g_custom_parameters.custom_packet_len);
				// MYLOG("UPLINK", "Get DR for packet len %d returned %d, current is %d", g_custom_parameters.custom_packet_len, new_dr, api.lorawan.dr.get());
				// if (new_dr <= api.lorawan.dr.get())
				// {
				// 	MYLOG("UPLINK", "Possible Datarate is ok or smaller than current");
				// }
				// else
				// {
				// 	api.lorawan.dr.set(new_dr);
				// 	delay(500);
				// 	MYLOG("UPLINK", "Datarate changed to %d", api.lorawan.dr.get());
				// 	if (has_oled && !g_settings_ui)
				// 	{
				// 		oled_add_line((char *)"Packet too large!");
				// 		sprintf(line_str, "New DR%d", new_dr);
				// 		oled_add_line(line_str);
				// 	}
				// 	MYLOG("UPLINK", "Datarate changed to %d", api.lorawan.dr.get());
				// }

				// Get gateway time
				if (sync_time_status == 0)
				{
					MYLOG("APP", "Request time");
					api.lorawan.timereq.set(1);
				}
				// Always send confirmed packet to make sure a reply is received
				if (g_custom_parameters.location_on)
				{
					if (!poll_gnss())
					{
						g_last_long = 0.0;
						g_last_lat = 0.0;
					}

					// Check if packet size fits DR
					if (!check_dr(g_solution_data.getSize()))
					{
						return;
					}
					if (!api.lorawan.send(g_solution_data.getSize(), g_solution_data.getBuffer(), 1, true, 1))
					{
						tx_active = false;
						MYLOG("APP", "LoRaWAN send returned error");
						ready_to_dump = true;
					}
					// Increase sent packet number
					packet_num++;
				}
				else
				{
					// Check if packet size fits DR
					if (!check_dr(g_custom_parameters.custom_packet_len))
					{
						return;
					}
					if (!api.lorawan.send(g_custom_parameters.custom_packet_len, g_custom_parameters.custom_packet, 2, true, 1))
					{
						tx_active = false;
						MYLOG("APP", "LoRaWAN send returned error");
						ready_to_dump = true;
					}
					// Increase sent packet number
					packet_num++;
				}
			}
			else
			{
				tx_active = false;
				MYLOG("APP", "Not joined, don't send packet");
				ready_to_dump = true;
			}
		}
		else
		{
			digitalWrite(LED_GREEN, HIGH);
			MYLOG("APP", "Send P2P packet");
			oled_add_line((char *)"Start sending");

			// Always send with CAD
			api.lora.psend(g_custom_parameters.custom_packet_len, g_custom_parameters.custom_packet, true);
			tx_active = true;
			// Increase sent packet number
			packet_num++;
		}
	}
}

/**
 * @brief Display handler
 *
 * @param reason 1 = RX packet display (only P2P mode)
 *               2 = TX failed display (only LPW LinkCheck mode)
 *               3 = Join failed (only LPW mode)
 *               4 = Linkcheck result display (only LPW LinkCheck mode)
 *               5 = Join success (only LPW mode)
 *               6 = FieldTester downlink packet (only FieldTester mode )
 *               7 = FieldTester no downlink packet (only FieldTester mode )
 *               8 = P2P TX finished (only P2P mode)
 */
void handle_display(void *reason)
{
	digitalWrite(LED_BLUE, LOW);
	digitalWrite(LED_GREEN, LOW);
	/** Update header and battery value */
	if (has_oled && !g_settings_ui)
	{
		oled_clear();
		if (g_custom_parameters.test_mode == MODE_FIELDTESTER)
		{
			sprintf(line_str, "RAK FieldTester");
		}
		else if (g_custom_parameters.test_mode == MODE_FIELDTESTER_V2)
		{
			sprintf(line_str, "RAK FieldTest V2");
		}
		else
		{
			sprintf(line_str, "RAK Signal Meter");
		}
		oled_write_header(line_str);
	}
	// Get the wakeup reason
	uint8_t *disp_reason = (uint8_t *)reason;

	// Check if we have a reason
	if (disp_reason == NULL)
	{
		MYLOG("APP", "Bug in code!");
	}
	else if (disp_reason[0] == 1) // RX packet display (only P2P mode)
	{
		// MYLOG("APP", "RX_EVENT %d, disp_reason[0]);
		// RX event display
		if (has_sd)
		{
			if (has_rtc)
			{
				read_rak12002();
			}
			else
			{
				get_mcu_time();
			}
			result.year = g_date_time.year;
			result.month = g_date_time.month;
			result.day = g_date_time.date;
			result.hour = g_date_time.hour;
			result.min = g_date_time.minute;
			result.sec = g_date_time.second;
			result.mode = MODE_P2P;
			result.gw = 0;
			result.lat = g_last_lat;
			result.lng = g_last_long;
			result.min_rssi = 0;
			result.max_rssi = 0;
			result.rx_rssi = last_rssi;
			result.rx_snr = last_snr;
			result.min_dst = 0;
			result.max_dst = 0;
			result.demod = 0;
			result.lost = packet_lost;
			write_sd_entry();
		}
		if (has_oled && !g_settings_ui)
		{
			sprintf(line_str, "LoRa P2P mode");
			oled_write_line(0, 0, line_str);
			sprintf(line_str, "Received packets %d", packet_num);
			oled_write_line(1, 0, line_str);
			sprintf(line_str, "F %.3f", (api.lora.pfreq.get() / 1000000.0));
			oled_write_line(2, 0, line_str);
			sprintf(line_str, "SF %d", api.lora.psf.get());
			oled_write_line(3, 0, line_str);
			// 0 = 125, 1 = 250, 2 = 500, 3 = 7.8, 4 = 10.4, 5 = 15.63, 6 = 20.83, 7 = 31.25, 8 = 41.67, 9 = 62.5
			char bw_str[7];
			switch (api.lora.pbw.get())
			{
			case 0:
				sprintf(bw_str, "125");
				break;
			case 1:
				sprintf(bw_str, "250");
				break;
			case 2:
				sprintf(bw_str, "500");
				break;
			case 3:
				sprintf(bw_str, "7.8");
				break;
			case 4:
				sprintf(bw_str, "10.4");
				break;
			case 5:
				sprintf(bw_str, "15.63");
				break;
			case 6:
				sprintf(bw_str, "20.83");
				break;
			case 7:
				sprintf(bw_str, "31.25");
				break;
			case 8:
				sprintf(bw_str, "41.67");
				break;
			case 9:
				sprintf(bw_str, "62.5");
				break;
			default:
				sprintf(bw_str, "???");
				break;
			}
			sprintf(line_str, "BW %s", p_bw_menu[api.lora.pbw.get()]); // bw_str
			oled_write_line(3, 64, line_str);
			sprintf(line_str, "CR 4/%d", api.lora.pcr.get() + 5);
			oled_write_line(2, 64, line_str);
			sprintf(line_str, "RSSI %d", last_rssi);
			oled_write_line(4, 0, line_str);
			sprintf(line_str, "SNR %d", last_snr);
			oled_write_line(4, 64, line_str);
			oled_display();
		}
		MYLOG("APP", "LPW P2P mode");
		MYLOG("APP", "Packet # %d RSSI %d SNR %d", packet_num, last_rssi, last_snr);
		MYLOG("APP", "F %.3f SF %d BW %d",
			  (float)api.lora.pfreq.get() / 1000000.0,
			  api.lora.psf.get(),
			  (api.lora.pbw.get() + 1) * 125);
	}
	else if (display_reason == 2) // TX failed display (only LPW LinkCheck mode)
	{
		tx_active = false;

		if (has_sd)
		{
			if (has_rtc)
			{
				read_rak12002();
			}
			else
			{
				get_mcu_time();
			}
			result.year = g_date_time.year;
			result.month = g_date_time.month;
			result.day = g_date_time.date;
			result.hour = g_date_time.hour;
			result.min = g_date_time.minute;
			result.sec = g_date_time.second;
			result.mode = MODE_LINKCHECK;
			result.gw = 0;
			result.lat = g_last_lat;
			result.lng = g_last_long;
			result.min_rssi = 0;
			result.max_rssi = 0;
			result.rx_rssi = 0;
			result.rx_snr = 0;
			result.min_dst = 0;
			result.max_dst = 0;
			result.demod = 0;
			result.lost = packet_lost;
			result.tx_dr = api.lorawan.dr.get();
			write_sd_entry();
		}
		if (has_oled && !g_settings_ui)
		{
			sprintf(line_str, "LinkCheck Mode");
			oled_write_line(0, 0, line_str);
			sprintf(line_str, "TX Error ", tx_fail_status);
			oled_write_line(2, 0, line_str);
			switch (tx_fail_status)
			{
			case RAK_LORAMAC_STATUS_ERROR:
				sprintf(line_str, "Service error");
				break;
			case RAK_LORAMAC_STATUS_TX_TIMEOUT:
				sprintf(line_str, "TX timeout");
				break;
			case RAK_LORAMAC_STATUS_RX1_TIMEOUT:
				sprintf(line_str, "RX1 timeout");
				break;
			case RAK_LORAMAC_STATUS_RX2_TIMEOUT:
				sprintf(line_str, "RX2 timeout");
				break;
			case RAK_LORAMAC_STATUS_RX1_ERROR:
				sprintf(line_str, "RX1 error");
				break;
			case RAK_LORAMAC_STATUS_RX2_ERROR:
				sprintf(line_str, "RX2 error");
				break;
			case RAK_LORAMAC_STATUS_JOIN_FAIL:
				sprintf(line_str, "Join failed");
				break;
			case RAK_LORAMAC_STATUS_DOWNLINK_REPEATED:
				sprintf(line_str, "Dowlink frame error");
				break;
			case RAK_LORAMAC_STATUS_TX_DR_PAYLOAD_SIZE_ERROR:
				sprintf(line_str, "Payload size error");
				break;
			case RAK_LORAMAC_STATUS_DOWNLINK_TOO_MANY_FRAMES_LOSS:
				sprintf(line_str, "Fcnt loss error");
				break;
			case RAK_LORAMAC_STATUS_ADDRESS_FAIL:
				sprintf(line_str, "Adress error");
				break;
			case RAK_LORAMAC_STATUS_MIC_FAIL:
				sprintf(line_str, "MIC error");
				break;
			case RAK_LORAMAC_STATUS_MULTICAST_FAIL:
				sprintf(line_str, "Multicast error");
				break;
			case RAK_LORAMAC_STATUS_BEACON_LOCKED:
				sprintf(line_str, "Beacon locked");
				break;
			case RAK_LORAMAC_STATUS_BEACON_LOST:
				sprintf(line_str, "Beacon lost");
				break;
			case RAK_LORAMAC_STATUS_BEACON_NOT_FOUND:
				sprintf(line_str, "Beacon not found");
				break;
			default:
				sprintf(line_str, "Unknown error");
				break;
			}
			oled_write_line(4, 0, line_str);
			oled_display();
		}
	}
	else if (disp_reason[0] == 3) // Join failed (only LPW mode)
	{
		// MYLOG("APP", "JOIN_ERROR %d\n", disp_reason[0]);
		if (has_oled && !g_settings_ui)
		{
			switch (g_custom_parameters.test_mode)
			{
			case MODE_LINKCHECK:
				oled_write_line(0, 0, (char *)"LinkCheck mode");
				break;
			case MODE_FIELDTESTER:
				oled_write_line(0, 0, (char *)"FieldTester mode");
				break;
			case MODE_FIELDTESTER_V2:
				oled_write_line(0, 0, (char *)"FieldTester V2 mode");
				break;
			}
			sprintf(line_str, "Test interval  %lds", g_custom_parameters.send_interval / 1000);
			oled_write_line(1, 0, line_str);
			oled_write_line(2, 0, (char *)" ");
			sprintf(line_str, "Join failed");
			oled_write_line(3, 0, line_str);
			oled_write_line(4, 0, (char *)" ");
			oled_display();
		}
	}
	else if (disp_reason[0] == 4) // Linkcheck result display (only LPW LinkCheck mode)
	{
		// MYLOG("APP", "LINK_CHECK %d\n", disp_reason[0]);
		// LinkCheck result event display
		if (has_sd)
		{
			if (has_rtc)
			{
				read_rak12002();
			}
			else
			{
				get_mcu_time();
			}
			result.year = g_date_time.year;
			result.month = g_date_time.month;
			result.day = g_date_time.date;
			result.hour = g_date_time.hour;
			result.min = g_date_time.minute;
			result.sec = g_date_time.second;
			result.mode = MODE_LINKCHECK;
			result.gw = link_check_gateways;
			result.lat = g_last_lat;
			result.lng = g_last_long;
			result.min_rssi = last_rssi;
			result.max_rssi = last_rssi;
			result.rx_rssi = last_rssi;
			result.rx_snr = last_snr;
			result.min_dst = 0;
			result.max_dst = 0;
			result.demod = link_check_demod_margin;
			result.lost = packet_lost;
			result.tx_dr = api.lorawan.dr.get();
			write_sd_entry();
		}
		if (has_oled && !g_settings_ui)
		{
			sprintf(line_str, "LPW LinkCheck %s", link_check_state == 0 ? "OK" : "NOK");
			oled_write_line(0, 0, line_str);

			if (link_check_state == 0)
			{
				sprintf(line_str, "UL Demod Margin  %d", link_check_demod_margin);
				oled_write_line(1, 0, line_str);
				sprintf(line_str, "UL DR %d", api.lorawan.dr.get());
				oled_write_line(2, 0, line_str);
				sprintf(line_str, "%d GW(s)", link_check_gateways);
				oled_write_line(2, 64, line_str);
				sprintf(line_str, "Sent %d", packet_num);
				oled_write_line(3, 0, line_str);
				sprintf(line_str, "Lost %d", packet_lost);
				oled_write_line(3, 64, line_str);
				sprintf(line_str, "DL RSSI %d", last_rssi);
				oled_write_line(4, 0, line_str);
				sprintf(line_str, "DL SNR %d", last_snr);
				oled_write_line(4, 64, line_str);
			}
			else
			{
				sprintf(line_str, "Sent %d", packet_num);
				oled_write_line(1, 0, line_str);
				sprintf(line_str, "Lost %d", packet_lost);
				oled_write_line(1, 64, line_str);
				sprintf(line_str, "LinkCheck result %d ", link_check_state);
				oled_write_line(2, 0, line_str);
				switch (link_check_state)
				{
				case RAK_LORAMAC_STATUS_ERROR:
					sprintf(line_str, "Service error");
					break;
				case RAK_LORAMAC_STATUS_TX_TIMEOUT:
					sprintf(line_str, "TX timeout");
					break;
				case RAK_LORAMAC_STATUS_RX1_TIMEOUT:
					sprintf(line_str, "RX1 timeout");
					break;
				case RAK_LORAMAC_STATUS_RX2_TIMEOUT:
					sprintf(line_str, "RX2 timeout");
					break;
				case RAK_LORAMAC_STATUS_RX1_ERROR:
					sprintf(line_str, "RX1 error");
					break;
				case RAK_LORAMAC_STATUS_RX2_ERROR:
					sprintf(line_str, "RX2 error");
					break;
				case RAK_LORAMAC_STATUS_JOIN_FAIL:
					sprintf(line_str, "Join failed");
					break;
				case RAK_LORAMAC_STATUS_DOWNLINK_REPEATED:
					sprintf(line_str, "Dowlink frame error");
					break;
				case RAK_LORAMAC_STATUS_TX_DR_PAYLOAD_SIZE_ERROR:
					sprintf(line_str, "Payload size error");
					break;
				case RAK_LORAMAC_STATUS_DOWNLINK_TOO_MANY_FRAMES_LOSS:
					sprintf(line_str, "Fcnt loss error");
					break;
				case RAK_LORAMAC_STATUS_ADDRESS_FAIL:
					sprintf(line_str, "Adress error");
					break;
				case RAK_LORAMAC_STATUS_MIC_FAIL:
					sprintf(line_str, "MIC error");
					break;
				case RAK_LORAMAC_STATUS_MULTICAST_FAIL:
					sprintf(line_str, "Multicast error");
					break;
				case RAK_LORAMAC_STATUS_BEACON_LOCKED:
					sprintf(line_str, "Beacon locked");
					break;
				case RAK_LORAMAC_STATUS_BEACON_LOST:
					sprintf(line_str, "Beacon lost");
					break;
				case RAK_LORAMAC_STATUS_BEACON_NOT_FOUND:
					sprintf(line_str, "Beacon not found");
					break;
				default:
					sprintf(line_str, "Unknown error");
					break;
				}
				oled_write_line(3, 0, line_str);

				sprintf(line_str, "TX DR %d", api.lorawan.dr.get());
				oled_write_line(4, 0, line_str);
				oled_display();
			}
			oled_display();
		}
		MYLOG("APP", "LinkCheck %s", link_check_state == 0 ? "OK" : "NOK");
		MYLOG("APP", "Packet # %d RSSI %d SNR %d", packet_num, last_rssi, last_snr);
		MYLOG("APP", "GW # %d Demod Margin %d", link_check_gateways, link_check_demod_margin);
	}
	else if (disp_reason[0] == 5) // Join success (only LPW mode)
	{
		// MYLOG("APP", "JOIN_SUCCESS %d\n", disp_reason[0]);
		if (has_oled && !g_settings_ui)
		{
			switch (g_custom_parameters.test_mode)
			{
			case MODE_LINKCHECK:
				oled_write_line(0, 0, (char *)"LinkCheck mode");
				break;
			case MODE_FIELDTESTER:
				oled_write_line(0, 0, (char *)"FieldTester mode");
				break;
			case MODE_FIELDTESTER_V2:
				oled_write_line(0, 0, (char *)"FieldTester V2 mode");
				break;
			}
			sprintf(line_str, "Test interval  %lds", g_custom_parameters.send_interval / 1000);
			oled_write_line(1, 0, line_str);
			oled_write_line(2, 0, (char *)" ");
			sprintf(line_str, "Device joined network");
			oled_write_line(3, 0, line_str);
			oled_write_line(4, 0, (char *)" ");
			oled_display();
		}
	}
	else if (display_reason == 6) // FieldTester downlink packet (only FieldTester mode )
	{
		// 01 01 a7 00 00 02 01 d5 09 20 ca
		if (g_custom_parameters.test_mode == MODE_FIELDTESTER_V2)
		{
			uint16_t plr_i = ((uint16_t)field_tester_pckg[0] << 8) + (uint16_t)field_tester_pckg[1];
			plr = plr_i / 10;
			int8_t max_rssi = field_tester_pckg[2] - 200;
			int16_t min_distance = (int16_t)field_tester_pckg[3] * 250;
			int16_t max_distance = (int16_t)field_tester_pckg[4] * 250;
			int8_t num_gateways = field_tester_pckg[5] >> 4;
			int16_t seq_id = (int16_t)field_tester_pckg[5] & 0x0F + (int16_t)field_tester_pckg[6];
			int8_t max_snr = field_tester_pckg[7] - 200;
			uint8_t gw_eui[8] = {0xac, 0x1f, 0x09, 0xff, 0xfe, 0x00, 0x00, 0x00};
			gw_eui[5] = field_tester_pckg[8];
			gw_eui[6] = field_tester_pckg[9];
			gw_eui[7] = field_tester_pckg[10];
			MYLOG("APP", "+EVT:FieldTester V2 %d gateways", num_gateways);
			MYLOG("APP", "+EVT:RSSI max %d, SNR max %d", max_rssi, max_snr);
			MYLOG("APP", "+EVT:Distance min %d max %d", min_distance, max_distance);
			if (has_sd)
			{
				if (has_rtc)
				{
					read_rak12002();
				}
				else
				{
					get_mcu_time();
				}
				result.year = g_date_time.year;
				result.month = g_date_time.month;
				result.day = g_date_time.date;
				result.hour = g_date_time.hour;
				result.min = g_date_time.minute;
				result.sec = g_date_time.second;
				result.mode = MODE_FIELDTESTER_V2;
				result.gw = num_gateways;
				result.lat = g_last_lat;
				result.lng = g_last_long;
				result.min_rssi = 0;
				result.max_rssi = max_rssi;
				result.max_snr = max_snr;
				result.rx_rssi = last_rssi;
				result.rx_snr = last_snr;
				result.min_dst = min_distance;
				result.max_dst = max_distance;
				result.demod = 0;
				result.lost = plr_i;
				result.tx_dr = api.lorawan.dr.get();
				write_sd_entry();
			}
			if (has_oled && !g_settings_ui)
			{
				oled_clear();
				oled_write_header((char *)"RAK FieldTest V2");

				sprintf(line_str, "DL RX SNR: %d RSSI: %d", last_snr, last_rssi);
				oled_write_line(0, 0, line_str);
				sprintf(line_str, "UL TX SNR: %d RSSI: %d", max_snr, max_rssi);
				oled_write_line(1, 0, line_str);
				sprintf(line_str, "GW(s): %d\n", num_gateways);
				oled_write_line(2, 0, line_str);
				oled_write_line(2, 50, "Min");
				oled_write_line(2, 80, "Max");
				oled_write_line(3, 0, "Distance");

				if (g_custom_parameters.location_on)
				{
					sprintf(line_str, "%d", min_distance);
					oled_write_line(3, 50, line_str);
					sprintf(line_str, "%d", max_distance);
					oled_write_line(3, 80, line_str);
					sprintf(line_str, "PLR: %.1f   Sent: %d", plr, packet_num);
					// sprintf(line_str, "L %.6f:%.6f", g_last_lat, g_last_long);
					oled_write_line(4, 0, line_str);
				}
				else
				{
					sprintf(line_str, "NA");
					oled_write_line(3, 50, line_str);
					oled_write_line(3, 80, line_str);
					sprintf(line_str, "PLR: %.1f   Sent: %d", plr, packet_num);
					// sprintf(line_str, "Location NA");
					oled_write_line(4, 0, line_str);
				}
				oled_display();
			}
		}
		else
		{
			int16_t min_rssi = field_tester_pckg[1] - 200;
			int16_t max_rssi = field_tester_pckg[2] - 200;
			int16_t min_distance = field_tester_pckg[3] * 250;
			int16_t max_distance = field_tester_pckg[4] * 250;
			int8_t num_gateways = field_tester_pckg[5];
			MYLOG("APP", "+EVT:FieldTester %d gateways", num_gateways);
			MYLOG("APP", "+EVT:RSSI min %d max %d", min_rssi, max_rssi);
			MYLOG("APP", "+EVT:Distance min %d max %d", min_distance, max_distance);

			if (has_sd)
			{
				if (has_rtc)
				{
					read_rak12002();
				}
				else
				{
					get_mcu_time();
				}
				result.year = g_date_time.year;
				result.month = g_date_time.month;
				result.day = g_date_time.date;
				result.hour = g_date_time.hour;
				result.min = g_date_time.minute;
				result.sec = g_date_time.second;
				result.mode = MODE_FIELDTESTER;
				result.gw = num_gateways;
				result.lat = g_last_lat;
				result.lng = g_last_long;
				result.min_rssi = min_rssi;
				result.max_rssi = max_rssi;
				result.rx_rssi = last_rssi;
				result.rx_snr = last_snr;
				result.min_dst = min_distance;
				result.max_dst = max_distance;
				result.demod = 0;
				result.lost = packet_lost;
				result.tx_dr = api.lorawan.dr.get();
				write_sd_entry();
			}
			if (has_oled && !g_settings_ui)
			{
				oled_clear();
				oled_write_header((char *)"RAK FieldTester");

				sprintf(line_str, "DL RX SNR: %d RSSI: %d", last_snr, last_rssi);
				oled_write_line(0, 0, line_str);
				sprintf(line_str, "GW(s): %d\n", num_gateways);
				oled_write_line(1, 0, line_str);
				oled_write_line(1, 50, "RSSI");
				oled_write_line(1, 80, "Distance");
				oled_write_line(2, 0, "Min");
				oled_write_line(3, 0, "Max");

				sprintf(line_str, "%d", min_rssi);
				oled_write_line(2, 50, line_str);
				sprintf(line_str, "%d", max_rssi);
				oled_write_line(3, 50, line_str);

				if (g_custom_parameters.location_on)
				{
					sprintf(line_str, "%d", min_distance);
					oled_write_line(2, 80, line_str);
					sprintf(line_str, "%d", max_distance);
					oled_write_line(3, 80, line_str);
					sprintf(line_str, "Lost: %d   Sent: %d", packet_lost, packet_num);
					// sprintf(line_str, "L %.6f:%.6f", g_last_lat, g_last_long);
					oled_write_line(4, 0, line_str);
				}
				else
				{
					sprintf(line_str, "NA");
					oled_write_line(2, 80, line_str);
					oled_write_line(3, 80, line_str);
					sprintf(line_str, "Lost: %d   Sent: %d", packet_lost, packet_num);
					// sprintf(line_str, "Location NA");
					oled_write_line(4, 0, line_str);
				}
				oled_display();
			}
		}
	}
	else if (display_reason == 7) // FieldTester no downlink packet (only FieldTester mode )
	{
		MYLOG("APP", "+EVT:FieldTester no downlink");

		if (has_sd)
		{
			if (has_rtc)
			{
				read_rak12002();
			}
			else
			{
				get_mcu_time();
			}
			result.year = g_date_time.year;
			result.month = g_date_time.month;
			result.day = g_date_time.date;
			result.hour = g_date_time.hour;
			result.min = g_date_time.minute;
			result.sec = g_date_time.second;
			result.mode = MODE_FIELDTESTER;
			result.gw = 0;
			result.lat = g_last_lat;
			result.lng = g_last_long;
			result.min_rssi = 0;
			result.max_rssi = 0;
			result.rx_rssi = 0;
			result.rx_snr = 0;
			result.min_dst = 0;
			result.max_dst = 0;
			result.demod = 0;
			result.lost = packet_lost;
			result.tx_dr = api.lorawan.dr.get();
			write_sd_entry();
		}
		if (has_oled && !g_settings_ui)
		{
			oled_clear();
			if (g_custom_parameters.test_mode == MODE_FIELDTESTER_V2)
			{
				oled_write_header((char *)"RAK FieldTester");
			}
			else
			{
				oled_write_header((char *)"RAK FieldTest V2");
			}
			sprintf(line_str, "No Downlink received");
			oled_write_line(0, 0, line_str);
			if (g_custom_parameters.test_mode == MODE_FIELDTESTER_V2)
			{
				sprintf(line_str, "PLR: %.1f   Sent: %d", plr, packet_num);
			}
			else
			{
				sprintf(line_str, "Lost: %d   Sent: %d", packet_lost, packet_num);
			}
			// sprintf(line_str, "L %.6f:%.6f", g_last_lat, g_last_long);
			oled_write_line(4, 0, line_str);
			oled_display();
		}
	}
	else if (display_reason == 8) // P2P TX finished (only P2P mode)
	{
		switch (g_custom_parameters.test_mode)
		{
		case MODE_LINKCHECK:
			oled_clear();
			oled_write_header((char *)"RAK Signal Meter");
			oled_write_line(0, 0, (char *)"LinkCheck mode");
			sprintf(line_str, "TX finished");
			oled_write_line(3, 0, line_str);
			oled_display();
			break;
		case MODE_P2P:
			if (has_oled && !g_settings_ui)
			{
				oled_clear();
				oled_write_header((char *)"RAK Signal Meter");
				oled_write_line(0, 0, (char *)"LoRa P2P mode");
				sprintf(line_str, "TX finished");
				oled_write_line(3, 0, line_str);
				oled_display();
			}
			break;
		}
		tx_active = false;
	}

	// digitalWrite(LED_GREEN, LOW);
}

/**
 * @brief Join network callback
 *
 * @param status status of join request
 */
void join_cb_lpw(int32_t status)
{
	if (status != 0)
	{
		display_reason = 3;
		api.system.timer.start(RAK_TIMER_1, 250, &display_reason);
	}
	else
	{
		display_reason = 5;
		api.system.timer.start(RAK_TIMER_1, 250, &display_reason);
	}
	tx_active = false;
}

/**
 * @brief Send callback for LoRa P2P mode
 *
 * @param data structure with RX packet information
 */
void send_cb_p2p(void)
{
	tx_active = false;

	display_reason = 8;
	api.system.timer.start(RAK_TIMER_1, 250, &display_reason);
}

/**
 * @brief Receive callback for LoRa P2P mode
 *
 * @param data structure with RX packet information
 */
void recv_cb_p2p(rui_lora_p2p_recv_t data)
{
	last_rssi = data.Rssi;
	last_snr = data.Snr;
	// packet_num++;
	tx_active = false;

	display_reason = 1;
	api.system.timer.start(RAK_TIMER_1, 250, &display_reason);
}

/**
 * @brief Receive callback for LoRaWAN mode
 *
 * @param data structure with RX packet information
 */
void recv_cb_lpw(SERVICE_LORA_RECEIVE_T *data)
{
	last_rssi = data->Rssi;
	last_snr = data->Snr;
	last_dr = data->RxDatarate;

	// packet_num++;
	tx_active = false;

	if (data->Port == 0)
	{
		MYLOG("RX-CB", "fPort 0");
		return;
	}
	if ((g_custom_parameters.test_mode == MODE_FIELDTESTER) || (g_custom_parameters.test_mode == MODE_FIELDTESTER_V2))
	{
		if (data->Port == 2)
		{
			display_reason = 6;
			memcpy(field_tester_pckg, data->Buffer, data->BufferSize);
			api.system.timer.start(RAK_TIMER_1, 250, &display_reason);
		}
		else
		{
			MYLOG("RX-CB", "Wrong fPort %d", data->Port);
		}
	}
}

/**
 * @brief Send finished callback for LoRaWAN mode
 *
 * @param status
 */
void send_cb_lpw(int32_t status)
{
	if (status != RAK_LORAMAC_STATUS_OK)
	{
		tx_active = false;
		MYLOG("APP", "LMC status %d\n", status);
		tx_fail_status = status;

		if ((g_custom_parameters.test_mode == MODE_FIELDTESTER) || (g_custom_parameters.test_mode == MODE_FIELDTESTER_V2))
		{
			packet_lost++;
			display_reason = 7;
			api.system.timer.start(RAK_TIMER_1, 250, &display_reason);
		}
		else
		{
			packet_lost++;
			display_reason = 2;
			api.system.timer.start(RAK_TIMER_1, 250, &display_reason);
		}
	}
	else if (tx_active)
	{
		display_reason = 8;
		api.system.timer.start(RAK_TIMER_1, 250, &display_reason);
	}
}

/**
 * @brief Linkcheck callback
 *
 * @param data structure with the result of the Linkcheck
 */
void linkcheck_cb_lpw(SERVICE_LORA_LINKCHECK_T *data)
{
	tx_active = false;
	if ((g_custom_parameters.test_mode == MODE_FIELDTESTER) || (g_custom_parameters.test_mode == MODE_FIELDTESTER_V2))
	{
		return;
	}
	// MYLOG("APP", "linkcheck_cb_lpw\n");
	last_snr = data->Snr;
	last_rssi = data->Rssi;
	link_check_state = data->State;
	link_check_demod_margin = data->DemodMargin;
	link_check_gateways = data->NbGateways;
	if (data->State != 0)
	{
		packet_lost++;
	}
	display_reason = 4;
	api.system.timer.start(RAK_TIMER_1, 250, &display_reason);
}

/**
 * @brief Request network time
 *
 * @param status 0 = got time, otherwise failed
 */
void timereq_cb_lpw(int32_t status)
{
	MYLOG("TREQ", "Time request status %d", status);
	if (sync_time_status == 0)
	{
		sync_time_status = 1;
		char local_time[30] = {0};
		struct tm localtime;
		SysTime_t UnixEpoch = SysTimeGet();
		UnixEpoch.Seconds -= 18;		  /*removing leap seconds*/
		UnixEpoch.Seconds += 8 * 60 * 60; // Make it GMT+8
		SysTimeLocalTime(UnixEpoch.Seconds, &localtime);
		sprintf(local_time, "%02dh%02dm%02ds on %02d/%02d/%04d", localtime.tm_hour, localtime.tm_min, localtime.tm_sec,
				localtime.tm_mon + 1, localtime.tm_mday, localtime.tm_year + 1900);
		MYLOG("TREQ", "%s", local_time);

		g_date_time.year = localtime.tm_year + 1900;
		g_date_time.month = localtime.tm_mon + 1;
		g_date_time.date = localtime.tm_mday;
		g_date_time.hour = localtime.tm_hour;
		g_date_time.minute = localtime.tm_min;
		g_date_time.second = localtime.tm_sec;
		if (has_rtc)
		{
			MYLOG("TREQ", "Sync RTC with LoRaWAN");
			sync_time_status = 2;
			set_rak12002(g_date_time.year, g_date_time.month, g_date_time.date, g_date_time.hour, g_date_time.minute, g_date_time.second);
			read_rak12002();
		}
	}
}

/**
 * @brief Callback for LoRaMAC stack to get battery level
 *   Requires changes in the RUI3 files
 *   service_lora.h add `uint8_t UserBattLevel(void) __attribute__((weak));`
 *   service_lora.c change `LoRaMacCallbacks.GetBatteryLevel = NULL;` to `LoRaMacCallbacks.GetBatteryLevel = UserBattLevel;`
 */
uint8_t UserBattLevel(void)
{
	// on USB return 0
	if (NRF_POWER->USBREGSTATUS == 3)
	{
		MYLOG("BAT", "On USB");
		return 0;
	}

	// else calculate the battery status
	float batt_voltage = api.system.bat.get();
	for (int idx = 0; idx < 10; idx++)
	{
		batt_voltage += api.system.bat.get();
	}
	batt_voltage = batt_voltage / 11;
	batt_voltage *= 1000;

	uint8_t lora_batt = batt_voltage * 255 / 4200;

	MYLOG("BAT", "Calculated %d from %.2fmV", lora_batt, batt_voltage);

	return lora_batt;
}

/**
 * @brief Setup routine
 *
 */
void setup(void)
{
	pinMode(WB_IO2, OUTPUT);
	pinMode(LED_GREEN, OUTPUT);
	pinMode(LED_BLUE, OUTPUT);

	// Check if OLED is available
	Wire.begin();
	has_oled = init_oled();
	if (!has_oled)
	{
		MYLOG("APP", "No OLED found");
	}

	if (has_oled)
	{
		sprintf(line_str, "RAK Signal Meter");
		oled_write_header(line_str);
		oled_add_line((char *)"Initializing");
		oled_add_line((char *)"Please wait");
	}

	// Shutdown modules power
	digitalWrite(WB_IO2, LOW);

	Serial.begin(115200);
#ifdef _VARIANT_RAK4630_
	if (NRF_POWER->USBREGSTATUS == 3)
	{
		// Give a chance to enter AT+BOOT
		delay(2000);
	}
	else
	{
		time_t serial_timeout = millis();
		// On nRF52840 the USB serial is not available immediately
		while (!Serial.available())
		{
			if ((millis() - serial_timeout) < 5000)
			{
				delay(100);
				digitalWrite(LED_GREEN, !digitalRead(LED_GREEN));
			}
			else
			{
				break;
			}
		}
	}
#else
	digitalWrite(LED_GREEN, HIGH);
	delay(5000);
#endif

	sprintf(line_str, "RUI3_Tester_V%d.%d.%d", SW_VERSION_0, SW_VERSION_1, SW_VERSION_2);
	api.system.firmwareVersion.set(line_str);

	digitalWrite(LED_GREEN, HIGH);

	digitalWrite(LED_GREEN, LOW);
	digitalWrite(LED_BLUE, LOW);

	// Initialize custom AT commands
	if (!init_app_ver_at())
	{
		MYLOG("APP", "Failed to initialize App Version AT command");
	}
	if (!init_status_at())
	{
		MYLOG("APP", "Failed to initialize Status AT command");
	}
	if (!init_interval_at())
	{
		MYLOG("APP", "Failed to initialize Send Interval AT command");
	}
	if (!init_test_mode_at())
	{
		MYLOG("APP", "Failed to initialize Test Mode AT command");
	}
	if (!init_custom_pckg_at())
	{
		MYLOG("APP", "Failed to initialize Custom Packet AT command");
	}
	if (!init_product_info_at())
	{
		MYLOG("APP", "Failed to initialize Product Info AT command");
	}

	// Get saved custom settings
	if (!get_at_setting())
	{
		MYLOG("APP", "Failed to read saved custom settings");
	}

	// Initialize Button
	if (!buttonInit())
	{
		MYLOG("APP", "Failed to initialize button");
	}

	// Initialize ACC (set to sleep as default)
	init_acc(false);

	// Initialize GNSS (set to sleep as default)
	if ((g_custom_parameters.test_mode == MODE_FIELDTESTER) || (g_custom_parameters.test_mode == MODE_FIELDTESTER_V2))
	{
		MYLOG("APP", "Init GNSS as active");
		has_gnss = init_gnss(true);
	}
	else
	{
		if (!g_custom_parameters.location_on)
		{
			MYLOG("APP", "Init GNSS as inactive");
			has_gnss = init_gnss(false);
		}
		else
		{
			MYLOG("APP", "Init GNSS as active");
			has_gnss = init_gnss(true);
		}
	}

	if (has_gnss)
	{
		oled_add_line((char *)"GNSS OK");
	}

	// Initialize RTC
	has_rtc = init_rak12002();
	if (has_rtc)
	{
		MYLOG("APP", "RTC found");
		SysTime_t UnixEpoch;
		read_rak12002();
		SysTimeSet(UnixEpoch);

		init_rtc_at();
	}
	else
	{
		MYLOG("APP", "No RTC found");
	}

	if (has_rtc)
	{
		oled_add_line((char *)"RTC OK");
	}

	// Initialize SD card
	has_sd = init_sd();
	if (!has_sd)
	{
		MYLOG("APP", "No SD card found");
	}
	else
	{
		// New file creation
		has_sd = create_sd_file();
		if (!has_sd)
		{
			MYLOG("APP", "Failed to create file");
		}
		MYLOG("APP", "New file created has_sd = %s", has_sd ? "true" : "false");
		init_dump_logs_at();
	}

	if (has_sd)
	{
		oled_add_line((char *)"SD Card OK");
	}

	// Setup callbacks and timers depending on test mode
	switch (g_custom_parameters.test_mode)
	{
	default:
		MYLOG("APP", "Invalid test mode, use LinkCheck");
		if (has_oled)
		{
			sprintf(line_str, "Invalid test mode");
			oled_write_line(0, 0, line_str);
			sprintf(line_str, "Using LinkCheck");
			oled_write_line(1, 0, line_str);
			oled_display();
		}
	case MODE_LINKCHECK:
		oled_add_line((char *)"LinkCheck mode");
		if (!api.lorawan.njs.get())
		{
			oled_add_line((char *)"Wait for join");
		}
		set_linkcheck();
		break;
	case MODE_P2P:
		set_p2p();
		oled_add_line((char *)"P2P mode");
		oled_add_line((char *)"Start testing");
		break;
	case MODE_FIELDTESTER:
		oled_add_line((char *)"FieldTester mode");
		if (!api.lorawan.njs.get())
		{
			oled_add_line((char *)"Wait for join");
		}
		set_field_tester();
		break;
	case MODE_FIELDTESTER_V2:
		oled_add_line((char *)"FieldTester V2 mode");
		if (!api.lorawan.njs.get())
		{
			oled_add_line((char *)"Wait for join");
		}
		set_field_tester();
		break;
	}

	// Keep GNSS active after reboot to enhance chances to get a valid location!
	// // Keep GNSS active if forced in setup ==> Leads to faster battery drainage!
	// if ((!g_custom_parameters.location_on) || (g_custom_parameters.test_mode != MODE_FIELDTESTER))
	// {
	// 	// Power down the module
	// 	digitalWrite(WB_IO2, LOW);
	// }

	sprintf(line_str, "Test interval  %lds", g_custom_parameters.send_interval / 1000);
	oled_add_line(line_str);

	// Create timer for periodic sending
	api.system.timer.create(RAK_TIMER_0, send_packet, RAK_TIMER_PERIODIC);
	// if (lorawan_mode)
	{
		if (g_custom_parameters.send_interval != 0)
		{
			api.system.timer.start(RAK_TIMER_0, g_custom_parameters.send_interval, NULL);
		}
	}

	//  Create timer for display handler
	api.system.timer.create(RAK_TIMER_1, handle_display, RAK_TIMER_ONESHOT);

	// Create timer for display saver
	api.system.timer.create(RAK_TIMER_2, oled_saver, RAK_TIMER_ONESHOT);
	if (g_custom_parameters.display_saver)
	{
		api.system.timer.start(RAK_TIMER_2, 60000, NULL);
	}

	// Create timer for GNSS location acquisition
	api.system.timer.create(RAK_TIMER_3, gnss_handler, RAK_TIMER_PERIODIC);

	// If LoRaWAN, start join if required
	if (lorawan_mode)
	{
		if (api.lorawan.njs.get() == 0)
		{
			api.lorawan.join(1, 1, 10, 50);
		}
	}
	MYLOG("APP", "Start testing");
	// Enable low power mode
	api.system.lpm.set(1);
}

/**
 * @brief Loop (unused)
 *
 */
void loop(void)
{
	// api.system.sleep.all(100);
	if (pressCount != 0)
	{
		mtmMain.Running(millis());
	}
}

/**
 * @brief Set the module for LoRaWAN LinkCheck testing
 *
 */
void set_linkcheck(void)
{
	MYLOG("APP", "Found LinkCheck Mode");
	lorawan_mode = true;
	if (api.lora.nwm.get())
	{
		// If in LoRa P2P mode, switch of RX
		api.lora.precv(0);
	}
	// Force LoRaWAN mode (might cause restart)
	api.lorawan.nwm.set();
	// Register callbacks
	api.lorawan.registerRecvCallback(recv_cb_lpw);
	api.lorawan.registerSendCallback(send_cb_lpw);
	api.lorawan.registerJoinCallback(join_cb_lpw);
	api.lorawan.registerLinkCheckCallback(linkcheck_cb_lpw);
	api.lorawan.registerTimereqCallback(timereq_cb_lpw);
	// Set unconfirmed packet mode
	api.lorawan.cfm.set(false);
	// Enable LinkCheck
	api.lorawan.linkcheck.set(2);
	api.lorawan.join(1, 1, 10, 50);

	if (g_custom_parameters.location_on)
	{
		// Enable GNSS module
		digitalWrite(WB_IO2, HIGH);
	}
	else
	{
		// Disable GNSS module
		digitalWrite(WB_IO2, LOW);
	}
}

/**
 * @brief Set the module for LoRa P2P testing
 *
 */
void set_p2p(void)
{
	MYLOG("APP", "Found P2P Mode");
	lorawan_mode = false;

	api.lora.precv(0);

	// Force LoRa P2P mode (might cause restart)
	if (!api.lora.nwm.set())
	{
		MYLOG("APP", "Failed to set P2P Mode");
	}
	// Register callbacks
	api.lora.registerPRecvCallback(recv_cb_p2p);
	api.lora.registerPSendCallback(send_cb_p2p);
	// Enable RX mode
	api.lora.precv(65533);
	if (g_custom_parameters.location_on)
	{
		// Enable GNSS module
		digitalWrite(WB_IO2, HIGH);
	}
	else
	{
		// Disable GNSS module
		digitalWrite(WB_IO2, LOW);
	}
}

/**
 * @brief Set the module into FieldTester Mode
 *
 */
void set_field_tester(void)
{
	lorawan_mode = true;
	if (api.lora.nwm.get())
	{
		// If in LoRa P2P mode, switch of RX
		api.lora.precv(0);
	}
	// Force LoRaWAN mode (might cause restart)
	api.lorawan.nwm.set();
	// Register callbacks
	api.lorawan.registerRecvCallback(recv_cb_lpw);
	api.lorawan.registerSendCallback(send_cb_lpw);
	api.lorawan.registerJoinCallback(join_cb_lpw);
	api.lorawan.registerLinkCheckCallback(linkcheck_cb_lpw);
	api.lorawan.registerTimereqCallback(timereq_cb_lpw);
	// Set unconfirmed packet mode
	api.lorawan.cfm.set(false);
	// Disable LinkCheck
	api.lorawan.linkcheck.set(0);
	api.lorawan.join(1, 1, 10, 50);
	if (g_custom_parameters.location_on)
	{
		// Enable GNSS module
		digitalWrite(WB_IO2, HIGH);
	}
}
