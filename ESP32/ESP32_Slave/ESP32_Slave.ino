#include <Arduino.h>
#include <WiFi.h>




/******************PINOUT CONFIGURATION**************************
Discrete Inputs
Digital In:  17, 18, 19, 21, 22, 23, 27, 32 (%IX0.0 - %IX0.7)

Coils
Digital Out: 02, 04, 05, 12, 13, 14, 15, 16  (%QX0.0 - %QX0.7)


Input Registers/Holding Registers - Read
Analog In:   34, 35, 36, 39                 (%IW0 - %IW2)

Holding Registers - Write
Analog Out:  25, 26                         (%QW0 - %QW1)
*****************************************************************/

/*********NETWORK CONFIGURATION*********/

const char* ssid = "WiFi Name";
const char* password = "WiFi Password";

/***************************************/

#define MAX_DISCRETE_INPUT 		8
#define MAX_DISCRETE_INPUT 		8
#define MAX_COILS 				8
#define MAX_INP_REGS			4
#define MAX_HOLD_REGS 			2


uint8_t pinMask_DIN[] = { 17, 18, 19, 21, 22, 23, 27, 32 };
uint8_t pinMask_DOUT[] = { 02, 04, 05, 12, 13, 14, 15, 16 };
uint8_t pinMask_AIN[] = { 34, 35, 36, 39 };
uint8_t pinMask_AOUT[] = { 25, 26 };

unsigned char modbus_buffer[100];

int processModbusMessage(unsigned char* buffer, int bufferSize);

#include "modbus.h"

extern bool mb_discrete_input[MAX_DISCRETE_INPUT];
extern bool mb_coils[MAX_COILS];
extern uint16_t mb_input_regs[MAX_INP_REGS];
extern uint16_t mb_holding_regs[MAX_HOLD_REGS];
//Create the modbus server instance
WiFiServer server(502);
void hardwareInit()
{
    for (int i = 0; i < MAX_DISCRETE_INPUT; i++)
    {
        pinMode(pinMask_DIN[i], INPUT);
    }

    for (int i = 0; i < MAX_INP_REGS; i++)
    {
        pinMode(pinMask_AIN[i], INPUT);
    }

    for (int i = 0; i < MAX_COILS; i++)
    {
        pinMode(pinMask_DOUT[i], OUTPUT);
    }

    for (int i = 0; i < MAX_HOLD_REGS; i++)
    {
        pinMode(pinMask_AOUT[i], OUTPUT);
    }
    Serial.println("Hardware Initialized");
}

void updateIO()
{
    for (int i = 0; i < sizeof(pinMask_DIN); i++)
    {
        mb_discrete_input[i] = digitalRead(pinMask_DIN[i]);
    }

    for (int i = 0; i < sizeof(pinMask_DOUT); i++)
    {
        digitalWrite(pinMask_DOUT[i], mb_coils[i]);
    }

    for (int i = 0; i < sizeof(pinMask_AIN); i++)
    {
        mb_input_regs[i] = (analogRead(pinMask_AIN[i]) * 64);
    }

    for (int i = 0; i < sizeof(pinMask_AOUT); i++)
    {
        dacWrite(pinMask_AOUT[i], (mb_holding_regs[i] / 256));
       // analogWrite(pinMask_AOUT[i], mb_holding_regs[i] / 64);
    }
}
void PrintHex(uint8_t* data, uint8_t length) // prints 8-bit data in hex with leading zeroes
{
    for (int i = 0; i < length; i++)
    {
        if (data[i] < 0x10)
            Serial.print("0");
        Serial.print(data[i], HEX); Serial.print(" ");
    }
    Serial.println();
}


void setup()
{
    Serial.begin(115200);
    delay(10);

    hardwareInit();

    // Connect to WiFi network
    Serial.println();
    Serial.println();
    Serial.print("Connecting to ");
    Serial.println(ssid);

    WiFi.begin(ssid, password);
    uint8_t counter = 0;
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
        counter++;
        if (counter > 30) {
            Serial.print("Restart ESP");
            ESP.restart();
        }
    }

    Serial.println("");
    Serial.println("WiFi connected");

    // Start the server
    server.begin();
    Serial.println("Server started");

    // Print the IP address
    Serial.print("My IP: ");
    Serial.println(WiFi.localIP());

    updateIO();
}
void loop()
{
    WiFiClient client = server.available();
    if (!client)
        return;

    Serial.println("new client!");

    while (client.connected())
    {
        // Wait until the client sends some data
        while (!client.available())
        {
            delay(1);
            if (!client.connected())
                return;
        }

        int i = 0;
        while (client.available())
        {
            modbus_buffer[i] = client.read();
            i++;
            if (i == 100)
                break;
        }

        //DEBUG
        /*
        Serial.print("Received MB frame: ");
        PrintHex(modbus_buffer, i);
        */
        updateIO();
        unsigned int return_length = processModbusMessage(modbus_buffer, i);
        client.write((const uint8_t*)modbus_buffer, return_length);
        updateIO();
        delay(1);
    }

    Serial.println("Client disonnected");
    delay(1);
}
