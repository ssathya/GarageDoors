#include <ArduinoJson.h>
#include <stdio.h>
#include "ESP8266WiFi.h"
#include <AverageList.h>

#define DEBUG_ON

#define TRIG_1 4
#define ECHO_1 5

#define TRIG_2 12
#define ECHO_2 13
#define LED 14

//Lets make the compiler happy. When you pass references the compiler needs to know
//ahead that type of references we are passing.
void ReadSensorValues(unsigned, unsigned, AverageList<unsigned>&, float&);
bool ValidatateValue(unsigned, AverageList<unsigned>&);

//readings from SR-04
#define MAX_READINGS 10
unsigned HistoricData1[MAX_READINGS] = { 0 };
unsigned HistoricData2[MAX_READINGS] = { 0 };
#define limits 0.2 //20% is huge but better keep a higher limits.
AverageList<unsigned> Sensor1Samples = AverageList<unsigned>(HistoricData1, MAX_READINGS);
AverageList<unsigned> Sensor2Samples = AverageList<unsigned>(HistoricData2, MAX_READINGS);

//JSON object
StaticJsonBuffer<128> jsonBuffer;
JsonObject& jsonObj = jsonBuffer.createObject();

//WIFI Related
char mcAddress[32];
const char* host = "api.thingspeak.com";
const char* ssid = "MY NETWORK NAME";
const char* password = "MY NETWORK PASSWORD";

//measurements
float Door1Distance = 0;
float Door2Distance = 0;

//Loop Controllers

#define sensorCycleTime 3000U
#define dataSendTime 60000U
#define dataGetTime 21312U
#define disconnectCheck 180000U
#define ledCycleTimer 250U

unsigned long sensorTimer = 0;
unsigned long dataSendTimer = 0;
unsigned long disconnetCheckTimer = 0;
unsigned long ledCheckTimer = 0;

//Strings for POST request
const char* postHeader1 = "POST /update?key=XXXXXXXXXXXXXXXX HTTP/1.1\r\n";
const char* postHeader2 = "Host: api.thingspeak.com\r\n";
const char* postHeader3 = "Content-Type: application/json\r\n";
const char* postHeader4 = "Cache-Control: no-cache\r\n";
const char* postHeader5 = "Content-Length: $JSONSTRINGLENGTH$";
const char* postHeader6 = "\r\nConnection: close\r\n\r\n";
String postMessageHeader;

//Policemen
short disconnectedCount = 0;

void setup()
{
#ifdef DEBUG_ON
	Serial.begin(9600);
	delay(10);
#endif
	ConnectToNetwork(ssid, password);

	//initialize values
	Door1Distance = 0;
	Door2Distance = 0;
	BuildPostMessageHeader();

	pinMode(TRIG_1, OUTPUT);
	pinMode(ECHO_1, INPUT);
	pinMode(TRIG_2, OUTPUT);
	pinMode(ECHO_2, INPUT);
	pinMode(LED, OUTPUT);

	for (int i = 0; i < MAX_READINGS; i++)
	{
		Sensor1Samples.addValue(0);
		Sensor2Samples.addValue(0);
	}
}

void loop()
{
	if (CycleCheck(&sensorTimer, sensorCycleTime))
	{
		ReadSensorValues(TRIG_1, ECHO_1, Sensor1Samples, Door1Distance);
		ReadSensorValues(TRIG_2, ECHO_2, Sensor2Samples, Door2Distance);
	}

	if (CycleCheck(&dataSendTimer, dataSendTime))
		SendDataToServer();
	if (CycleCheck(&disconnetCheckTimer, disconnectCheck))
		CheckWiFiHealth(ssid, password);
	if (CycleCheck(&ledCheckTimer, ledCycleTimer))
		BlinkLed();
}

void ReadSensorValues(unsigned triggerPin, unsigned echoPin,
	AverageList<unsigned>& samples, float& distance)
{
	//Bring the pin low - should be low but just in case
	digitalWrite(triggerPin, LOW);
	delayMicroseconds(2);
	digitalWrite(triggerPin, HIGH);
	delayMicroseconds(10);
	digitalWrite(triggerPin, LOW);
	//read values
	unsigned intervalCount = pulseIn(echoPin, HIGH, dataGetTime);//timing out for 12 ft.
	intervalCount = ValidatateValue(intervalCount, samples) ? intervalCount : samples.getAverage();
	distance = (float)intervalCount / 74.0 / 2.0;
#ifdef DEBUG_ON
	SerialPrint("Reporting for: ");
	SerialPrintln(triggerPin == TRIG_1 ? "Sensor 1" : "Sensor 2");
	char buffer[128];
	sprintf(buffer, "Interval %u Distance %d", intervalCount, (int)distance);
	SerialPrintln(buffer);
#endif
}
bool ValidatateValue(unsigned intervalCounter, AverageList<unsigned>& samples)
{
	samples.addValue(intervalCounter);
	unsigned averageValue = samples.getAverage();
	unsigned minValue = averageValue - (averageValue*limits);
	unsigned maxValue = averageValue + (averageValue*limits);
	if (intervalCounter < minValue || intervalCounter > maxValue)
	{
		SerialPrint("Interval count:");
		SerialPrintln(String(intervalCounter));
		SerialPrint("Average count:");
		SerialPrintln(String(averageValue));
		return false;
	}

	return true;
}
void SendDataToServer()
{
	SerialPrint("Connecting to ");
	SerialPrintln(host);

	// Use WiFiClient class to create TCP connections
	WiFiClient client;
	const int httpPort = 80;
	if (!client.connect(host, httpPort)) {
		SerialPrintln("connection failed");
		++disconnectedCount;
		return;
	}
	disconnectedCount = 0;
	client.setTimeout(5000);
	String serverMessage = BuildOutMessage();
	client.print(serverMessage);
	SerialPrint(serverMessage);
	delay(10);

	// Read all the lines of the reply from server and print them to Serial

	while (client.available()){
		String line = client.readString();
		SerialPrintln(line);
	}

	SerialPrintln("");
	SerialPrintln("closing connection");
}
String BuildOutMessage()
{
	String outMessage(postMessageHeader);
	jsonObj["field1"] = Door1Distance;
	jsonObj["field2"] = Door2Distance;
	char buffer[128];
	jsonObj.printTo(buffer, 128);
	int stringLen = strlen(buffer);
	outMessage.replace("$JSONSTRINGLENGTH$", String(stringLen));
	outMessage += buffer;
	outMessage += "\r\n";
	return outMessage;
}