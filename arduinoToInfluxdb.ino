#include <InfluxDbClient.h>
#include <DateTime.h>
#include <ESPDateTime.h>
#include <TimeElapsed.h>
#include <DHTesp.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <DHT.h>
#include <Wire.h>
#include "SSD1306Wire.h"
#include "OLEDDisplayUi.h"

unsigned long lastMs = 0;
unsigned long ms = millis();

// InfluxDB server url, e.g. http://192.168.1.48:8086 (don't use localhost, always server name or ip address)
#define INFLUXDB_URL "http://192.168.1.12:8086"
// InfluxDB database name
#define INFLUXDB_DB_NAME "ATMO"
#define INFLUXDB_USER "EDWARD" 
#define INFLUXDB_PASSWORD "********"
#define DEVICE_ID "myroomtemp"
#define DHTTYPE DHT11   // DHT 11
#ifndef WIFI_SSID
#define WIFI_SSID "NETGEAR25-5G-2_2.4GEXT"
#define WIFI_PASS "************"
#endif
// Setup
const int UPDATE_INTERVAL_SECS = 20 * 60; // Update every 20 minutes
// Display Settings
const int I2C_DISPLAY_ADDRESS = 0x3c;
#if defined(ESP8266)
//const int SDA_PIN = D1;
//const int SDC_PIN = D2;
const int SDA_PIN = D3;
const int SDC_PIN = D4;
#else
//const int SDA_PIN = GPIO5;
//const int SDC_PIN = GPIO4 
const int SDA_PIN = GPIO0;
const int SDC_PIN = GPIO2 
#endif
// Initialize the oled display for address 0x3c
SSD1306Wire     display(I2C_DISPLAY_ADDRESS, SDA_PIN, SDC_PIN);
OLEDDisplayUi   ui( &display );
#define pin 14   
void readTemperatureHumidity();
int temp = 0; //temperature
int humi = 0; //humidity
long readTime = 0; 

// InfluxDB client instance for InfluxDB 1
InfluxDBClient client(INFLUXDB_URL, INFLUXDB_DB_NAME);

ESP8266WebServer server(80);

// DHT Sensor
uint8_t DHTPin = D5;                
// Initialize DHT sensor.
DHT dht(DHTPin, DHTTYPE);                

float Temperature;
float Humidity;
char temperature[] = "00.0 C";
char humidity[]    = "00.0 %";

void handle_OnConnect() {
  
  Temperature = Fahrenheit(dht.readTemperature()); // Gets the values of the temperature
  Humidity = dht.readHumidity(); // Gets the values of the humidity 
  server.send(200, "text/html", SendHTML(Temperature,Humidity)); 
}

void handle_NotFound(){
  server.send(404, "text/plain", "Not found");
}

String SendHTML(float Temperaturestat,float Humiditystat){
  String ptr = "<!DOCTYPE html> <html>\n";
  ptr +="<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0, user-scalable=no\">\n";
  ptr +="<title>ESP8266 Weather Report</title>\n";
  ptr +="<link rel=\"stylesheet\" href=\"https://maxcdn.bootstrapcdn.com/bootstrap/3.4.1/css/bootstrap.min.css\">\n";
  ptr +="<script src=\"https://ajax.googleapis.com/ajax/libs/jquery/3.5.1/jquery.min.js\"></script>\n";
  ptr +="<script src=\"https://maxcdn.bootstrapcdn.com/bootstrap/3.4.1/js/bootstrap.min.js\"></script>\n";
  ptr +="";
  ptr +="</head>\n";
  ptr +="<body>\n";
  ptr +="<div id=\"webpage\">\n";
  ptr +="<h1>ESP8266 NodeMCU Weather Report</h1>\n";
  
  ptr +="<p>Temperature: ";
  ptr +=(float)Temperaturestat;
  ptr +=" F</p>";
  
  ptr +="<p>Humidity: ";
  ptr +=(int)Humiditystat;
  ptr +="%</p>";

  ptr +="<p>Time: ";
  ptr +=DateTime.toString().c_str();
  ptr +="</p>";
  
  
  ptr +="</div>\n";
  ptr +="</body>\n";
  ptr +="</html>\n";
  return ptr;
}

void setupWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.println(millis());
  Serial.print("WiFi Connecting");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
}

void setupDateTime() {
  // setup this after wifi connected
  // you can use custom timeZone,server and timeout
  // DateTime.setTimeZone(-4);
  //   DateTime.setServer("asia.pool.ntp.org");
  //   DateTime.begin(15 * 1000);
  DateTime.setTimeZone(-10);
  DateTime.begin();
}

void showSerialData(){

  // Read humidity
  byte RH = dht.readHumidity();
  //Read temperature in degree Celsius
  float Temp = dht.readTemperature();
  
  Serial.print("Temp = ");
  Serial.print(Fahrenheit(Temp));
  Serial.print("F");
  Serial.print(" | Temp C = ");
  Serial.print(Temp);
  Serial.print("C");
  Serial.print(" | Humidity = ");
  Serial.print(String(RH));
  Serial.print("%\n");
  Serial.println("-----------------");
}

void showDisplayData(){
  // Read humidity
  byte RH = dht.readHumidity();
  //Read temperature in degree Celsius
  float Temp = dht.readTemperature();
  
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.setFont(ArialMT_Plain_10);
  display.drawString(5, 10, String(Fahrenheit(Temp)));
  display.drawString(40, 10, "F");
  display.drawString(5, 20, String(RH));
  display.drawString(20, 20, "%");
  display.drawString(5,30, DateTime.toString().c_str());
}

void showTimeData(){
  if (millis() - ms > 5000) {
      ms = millis();
      Serial.println("--------------------");
      if (!DateTime.isTimeValid()) {
        Serial.println("Failed to get time from server, retry.");
        DateTime.begin();
      } else {
     
        Serial.printf("Up     Time:   %lu seconds\n", millis() / 1000);
        Serial.printf("Local  Time:   %ld\n", DateTime.now());
        Serial.printf("Local  Time:   %s\n", DateTime.toString().c_str());
     }
  }
}

float Fahrenheit(float celsius){
  return 1.8 * celsius + 32;
}


Point pointDevice("mymeasurement"); // create a new measurement point (the same point can be used for Cloud and v1 InfluxDB)

void setup() {
  Serial.begin(115200);
  delay(100);

  Wire.begin(0,2);
  
   // initialize dispaly
  display.init();
  display.clear();
  display.display();
  display.setFont(ArialMT_Plain_10);
  ui.setTargetFPS(60);
  pinMode(DHTPin, INPUT);
  
  dht.begin();     
  
  setupWiFi();
  
  setupDateTime();
  
  DateTimeParts p = DateTime.getParts();
  time_t t = DateTime.now();

  
  server.on("/", handle_OnConnect);
  server.onNotFound(handle_NotFound);
  server.begin();
  Serial.println("HTTP server started");
  Serial.println(WiFi.localIP());

  // Set InfluxDB 1 authentication params
  client.setConnectionParamsV1(INFLUXDB_URL, INFLUXDB_DB_NAME, INFLUXDB_USER, INFLUXDB_PASSWORD);
  // Check server connection
  if (client.validateConnection()) {
    Serial.print("Connected to InfluxDB: ");
    Serial.println(client.getServerUrl());
  } else {
    Serial.print("InfluxDB connection failed: ");
    Serial.println(client.getLastErrorMessage());
  }

 
  display.display();
   
}


void loop() {
  server.handleClient();
  
  
  showTimeData();
   
  showSerialData();

  // Read humidity
  byte RH = dht.readHumidity();
  //Read temperature in degree Celsius
  float Temp = dht.readTemperature();

  
  // add tags to the datapoints so you can filter them
  pointDevice.addTag("device", DEVICE_ID);
  pointDevice.addTag("SSID", WiFi.SSID());
  // Add data fields (values)
  pointDevice.addField("temp", Temp);
  pointDevice.addField("humidity", RH);
  pointDevice.addField("time", DateTime.now()); // in addition send the uptime of the Arduino
  
  Serial.print("written to InfluxDB Cloud: ");
  Serial.println(client.writePoint(pointDevice)); // returns true if success, false otherwise

  client.setConnectionParamsV1(INFLUXDB_URL, INFLUXDB_DB_NAME, INFLUXDB_USER, INFLUXDB_PASSWORD);
  Serial.print("written to local InfluxDB instance: ");
  Serial.println(client.writePoint(pointDevice)); // returns true if success, false otherwise

  
  display.clear();
  showDisplayData();
  display.display();
  delay(20000);
  
}








