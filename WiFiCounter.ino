#include "Arduino.h"
#include <ESP8266WiFi.h>
#include "EspSoftwareSerial.h"
#include <ESP8266HTTPClient.h>
#include "FS.h"
#include <ArduinoOTA.h>

const char* ssid     = "net-xxx";
const char* password = "xxxxxxx";

const char* host = "192.168.88.245";
const char* httpPort = "80";
const char* url = "/logdata";
String fullUrl = "";
WiFiClient client;

//software serial: GPIO5 as RX, GPIO2 as TX
EspSoftwareSerial receiver(D5,D2); // https://github.com/plerup/espsoftwareserial

String dataCh = "";
String dataWh = "";

bool dataAwaitsTransmission = false;
bool dataAwaitsBackup = false;

void prepareOTA(){
	ArduinoOTA.onStart([]() {
    		Serial.println("OTA Start");
	});
	ArduinoOTA.onEnd([]() {
		Serial.println("OTA End");
		Serial.println("Rebooting...");
	});
  	ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    		Serial.printf("Progress: %u%%\r\n", (progress / (total / 100)));
  	});
	ArduinoOTA.onError([](ota_error_t error) {
		Serial.printf("Error[%u]: ", error);
		if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
		else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
		else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
		else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
		else if (error == OTA_END_ERROR) Serial.println("End Failed");
	});
	ArduinoOTA.begin();
}

void setup()
{
	Serial.begin(115200);
	Serial.println("hello");

	SPIFFS.begin();

	WiFi.begin(ssid, password);

	while (WiFi.status() != WL_CONNECTED) {
	    delay(50);
	    Serial.print(".");
	}

	prepareOTA();

	fullUrl = String("http://");
	fullUrl.concat(host);
	fullUrl.concat(':');
	fullUrl.concat(httpPort);
	fullUrl.concat('/');
	fullUrl.concat(url);
}

int dataReadPeriodMs = 50;
unsigned long old_time_dataRead = 0;

void receiveData(){
	if (Serial.available() > 0){
		// read buffer, choose the last message, clean the buffer
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
		//save data to SPIFFS
		File log = SPIFFS.open("/log.txt", "w");
		if (!log) {
			//Serial.println("file open failed");
		} else {
			log.print("Ch=");
			log.print(dataCh);
			log.print(',');
			log.print("Wh=");
			log.println(dataWh);
		}
		log.close();

		dataAwaitsBackup = false;
	}
}

int transmitDataPeriodMs = 500;
unsigned long old_time_transmitData = 0;
void transmitData(){
	if (dataAwaitsTransmission){
		//transmit data over the web
		HTTPClient http;    //Declare object of class HTTPClient

		http.begin(fullUrl);      //Specify request destination
		http.addHeader("Content-Type", "application/json");  //Specify content-type header

		int httpCode = http.POST("{Ch:"+dataCh+",Wh:"+dataWh+"}");   //Send the request
		String payload = http.getString();                  //Get the response payload

		//Serial.println(httpCode);   //Print HTTP return code
		//Serial.println(payload);    //Print request response payload

		http.end();  //Close connection

		dataAwaitsTransmission = false;

	}
}

void checkTime(int period, unsigned long& old_time, void callback(void), void* args){
	if (millis() - old_time > period){
		callback();
		old_time = millis();
	}
}

// The loop function is called in an endless loop
void loop()
{
	ArduinoOTA.handle();

	checkTime(dataReadPeriodMs, old_time_dataRead, receiveData, NULL);
	checkTime(saveDataPeriodMs, old_time_saveData, saveData, NULL);
	checkTime(transmitDataPeriodMs, old_time_transmitData, saveData, NULL);
}
