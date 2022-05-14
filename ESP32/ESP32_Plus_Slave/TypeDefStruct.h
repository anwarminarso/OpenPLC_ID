//-----------------------------------------------------------------------------
// MIT License
//
// Copyright(c) 2022 Anwar Minarso(https://github.com/anwarminarso/)
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files(the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and /or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions :
//
// The above copyright noticeand this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//
// Anwar Minarso, May 2022
//------------------------------------------------------------------------------

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