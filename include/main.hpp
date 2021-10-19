#ifndef _MAIN_HPP__
#define _MAIN_HPP__

#include <Arduino.h>
#include <SPIFFS.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <map>
#include <list>
#include <string>

/* Baudrate for serial-connection */
#define SERBAUD 115200

/* Prefix to find the networks
 * "Homie-" should be the default */
#define SSIDPREFIX "Homie-"

/* When CONFIG_WITHOUT_NAME is definied,
 * the Homie-device will be configured even
 * when no entry is found in "nameMapping.json" */
//#define CONFIG_WITHOUT_NAME

/* When CONFIG_WITH_SSID_MATCH is defined,
 * the Homie-device will only be configured,
 * when the Homie-networklist contains the
 * SSID from baseConfig.json */
//#define CONFIG_WITH_SSID_MATCH
#ifdef CONFIG_WITH_SSID_MATCH
    #warning CONFIG_WITH_SSID_MATCH is not fully implemented - disable this option
    #undef CONFIG_WITH_SSID_MATCH
#endif

/* Holdcounter for keep the network in
 * in the last proceeded Homie-Clients map.
 * Real holdcounter is calculated via
 * HOLDCTR * HOLD_TIMER_VALUE */
#define HOLDCTR 24
#if HOLDCTR < 1 || HOLDCTR > 255
    #error Invalid value for HOLDCTR in main.hpp (valid 1 - 255)
#endif

/* Defines for the filenames ot the config-file
 * BASECFG_FILENAME: all neccessary MQTT / WiFi settings
 * NAMECFG_FILENAME: contains the names for the devices */
#define BASECFG_FILENAME    "/baseConfig.json"
#define NAMECFG_FILENAME    "/nameMapping.json"

/* Homie API-Points */
#define HOMIE_CONFIG    "config"
#define HOMIE_NETWORK   "networks"

/* Applicationprotocol for Homie-connect
 * should be http (but maybe, somewhen https is supported by Homie) */
#define HOMIE_PROTO     "http"

/* This map stores the recently connected
 * Homie-Clients and the assigned holdtimer.
 * When the holdcounter is aged out, the entry
 * is removed.
 * If a client is in this map, it will be skipped
 * and not configured. This is only to prevent
 * a "configuration-loop". */
std::map<std::string, uint8_t> mapRecentlyClients;

/* Timervalue in seconds for the hold-timer-tick.
 * This timeout is used to call an ISR, which decrement
 * the "holdcounter". */
#define HOLD_TIMER_VALUE 5

/* Timervalue in seconds for the WiFi-Scan-Intervall.
 * This timeout is used to call an ISR, which starts a
 * WiFi-Scan. */
#define SCAN_TIMER_VALUE 60

/* struct for the MQTT / WiFi / OTA global settings */
struct struct_baseCfg {
    bool bvalidCfg;
    String astrWifiName;
    String astrWifiPass;
    String astrMQTTHost;
    int intMQTTPort;
    String astrMQTTbaseTopic;
    bool bMQTTAuth;
    String astrMQTTUsername;
    String astrMQTTPassword;
    bool bOTA;
} s_baseCfg;

/* This map contains the mapping MAC-Address to Devicenames */
std::map<std::string, std::string> mapMAC2FriendlyName;

/* Timer to use (0-3)
 * 0,1 : Timer 1 & 2 Group 1
 * 2,3 : Timer 3 & 4 Group 2 */
#define HOLD_TMR_NUMBER 0
#if HOLD_TMR_NUMBER < 0 || HOLD_TMR_NUMBER > 3
    #error Invalid ESP32 Timer choosen (valid values 0 - 3)
#endif

/* Timer to use (0-3)
 * 0,1 : Timer 1 & 2 Group 1
 * 2,3 : Timer 3 & 4 Group 2 */
#define SCAN_TMR_NUMBER 2
#if SCAN_TMR_NUMBER < 0 || SCAN_TMR_NUMBER > 3
    #error Invalid ESP32 Timer choosen (valid values 0 - 3)
#endif

/* Define the peripheral clockfrequency (in MHz) - it's used in the comparevalue calculation.
 * See also https://www.espressif.com/sites/default/files/documentation/esp32_technical_reference_manual_en.pdf
 * APB should be 80 MHz (Default) */
#define APB_FREQ 80
/* Define the clockdivider (Prescaler) - if set equal to APB_FREQ we get the ticks for one second */
#define TMR_DIVIDER 80

/* Flag for signaling that the timer (Holdtimer) has reached the compare-value (=if true, assume timeout) */
volatile bool HoldTimerEnd;

/* Flag for signaling that the timer (WiFi-Scan-Intervall) has reached the compare-value (=if true, assume timeout) */
volatile bool ScanIntervallTimer;

/* pointer to timerobject (Holdtimer) */
hw_timer_t *t_Hold_Timer = NULL;

/* pointer to timerobject (WiFi-Scan-Intervall) */
hw_timer_t *t_ScanInt_Timer = NULL;

/* WiFi object for the Homie-client communication */
WiFiClass ClientHomieWiFi;

/* forward-declaration */
void IRAM_ATTR HoldTMRHandler(void);
void IRAM_ATTR ScanIntTMRHandler(void);

#endif /* _MAIN_HPP__ */