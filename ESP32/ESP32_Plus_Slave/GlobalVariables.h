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

#include "TypeDefStruct.h"

#ifndef _GlobalVariables_H
#define _GlobalVariables_H

//#define ESP32_PLC_MASTER

extern uint8_t NUM_DISCRETE_INPUT;
extern uint8_t NUM_DISCRETE_OUTPUT;
extern uint8_t NUM_ANALOG_INPUT;
extern uint8_t NUM_ANALOG_OUTPUT;

extern memConfig_t config;
extern deviceState_t deviceState;

extern uint8_t pinMask_DIN[];
extern uint8_t pinMask_DOUT[];
extern uint8_t pinMask_AIN[];
extern uint8_t pinMask_AOUT[];


extern uint8_t DIN_Values[];
extern uint8_t DOUT_Values[];
extern uint16_t AIN_Values[];
extern uint16_t AOUT_Values[];

#endif
