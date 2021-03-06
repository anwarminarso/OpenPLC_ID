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
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "TypeDefStruct.h"
#include "GlobalVariables.h"
#include "ClientAPI.h"
#include "Configuration.h"
#include "Utils.h"
#include "openplc.h"

WiFiClient espClient;
PubSubClient client(espClient);

String mqtt_id;
String publishTopic;
String subscribeTopic;
String resultTopic;
String mqttUsr;
String mqttPwd;
String mqttServerAddr;
bool mqttEnabled;
ulong teleInterval;
ulong latestTele;
ulong currentMillis;

StaticJsonDocument<1024> pubDoc;
StaticJsonDocument<512> subDoc;

#define MQTT_RESULT_MSG_DATA_ERROR			"Data format error.."
#define MQTT_RESULT_MSG_PARSING_ERROR		"Error parsing data.."
#define MQTT_RESULT_MSG_CMD_NOT_SUPPORTED	"Command not supported"
#define MQTT_RESULT_MSG_OK					"OK"

void publishResult(const char* msg) {
	client.publish(resultTopic.c_str(), msg);
}
void clientAPI_CB(char* topic, byte* message, unsigned int length) {
	String msgValue;
	for (int i = 0; i < length; i++) {
		msgValue += (char)message[i];
	}
	if (String(topic).equals(subscribeTopic)) {
		DeserializationError error = deserializeJson(subDoc, msgValue);
		if (error) {
			publishResult(MQTT_RESULT_MSG_DATA_ERROR);
			return;
		}
		try
		{
			uint8_t cmd = 0;
			if (subDoc["cmd"].is<uint8_t>())
				cmd = subDoc["cmd"].as<uint8_t>();
			switch (cmd)
			{
				case 5: //write single coil
					{
						uint8_t reg = 0;
						uint8_t val = 0;
						if (!subDoc["reg"].is<uint8_t>() || !subDoc["val"].is<uint8_t>()) {
							publishResult(MQTT_RESULT_MSG_PARSING_ERROR);
							return;
						}
						reg = subDoc["reg"].as<uint8_t>();
						val = subDoc["val"].as<uint8_t>();
						if (val > 1)
							val = 1;
						if (reg >= NUM_DISCRETE_OUTPUT) {
							publishResult(MQTT_RESULT_MSG_PARSING_ERROR);
							return;
						}
						DOUT_Values[reg] = val;
						if (bool_output[reg / 8][reg % 8] != NULL)
							*bool_output[reg / 8][reg % 8] = DOUT_Values[reg];
						publishResult(MQTT_RESULT_MSG_OK);
					}
					break;
				case 6: //Write Single Register
					{
						uint8_t reg = 0;
						uint16_t val = 0;
						if (!subDoc["reg"].is<uint8_t>() || !subDoc["val"].is<uint16_t>()) {
							publishResult(MQTT_RESULT_MSG_PARSING_ERROR);
							return;
						}
						reg = subDoc["reg"].as<uint8_t>();
						val = subDoc["val"].as<uint16_t>();
						if (reg >= NUM_ANALOG_OUTPUT) {
							publishResult(MQTT_RESULT_MSG_PARSING_ERROR);
							return;
						}
						AOUT_Values[reg] = val;
						if (int_output[reg] != NULL)
							*int_output[reg] = AOUT_Values[reg];
						publishResult(MQTT_RESULT_MSG_OK);
					}
					break;
				case 16: //Write Multiple Register
					{
						JsonArray regs;
						JsonArray vals;
						if (!subDoc["regs"].is<JsonArray>() || !subDoc["vals"].is<JsonArray>()) {
							publishResult(MQTT_RESULT_MSG_PARSING_ERROR);
							return;
						}
						regs = subDoc["regs"].as<JsonArray>();
						vals = subDoc["vals"].as<JsonArray>();
						if (regs.size() != vals.size()) {
							publishResult(MQTT_RESULT_MSG_PARSING_ERROR);
							return;
						}
						size_t len = regs.size();
						for (uint8_t i = 0; i < len; i++)
						{
							if (!regs[i].is<uint8_t>() || !vals[i].is<uint16_t>() || regs[i] >= NUM_ANALOG_OUTPUT) {
								publishResult(MQTT_RESULT_MSG_PARSING_ERROR);
								return;
							}
						}
						for (uint8_t i = 0; i < len; i++)
						{
							uint8_t reg = regs[i].as<uint8_t>();
							uint16_t val = vals[i];
							AOUT_Values[reg] = val;
							if (int_output[reg] != NULL)
								*int_output[reg] = AOUT_Values[reg];
						}
						publishResult(MQTT_RESULT_MSG_OK);
					}
					break;
				default:
					publishResult(MQTT_RESULT_MSG_CMD_NOT_SUPPORTED);
					break;
			}
		}
		catch (const std::exception&)
		{
			publishResult(MQTT_RESULT_MSG_PARSING_ERROR);
		}
	}
}

void reconnectClientAPI() {
	// Loop until we're reconnected
	if (WiFi.isConnected() && !client.connected()) {
	 //if (!client.connected()) {
		Serial.print("Attempting MQTT connection: ");
		// Attempt to connect
		if (client.connect(mqtt_id.c_str(), mqttUsr.c_str(), mqttPwd.c_str())) {
			Serial.println("connected");
			// Subscribe
			client.subscribe(subscribeTopic.c_str());
		}
		else {
			Serial.print("failed, rc=");
			Serial.print(client.state());
			Serial.println();
		}
	}
}


void initClientAPI() {
	teleInterval = config.MQTT_Tele_Interval * 1000;
	mqttEnabled = config.MQTT_Enabled > 0;
	if (!mqttEnabled)
		return;
	String macAddress = getWifiMacAddress();
	mqtt_id = "ESP32-PLUS-";
	mqtt_id += macAddress;
	publishTopic = config.MQTT_Root_Topic;
	publishTopic += "/tele";
	subscribeTopic = config.MQTT_Root_Topic;
	subscribeTopic += "/command";
	resultTopic = config.MQTT_Root_Topic;
	resultTopic += "/result";
	mqttServerAddr = String(config.MQTT_Server_Address);
	mqttServerAddr.trim();

	mqttUsr = String(config.MQTT_Server_UserName);
	mqttUsr.trim();
	mqttPwd = String(config.MQTT_Server_Password);
	mqttPwd.trim();


	/*Serial.print("MQTT Publish Topic: ");
	Serial.println(publishTopic);
	Serial.print("MQTT Subscribe Topic: ");
	Serial.println(subscribeTopic);
	Serial.print("MQTT Result Topic: ");
	Serial.println(resultTopic);
	Serial.print("MQTT Server Address: ");
	Serial.print(mqttServerAddr);
	Serial.print(", Port: ");
	Serial.println(config.MQTT_Server_Port);*/
	client.setServer(mqttServerAddr.c_str(), config.MQTT_Server_Port);
	client.setCallback(clientAPI_CB);
}

void clientAPILoop() {
	if (!mqttEnabled)
		return;
	if (!WiFi.isConnected())
		return;
	if (client.connected()) {
		client.loop();
		currentMillis = millis();
		if (currentMillis - latestTele > teleInterval)
		{
			publishClientAPI();
			latestTele = currentMillis;
		}
	}
	else {
		reconnectClientAPI();
	}
}

void publishClientAPI() {
	String resultJsonValue;
	for (int i = 0; i < NUM_DISCRETE_INPUT; i++)
	{
		pubDoc["DiscreteInputs"][i] = DIN_Values[i];
	}

	for (int i = 0; i < NUM_ANALOG_INPUT; i++)
	{
		pubDoc["InputRegisters"][i] = AIN_Values[i];
	}

	for (int i = 0; i < NUM_DISCRETE_OUTPUT; i++)
	{
		pubDoc["Coils"][i] = DOUT_Values[i];
	}

	for (int i = 0; i < NUM_ANALOG_OUTPUT; i++)
	{
		pubDoc["HoldingRegisters"][i] = AOUT_Values[i];
	}

	serializeJson(pubDoc, resultJsonValue);

	if (!client.publish(publishTopic.c_str(), resultJsonValue.c_str()))
		Serial.println("FAILED PUBLISH");
}