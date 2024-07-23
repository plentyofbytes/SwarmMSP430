/*
 * SatModem.h
 *
 *  Created on: Jul 26, 2022
 *      Author: MicroTechEE
 */

#ifndef SATMODEM_H_
#define TOPLEVEL_INTERNET_SATMODEM_H_

#include <stdbool.h>
#include "stdlib.h"
#include "misc.h"

#ifndef PINS_H
#define SAT_GPIO_Port                                   GPIO_PORT_P8
#define SAT_GPIO_Pin                                    BIT1
#endif

/* Configure these for your situation */
#define SAT_UART                        EUSCI_A0_BASE
#define SAT_RX_TIMEOUT_ms               RX_BUFF_SIZE_LARGE / 10
#define SAT_STARTBYTE                   '$'

// How to Wake Swarm Modem -- SEE SWARM PRODUCT MANUAL -- Link inside INFO comments
#define SAT_GPIO_WAKE_LOW_HIGH          "$GP 3"                         // Low->High Transition WAKES Modem
#define SAT_GPIO_WAKE_HIGH_LOW          "$GP 4"                         // High->Low Transition WAKES Modem
#define SAT_GPIO_CONFIGURATION          SAT_GPIO_WAKE_LOW_HIGH          // Change this as desired
#define SAT_GPIO_HIGH                   1
#define SAT_GPIO_LOW                    0

/* INFO
 *
 *      Product Manual:
 *      https://swarm.space/wp-content/uploads/2022/09/Swarm-M138-Modem-Product-Manual.pdf
 *
 *      NMEA Checksum Calculator
 *      https://nmeachecksum.eqth.net/
 *
 *      Transmission Troubleshooting Guide
 *      https://github.com/Swarm-Technologies/Getting-Started/wiki/2.-Modem-Transmission
 *
 * BOOT:
 *
 * Depending on several factors, boot can take around 5 minutes or more if signal strength is weak!
 * Swarm responds with $M138 DATETIME*56 when it gets a read and then will read out $M138 POSITION*4E when it has a GPS fix
 * We cannot give it Transmit commands until it gets Date/Time
 *
 *
 * Commands Brief Description: CHECK THE PRODUCT MANUAL LINKED ABOVE FOR MORE DETAIL
 *
 *
 * ---> ALL COMMANDS CAN RETURN ERRORS. ERRORS AREN'T INCLUDED IN THIS INFO <---
 *      Input will always be $<command-symbol><optional-input>*xx         where xx = NMEA Checksum
 *      NEVER ACTUALLY SEND "<" OR ">".
 *
 * CS - Configuration Settings - pg39:
 *      INPUT = Only ever use $CS*10
 *      Returns: $CS DI=<dev_ID>,DN=<dev_name>*xx  ->  example $CS DI=0x00e57,DN=M138*43
 *
 * DT - Date and Time - pg40:
 *      INPUTS = $DT<"@" to get recent DT, "?" to get DT rate,  or "rate" to set or disable rate of DT messages>
 *      Returns: $DT <YYYY><MM><DD><hh><mm><ss>*xx, $DT <rate>*xx, or $DT OK*xx respectively
 *
 * FV - Firmware Version - pg43:
 *      INPUT - Only ever use $FV*10
 *      Returns: $FV <version_string>*xx  ->  example $FV 2021-07-16T00:10:21,v1.1.0*74
 *
 * GJ - GPS Jamming / Spoofing Indication - pg44:
 *      INPUTS = $GJ<"@" to get recent GJ, "?" to get GJ rate,  or "rate" to set or disable rate of GJ messages>
 *      Returns: $GJ <spoof-state>,<jamming-level>*xx  , $GJ <rate>*xx, or $GJ OK*xx respectively
 *
 * GN - Geospacial Information (GPS Coordinates, etc) - pg46:
 *      INPUTS = $GN<"@" to get recent GN, "?" to get GN rate,  or "rate" to set or disable rate of GN messages>
 *      Returns: $GN <latitude>,<longitutde>,<altitude>,<course>,<speed>*xx  , $GN <rate>*xx, or $GN OK*xx respectively
 *
 * GP - GPIO1 Control / Status - pg50:
 *      INPUTS = $GP<"@" to read pin state, "?" to display current GPIO1 mode,  or "mode" to set GPIO1 pin mode>
 *      Returns: $GN <latitude>,<longitutde>,<altitude>,<course>,<speed>*xx  , $GN <rate>*xx, or $GN OK*xx respectively
 *
 * GS - GPS Fix Quality - pg53:
 *      INPUTS = $GS<("@" to get recent GS, "?" to get GS rate,  or "rate" to set or disable rate of GS messages)>
 *      Returns: $GS <hdop>,<vdop>,<gnss_sats>,<unused>,<fix>*xx  , $GS <rate>*xx, or $GS OK*xx respectively
 *
 * MM - Messages Received Management - pg56:
 *      SEE MANUAL
 *
 * MT - Messages to Transmit Management - pg61:
 *      SEE MANUAL
 *
 * PO - Power Off - pg64:
 *      INPUT = Only ever use $PO*1F
 *      Returns: $PO OK*xx
 *      NOTES: Swarm will continue to draw 3mA until power is completely removed! USE SLEEP MODE INSTEAD.
 *             If power is going to be removed, use this before doing so!
 *
 * PW - Power Status - pg65:
 *      INPUTS = $PW<"@" to get recent PW, "?" to get PW rate,  or "rate" to set or disable rate of PW messages>
 *      Returns: $PW <cpu_volts><unused><unused><unused><temp>*xx, $PW <rate>*xx, or $PW OK*xx respectively
 *
 * RD - Received Data ( Unsolicited Message ) - pg67:
 *      Returns: $RD <appID>,<rssi>,<snr>,<fdev>,<data>*xx
 *      appID -> Application ID tag of message
 *      rssi  -> Received signal strength in dBm for packet (integer)
 *      snr   -> Signal to noise ratio in dB for packet (integer)
 *      fdev  -> Frequency deviation in Hz for packet (integer)
 *      data  -> ASCII encoded data packet
 *
 * RS - Restart Device - pg68:
 *      INPUTS = Either $RS*xx or $RS[deletedb]*xx   -> use latter to delete all message data, don't send "[" or "]"
 *      Returns: $RS OK*xx or error
 *
 * RT - Receive Test - pg 69:
 *      INPUTS = $RT<"@" to get recent RT, "?" to get RT rate,  or "rate" to set or disable rate of RT messages>
 *      Returns: $RT RSSI=<rssi_sat>,SNR=<snr>,FDEV=<fdev>,TS=<time>,DI=<sat_id>*xx  ,  $RT RSSI=<rssi_bkgnd>*xx,  $RT <rate>*xx, or $RT OK*xx respectively
 *      SEE MANUAL
 *
 * SL - Sleep Mode - pg72:
 *      INPUTS = $SL<S=<seconds-to-sleep>> or $SL<U=YYYY-MM-DD>T<HH:MM:SS>>  no spaces or "<"/">"
 *      Returns: $SL OK*xx on set or $SL WAKE,<cause>*xx when woken
 *      SEE MANUAL ->  Cause can be GPIO, SERIAL, or TIME
 *      NOTES: Current usage UNKNOWN ...
 *
 * M138 - Modem Status Unsolicited Message - pg75
 *      Returns: <msg>,[<data>]*xx
 *      msg:
 *          BOOT - Boot process progress with the following data reason:
 *          ABORT - A firmware crash occurred that caused a restart
 *          DEVICEID - Displays the device ID of the Modem
 *          POWERON - Power has been applied
 *          RUNNING - Boot has completed and ready to accept commands
 *          UPDATED - A firmware update was performed
 *          VERSION - Current firmware version information
 *          DATETIME - The first time GPS has acquired a valid date/time reference
 *          POSITION - The first time GPS has acquired a valid position 3D fix
 *          DEBUG - Debug message (data - debug text)
 *          ERROR - Error message (data - error text)
 *
 * TD - Transmit Data - pg76
 *      INPUT = $TD [AI=<appID>,HD=<hold_dur>,ET=<expire_time>]<[data]>*xx  Separate parameters with commas (,)
 *      Returns: $TD OK,<msg_id>*xx or $TD SENT RSSI=<rssi_sat>,SNR=<snr>,FDEV=<fdev>,<msg_id>*xx or error
 *      AI=<appID> Application ID tag for message (optional, default = 0, maximum is 64999)
 *      HD=<hold_dur> Hold duration of message in seconds (optional, default = 172800 seconds, minimum = 60 seconds)
 *      ET=<expire_time> Expiration time of message in epoch seconds (optional, if omitted, same as hold_dur)
 *      <string|data> 1 to 192 bytes of data (ASCII string) 2 to 384 bytes (hexadecimal written as ascii)
 *      SEE MANUAL -> There are a lot of things to consider here
 *
 */

/* * * * * * * * STRUCTS * * * * * * * * * */
typedef enum{
    SignalStrengthUndetermined = -2,
    SignalStrengthBad = -1,
    SignalStrengthMarginal = 0,
    SignalStrengthOK,
    SignalStrengthGood,
    SignalStrengthExcellent
}SignalRating;

typedef struct{
    char background;
    char satellite;
    char snr;
}RSSI;

typedef struct{
    EasyFloat longitude;
    EasyFloat latitude;
    // you can put FDEV and other stuff in here if desired
}GPS;

typedef struct{
    Calendar dateTime;
    EasyLong deviceID;
    bool isSleeping;
    bool satConIsEstablished;
    bool satFullyInitialized;
    RSSI rssi;
    GPS gps;
    SignalRating signalRating;
    unsigned int satCounter;
    unsigned int secondsSinceTransmit;
}SatInfo;


/* * * * * * * * COMMANDS * * * * * * * * * */
#define SAT_CMD_DEVICE_ID                              "$CS"
#define SAT_CMD_DATE_TIME                              "$DT"
#define SAT_CMD_FW_VERSION                             "$FV"
#define SAT_CMD_GPS_JAMMING                            "$GJ"
#define SAT_CMD_GPS_INFO                               "$GN"
#define SAT_CMD_GPIO_CONFIG                            "$GP"
#define SAT_CMD_GPS_FIX_QUALITY                        "$GS"
#define SAT_CMD_MSG_RX_MANAGEMENT                      "$MM"
#define SAT_CMD_MSG_TX_MANAGEMENT                      "$MT"
#define SAT_CMD_POWER_OFF                              "$PO"
#define SAT_CMD_POWER_STATUS                           "$PW"
#define SAT_CMD_RECEIVE_DATA                           "$RD"
#define SAT_CMD_RESTART_DEVICE                         "$RS"
#define SAT_CMD_RECEIVE_TEST                           "$RT"
#define SAT_CMD_SLEEP_MODE                             "$SL"
#define SAT_CMD_TRANSMIT_DATA                          "$TD"

/* Command Params */
#define SAT_CMD_PARAM_NO_PARAMS                        ""
#define SAT_CMD_PARAM_QUERY_LAST_MESSAGE               "@"
#define SAT_CMD_PARAM_QUERY_CURRENT_RATE               "?"
#define SAT_CMD_PARAM_BACKGROUND_RSSI_RATE             "180"              // in seconds, 3m
#define SAT_CMD_PARAM_QUERY_UNSENT_MESSAGES            "C=U"
#define SAT_CMD_PARAM_DELETE_UNSENT_MESSAGES           "D=U"
#define SAT_CMD_PARAM_DELETE_DB                        "deletedb"

/* Command Headers for RX Handling */
#define SAT_HEADER_MODEM_MSG                           'M' <<8 | '1'
#define SAT_HEADER_SLEEP                               'S' <<8 | 'L'
#define SAT_HEADER_GPS_INFO                            'G' <<8 | 'N'
#define SAT_HEADER_GPIO_MSG                            'G' <<8 | 'P'
#define SAT_HEADER_DEVICE_INFO                         'C' <<8 | 'S'
#define SAT_HEADER_DATE_TIME                           'D' <<8 | 'T'
#define SAT_HEADER_TRANSMIT_DATA                       'T' <<8 | 'D'
#define SAT_HEADER_MESSAGE_MANAGEMENT_TX               'M' <<8 | 'T'
#define SAT_HEADER_MESSAGE_MANAGEMENT_RX               'M' <<8 | 'M'
#define SAT_HEADER_RECEIVE_TEST                        'R' <<8 | 'T'
#define SAT_HEADER_INTERNAL_MESSAGE                    'I' <<8 | 'M'
#define SAT_HEADER_INTERNAL_INIT_COMMAND               'I' <<8 | 'C'
#define SAT_HEADER_INTERNAL_GPS_QUERY                  'L' <<8 | 'L'
#define SAT_HEADER_INTERNAL_QUERY_UNSENT_MESSAGES      'U' <<8 | 'M'
#define SAT_HEADER_INTERNAL_RSSI                       'S' <<8 | 'S'
#define SAT_HEADER_INTERNAL_SEND_AGGREGATED_DATA       'S' <<8 | 'D'
#define SAT_HEADER_INTERNAL_SLEEP                      'S' <<8 | 'L'

#define NEWLINE                                         "\n"
#define SAT_MSG_START_BYTE                              '$'
#define SAT_MSG_HOLD_TIME_1DAY                          "HD=86400,"  // this needs a comma after
#define SAT_MSG_APPLICATION_ID                          "AI=7777,"   // this needs a comma after, must be integer

#define SAT_DEFAULT_SLEEP_TIME                          "S=86400"    // 1 day   // sleep time can range from 5s to 31,536,000 ( 8,760 hours, 365 days )

#define SAT_NUMBER_MAX_PACKET_BYTES_HEX                 192          // max payload is 192 hex bytes, if using ASCII, one byte will be used per NIBBLE
#define SAT_NUMBER_MAX_PACKET_BYTES_ASCII               SAT_NUMBER_MAX_PACKET_BYTES_HEX*2  // i.e. instead of 0x35 you will send 0x33 0x35 which is 3 and 5 in ASCII

/* * * * * * * * FUNCTIONS * * * * * * * * * */
/* Basic */
void swarm_startup(void);
void swarm_shutdown(void);
void swarm_gpio(char pinState);
void swarm_wake(void);
void swarm_sleep(void);

/* Sending Commands */
void swarm_sendCommand(char* cmd_define, char* params);
void swarm_sendInitCommand(void);
void swarm_sendData(unsigned char* data, int datalen);
unsigned char swarm_checksum(const char* sz, size_t len);

/* Message Handling */
void swarm_handleMsg(void);
bool swarm_isErrorMessage(Message* message);
void swarm_handleError(char* msgPtr);
void swarm_parseSleepMessage(char* msgPtr);
void swarm_parseModemMessage(char* msgPtr);
void swarm_parseRssiMessage(char* msgPtr);
void swarm_parseDateTimeMessage(char* msgPtr);
void swarm_parseTransmitDataMessage(char*msgPtr);
void swarm_parseGpioMessage(char* msgPtr);
void swarm_parseDeviceIdMessage(char* msgPtr);
void swarm_parseGpsMessage(char* msgPtr);
void swarm_parseMessageManagementMessage(char* msgPtr);

/* Outgoing Data */
void swarm_transmitData(char* applicationID, char* holdTime, char* payload, unsigned int numberOfBytes);



#endif /* SATMODEM_H_ */
