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

#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <WebAuthentication.h>
#include <ArduinoJson.h>
#include <SPIFFS.h>

#include "defines.h"
#include "openplc.h"
#include "GlobalVariables.h"
#include "WebSvr.h"
#include "Utils.h"
#include "Configuration.h"

#ifdef  OTA_ENABLED
#include <Update.h>
#endif

//#define DEBUG_WEB

AsyncWebServer webServer(80);
AsyncWebSocket ws("/ws");

#define MSG_BUFFER_SIZE 255
static uint8_t msgCode;
static uint8_t msgBuff[MSG_BUFFER_SIZE];
static uint8_t msgBuffIndex;
static bool IsUploading = false;

String uname;
String pwd;


TimerHandle_t tmrWSHealth;

static void tmrWSHealthFun(xTimerHandle pxTimer) {
	ws.cleanupClients();
}

void setWebSocketHeader(uint8_t cmd) {
	msgBuffIndex = 0;
	msgBuff[msgBuffIndex++] = cmd;
}
void setWebSocketContent(uint8_t* data, size_t len) {
	int i = len;
	while (i--)
		msgBuff[msgBuffIndex++] = *data++;
}
void sendWebSocketMessage(AsyncWebSocketClient* client) {
	client->binary(msgBuff, msgBuffIndex);
}
void sendAllWebSocketMessage() {
	ws.binaryAll(msgBuff, msgBuffIndex);
}
void executeWebSocketMessage(AsyncWebSocketClient* client, uint8_t* data, size_t len) {
	msgCode = data[0];
	switch (msgCode)
	{
		case 1:
			setWebSocketHeader(msgCode);
			for (int i = 0; i < NUM_DISCRETE_INPUT; i++)
			{
				uint8_t val = (uint8_t)*bool_input[i / 8][i % 8];
				setWebSocketContent((uint8_t*)&val, 1);
			}
			for (int i = 0; i < NUM_DISCRETE_OUTPUT; i++)
			{
				uint8_t val = (uint8_t)*bool_output[i / 8][i % 8];
				setWebSocketContent((uint8_t*)&val, 1);
			}
			for (int i = 0; i < NUM_ANALOG_INPUT; i++)
			{
				uint16_t val = (uint16_t)*int_input[i];
				setWebSocketContent((uint8_t*)&val, 2);
			}
			for (int i = 0; i < NUM_ANALOG_OUTPUT; i++)
			{
				uint16_t val = (uint16_t)*int_output[i];
				setWebSocketContent((uint8_t*)&val, 2);
			}
			sendWebSocketMessage(client);
			break;
		case 255:
#ifdef ESP32_PLC_MASTER
			uint8_t buff[3] = { msgCode, deviceState.IsPLCStarted, 0 };
#else
			uint8_t buff[3] = { msgCode, deviceState.IsPLCStarted, 1 };
#endif
			client->binary(buff, 3);
			break;
	}
}
void onWsEvent(AsyncWebSocket* server, AsyncWebSocketClient* client, AwsEventType type, void* arg, uint8_t* data, size_t len)
{
	if (type == WS_EVT_CONNECT) {
		Serial.printf("ws[%s][%u] connect\n", server->url(), client->id());
		client->ping();
	}
	else if (type == WS_EVT_DISCONNECT) {
		Serial.printf("ws[%s][%u] disconnect: %u\n", server->url(), client->id());
	}
	else if (type == WS_EVT_ERROR) {
		Serial.printf("ws[%s][%u] error(%u): %s\n", server->url(), client->id(), *((uint16_t*)arg), (char*)data);
	}
	else if (type == WS_EVT_PONG) {
		Serial.printf("ws[%s][%u] pong[%u]: %s\n", server->url(), client->id(), len, (len) ? (char*)data : "");
	}
	else if (type == WS_EVT_DATA) {
		AwsFrameInfo* info = (AwsFrameInfo*)arg;
		String msg = "";
		if (info->final && info->index == 0 && info->len == len) {
			if (info->opcode != WS_TEXT) {
				if (info->len > 0) {
					if (IsUploading)
						return;
					executeWebSocketMessage(client, data, len);
				}
			}
		}
	}
}
void handleUploadFirmware(AsyncWebServerRequest* request, String filename, size_t index, uint8_t* data, size_t len, bool final) {
	
#ifndef DEBUG_WEB
	if (!request->authenticate(uname.c_str(), pwd.c_str()))
		return request->requestAuthentication();
#endif
	IsUploading = true;
	if (!index) {
		Serial.println("Updating...");
		//content_len = request->contentLength();
		//int cmd = (filename.indexOf("spiffs") > -1) ? U_PART : ;

		deviceState.IsPLCStarted = 0;
		sendDeviceStatus();
		if (!Update.begin(UPDATE_SIZE_UNKNOWN, U_FLASH)) {
			Update.printError(Serial);
		}
	}
	if (!Update.hasError()) {
		if (Update.write(data, len) != len) {
			Update.printError(Serial);
		}
	}
	if (final) {
		AsyncWebServerResponse* response = request->beginResponse(302, "text/plain", "Please wait while the device reboots");
		response->addHeader("Refresh", "20");
		response->addHeader("Location", "/");
		request->send(response);
		if (!Update.end(true)) {
			Update.printError(Serial);
			IsUploading = false;
		}
		else {
			Serial.println("Update complete");
			Serial.flush();
			ESP.restart();
		}
	}
}

void initWebServer() {
	if (!SPIFFS.begin(true)) {
		return;
	}
	uname = String(config.Admin_UserName);
	pwd = String(config.Admin_Password);
	uname.trim();
	pwd.trim();
#ifdef DEBUG_WEB
	webServer.serveStatic("/", SPIFFS, "/").setDefaultFile("index.html").setCacheControl("max-age=6000");
#else
	webServer.serveStatic("/", SPIFFS, "/").setDefaultFile("index.html").setCacheControl("max-age=6000").setAuthentication(uname.c_str(), pwd.c_str());
#endif
	webServer.on("/logout.html", HTTP_GET, [](AsyncWebServerRequest* request) {
		request->send(200, "text/html", "<html><head><title>Logged Out</title></head><body><h1>You have been logged out...</h1></body></html>");
	});
	webServer.onNotFound([](AsyncWebServerRequest* request) {
		request->send(404, "text/plain", "The content you are looking for was not found.");
	});

	// REST API
	webServer.on("/api/logout", HTTP_GET, [](AsyncWebServerRequest* request) {
		request->send(401);
	});

	webServer.on("/api/config", HTTP_GET, [](AsyncWebServerRequest* request) {
#ifndef DEBUG_WEB
		if (!request->authenticate(uname.c_str(), pwd.c_str()))
			return request->requestAuthentication();
#endif
		DynamicJsonDocument resultDoc(1024);
		String resultJsonValue;
		resultDoc["Admin_UserName"] = trimChar(config.Admin_UserName);
		//resultDoc["Admin_Password"] = trimChar(config.Admin_Password);

		resultDoc["WiFi_AP_HostName"] = trimChar(config.WiFi_AP_HostName);
		resultDoc["WiFi_AP_SSID"] = trimChar(config.WiFi_AP_SSID);
		resultDoc["WiFi_AP_Password"] = trimChar(config.WiFi_AP_Password);

		resultDoc["WiFi_STA_Enabled"] = config.WiFi_STA_Enabled;
		resultDoc["WiFi_STA_HostName"] = trimChar(config.WiFi_STA_HostName);
		resultDoc["WiFi_STA_SSID"] = trimChar(config.WiFi_STA_SSID);
		resultDoc["WiFi_STA_Password"] = trimChar(config.WiFi_STA_Password);

		resultDoc["WiFi_MAC_Address"] = WiFi.macAddress();
		resultDoc["WiFi_IP_Address"] = WiFi.getMode() == WIFI_MODE_STA ? WiFi.localIP().toString() : WiFi.softAPIP().toString();


		resultDoc["MQTT_Server_Address"] = trimChar(config.MQTT_Server_Address);
		resultDoc["MQTT_Server_Port"] = config.MQTT_Server_Port;
		resultDoc["MQTT_Root_Topic"] = trimChar(config.MQTT_Root_Topic);
		resultDoc["MQTT_Server_UserName"] = trimChar(config.MQTT_Server_UserName);
		resultDoc["MQTT_Server_Password"] = trimChar(config.MQTT_Server_Password);
		resultDoc["MQTT_Tele_Interval"] = config.MQTT_Tele_Interval;
		resultDoc["MQTT_Enabled"] = config.MQTT_Enabled;


		resultDoc["ModBus_TCP_Port"] = config.ModBus_TCP_Port;
		resultDoc["ModBus_TCP_Enabled"] = config.ModBus_TCP_Enabled;

		resultDoc["ModBus_RTU_SlaveID"] = config.ModBus_RTU_SlaveID;
		resultDoc["ModBus_RTU_Interface"] = config.ModBus_RTU_Interface;
		resultDoc["ModBus_RTU_Baud"] = config.ModBus_RTU_Baud;
		resultDoc["ModBus_RTU_Enabled"] = config.ModBus_RTU_Enabled;

		resultDoc["IsPLCStarted"] = deviceState.IsPLCStarted;

#ifdef ESP32_PLC_MASTER
		resultDoc["IsPLCSlave"] = 0;
#else
		resultDoc["IsPLCSlave"] = 1;
#endif


#ifdef OTA_ENABLED
		resultDoc["IsOTAEnabled"] = 1;
#else
		resultDoc["IsOTAEnabled"] = 0;
#endif



		serializeJson(resultDoc, resultJsonValue);
#ifdef DEBUG_WEB
		AsyncWebServerResponse* response = request->beginResponse(200, "application/json", resultJsonValue);
		response->addHeader("Access-Control-Allow-Origin", "*");
		request->send(response);
#else
		request->send(200, "application/json", resultJsonValue);
#endif
	});
	webServer.on("/api/update/security", HTTP_POST, [](AsyncWebServerRequest* request) {
#ifndef DEBUG_WEB
		if (!request->authenticate(uname.c_str(), pwd.c_str()))
			return request->requestAuthentication();
#endif
		String oldPassword;
		String newPassword;
		String comparePassword = String(config.Admin_Password);
		comparePassword.trim();
		if (request->hasParam("Admin_Old_Password", true)) {
			oldPassword = request->getParam("Admin_Old_Password", true)->value();
			oldPassword.trim();
		}
		if (request->hasParam("Admin_New_Password", true)) {
			newPassword = request->getParam("Admin_New_Password", true)->value();
			newPassword.trim();
		}
		if (oldPassword.equals(comparePassword)) {
			if (request->hasParam("Admin_UserName", true)) {
				String val = request->getParam("Admin_UserName", true)->value();
				strncpy((char*)&config.Admin_UserName, val.c_str(), 32);
			}
			strncpy((char*)&config.Admin_Password, newPassword.c_str(), 32);
			saveConfig();
			request->send(200);
		}
		else {
			request->send(400, "application/json", "Invalid password");
		}
	});
	webServer.on("/api/update/wifi", HTTP_POST, [](AsyncWebServerRequest* request) {
#ifndef DEBUG_WEB
		if (!request->authenticate(uname.c_str(), pwd.c_str()))
			return request->requestAuthentication();
#endif
		String val;
		if (request->hasParam("WiFi_AP_HostName", true)) {
			val = request->getParam("WiFi_AP_HostName", true)->value();
			strncpy((char*)&config.WiFi_AP_HostName, val.c_str(), 32);
		}
		if (request->hasParam("WiFi_AP_SSID", true)) {
			val = request->getParam("WiFi_AP_SSID", true)->value();
			strncpy((char*)&config.WiFi_AP_SSID, val.c_str(), 32);
		}
		if (request->hasParam("WiFi_AP_Password", true)) {
			val = request->getParam("WiFi_AP_Password", true)->value();
			strncpy((char*)&config.WiFi_AP_Password, val.c_str(), 64);
		}
		if (request->hasParam("WiFi_STA_Enabled", true)) {
			val = request->getParam("WiFi_STA_Enabled", true)->value();
			config.WiFi_STA_Enabled = val.toInt();
		}
		else
			config.WiFi_STA_Enabled = 0;
		if (request->hasParam("WiFi_STA_HostName", true)) {
			val = request->getParam("WiFi_STA_HostName", true)->value();
			strncpy((char*)&config.WiFi_STA_HostName, val.c_str(), 32);
		}
		if (request->hasParam("WiFi_STA_SSID", true)) {
			val = request->getParam("WiFi_STA_SSID", true)->value();
			strncpy((char*)&config.WiFi_STA_SSID, val.c_str(), 32);
		}
		if (request->hasParam("WiFi_STA_Password", true)) {
			val = request->getParam("WiFi_STA_Password", true)->value();
			strncpy((char*)&config.WiFi_STA_Password, val.c_str(), 64);
		}
		saveConfig();
		request->send(200);

	});
	webServer.on("/api/update/mqtt", HTTP_POST, [](AsyncWebServerRequest* request) {
#ifndef DEBUG_WEB
		if (!request->authenticate(uname.c_str(), pwd.c_str()))
			return request->requestAuthentication();
#endif
		String val;
		if (request->hasParam("MQTT_Server_Address", true)) {
			val = request->getParam("MQTT_Server_Address", true)->value();
			strncpy((char*)&config.MQTT_Server_Address, val.c_str(), 100);
		}
		if (request->hasParam("MQTT_Root_Topic", true)) {
			val = request->getParam("MQTT_Root_Topic", true)->value();
			strncpy((char*)&config.MQTT_Root_Topic, val.c_str(), 100);
		}
		if (request->hasParam("MQTT_Server_UserName", true)) {
			val = request->getParam("MQTT_Server_UserName", true)->value();
			strncpy((char*)&config.MQTT_Server_UserName, val.c_str(), 32);
		}
		if (request->hasParam("MQTT_Server_Password", true)) {
			val = request->getParam("MQTT_Server_Password", true)->value();
			strncpy((char*)&config.MQTT_Server_Password, val.c_str(), 32);
		}
		if (request->hasParam("MQTT_Server_Port", true)) {
			val = request->getParam("MQTT_Server_Port", true)->value();
			config.MQTT_Server_Port = val.toInt();
		}
		if (request->hasParam("MQTT_Tele_Interval", true)) {
			val = request->getParam("MQTT_Tele_Interval", true)->value();
			config.MQTT_Tele_Interval = val.toInt();
		}
		if (request->hasParam("MQTT_Enabled", true)) {
			val = request->getParam("MQTT_Enabled", true)->value();
			config.MQTT_Enabled = val.toInt();
		}
		else
			config.MQTT_Enabled = 0;
		saveConfig();
		request->send(200);
	});
	webServer.on("/api/update/modbus", HTTP_POST, [](AsyncWebServerRequest* request) {
#ifndef DEBUG_WEB
		if (!request->authenticate(uname.c_str(), pwd.c_str()))
			return request->requestAuthentication();
#endif
		String val;
		if (request->hasParam("ModBus_TCP_Port", true)) {
			val = request->getParam("ModBus_TCP_Port", true)->value();
			config.ModBus_TCP_Port = val.toInt();
		}
		if (request->hasParam("ModBus_TCP_Enabled", true)) {
			val = request->getParam("ModBus_TCP_Enabled", true)->value();
			config.ModBus_TCP_Enabled = val.toInt();
		}
		else
			config.ModBus_TCP_Enabled = 0;

		if (request->hasParam("ModBus_RTU_SlaveID", true)) {
			val = request->getParam("ModBus_RTU_SlaveID", true)->value();
			config.ModBus_RTU_SlaveID = val.toInt();
		}
		if (request->hasParam("ModBus_RTU_Interface", true)) {
			val = request->getParam("ModBus_RTU_Interface", true)->value();
			config.ModBus_RTU_Interface = val.toInt();
		}
		if (request->hasParam("ModBus_RTU_Baud", true)) {
			val = request->getParam("ModBus_RTU_Baud", true)->value();
			config.ModBus_RTU_Baud = val.toInt();
		}
		if (request->hasParam("ModBus_RTU_Enabled", true)) {
			val = request->getParam("ModBus_RTU_Enabled", true)->value();
			config.ModBus_RTU_Enabled = val.toInt();
		}
		else
			config.ModBus_RTU_Enabled = 0;
		saveConfig();
		request->send(200);
	});
	webServer.on("/api/update/plc", HTTP_POST, [](AsyncWebServerRequest* request) {
#ifndef DEBUG_WEB
		if (!request->authenticate(uname.c_str(), pwd.c_str()))
			return request->requestAuthentication();
#endif
		String val;
		if (request->hasParam("IsPLCStarted", true)) {
			val = request->getParam("IsPLCStarted", true)->value();
			deviceState.IsPLCStarted = val.toInt();
		}
		updateDeviceState();
		request->send(200);
		sendDeviceStatus();
	});

	webServer.on("/api/system/status", HTTP_GET, [](AsyncWebServerRequest* request) {
#ifndef DEBUG_WEB
		if (!request->authenticate(uname.c_str(), pwd.c_str()))
			return request->requestAuthentication();
#endif
		DynamicJsonDocument resultDoc(32);
		String resultJsonValue;
		resultDoc["PLCStatus"] = deviceState.IsPLCStarted;
		serializeJson(resultDoc, resultJsonValue);
#ifdef DEBUG_WEB
		AsyncWebServerResponse* response = request->beginResponse(200, "application/json", resultJsonValue);
		response->addHeader("Access-Control-Allow-Origin", "*");
		request->send(response);
#else
		request->send(200, "application/json", resultJsonValue);
#endif

	});
	webServer.on("/api/system/reboot", HTTP_POST, [](AsyncWebServerRequest* request) {
#ifndef DEBUG_WEB
		if (!request->authenticate(uname.c_str(), pwd.c_str()))
			return request->requestAuthentication();
#endif
		AsyncWebServerResponse* response = request->beginResponse(302, "text/plain", "Please wait while the device reboots");
		response->addHeader("Refresh", "30");
		response->addHeader("Location", "/");
		request->send(response);
		ESP.restart();
	});
	webServer.on("/api/system/reset", HTTP_POST, [](AsyncWebServerRequest* request) {
#ifndef DEBUG_WEB
		if (!request->authenticate(uname.c_str(), pwd.c_str()))
			return request->requestAuthentication();
#endif
		resetConfig();
		AsyncWebServerResponse* response = request->beginResponse(302, "text/plain", "Please wait while the device reboots");
		response->addHeader("Refresh", "30");
		response->addHeader("Location", "/");
		request->send(response);
		ESP.restart();
	});
	webServer.on("/api/pinout", HTTP_GET, [](AsyncWebServerRequest* request) {
#ifndef DEBUG_WEB
		if (!request->authenticate(uname.c_str(), pwd.c_str()))
			return request->requestAuthentication();
#endif
		DynamicJsonDocument resultDoc(1024);
		String resultJsonValue;
		for (int i = 0; i < NUM_DISCRETE_INPUT; i++)
		{
			resultDoc["DI"][i] = pinMask_DIN[i];
		}
		for (int i = 0; i < NUM_DISCRETE_OUTPUT; i++)
		{
			resultDoc["DO"][i] = pinMask_DOUT[i];
		}
		for (int i = 0; i < NUM_ANALOG_INPUT; i++)
		{
			resultDoc["AI"][i] = pinMask_AIN[i];
		}
		for (int i = 0; i < NUM_ANALOG_OUTPUT; i++)
		{
			resultDoc["AO"][i] = pinMask_AOUT[i];
		}
		serializeJson(resultDoc, resultJsonValue);
#ifdef DEBUG_WEB
		AsyncWebServerResponse* response = request->beginResponse(200, "application/json", resultJsonValue);
		response->addHeader("Access-Control-Allow-Origin", "*");
		request->send(response);
#else
		request->send(200, "application/json", resultJsonValue);
#endif
	});

	webServer.on("/api/info", HTTP_GET, [](AsyncWebServerRequest* request) {
#ifndef DEBUG_WEB
		if (!request->authenticate(uname.c_str(), pwd.c_str()))
			return request->requestAuthentication();
#endif
		DynamicJsonDocument resultDoc(1024);
		String resultJsonValue;
		resultDoc["ChipRevision"] = String(ESP.getChipRevision());
		resultDoc["CpuFreqMHz"] = String(ESP.getCpuFreqMHz());
		resultDoc["SketchMD5"] = String(ESP.getSketchMD5());
		resultDoc["SketchSize"] = String(ESP.getSketchSize());
		resultDoc["CycleCount"] = String(ESP.getCycleCount());
		resultDoc["SdkVersion"] = String(ESP.getSdkVersion());
		resultDoc["HeapSize"] = String(ESP.getHeapSize());
		resultDoc["FreeHeap"] = String(ESP.getFreeHeap());
		resultDoc["FreePsram"] = String(ESP.getFreePsram());
		resultDoc["MaxAllocPsram"] = String(ESP.getMaxAllocPsram());
		resultDoc["MaxAllocHeap"] = String(ESP.getMaxAllocHeap());
		resultDoc["FreeSketchSpace"] = String(ESP.getFreeSketchSpace());
		resultDoc["WiFiMacAddress"] = WiFi.macAddress();
		serializeJson(resultDoc, resultJsonValue);
#ifdef DEBUG_WEB
		AsyncWebServerResponse* response = request->beginResponse(200, "application/json", resultJsonValue);
		response->addHeader("Access-Control-Allow-Origin", "*");
		request->send(response);
#else
		request->send(200, "application/json", resultJsonValue);
#endif
	});

#ifdef OTA_ENABLED

	webServer.on("/api/update/firmware", HTTP_POST, [](AsyncWebServerRequest* request) {
#ifndef DEBUG_WEB
		if (!request->authenticate(uname.c_str(), pwd.c_str())) {
			request->send(401);
			return;
		}
#endif
		}, [](AsyncWebServerRequest* request, const String& filename, size_t index, uint8_t* data, size_t len, bool final)
		{
			handleUploadFirmware(request, filename, index, data, len, final);
		});
#endif

	ws.onEvent(onWsEvent);
	webServer.addHandler(&ws);
	webServer.begin();
	ws.enable(true);
	tmrWSHealth = xTimerCreate(
		"tmrWSHealth", /* name */
		pdMS_TO_TICKS(5000), /* period/time */
		pdTRUE, /* auto reload */
		(void*)1, /* timer ID */
		&tmrWSHealthFun); /* callback */
	if (xTimerStart(tmrWSHealth, 10) != pdPASS) {
		Serial.println("Timer start error");
	}
}

void sendDeviceStatus() {
	msgCode = 255;
	setWebSocketHeader(msgCode);
	setWebSocketContent((uint8_t*)&deviceState.IsPLCStarted, 1);
	sendAllWebSocketMessage();
}
void resetWebServer() {
	ws.closeAll();
	delay(500);
	webServer.reset();
}
