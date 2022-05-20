//------------------------------------------------------------------
// Copyright(c) 2022 Anwar Minarso (anwar.minarso@gmail.com)
// https://github.com/anwarminarso/
// This file is part of the ESP32+ PLC.
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the GNU
// Lesser General Public License for more details
//------------------------------------------------------------------

#include "Arduino.h"
#ifndef _TYPEDEFSTRUCT_H_
#define _TYPEDEFSTRUCT_H_

typedef struct {
    char WiFi_STA_HostName[32];
    char WiFi_STA_SSID[32];
    char WiFi_STA_Password[64];
    uint8_t WiFi_STA_Enabled;

    char WiFi_AP_HostName[32];
    char WiFi_AP_SSID[32];
    char WiFi_AP_Password[64];


    char Admin_UserName[32];
    char Admin_Password[32];

    char MQTT_Server_Address[100];
    uint16_t MQTT_Server_Port;
    char MQTT_Root_Topic[100];
    char MQTT_Server_UserName[32];
    char MQTT_Server_Password[32];
    uint16_t MQTT_Tele_Interval;
    uint8_t MQTT_Enabled;


    uint16_t ModBus_TCP_Port;
    uint8_t ModBus_TCP_Enabled;

    uint8_t ModBus_RTU_SlaveID;
    uint8_t ModBus_RTU_Interface;
    uint16_t ModBus_RTU_Baud;
    uint8_t ModBus_RTU_Enabled;
} memConfig_t;



typedef struct {
    uint8_t ResetCount;
    uint8_t ReconnectCount;
    uint8_t IsPLCStarted;
} deviceState_t;
#endif