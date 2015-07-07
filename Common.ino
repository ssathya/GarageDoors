void SerialPrintln(String message)
{
#ifdef DEBUG_ON
	Serial.println(message);
#endif
}
void SerialPrintln(const char* message)
{
#ifdef DEBUG_ON
	Serial.println(message);
#endif
}
void SerialPrint(String message)
{
#ifdef DEBUG_ON
	Serial.print(message);
#endif
}
void SerialPrint(const char* message)
{
#ifdef DEBUG_ON
	Serial.print(message);
#endif
}

void ConnectToNetwork(const char* ssid, const char* password)
{
	//just in case
	WiFi.disconnect();

	SerialPrintln("");
	SerialPrintln("");
	SerialPrintln("Connecting to ");
	SerialPrintln(ssid);
	WiFi.begin(ssid, password);
	while (WiFi.status() != WL_CONNECTED) {
		delay(500);
		SerialPrint(".");
	}

	SerialPrintln("");
	SerialPrintln("WiFi connected");
	SerialPrintln("IP address: ");
#ifdef DEBUG_ON
	Serial.println(WiFi.localIP());
#endif
	byte mac[6];
	WiFi.macAddress(mac);

	const char cSep = '-';
	sprintf(mcAddress, "%02X%c%02X%c%02X%c%02X%c%02X%c%02X",
		mac[0] & 0xff,
		cSep,
		mac[1] & 0xff,
		cSep,
		mac[2] & 0xff,
		cSep,
		mac[3] & 0xff,
		cSep,
		mac[4] & 0xff,
		cSep,
		mac[5] & 0xff);
	SerialPrint("MAC Address: ");
	SerialPrintln(mcAddress);
}
void BuildPostMessageHeader()
{
	postMessageHeader = postHeader1;
	postMessageHeader += postHeader2;
	postMessageHeader += postHeader3;
	postMessageHeader += postHeader4;
	postMessageHeader += postHeader5;
	postMessageHeader += postHeader6;
}
boolean CycleCheck(unsigned long *lastMillis, unsigned int cycle)
{
	unsigned long currentMillis = millis();
	if (currentMillis - *lastMillis >= cycle)
	{
		*lastMillis = currentMillis;
		return true;
	}
	else
		return false;
}
void CheckWiFiHealth(const char* ssid, const char* password)
{
	if (disconnectedCount >= 10) //could not send for 10 minutes
	{
		ConnectToNetwork(ssid, password);
		disconnectedCount = 0;
	}
}
void BlinkLed()
{
	static short eventCounter;
	switch (++eventCounter)
	{
	case 1:
		digitalWrite(LED, HIGH);
		break;
	case 2:
		digitalWrite(LED, LOW);
		break;
	case 3:
		digitalWrite(LED, HIGH);
		break;
	case 4:
		digitalWrite(LED, LOW);
		break;
	case 5:
	case 6:
		digitalWrite(LED, HIGH);
		break;
	default:
		digitalWrite(LED, LOW);
		eventCounter = 0;
		break;
	}
}