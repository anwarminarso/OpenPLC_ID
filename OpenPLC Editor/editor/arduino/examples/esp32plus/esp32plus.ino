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
#include <Arduino.h>
#include <WiFi.h>


#include "Arduino_OpenPLC.h"
#include "defines.h"

#include "Modbus.h"
#include "ModbusESP.h"
#include <BluetoothSerial.h>

#include "GlobalVariables.h"
#include "WebSvr.h"
#include "Configuration.h"
#include "WiFiCom.h"
#include "ClientAPI.h"

/**************************************************************/
// Unused OpenPLC Implementation (openplc.h)
void cycleDelay() {}
/**************************************************************/

ModbusESP modbus;
BluetoothSerial SerialBT;

unsigned long __tick = 0;

unsigned long scan_cycle;
unsigned long timer_ms = 0;
bool enableModbus[3] = { false, false, false };

void syncModbusBuffers()
{
    //Sync OpenPLC Buffers with Modbus Buffers	
    for (int i = 0; i < MAX_DIGITAL_OUTPUT; i++)
    {
        if (i < NUM_DISCRETE_OUTPUT) {
            modbus.Coil(i, (bool)DOUT_Values[i]);
        }
        else if (bool_output[i / 8][i % 8] != NULL)
            modbus.Coil(i, (bool)*bool_output[i / 8][i % 8]);
    }
    for (int i = 0; i < MAX_ANALOG_OUTPUT; i++)
    {
        if (i < NUM_ANALOG_OUTPUT) {
            modbus.Hreg(i, AOUT_Values[i]);
        }
        else if (int_output[i] != NULL)
            modbus.Hreg(i, *int_output[i]);
    }
    for (int i = 0; i < MAX_DIGITAL_INPUT; i++)
    {
        if (i < NUM_DISCRETE_INPUT) {
            modbus.Ists(i, (bool)DIN_Values[i]);
        }
        else if (bool_input[i / 8][i % 8] != NULL)
            modbus.Ists(i, (bool)*bool_input[i / 8][i % 8]);
    }
    for (int i = 0; i < MAX_ANALOG_INPUT; i++)
    {
        if (i < NUM_ANALOG_INPUT) {
            modbus.Ireg(i, AIN_Values[i]);
        }
        else if (int_input[i] != NULL)
            modbus.Ireg(i, *int_input[i]);
    }

    //Read changes from clients

    if (enableModbus[1])
        modbus.taskTCP();
    if (enableModbus[2])
        modbus.taskRTU();
    //Write changes back to OpenPLC Buffers
    for (int i = 0; i < MAX_DIGITAL_OUTPUT; i++)
    {
        if (i < NUM_DISCRETE_OUTPUT) {
            DOUT_Values[i] = (uint8_t)modbus.Coil(i);
            if (bool_output[i / 8][i % 8] != NULL)
                *bool_output[i / 8][i % 8] = DOUT_Values[i];
        }
        else if (bool_output[i / 8][i % 8] != NULL)
            *bool_output[i / 8][i % 8] = (uint8_t)modbus.Coil(i);
    }
    for (int i = 0; i < MAX_ANALOG_OUTPUT; i++)
    {
        if (i < NUM_ANALOG_OUTPUT) {
            AOUT_Values[i] = (uint16_t)modbus.Hreg(i);;
            if (int_output[i] != NULL)
                *int_output[i] = AOUT_Values[i];
        }
        else if (int_output[i] != NULL)
            *int_output[i] = (uint16_t)modbus.Hreg(i);
    }
}

void setupCycleDelay(unsigned long long cycle_time)
{
    scan_cycle = (uint32_t)(cycle_time / 1000000);
    timer_ms = millis() + scan_cycle;
}


void setup()
{
    Serial.begin(115200);
    loadConfig();
    hardwareInit();
    config_init__();
    glueVars();
    delay(10);
    initWiFi();
    if (config.ModBus_TCP_Enabled) {
        modbus.config(config.ModBus_TCP_Port);
        enableModbus[0] = true;
        enableModbus[1] = true;
    }
    if (config.ModBus_RTU_Enabled) {
        switch (config.ModBus_RTU_Interface)
        {
        case 0:
            modbus.setSlaveId(config.ModBus_RTU_SlaveID);
            modbus.config(&Serial2, config.ModBus_RTU_Baud, -1);
            Serial2.flush();
            break;
        case 1:
            modbus.setSlaveId(config.ModBus_RTU_SlaveID);
            modbus.config(&SerialBT, "ESP32Plus");
            SerialBT.flush();
            break;
        default:
            modbus.setSlaveId(config.ModBus_RTU_SlaveID);
            modbus.config(&Serial2, config.ModBus_RTU_Baud, -1);
            Serial2.flush();
            break;
        }
        enableModbus[0] = true;
        enableModbus[2] = true;
    }
    //Add all modbus registers
    for (int i = 0; i < MAX_DIGITAL_INPUT; ++i)
    {
        modbus.addIsts(i);
    }
    for (int i = 0; i < MAX_ANALOG_INPUT; ++i)
    {
        modbus.addIreg(i);
    }
    for (int i = 0; i < MAX_DIGITAL_OUTPUT; ++i)
    {
        modbus.addCoil(i);
    }
    for (int i = 0; i < MAX_ANALOG_OUTPUT; ++i)
    {
        modbus.addHreg(i);
    }

    initWebServer();
    initClientAPI();

    setupCycleDelay(common_ticktime__);
    Serial.println("ESP32 Plus PLC Initialized");
}

void loop()
{
    if (!deviceState.IsPLCStarted)
        return;
    clientAPILoop();

    if (timer_ms > millis()) {
        if (enableModbus[0] && (timer_ms - millis() >= 1)) 
        {
            syncModbusBuffers();
        }
    }
    else {
        timer_ms += scan_cycle;
        updateInputBuffers();
        if (enableModbus[0]) {
            syncModbusBuffers();
        }
        config_run__(__tick++);
        updateOutputBuffers();
        updateTime();
    }
}
