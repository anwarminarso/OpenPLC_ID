//------------------------------------------------------------------
// Copyright(c) 2022 Anwar Minarso(https://github.com/anwarminarso/)
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
// Lesser General Public License for more details.
//------------------------------------------------------------------

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
