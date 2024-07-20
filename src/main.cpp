// --------------------------------------------------
//
// Code setting up a simple webserver on the ESP32. 
// device used for tests: ESP32-WROOM-32D
//
// Written by mo thunderz (last update: 27.08.2022)
//
// --------------------------------------------------

#include <WiFi.h>
#include <WebServer.h>                                // Make sure tools -> board is set to ESP32, otherwise you will get a "WebServer.h: No such file or directory" error
#include <WiFiClientSecure.h>
#include <NTPClient.h>

// SSID and password of Wifi connection:
const char* ssid = "HonzaNiki";
const char* password = "42AuGs4tUnpG";

String website1 = "<!DOCTYPE html><html><head> <meta name='viewport' content='width=device-width, initial-scale=1'> <meta charset='UTF-8'> <link rel='icon' href='data:,'> <style> html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center; } .button { background-color: #4CAF50; border: none; color: white; padding: 16px 40px; text-decoration: none; font-size: 30px; margin: 2px; cursor: pointer; } .button2 { background-color: #939599; border: none; color: white; padding: 16px 40px; } </style></head><body> <h1>Naše zahrádka</h1> <svg width='300' height='200' xmlns='http://www.w3.org/2000/svg'> <!-- Second rectangle with text above it --> <rect width='30' height='30' x='135' y='165' rx='5' ry='5' style='fill:red; stroke:black; stroke-width:2; opacity:0.8' /> <!-- Text above the second rectangle --> <text x='150' y='160' font-family='Helvetica' font-size='15' fill='black' text-anchor='middle'>Sensor</text> <!-- Static time '08:10:55' with 'Actual time' string --> <text x='150' y='10' font-family='Helvetica' font-size='15' fill='black' font-weight= 'bold' text-anchor='middle'>Actual time</text> <text x='150' y='30' font-family='Helvetica' font-size='15' fill='black' text-anchor='middle'>";
String website2 = "</text> <!-- Static time '10:00:00' with 'Next watering' string --> <text x='150' y='50' font-family='Helvetica' font-size='15' fill='black' font-weight= 'bold' text-anchor='middle'>Next watering</text> <text x='150' y='70' font-family='Helvetica' font-size='15' fill='black' text-anchor='middle'>10:00:00</text> </svg> <p>Čerpadlo - běží</p> <p><a href='/26/on'><button class='button'>ON</button></a></p></body></html>";
String website = "";

String formatedTime = "";
int interval = 5000; // virtual delay
unsigned long previousMillis = 0; // Tracks the time since last event fired

WebServer server(80);

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, 7200);

void setup() {
  Serial.begin(115200);                 
 
  WiFi.begin(ssid, password);
  Serial.println("Establishing connection to WiFi with SSID: " + String(ssid));
 
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.print("Connected to network with IP address: ");
  Serial.println(WiFi.localIP());
  
  website = website1 + formatedTime + website2;
  server.on("/", []() {
  server.send(200, "text/html", website);
  });
  server.begin(); // init the server
  timeClient.begin();
}

void loop() {
  server.handleClient();  // webserver methode that handles all Client

  unsigned long currentMillis = millis(); // call millis  and Get snapshot of time
  if ((unsigned long)(currentMillis - previousMillis) >= interval) { // How much time has passed, accounting for rollover with subtraction!
    formatedTime = timeClient.getFormattedTime();
    website = website1 + formatedTime + website2;
  }
  timeClient.update();
}