#include "Arduino.h"
#include "VescUart.h"
#include "datatypes.h"
#include <SoftwareSerial.h>

//RX on pin 10, TX on pin 11
SoftwareSerial Serial1(9, 10);

#define DEBUG
unsigned long count;

void setup() {
	#ifdef DEBUG
	//SEtup debug port
	Serial.begin(115200);
	#endif
	//Setup UART port
	Serial1.begin(115200);
}

struct bldcMeasure measuredValues;
	
// the loop function runs over and over again until power down or reset
void loop() {
	//int len=0;
	//len = ReceiveUartMessage(message);
	//if (len > 0)
	//{
	//	len = PackSendPayload(message, len);
	//	len = 0;
	//}
	
	if (VescUartGetValue(measuredValues)) {
		Serial.print("Loop: "); Serial.println(count++);
		SerialPrint(measuredValues);
	}
	else
	{
		Serial.println("Failed to get data!");
	}
	
}