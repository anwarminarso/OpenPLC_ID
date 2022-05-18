#include <stdlib.h>
extern "C" {
#include "openplc.h"
}
#include "Arduino.h"
extern "C" {
#include "GlobalVariables.h"
}

/******************PINOUT CONFIGURATION**************************
Discrete Inputs
Digital In:  18, 19, 21, 22, 23, 27, 32 (%IX0.0 - %IX0.6)
Coils
Digital Out: 02, 04, 05, 12, 13, 14, 15  (%QX0.0 - %QX0.6)


Input Registers/Holding Registers - Read
Analog In:   36, 39, 34, 35                 (%IW0 - %IW2)

Holding Registers - Write
Analog Out:  25, 26                         (%QW0 - %QW1)
*****************************************************************/

uint8_t NUM_DISCRETE_INPUT = 7;
uint8_t NUM_DISCRETE_OUTPUT = 7;
uint8_t NUM_ANALOG_INPUT = 4;
uint8_t NUM_ANALOG_OUTPUT = 2;

uint8_t pinMask_DIN[] = { 18, 19, 21, 22, 23, 27, 32 };
uint8_t pinMask_DOUT[] = { 02, 04, 05, 12, 13, 14, 15 };
uint8_t pinMask_AIN[] = { 36, 39, 34, 35 };
uint8_t pinMask_AOUT[] = { 25, 26 };


uint8_t DIN_Values[7];
uint8_t DOUT_Values[7];
uint16_t AIN_Values[4];
uint16_t AOUT_Values[2];

void hardwareInit()
{
    for (int i = 0; i < NUM_DISCRETE_INPUT; i++)
    {
        DIN_Values[i] = 0;
        pinMode(pinMask_DIN[i], INPUT);
    }
    
    for (int i = 0; i < NUM_ANALOG_INPUT; i++)
    {
        AIN_Values[i] = 0;
        pinMode(pinMask_AIN[i], INPUT);
    }
    
    for (int i = 0; i < NUM_DISCRETE_OUTPUT; i++)
    {
        DOUT_Values[i] = 0;
        pinMode(pinMask_DOUT[i], OUTPUT);
    }

    for (int i = 0; i < NUM_ANALOG_OUTPUT; i++)
    {
        AOUT_Values[i] = 0;
        pinMode(pinMask_AOUT[i], OUTPUT);
    }
}

void updateInputBuffers()
{
    for (int i = 0; i < NUM_DISCRETE_INPUT; i++)
    {
        if (bool_input[i / 8][i % 8] != NULL) {
            DIN_Values[i] = digitalRead(pinMask_DIN[i]);
            *bool_input[i / 8][i % 8] = DIN_Values[i];
        }
        else {
            DIN_Values[i] = digitalRead(pinMask_DIN[i]);
        }
    }
    
    for (int i = 0; i < NUM_ANALOG_INPUT; i++)
    {
        if (int_input[i] != NULL) {
            AIN_Values[i] = (analogRead(pinMask_AIN[i]) * 64); 
            *int_input[i] = AIN_Values[i];
        }
        else {
            AIN_Values[i] = (analogRead(pinMask_AIN[i]) * 64);
        }
    }
}

void updateOutputBuffers()
{
    for (int i = 0; i < NUM_DISCRETE_OUTPUT; i++)
    {
        if (bool_output[i / 8][i % 8] != NULL)
            DOUT_Values[i] = *bool_output[i / 8][i % 8];
        digitalWrite(pinMask_DOUT[i], DOUT_Values[i]);
    }
    for (int i = 0; i < NUM_ANALOG_OUTPUT; i++)
    {
        if (int_output[i] != NULL)
            AOUT_Values[i] = *int_output[i];
        dacWrite(pinMask_AOUT[i], (AOUT_Values[i] / 256));
    }
}
