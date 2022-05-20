#include <Arduino.h>
#include <WiFi.h>

//#define ESP32_PLC_MASTER

/******************PINOUT CONFIGURATION**************************
Discrete Inputs
Digital In:  36, 39, 34, 35, 32, 33, 27 (%IX0.0 - %IX0.6)

Coils
Digital Out: 14, 12, 13, 04, 00, 02, 15 (%QX0.0 - %QX0.6)

Input Registers/Holding Registers - Read
Analog In:   0, 1, 2, 3                 (%IW0 - %IW3)

Holding Registers - Write
Analog Out:  25, 26                     (%QW0 - %QW1)
*****************************************************************/


/******************PINOUT CONFIGURATION**************************
Discrete Inputs
Digital In:  17, 18, 19, 21, 22, 23, 27, 32 (%IX0.0 - %IX0.7)

Coils
Digital Out: 02, 04, 05, 12, 13, 14, 15, 16  (%QX0.0 - %QX0.7)


Input Registers/Holding Registers - Read
Analog In:   34, 35, 36, 39                 (%IW0 - %IW2)

Holding Registers - Write
Analog Out:  25, 26                         (%QW0 - %QW1)
*****************************************************************/

uint8_t NUM_DISCRETE_INPUT = 7;
uint8_t NUM_DISCRETE_OUTPUT = 7;
uint8_t NUM_ANALOG_INPUT = 4;
uint8_t NUM_ANALOG_OUTPUT = 2;

uint8_t pinMask_DIN[] = { 18, 19, 21, 22, 23, 27, 32 };
uint8_t pinMask_DOUT[] = { 02, 04, 05, 12, 13, 14, 15 };
uint8_t pinMask_AIN[] = { 36, 39, 34, 35  };
uint8_t pinMask_AOUT[] = { 25, 26 };

uint8_t DIN_Values[7];
uint8_t DOUT_Values[7];
uint16_t AIN_Values[4];
uint16_t AOUT_Values[2];

#include "defines.h"
#include "openplc.h"

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
void config_run__(unsigned long tick) {}
void config_init__(void) {}
unsigned long long common_ticktime__;
void updateTime() {}
void glueVars() {}
void setupCycleDelay(unsigned long long cycle_time) {}
void cycleDelay() {}
/**************************************************************/

IEC_BOOL *bool_input[MAX_DIGITAL_INPUT / 8][8];
IEC_BOOL *bool_output[MAX_DIGITAL_OUTPUT / 8][8];
IEC_UINT *int_input[MAX_ANALOG_INPUT];
IEC_UINT *int_output[MAX_ANALOG_OUTPUT];

ModbusESP modbus;
BluetoothSerial SerialBT;

bool enableModbus[3] = { false, false, false };
void hardwareInit()
{
    Serial.begin(115200);
    delay(10);

    loadConfig();

    for (int i = 0; i < NUM_DISCRETE_INPUT; i++)
    {
        pinMode(pinMask_DIN[i], INPUT);
        DIN_Values[i] = 0;
        bool_input[i / 8][i % 8] = &DIN_Values[i];
    }

    for (int i = 0; i < NUM_ANALOG_INPUT; i++)
    {
        pinMode(pinMask_AIN[i], INPUT);
        AIN_Values[i] = 0;
        int_input[i] = &AIN_Values[i];
    }

    for (int i = 0; i < NUM_DISCRETE_OUTPUT; i++)
    {
        pinMode(pinMask_DOUT[i], OUTPUT);
        DOUT_Values[i] = 0;
        bool_output[i / 8][i % 8] = &DOUT_Values[i];
    }

    for (int i = 0; i < NUM_ANALOG_OUTPUT; i++)
    {
        pinMode(pinMask_AOUT[i], OUTPUT);
        AOUT_Values[i] = 0;
        int_output[i] = &AOUT_Values[i];
    }
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

    Serial.println("ESP32 Plus PLC Initialized");

}
void updateInputBuffers()
{
    for (int i = 0; i < NUM_DISCRETE_INPUT; i++)
    {
        if (bool_input[i / 8][i % 8] != NULL)
            *bool_input[i / 8][i % 8] = digitalRead(pinMask_DIN[i]);
    }

    for (int i = 0; i < NUM_ANALOG_INPUT; i++)
    {
        if (int_input[i] != NULL)
            *int_input[i] = (analogRead(pinMask_AIN[i]) * 64);
    }



}
void updateOutputBuffers() {
    for (int i = 0; i < NUM_DISCRETE_OUTPUT; i++)
    {
        if (bool_output[i / 8][i % 8] != NULL)
            digitalWrite(pinMask_DOUT[i], *bool_output[i / 8][i % 8]);
    }
    for (int i = 0; i < NUM_ANALOG_OUTPUT; i++)
    {
        if (int_output[i] != NULL)
            dacWrite(pinMask_AOUT[i], (*int_output[i] / 256));
    }


}

void syncModbusBuffers()
{
    //Sync OpenPLC Buffers with Modbus Buffers	
    for (int i = 0; i < MAX_DIGITAL_OUTPUT; i++)
    {
        if (bool_output[i / 8][i % 8] != NULL)
        {
            modbus.Coil(i, (bool)*bool_output[i / 8][i % 8]);
        }
    }
    for (int i = 0; i < MAX_ANALOG_OUTPUT; i++)
    {
        if (int_output[i] != NULL)
        {
            modbus.Hreg(i, *int_output[i]);
        }
    }
    for (int i = 0; i < MAX_DIGITAL_INPUT; i++)
    {
        if (bool_input[i / 8][i % 8] != NULL)
        {
            modbus.Ists(i, (bool)*bool_input[i / 8][i % 8]);
        }
    }
    for (int i = 0; i < MAX_ANALOG_INPUT; i++)
    {
        if (int_input[i] != NULL)
        {
            modbus.Ireg(i, *int_input[i]);
        }
    }

    //Read changes from clients
    if (enableModbus[1])
        modbus.taskTCP();
    if (enableModbus[2])
        modbus.taskRTU();

    for (int i = 0; i < MAX_DIGITAL_OUTPUT; i++)
    {
        if (bool_output[i / 8][i % 8] != NULL)
        {
            *bool_output[i / 8][i % 8] = (uint8_t)modbus.Coil(i);
        }
    }
    for (int i = 0; i < MAX_ANALOG_OUTPUT; i++)
    {
        if (int_output[i] != NULL)
        {
            *int_output[i] = (uint16_t)modbus.Hreg(i);
        }
    }
}


void setup()
{
    hardwareInit();
}

void loop()
{
    if (!deviceState.IsPLCStarted)
        return;
    clientAPILoop();
    updateInputBuffers();

    if (enableModbus[0]) {
        syncModbusBuffers();
    }
    updateOutputBuffers();

    //updateTime();

    //sleep until next scan cycle
    //cycleDelay();
}
