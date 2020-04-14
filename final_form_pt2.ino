#include <ArduinoJson.h>
#include <Losant.h>
#include <PubSubClient.h>
#include <WiFi101.h>
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_Sensor.h>
#include "Adafruit_BME680.h"

//WiFi credentials
const char* WIFI_SSID = "";
const char* WIFI_PASS = "";

//Losant credentials
const char* LOSANT_DEVICE_ID = "5e8ca146cf671000062b5d4f";
const char* LOSANT_ACCESS_KEY = "ed90692b-67e4-46a7-9287-523842cccbf7";
const char* LOSANT_ACCESS_SECRET = "49eec7e12daf2892eeb685e9b0fad145fa8e1e362b7eaac52fdee29559093f10";

//instance of the wificlient
WiFiSSLClient wifiClient;

//instance of Losant device
LosantDevice device(LOSANT_DEVICE_ID);

#define BME_SCK 13
#define BME_MISO 12
#define BME_MOSI 11
#define BME_CS 10
#define SEALEVELPRESSURE_HPA (1013.25)

Adafruit_BME680 bme;

//connect to WiFi. Taken from Losant getting started code.
void connect(){
  Serial.print("Connecting to ");
  Serial.println(WIFI_SSID);

  WiFi.begin(WIFI_SSID, WIFI_PASS);

  while(WiFi.status() != WL_CONNECTED){
    delay(100);//wait a second
    Serial.print(".");
  }

  Serial.println("WiFi Connected");

  //should be connected to WiFi at this point. Proceed to Losant Connection. Also taken from Losant getting started code.
  Serial.println();
  Serial.println("Connecting to Losant...");
  device.connectSecure(wifiClient, LOSANT_ACCESS_KEY, LOSANT_ACCESS_SECRET);

  while(!device.connected()){
    delay(500);
    Serial.print(".");
  }
  Serial.println("Connected to Losant!"); //connected to Losant and WiFi
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  while(!Serial){
    }

if (!bme.begin()){
  Serial.println("Check BME wiring, please!");
  while(1); //infinite loop because BME isn't valid
}

// Set up oversampling and filter initialization. Taken from BMEtest Example code.
  bme.setTemperatureOversampling(BME680_OS_8X);
  bme.setHumidityOversampling(BME680_OS_2X);
  bme.setPressureOversampling(BME680_OS_4X);
  bme.setIIRFilterSize(BME680_FILTER_SIZE_3);
  bme.setGasHeater(320, 150); // 320*C for 150 ms

  connect();
}

//must make into JSON to pass to Losant for display.
void bmeData(double bmeTemp, double bmePress, double bmeHumidity){
  StaticJsonBuffer<200> jsonBuffer;
  //this has probably changed with a newer version of Json. Using JSON 5.
  JsonObject &root = jsonBuffer.createObject();
  //names must match attributes on Losant device settings.
  root["tempC"] = bmeTemp;
  root["humidity"] = bmeHumidity;
  root["pressure"] = bmePress;
  delay(1000);//wait a second to not violate message throughput.
  device.sendState(root);
}

void loop() {
  // put your main code here, to run repeatedly:
  bool toReconnect =false;

  //check if still connected to WiFi
  if(WiFi.status() != WL_CONNECTED){
    Serial.println("Disconnected from WiFi");
    toReconnect = true;
  }

  //check if still connected to Losant
  if(!device.connected()){
    Serial.println("Disconnected from Losant :(");
    toReconnect = true;
  }
  //if either disconnect, rerun the connecting series
  if(toReconnect){
    connect();
  }

  if(!bme.performReading()){
    Serial.println("Failed to perform reading");
    return;
  }
  double bmeTemp = bme.temperature;
  double bmePress = bme.pressure/100.0;//changes to hPa. Pa is too large for easy display.
  double bmeHumidity = bme.humidity;
  bmeData(bmeTemp, bmePress, bmeHumidity);//passes to JSON method

  //Performs necessary communication between the device and Losant
  device.loop();
}

/*
 * Access Key: ed90692b-67e4-46a7-9287-523842cccbf7
 * Access Secret: 49eec7e12daf2892eeb685e9b0fad145fa8e1e362b7eaac52fdee29559093f10
 */
