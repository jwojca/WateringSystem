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
#include <ArduinoJson.h>

// SSID and password of Wifi connection:
const char* ssid = "HonzaNiki";
const char* password = "42AuGs4tUnpG";

bool pumpManCmd = false;
String pumpStateStr = "";

struct timeSetStruct
{
  int hourSet = 5;
  int minuteSet = 0;
  int duration = 15;
};

timeSetStruct timeSet;

String website = "<!DOCTYPE html><html><head> <meta name='viewport' content='width=device-width, initial-scale=1'> <meta charset='UTF-8'> <link rel='icon' href='data:,'> <style> html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center; } .buttonOn { background-color: #4CAF50; border: none; color: white; padding: 16px 40px; text-decoration: none; font-size: 30px; margin: 2px; cursor: pointer; } .buttonOff { background-color: #939599; border: none; color: white; padding: 16px 40px; text-decoration: none; font-size: 30px; margin: 2px; cursor: pointer; } .timeInputClass { font-size: 15px; padding: 10px; border: 1px solid; border-radius: 5px; } </style></head><body> <h1>Naše zahrádka</h1> <p><b>Aktuální čas</b><br> <span id='actTime'>-</span></p> <p><b>Příští zalévání</b><br> <span id='timeToWater'>-</span><br> <span id='timeWatDuration'>-</span></p> <!-- <text x='150' y='10' font-family='Helvetica' font-size='15' fill='black' font-weight= 'bold' text-anchor='middle'>Actual time</text> <text x='150' y='30' font-family='Helvetica' font-size='15' fill='black' text-anchor='middle'>09:00:00</text> <text x='150' y='50' font-family='Helvetica' font-size='15' fill='black' font-weight= 'bold' text-anchor='middle'>Next watering</text> <text x='150' y='70' font-family='Helvetica' font-size='15' fill='black' text-anchor='middle'>10:00:00</text> <text x='150' y='15' font-family='Helvetica' font-size='15' fill='black' text-anchor='middle'>Sensor</text> --> <svg width='300' height='60' xmlns='http://www.w3.org/2000/svg'> <text x='150' y='15' fill='black' text-anchor='middle'>Sensor</text> <rect width='30' height='30' x='135' y='20' rx='5' ry='5' style='fill:red; stroke:black; stroke-width:2; opacity:0.8' /> </svg> <p><span id='pumpState'>Čerpadlo neběží<span></p> <p><button class='buttonOn' onclick='changeBtnClass()' id='buttonPumpCmd'>ON</button></p> <p> <label for='timeInput'>Čas zalevání: </label> <input type='time' id='timeInput' class='timeInputClass'/> </p> <p> <label for='timeDuration'>Délka zalevání: </label> <input type='time' id='timeDuration' class='timeInputClass' step='2' value='00:00:00' min='00:00:01' max='00:00:59' /> <label for='timeDuration'>s</label> </p> <p><button onclick='sentTime()' id='buttonSendTime'>Uložit</button></p></body><script> var Socket; document.getElementById('buttonPumpCmd').addEventListener('click', buttonPumpCmd); function init() { Socket = new WebSocket('ws://' + window.location.hostname + ':81/'); Socket.onmessage = function(event) { processCommand(event); }; } function buttonPumpCmd() { var msg = { pumpCmd: 'changeState' }; Socket.send(JSON.stringify(msg)); } function changeBtnClass() { if(document.getElementById('buttonPumpCmd').innerHTML == 'ON') { document.getElementById('buttonPumpCmd').className = 'buttonOff'; document.getElementById('buttonPumpCmd').innerHTML = 'OFF'; document.getElementById('pumpState').innerHTML = 'Čerpadlo běží'; } else { document.getElementById('buttonPumpCmd').className = 'buttonOn'; document.getElementById('buttonPumpCmd').innerHTML = 'ON'; document.getElementById('pumpState').innerHTML = 'Čerpadlo neběží'; } } function sentTime() { var msg = { timeInputValue: document.getElementById('timeInput').value, timeDurationValue: document.getElementById('timeDuration').value }; Socket.send(JSON.stringify(msg)); } function processCommand(event) { var obj = JSON.parse(event.data); document.getElementById('actTime').innerHTML = obj.actTime; document.getElementById('timeToWater').innerHTML = obj.timeToWater; document.getElementById('timeWatDuration').innerHTML = obj.timeWatDuration; console.log(obj.actTime); console.log(obj.timeToWater); } window.onload = function(event) { init(); } </script></html>";

int interval = 1000; // virtual delay
unsigned long previousMillis = 0; // Tracks the time since last event fired

// Assign output variables to GPIO pins
const int outputPump = 17;
const int inputSensor = 16;

WebServer server(80);
WebSocketsServer webSocket = WebSocketsServer(81);

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, 7200);

// Static IP configuration
IPAddress staticIP(10, 0, 1, 100); // ESP32 static IP
IPAddress gateway(10, 0, 1, 138);    // IP Address of your network gateway (router)
IPAddress subnet(255, 255, 255, 0);   // Subnet mask
IPAddress primaryDNS(10, 0, 1, 138); // Primary DNS (optional)
IPAddress secondaryDNS(0, 0, 0, 0);   // Secondary DNS (optional)

void webSocketEvent(byte num, WStype_t type, uint8_t * payload, size_t length);
void setWaterTime(char* timeInput, char* timeDuration, timeSetStruct &timeSet);

void setup() {
  Serial.begin(115200);                

  // Initialize inputs and outputs
  pinMode(outputPump, OUTPUT);
  //pinMode(inputSensor, INPUT);
   
  // Set outputs
  digitalWrite(outputPump, HIGH); 
 
  WiFi.begin(ssid, password);
  Serial.println("Establishing connection to WiFi with SSID: " + String(ssid));
 
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  // Configuring static IP
  if(!WiFi.config(staticIP, gateway, subnet, primaryDNS, secondaryDNS)) {
    Serial.println("Failed to configure Static IP");
  } else {
    Serial.println("Static IP configured!");
  }

  Serial.print("Current ESP32 IP: ");
  Serial.println(WiFi.localIP());
  Serial.print("Gateway (router) IP: ");
  Serial.println(WiFi.gatewayIP());
  Serial.print("Subnet Mask: " );
  Serial.println(WiFi.subnetMask());
  Serial.print("Primary DNS: ");
  Serial.println(WiFi.dnsIP(0));
  Serial.print("Secondary DNS: ");
  Serial.println(WiFi.dnsIP(1));
  
 
  server.on("/", []() {
  server.send(200, "text/html", website);
  });
  server.begin(); // init the server
  timeClient.begin();
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);
}

void loop() 
{
  server.handleClient();  // webserver methode that handles all Client
  webSocket.loop();       // Update function for the webSockets 

  int day = timeClient.getDay();
  int hours = timeClient.getHours();
  int minutes = timeClient.getMinutes();
  int seconds = timeClient.getSeconds();
  
  //Activate wattering in certain day and time or in manual operation
  bool timeToWater = ((day % 2) == 0) && (hours == timeSet.hourSet) && (minutes == timeSet.minuteSet) && (seconds >= 0 && seconds <= timeSet.duration);
   
  if(pumpManCmd || timeToWater)
     digitalWrite(outputPump, LOW);
  else
    digitalWrite(outputPump, HIGH);

  unsigned long now = millis(); // call millis  and Get snapshot of time
  if ((unsigned long)(now - previousMillis) >= interval)  // How much time has passed, accounting for rollover with subtraction!
  {
    String jsonString = "";                           // create a JSON string for sending data to the client
    StaticJsonDocument<200> doc;                      // create a JSON container
    JsonObject object = doc.to<JsonObject>();         // create a JSON Object
    object["actTime"] = timeClient.getFormattedTime();  // write data into the JSON object -> I used "rand1" and "rand2" here, but you can use anything else

    char timeSetStr[9];
    sprintf(timeSetStr, "%02d:%02d", timeSet.hourSet, timeSet.minuteSet);
    object["timeToWater"] = ((day % 2) == 0) ? "Dnes v: " : "Zítra v: " + String(timeSetStr);
    Serial.println(timeSet.duration);
    object["timeWatDuration"] = "Doba zalévání: " + String(timeSet.duration) + " s";
    
    serializeJson(doc, jsonString);                   // convert JSON object to string
    //Serial.println(jsonString);                       // print JSON string to console for debug purposes (you can comment this out)
    webSocket.broadcastTXT(jsonString);               // send JSON string to clients

    
    /*
    String str = timeClient.getFormattedTime();
    int str_len = str.length() + 1;                   
    char char_array[str_len];
    str.toCharArray(char_array, str_len);             // convert to char array
    webSocket.broadcastTXT(char_array);               // send char_array to clients
    */
    previousMillis = now;                             // reset previousMillis 
  }

  timeClient.update();
}

void setWaterTime(const char* timeInput, const char* timeDuration, timeSetStruct &timeSet)
{
  int hours, minutes;

  // Parse the time string
  sscanf(timeInput, "%d:%d", &hours, &minutes);
  /*
  // Print the extracted values
  printf("Hours: %d\n", hours);
  printf("Minutes: %d\n", minutes);
  */
 timeSet.hourSet = hours;
 timeSet.minuteSet = minutes;

 int duration;
 int tmp1, tmp2;
 // Parse the duration string
 sscanf(timeDuration, "%d:%d:%d", &tmp1, &tmp2, &duration);
 timeSet.duration = duration;
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
      // try to decipher the JSON string received
      StaticJsonDocument<200> doc;                    // create a JSON container
      DeserializationError error = deserializeJson(doc, payload);
      if (error) 
      {
        Serial.print(F("deserializeJson() failed: "));
        Serial.println(error.f_str());
        return;
      }
      else 
      {
        // JSON string was received correctly, so information can be retrieved:
        //Change pump state when button pressed
        const char* g_pumpCmd = doc["pumpCmd"];
        const char* g_timeInput = doc["timeInputValue"];
        const char* g_timeDuration = doc["timeDurationValue"];
        
        if(String(g_pumpCmd).equals("changeState"))
          pumpManCmd = !pumpManCmd;
        Serial.println("Command: " + String(pumpManCmd));
        Serial.println("Time input: " + String(g_timeInput));
        Serial.println("Time duration: " + String(g_timeDuration));

        //time set struct
  
        setWaterTime(g_timeInput, g_timeDuration, timeSet);
      }
      Serial.println("");
      break;
  }
}