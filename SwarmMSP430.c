/*
 * SatModem.c
 *
 *  Created on: Jul 26, 2022
 *      Author: AverySimon
 */

// STD C
#include <stdbool.h>
#include "stdlib.h"
#include "string.h"

// TI Libraries
#include "gpio.h"
#include "eusci_a_uart.h"

//
//#include "Pins.h"
#include <Message.h>
#include <SwarmMSP430.h>
#include "misc.h"
#include "UART.h"
#include "xbee_hal.h"
#include "xmesh.h"

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~GLOBALS~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Message * satRxMsg = 0;  //pointer to message struct for message we are actively getting

/* These comands will need to be responded to before satellite module is considered "initialized".
 * You'll want GPS too, but it can't be ascertained until modem is fully ready, figure that out yourself
 * If the order of these is changed, the end of the respective Parse functions need to be changed also */
char cmdArrayIndex = 0;  // this index describes which init command is next to be sent
char* initCommandArray[3] = {"$CS", "$DT @", SAT_GPIO_WAKE_LOW_HIGH};


// initialize the global struct, if you add more things to the sctruct, add another 0
SatInfo satInfo = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~LOCAL FUNCTIONS PROTOTYPES~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~FUNCTIONS~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void swarm_startup(void)
{
    uart_setup(BAUD115200);                        // Swarm only uses 115200

    uart_configureRxInterrupt(UART_ENABLE_INTERRUPT);

    swarm_sendInitCommand();
}

void swarm_shutdown(void)
{
    uart_configureRxInterrupt(UART_DISABLE_INTERRUPT);
    uart_configureTxInterrupt(UART_DISABLE_INTERRUPT);
    swarm_sleep();
}

void swarm_restart(void){

    swarm_sendCommand(SAT_CMD_RESTART_DEVICE, SAT_CMD_PARAM_DELETE_DB);

}

bool swarm_isConnected(void)
{
	return satInfo.satConIsEstablished;
}

bool swarm_handleMsg(void)
{
    char* msgPtr;

    satRxMsg = message_getMsg(UART);
    if(!satRxMsg)
        return false;

    msgPtr = (char*)satRxMsg->msgPtr;  // this shortens and simplifies things

    if(SAT_TESTING)
        xmesh_sendAPITransparentTX(BROADCAST_ADDR, satRxMsg->msgPtr, satRxMsg->dataLength);

    if(swarm_isErrorMessage(satRxMsg)){
        swarm_handleError(msgPtr);
        message_freeMsg(satRxMsg);
        return true;  // it was a satellite message, it was just an error
    }
    switch(msgPtr[1] << 8 | msgPtr[2]){
    case SAT_HEADER_DEVICE_INFO:
        swarm_parseDeviceIdMessage(msgPtr);
        break;
    case SAT_HEADER_GPS_INFO:
        swarm_parseGpsMessage(msgPtr);
        break;
    case SAT_HEADER_SLEEP:
        swarm_parseSleepMessage(msgPtr);
        break;
    case SAT_HEADER_DATE_TIME:
        swarm_parseDateTimeMessage(msgPtr);
        break;
    case SAT_HEADER_RECEIVE_TEST:
        swarm_parseRssiMessage(msgPtr);
        break;
//    case SAT_HEADER_INTERNAL_MESSAGE:
//        swarm_parseInternalMessage(msgPtr);
//        break;
    case SAT_HEADER_TRANSMIT_DATA:
        swarm_parseTransmitDataMessage(msgPtr);
        break;
    case SAT_HEADER_GPIO_MSG:
        swarm_parseGpioMessage(msgPtr);
        break;
    case SAT_HEADER_MESSAGE_MANAGEMENT_TX:
    case SAT_HEADER_MESSAGE_MANAGEMENT_RX:
        swarm_parseMessageManagementMessage(msgPtr);
        break;
    case SAT_HEADER_MODEM_MSG:
    default: swarm_parseModemMessage(msgPtr);
        break;
    }
    message_freeMsg(satRxMsg);

    return true;
}

void swarm_parseSleepMessage(char* msgPtr){

    if(msgPtr[4] == 'O' && msgPtr[5] == 'K')    // update
        satInfo.isSleeping = true;              // we'll need to us this to help communication when we're ready to start sleeping this modem
    else if(msgPtr[4] == 'W' && msgPtr[5] == 'A' && msgPtr[6] == 'K' && msgPtr[7] == 'E') // can I just do *(msgPtr+4) == "WAKE" ?
        satInfo.isSleeping = false;
}

void swarm_parseModemMessage(char* msgPtr){

    __no_operation();

}

void swarm_parseRssiMessage(char* msgPtr){
    char* indexPtr;
    unsigned char i; // messageToDesk[4] = {},

    if (strlen(msgPtr) > 20) {                         // if message is long, its a satellite RSSI message
        satInfo.rssi.satellite = 0xFF;
        satInfo.rssi.snr = 0xFF;

        // Parse satellite rssi
        indexPtr = strchr(msgPtr, '=') + 1; // returns string after '=' ( our number )
        // Iterate through characters after = until ',' is hit
        for(i=0;i<5;i++){
            if(indexPtr[i] == ',')          // find how long the input string will be
                break;
        }
        satInfo.rssi.satellite = ascii_to_char(indexPtr, i);

        // Parse SNR
        indexPtr = strchr(indexPtr, '=') + 1; // returns string after '=' ( our number )
        // Iterate through characters after = until ',' is hit
        for(i=0;i<5;i++){
            if(indexPtr[i] == ',')            // find how long the input string will be
                break;
        }
        satInfo.rssi.snr = ascii_to_char(indexPtr, i);

        // if we want, we can also get Frequency Deviation, Time Received, and Satellite ID

    } else {
        // Parse background RSSI
        indexPtr = strchr(indexPtr, '=') + 1; // returns string after '=' ( our number )
        // Iterate through characters after = until ',' is hit
        for(i=0;i<5;i++){
            if(indexPtr[i] == ',')            // find how long the input string will be
                break;
        }
        satInfo.rssi.background = ascii_to_char(indexPtr, i);

        if (satInfo.rssi.background > 104)
            satInfo.signalRating = SignalStrengthExcellent;
        else if (satInfo.rssi.background > 99)
            satInfo.signalRating = SignalStrengthGood; // if message is very short, its a background RSSI message
        else if (satInfo.rssi.background > 96)
            satInfo.signalRating = SignalStrengthOK;
        else if (satInfo.rssi.background > 92)
            satInfo.signalRating = SignalStrengthMarginal;
        else if (satInfo.rssi.background > 85)
            satInfo.signalRating = SignalStrengthBad;
        else
            satInfo.signalRating = SignalStrengthUndetermined;
    }
}

void swarm_parseDateTimeMessage(char* msgPtr){

    if(msgPtr[19] == 'V'){   // V means Valid Date/Time, I means Invalid
        satInfo.dateTime.Year = ascii_to_int(msgPtr+4, 4);
        satInfo.dateTime.Month = ascii_to_int(msgPtr+8, 2)-1;    // RTC Month is 0-starting (January = 0)
        satInfo.dateTime.DayOfMonth = ascii_to_int(msgPtr+10, 2);
        //satInfo.dateTime.DayOfWeek = ascii_to_int(msgPtr[12], 2);
        satInfo.dateTime.Hours = ascii_to_int(msgPtr+12, 2);
        satInfo.dateTime.Minutes = ascii_to_int(msgPtr+14, 2);
        satInfo.dateTime.Seconds = ascii_to_int(msgPtr+16, 2);
        initRTC(satInfo.dateTime);

        cmdArrayIndex++;  // since we got the first init command response, send next command
        swarm_sendInitCommand();

    }
}

void swarm_parseTransmitDataMessage(char* msgPtr){
//    If you want to do something on Transmit OK response, do it here
//    if(msgPtr[4] == 'O' && msgPtr[5] == 'K'){
//        your code here
//    }

}

void swarm_parseDeviceIdMessage(char* msgPtr){
    char* idToken;
    char* idPtr;
    const char delimiter[1] = {'x'};

    idToken = strtok(msgPtr+4, delimiter);  // break msgPtr into strings delimited by 0x
    idToken = strtok(NULL, delimiter);  // this token will be the device ID

    satInfo.deviceID.asLong = strtol(idToken, &idPtr, 16);  // this converts string to hex (third argument base 16)

    cmdArrayIndex++;  // since we got the first init command response, send next command
    swarm_sendInitCommand();
}

void swarm_parseGpsMessage(char* msgPtr){
    char* gpsToken;
    char* gpsPtr;
    const char delimiter[1] = {','};

    // Parse latitude
    gpsToken = strtok(msgPtr+4, delimiter);  // break msgPtr into strings delimited by comma, ignore first 4 bytes of msgPtr
    satInfo.gps.latitude.asFloat = strtof(gpsToken, &gpsPtr);

    // Parse longitude
    gpsToken = strtok(NULL, delimiter);
    satInfo.gps.longitude.asFloat = strtof(gpsToken, &gpsPtr);

    // If we want to parse Altitude, Course, and Speed, continue the pattern
}

void swarm_parseGpioMessage(char* msgPtr){

    if(msgPtr[4] == 'O' && msgPtr[5] == 'K'){
        cmdArrayIndex++;
        swarm_sendInitCommand();

        satInfo.satFullyInitialized = true;
        satInfo.satConIsEstablished = true;
        // This satellite module is officially initialized
    }
}

void swarm_parseMessageManagementMessage(char* msgPtr){
    unsigned int unsentMessages = 0;
    char* tdPtr;
    if(msgPtr[2] == 'T'){ // "MT"
        unsentMessages |= strtol(msgPtr+4, &tdPtr, 10);  // base 10 for decimal, returns long
        if(unsentMessages > 20)
            swarm_sendCommand(SAT_CMD_MSG_TX_MANAGEMENT, SAT_CMD_PARAM_DELETE_UNSENT_MESSAGES);
    }
    else if(msgPtr[2] == 'M'){ // "MM"

    }

}

void swarm_sendInitCommand(void){
    swarm_sendCommand(initCommandArray[cmdArrayIndex], SAT_CMD_PARAM_NO_PARAMS);
}

void swarm_sleep(void){

    swarm_sendCommand(SAT_CMD_SLEEP_MODE, SAT_DEFAULT_SLEEP_TIME);
}

unsigned char swarm_checksum(const char *sz, size_t len){
    size_t i = 0;
    unsigned char cs;
    if (sz [0] == '$')
        i++;
    for (cs = 0; (i < len) && sz [i]; i++)
    cs ^= ((unsigned char) sz [i]);
    return cs;
}

void swarm_sendCommand(char* cmd_define, char* params){
    // pass in entire command string without *checksum i.e.: "DT @"
    Message * txCmd = 0;
    unsigned char checksum;

    // it's much easier to create a new string each time than to handle static memory when passing string literal
    if((strlen(cmd_define)+strlen(params)+5)<=MESSAGE_BUFF_SIZE_SMALL)
        txCmd = message_requestMsgBuff(small);
    else
        txCmd = message_requestMsgBuff(large);
    message_append(txCmd, (unsigned char*)cmd_define, strlen(cmd_define));

    if(*params != 0){
        message_append(txCmd, " ", 1);
        message_append(txCmd, (unsigned char*)params, strlen(params));
    }

    checksum = swarm_checksum((char*)txCmd->msgPtr, txCmd->dataLength);
    message_append(txCmd, "*", 1);
    hexByte_to_ascii(checksum, (unsigned char*)txCmd->msgPtr+txCmd->dataLength);
    txCmd->dataLength += 2;                // increment by 2 bytes because the conversion function doesn't do this for us
    message_append(txCmd, NEWLINE, 1);

    swarm_sendData(txCmd->msgPtr, txCmd->dataLength);
    cycleDelay_ms(10);

    message_killMsg(&txCmd);
}

void swarm_sendData(unsigned char* data, int datalen)
{
    // This sends data to Swarm Modem, not satellite, This is used when Sending Commands
    while(uart_isTransmitting()); // Wait until UART is free
    cycleDelay_ms(30);

    uart_send(data, datalen);
}

void swarm_transmitData(char* applicationID, char* holdTime, char* payload, unsigned int numberOfBytes){
//     This transmits data to a satellite
    Message * dataBuff = 0;
    unsigned int i;
    unsigned char checksum;

    dataBuff = message_requestMsgBuff(large);
    if(!dataBuff)
        return;

    message_append(dataBuff, "$TD ", 4);

    if(applicationID != 0){
        for(i=0;i<strlen(applicationID);i++)
            message_append(dataBuff, (unsigned char* )applicationID[i], 1);
    }

    if(holdTime != 0){
        for(i=0;i<strlen(holdTime);i++)
            message_append(dataBuff, (unsigned char* )holdTime[i], 1);
    }

    hex_to_ascii((unsigned char*)payload, dataBuff->msgPtr+dataBuff->dataLength, numberOfBytes);
    dataBuff->dataLength += (numberOfBytes*2);  // increment by 2*bytes because the conversion function doesn't do this for us
    checksum = swarm_checksum((char*)dataBuff->msgPtr, dataBuff->dataLength);
    message_append(dataBuff, "*", 1);
    hexByte_to_ascii(checksum, dataBuff->msgPtr+dataBuff->dataLength);
    dataBuff->dataLength += 2;                  // increment by 2 bytes because the conversion function doesn't do this for us
    message_append(dataBuff, NEWLINE, 1);

    swarm_sendData(dataBuff->msgPtr, dataBuff->dataLength);
    cycleDelay_ms(10);

    satInfo.secondsSinceTransmit = 0; // initially put this inside "$TD OK" handling, but message gets deleted either way
    message_freeMsg(dataBuff);
}

void swarm_setConnectionStatus(bool trueMeansConnected){
    // could do this like satInfo.satConIsEstablished = argument, but arg might be > 1
    if(trueMeansConnected)
        satInfo.satConIsEstablished = true;
    else
        satInfo.satConIsEstablished = false;
}

void swarm_handleError(char* msgPtr){

    // Handle Errors however you see fit

    // Options are to ignore it, parse it and handle, or to just simply notify that an error occurred
    if(msgPtr[0] == '.'){  // module rebooted meaning we need to restart the init process on MCU side
        satInfo.satConIsEstablished = false;
        satInfo.satFullyInitialized = false;
        cmdArrayIndex = 0;
        swarm_sendInitCommand();
    }

}

bool swarm_isErrorMessage(Message* message) {
    // If the String "ERR" is inside a response, return true
    unsigned char ch;
    for (ch = 0; ch < message->dataLength; ch++) {
        if (message->msgPtr[ch] == 'R') {
            if (message->msgPtr[ch - 2] == 'E' && message->msgPtr[ch - 1] == 'R')
                return true;
        }
    }
    if(message->msgPtr[0] == '.') // When swarm boots, it sends a bunch of data thats hard to catch, but the .'s are easy to catch
        return true;
    return false;
}




