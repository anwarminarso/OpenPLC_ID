#include <stdlib.h>
extern "C" {
#include "openplc.h"
}
#include "Arduino.h"
#include <Adafruit_ADS1X15.h>

//OpenPLC HAL for ESP32 IndieOprek boards

/******************PINOUT CONFIGURATION**************************
Digital In:  36, 39, 34, 35, 32, 33, 27 (%IX0.0 - %IX0.6)
Digital Out: 14, 12, 13, 04, 00, 02, 15 (%QX0.0 - %QX0.6)
Analog In:   0, 1, 2, 3                 (%IW0 - %IW3)
Analog Out:  25, 26                     (%QW0 - %QW1)
*****************************************************************/

//Define the number of inputs and outputs for this board (mapping for the NodeMCU 1.0)
#define NUM_DISCRETE_INPUT          7
#define NUM_ANALOG_INPUT            4
#define NUM_DISCRETE_OUTPUT         7
#define NUM_ANALOG_OUTPUT           2

uint8_t pinMask_DIN[] = { 36, 39, 34, 35, 32, 33, 27 };
uint8_t pinMask_DOUT[] = { 14, 12, 13, 04, 00, 02, 15 };
uint8_t pinMask_AIN[] = { 0, 1, 2, 3 };
uint8_t pinMask_AOUT[] = { 25, 26 };

Adafruit_ADS1115 ads;  /* Use this for the 16-bit version */

bool isADSError = false;

void hardwareInit()
{
    for (int i = 0; i < NUM_DISCRETE_INPUT; i++)
    {
        pinMode(pinMask_DIN[i], INPUT);
    }

    // ads.setGain(GAIN_TWOTHIRDS);  // 2/3x gain   , ADC Range +/- 6.144V 0.1875mV (default)
    // ads.setGain(GAIN_ONE);        // 1x gain     , ADC Range +/- 4.096V 0.125mV
    // ads.setGain(GAIN_TWO);        // 2x gain     , ADC Range +/- 2.048V 0.0625mV
    // ads.setGain(GAIN_FOUR);       // 4x gain     , ADC Range +/- 1.024V 0.03125mV
    // ads.setGain(GAIN_EIGHT);      // 8x gain     , ADC Range +/- 0.512V 0.015625mV
    // ads.setGain(GAIN_SIXTEEN);    // 16x gain    , ADC Range +/- 0.256V 0.0078125mV
    

    // set default GAIN_TWOTHIRDS
    // adcValue => int16_t (16 bit)
    // voltage = adcValue * 0.1875mV
    isADSError = !ads.begin();
    
    //for (int i = 0; i < NUM_ANALOG_INPUT; i++)
    //{
    //    pinMode(pinMask_AIN[i], INPUT);
    //}
    
    for (int i = 0; i < NUM_DISCRETE_OUTPUT; i++)
    {
        pinMode(pinMask_DOUT[i], OUTPUT);
    }

    for (int i = 0; i < NUM_ANALOG_OUTPUT; i++)
    {
        pinMode(pinMask_AOUT[i], OUTPUT);
    }
}

void updateInputBuffers()
{
    for (int i = 0; i < NUM_DISCRETE_INPUT; i++)
    {
        if (bool_input[i/8][i%8] != NULL) 
            *bool_input[i/8][i%8] = digitalRead(pinMask_DIN[i]);
    }
    
    //for (int i = 0; i < NUM_ANALOG_INPUT; i++)
    //{
    //    if (int_input[i] != NULL)
    //        *int_input[i] = (analogRead(pinMask_AIN[i]) * 64);
    //}
    for (int i = 0; i < NUM_ANALOG_INPUT; i++)
    {
        if (int_input[i] != NULL)
            *int_input[i] = ads.readADC_SingleEnded(pinMask_AIN[i]); // value range -32768 to +32768
    }
}

void updateOutputBuffers()
{
    for (int i = 0; i < NUM_DISCRETE_OUTPUT; i++)
    {
        if (bool_output[i/8][i%8] != NULL) 
            digitalWrite(pinMask_DOUT[i], *bool_output[i/8][i%8]);
    }
    for (int i = 0; i < NUM_ANALOG_OUTPUT; i++)
    {
        if (int_output[i] != NULL) 
            dacWrite(pinMask_AOUT[i], (*int_output[i] / 256));
    }
}
