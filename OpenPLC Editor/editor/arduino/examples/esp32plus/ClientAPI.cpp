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

void clientAPI_CB(char* topic, byte* message, unsigned int length) {
	String msgValue;

	for (int i = 0; i < length; i++) {
		msgValue += (char)message[i];
	}
	if (String(topic) == subscribeTopic) {
		DeserializationError error = deserializeJson(subDoc, msgValue);
		if (error) {
			Serial.println("Error parsing json");
			return;
		}
		try
		{
			/*uint8_t cmd = doc["cmd"];
			switch (cmd)
			{
				case MSG_SET_RELAY: 
					{
						uint8_t relay = staticDoc["args"];
						setGPIO(relay);
						client.publish(resultTopic.c_str(), "Success");
					}
					break;
				case MSG_RESET_CONFIG:
					{
						resetConfig();
						client.publish(resultTopic.c_str(), "Success");
						resetServer();
					}
					break;
				case MSG_RESET_ENERGY:
					{
						resetEnergy();
						client.publish(resultTopic.c_str(), "Success");
					}
					break;
				case MSG_RESET:
					{
						resetServer();
					}
					break;
				default:
					client.publish(resultTopic.c_str(), "Command unavailable");
					break;
			}*/
		}
		catch (const std::exception&)
		{
			client.publish(resultTopic.c_str(), "Error parsing json");
			Serial.println("Error parsing json");
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
	resultTopic += macAddress;
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