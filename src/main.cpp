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
#include <WebSocketsServer.h>

// SSID and password of Wifi connection:
const char* ssid = "HonzaNiki";
const char* password = "42AuGs4tUnpG";

String website = "<!DOCTYPE html><html><head> <meta name='viewport' content='width=device-width, initial-scale=1'> <meta charset='UTF-8'> <link rel='icon' href='data:,'> <style> html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center; } .button { background-color: #4CAF50; border: none; color: white; padding: 16px 40px; text-decoration: none; font-size: 30px; margin: 2px; cursor: pointer; } .button2 { background-color: #939599; border: none; color: white; padding: 16px 40px; } </style></head><body> <h1>Naše zahrádka</h1> <p><b>Aktuální čas</b><br> <span id='actTime'>-</span></p> <p><b>Příští zalévání</b><br> 09:00:00</p> <!-- <text x='150' y='10' font-family='Helvetica' font-size='15' fill='black' font-weight= 'bold' text-anchor='middle'>Actual time</text> <text x='150' y='30' font-family='Helvetica' font-size='15' fill='black' text-anchor='middle'>09:00:00</text> <text x='150' y='50' font-family='Helvetica' font-size='15' fill='black' font-weight= 'bold' text-anchor='middle'>Next watering</text> <text x='150' y='70' font-family='Helvetica' font-size='15' fill='black' text-anchor='middle'>10:00:00</text> <text x='150' y='15' font-family='Helvetica' font-size='15' fill='black' text-anchor='middle'>Sensor</text> --> <svg width='300' height='60' xmlns='http://www.w3.org/2000/svg'> <text x='150' y='15' fill='black' text-anchor='middle'>Sensor</text> <rect width='30' height='30' x='135' y='20' rx='5' ry='5' style='fill:red; stroke:black; stroke-width:2; opacity:0.8' /> </svg> <p>Čerpadlo - běží</p> <p><button class='button', id='buttonPumpCmd'>ON</button></p></body><script> var Socket; document.getElementById('buttonPumpCmd').addEventListener('click', buttonPumpCmd); function init() { Socket = new WebSocket('ws://' + window.location.hostname + ':81/'); Socket.onmessage = function(event) { processCommand(event); }; } function buttonPumpCmd() { Socket.send('Sending back some random stuff'); } function processCommand(event) { document.getElementById('actTime').innerHTML = event.data; console.log(event.data); } window.onload = function(event) { init(); }</script></html>";

int interval = 1000; // virtual delay
unsigned long previousMillis = 0; // Tracks the time since last event fired

WebServer server(80);
WebSocketsServer webSocket = WebSocketsServer(81);

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, 7200);

void webSocketEvent(byte num, WStype_t type, uint8_t * payload, size_t length);

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
  
 
  server.on("/", []() {
  server.send(200, "text/html", website);
  });
  server.begin(); // init the server
  timeClient.begin();
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);
}

void loop() {
  server.handleClient();  // webserver methode that handles all Client
  webSocket.loop();       // Update function for the webSockets 

  unsigned long now = millis(); // call millis  and Get snapshot of time
  if ((unsigned long)(now - previousMillis) >= interval) { // How much time has passed, accounting for rollover with subtraction!
    String str = timeClient.getFormattedTime();
    int str_len = str.length() + 1;                   
    char char_array[str_len];
    str.toCharArray(char_array, str_len);             // convert to char array
    webSocket.broadcastTXT(char_array);               // send char_array to clients
    previousMillis = now;                             // reset previousMillis 
  }
  timeClient.update();
}

void webSocketEvent(byte num, WStype_t type, uint8_t * payload, size_t length) {      // the parameters of this callback function are always the same -> num: id of the client who send the event, type: type of message, payload: actual data sent and length: length of payload
  switch (type) {                                     // switch on the type of information sent
    case WStype_DISCONNECTED:                         // if a client is disconnected, then type == WStype_DISCONNECTED
      Serial.println("Client " + String(num) + " disconnected");
      break;
    case WStype_CONNECTED:                            // if a client is connected, then type == WStype_CONNECTED
      Serial.println("Client " + String(num) + " connected");
      // optionally you can add code here what to do when connected
      break;
    case WStype_TEXT:                                 // if a client has sent data, then type == WStype_TEXT
      for (int i=0; i<length; i++) {                  // print received data from client
        Serial.print((char)payload[i]);
      }
      Serial.println("");
      break;
  }
}