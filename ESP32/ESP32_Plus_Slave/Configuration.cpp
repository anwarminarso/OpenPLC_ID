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



#include "TypeDefStruct.h"
#include "GlobalVariables.h"

#include "Configuration.h"
#include <Preferences.h>
#include <nvs_flash.h>
#include "Utils.h"

// set global vars
memConfig_t config;
deviceState_t deviceState;
Preferences prefs;

#define prefNamespace "ESP32"

void loadConfig() {
	bool isValid = true;
	size_t schLen;

	prefs.begin(prefNamespace, true);
	schLen = prefs.getBytesLength("config");
	if (schLen != sizeof(config))
		isValid = false;
	schLen = prefs.getBytesLength("state");
	if (schLen != sizeof(deviceState))
		isValid = false;

	if (isValid) {
		prefs.getBytes("config", (uint8_t*)&config, sizeof(config));
		prefs.getBytes("state", (uint8_t*)&deviceState, sizeof(deviceState));
	}
	prefs.end();
	if (!isValid)
		resetConfig();
	else
		Serial.println("Config Loaded");
}

void resetConfig() {

	String chipId = getChipID();

	String staHostName = String("ESP32-STA-");
	String apHostName = String("ESP32-AP-");
	String rootTopic = String("/esp32/plc/");
	staHostName += chipId;
	apHostName += chipId;
	rootTopic += chipId;

	strncpy((char*)&config.WiFi_STA_HostName, staHostName.c_str(), 32);
	strncpy((char*)&config.WiFi_STA_SSID, "yourSSID", 32);
	strncpy((char*)&config.WiFi_STA_Password, "yourPassword", 64);
	config.WiFi_STA_Enabled = 0;

	strncpy((char*)&config.WiFi_AP_HostName, apHostName.c_str(), 32);
	strncpy((char*)&config.WiFi_AP_SSID, apHostName.c_str(), 32);
	strncpy((char*)&config.WiFi_AP_Password, "", 64);

	strncpy((char*)&config.Admin_UserName, "admin", 32);
	strncpy((char*)&config.Admin_Password, "admin", 32);

	strncpy((char*)&config.MQTT_Server_Address, "broker.hivemq.com", 100);
	config.MQTT_Server_Port = 1883;
	strncpy((char*)&config.MQTT_Server_UserName, "", 32);
	strncpy((char*)&config.MQTT_Server_Password, "", 32);
	strncpy((char*)&config.MQTT_Root_Topic, rootTopic.c_str(), 100);
	config.MQTT_Tele_Interval = 10;
	config.MQTT_Enabled = 0;

	config.ModBus_TCP_Enabled = 0;
	config.ModBus_TCP_Port = 502;

	config.ModBus_RTU_Enabled = 0;
	config.ModBus_RTU_Interface = 0;
	config.ModBus_RTU_SlaveID = 1;
	config.ModBus_RTU_Baud = 9600;

	deviceState.ReconnectCount = 0;
	deviceState.ResetCount = 0;
	deviceState.IsPLCStarted = 0;

	prefs.begin(prefNamespace, false);
	prefs.clear();
	prefs.putBytes("config", (uint8_t*)&config, sizeof(config));
	prefs.putBytes("state", (uint8_t*)&deviceState, sizeof(deviceState));
	prefs.end();
	Serial.println("Reset Config");
}

void saveConfig() {
	prefs.begin(prefNamespace, false);
	prefs.putBytes("config", (uint8_t*)&config, sizeof(config));
	prefs.putBytes("state", (uint8_t*)&deviceState, sizeof(deviceState));
	prefs.end();
	Serial.println("Config Saved");
}
void updateDeviceState() {
	prefs.begin(prefNamespace, false);
	prefs.putBytes("state", (uint8_t*)&deviceState, sizeof(deviceState));
	prefs.end();
	Serial.println("State Saved");
}

void eraseConfig() {
	nvs_flash_erase(); // erase the NVS partition and...
	nvs_flash_init(); // initialize the NVS partition.
}