#include "Arduino.h"
#include <ESP8266WiFi.h>
#include <WiFiManager.h>
#include "EspSoftwareSerial.h"
const char* host = "192.168.88.245";
WiFiClient client;

//software serial: GPIO5 as RX, GPIO2 as TX
EspSoftwareSerial receiver(D5,D2); // https://github.com/plerup/espsoftwareserial

String dataCh = "";
String dataWh = "";

bool dataAwaitsTransmission = false;
bool dataAwaitsBackup = false;



void setup()
{
	Serial.begin(115200);
	Serial.println("hello");

	receiver.begin(38400);
}

int dataReadPeriodMs = 50;
unsigned long old_time_dataRead = 0;

void receiveData(){
	if (receiver.available() > 0){
		//TODO: read buffer, choose the last message, clean the buffer
		String incomingData = receiver.readString();
		int posR = incomingData.lastIndexOf('\n');
		int posL = 0;
		for (int i=posR-1; i>=0; i--){
			if (incomingData.charAt(i)=='\n'){
				posL = i;
				break;
			}
		}
		String dataSubstr = incomingData.substring(posL,posR);
		dataCh = dataSubstr.substring(dataSubstr.indexOf("Ch = ")+5,dataSubstr.indexOf(','));
		dataWh = dataSubstr.substring(dataSubstr.indexOf("Wh = ")+5);

		dataAwaitsTransmission = true;
		dataAwaitsBackup = true;
	}
}

int saveDataPeriodMs = 50;
unsigned long old_time_saveData = 0;
void saveData(){
	if (dataAwaitsBackup){
		//TODO: save data to SPIFFS

		dataAwaitsBackup = false;
	}
}

void transmitData(){
	if (dataAwaitsTransmission){
		//TODO: transmit data over the web

		dataAwaitsTransmission = false;
	}
}

void checkTime(int period, unsigned long& old_time, void callback(void*), void* args){
	if (millis() - old_time > period){
		callback();
		old_time = millis();
	}
}

// The loop function is called in an endless loop
void loop()
{
	checkTime(dataReadPeriodMs, old_time_dataRead, receiveData, NULL);
	checkTime(saveDataPeriodMs, old_time_saveData, saveData, NULL);
}