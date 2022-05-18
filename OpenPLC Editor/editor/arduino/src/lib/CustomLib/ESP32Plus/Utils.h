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

#ifndef _Utils_H
#define _Utils_H

String getChipID();
String getWifiMacAddress();
String trimChar(char* value);

#endif