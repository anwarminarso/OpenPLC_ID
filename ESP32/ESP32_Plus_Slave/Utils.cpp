

#include <WiFi.h>
#include "Utils.h"

String getChipID() {
	uint64_t id = ESP.getEfuseMac();
	uint16_t chip_h = (uint16_t)(id >> 32);
	uint32_t chip_d = (uint32_t)id;
	char result[13];
	sprintf(result, "%04X%08X", chip_h, chip_d);
	return String(result);
}
String getWifiMacAddress() {
	String macAddress = WiFi.macAddress();
	macAddress.remove(14, 1);
	macAddress.remove(11, 1);
	macAddress.remove(8, 1);
	macAddress.remove(5, 1);
	macAddress.remove(2, 1);
	macAddress.toUpperCase();
	return macAddress;
}
String trimChar(char* value) {
	String result = String(value);
	result.trim();
	return result;
}