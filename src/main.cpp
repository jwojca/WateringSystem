/*********
  Rui Santos
  Complete project details at http://randomnerdtutorials.com  
*********/

// Load Wi-Fi library
#include <NTPClient.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <WebSocketsServer.h>

// Replace with your network credentials
const char* ssid = "HonzaNiki_Ext";
const char* password = "42AuGs4tUnpG";
String formatedTime;

//NTP
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);

// Set web server port number to 80
WiFiServer server(80);

//Set web socket server
WebSocketsServer WebSocket = WebSocketsServer(81);

// Variable to store the HTTP request
String header;


// Auxiliar variables to store the current output state
String outputPumpState = "off";

// Assign output variables to GPIO pins
const int outputPump = 17;
const int inputSensor = 16;

// Current time
unsigned long actTimeTOut = millis();
unsigned long actTimeRefr = millis();
// Previous time
unsigned long prevTimeTOut = 0; 
unsigned long prevTimeRefr = 0; 

// Define timeout time in milliseconds (example: 2000ms = 2s)
const long timeoutTime = 2000;
const long refreshTime = 1000;

bool refreshClient = false;
bool wateringActive = false;

void setup() {
  Serial.begin(115200);

  // Connect to Wi-Fi network with SSID and password
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  // Print local IP address and start web server
  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  server.begin();
  timeClient.begin();

  // Initialize inputs and outputs
  pinMode(outputPump, OUTPUT);
  //pinMode(inputSensor, INPUT);
   
  // Set outputs
  digitalWrite(outputPump, HIGH);
}

void loop(){
  WiFiClient client = server.available();   // Listen for incoming clients

  // If a new client connects,
  if (client) 
  {                             
    actTimeTOut = millis();
    prevTimeTOut = actTimeTOut;

    bool sensorTop = digitalRead(inputSensor);

    Serial.println("New Client.");          // print a message out in the serial port
    String currentLine = "";                // make a String to hold incoming data from the client
    while (client.connected() && actTimeTOut - prevTimeTOut <= timeoutTime) {  // loop while the client's connected
      actTimeTOut = millis();
      if (client.available()) {             // if there's bytes to read from the client,
        char c = client.read();             // read a byte, then
        Serial.write(c);                    // print it out the serial monitor
        header += c;
        if (c == '\n') {                    // if the byte is a newline character
        
          // if the current line is blank, you got two newline characters in a row.
          // that's the end of the client HTTP request, so send a response:
          if (currentLine.length() == 0) {
            // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
            // and a content-type so the client knows what's coming, then a blank line:
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println("Connection: close");
            client.println();
            
            // turns the GPIOs on and off
            if (header.indexOf("GET /26/on") >= 0) {
              Serial.println("GPIO 26 on");
              outputPumpState = "on";
              digitalWrite(outputPump, LOW);
              //wateringActive = true;
            } else if (header.indexOf("GET /26/off") >= 0) {
              Serial.println("GPIO 26 off");
              outputPumpState = "off";
              //wateringActive = false;
              digitalWrite(outputPump, HIGH);
            } 
            
            /*
            // Display the HTML web page
            client.println("<!DOCTYPE html><html>");
            client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
            client.println("<meta charset=\"UTF-8\">");
            client.println("<link rel=\"icon\" href=\"data:,\">");
            // CSS to style the on/off buttons 
            // Feel free to change the background-color and font-size attributes to fit your preferences
            client.println("<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}");
            client.println(".button { background-color: #4CAF50; border: none; color: white; padding: 16px 40px;");
            client.println("text-decoration: none; font-size: 30px; margin: 2px; cursor: pointer;}");
            client.println(".button2 {background-color: #939599; border: none; color: white; padding: 16px 40px;}</style></head>");
            
            // Web Page Heading
            client.println("<body><h1>Naše zahrádka</h1>");
            
            // Display current state, and ON/OFF buttons for GPIO 26  
        
            // If the outputPumpState is off, it displays the ON button       
            if (outputPumpState=="off") {
              client.println("<p><a href=\"/26/on\"><button class=\"button\">ON</button></a></p>");
            } else {
              client.println("<p><a href=\"/26/off\"><button class=\"button button2\">OFF</button></a></p>");
            } 
            
            client.println("</body></html>");
            */
            client.println("<!DOCTYPE html>");
            client.println("<html>");
            client.println("<head>");
            client.println("  <meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
            client.println("  <meta charset=\"UTF-8\">");
            client.println("  <link rel=\"icon\" href=\"data:,\">");
            client.println("  <style>");
            client.println("    html {");
            client.println("      font-family: Helvetica;");
            client.println("      display: inline-block;");
            client.println("      margin: 0px auto;");
            client.println("      text-align: center;");
            client.println("    }");
            client.println("");
            client.println("    .button {");
            client.println("      background-color: #4CAF50;");
            client.println("      border: none;");
            client.println("      color: white;");
            client.println("      padding: 16px 40px;");
            client.println("      text-decoration: none;");
            client.println("      font-size: 30px;");
            client.println("      margin: 2px;");
            client.println("      cursor: pointer;");
            client.println("    }");
            client.println("");
            client.println("    .button2 {");
            client.println("      background-color: #939599;");
            client.println("      border: none;");
            client.println("      color: white;");
            client.println("      padding: 16px 40px;");
            client.println("    }");
            client.println("  </style>");
            client.println("</head>");
            client.println("<body>");
            client.println("  <h1>Naše zahrádka</h1>");
            client.println("  <svg width=\"300\" height=\"200\" xmlns=\"http://www.w3.org/2000/svg\">");
            client.println("");
            client.println("    <!-- Second rectangle with text above it -->");
            client.println("    <rect width=\"30\" height=\"30\" x=\"135\" y=\"165\" rx=\"5\" ry=\"5\" style=\"fill:red; stroke:black; stroke-width:2; opacity:0.8\" />");
            client.println("    <!-- Text above the second rectangle -->");
            client.println("    <text x=\"150\" y=\"160\" font-family=\"Helvetica\" font-size=\"15\" fill=\"black\" text-anchor=\"middle\">Sensor</text>");
            client.println("    ");
            client.println("    <!-- Static time \"10:55:52\" with \"Actual time\" string -->");
            client.println("    <text x=\"150\" y=\"10\" font-family=\"Helvetica\" font-size=\"15\" fill=\"black\" font-weight= \"bold\" text-anchor=\"middle\">Actual time</text>");
            client.println("    <text x=\"150\" y=\"30\" font-family=\"Helvetica\" font-size=\"15\" fill=\"black\" text-anchor=\"middle\">" + formatedTime + "</text>");
            client.println("    <!-- Static time \"10:00:00\" with \"Next watering\" string -->");
            client.println("    <text x=\"150\" y=\"50\" font-family=\"Helvetica\" font-size=\"15\" fill=\"black\" font-weight= \"bold\" text-anchor=\"middle\">Next watering</text>");
            client.println("    <text x=\"150\" y=\"70\" font-family=\"Helvetica\" font-size=\"15\" fill=\"black\" text-anchor=\"middle\">10:00:00</text>");
            client.println("  </svg>");
            client.println("  ");

            // Display current state, and ON/OFF buttons for GPIO 26  
            // If the outputPumpState is off, it displays the ON button       
            if (outputPumpState=="off") 
            {
              client.println("  <p>Čerpadlo - neběží</p>");
              client.println("<p><a href=\"/26/on\"><button class=\"button\">ON</button></a></p>");
            } else 
            {
              client.println("  <p>Čerpadlo - běží</p>");
              client.println("<p><a href=\"/26/off\"><button class=\"button button2\">OFF</button></a></p>");
            } 
            client.println("");
            client.println("</body>");
            client.println("</html>");
            
            // The HTTP response ends with another blank line
            client.println();
            // Break out of the while loop
            break;
          } else { // if you got a newline, then clear currentLine
            currentLine = "";
          }
        } else if (c != '\r') {  // if you got anything else but a carriage return character,
          currentLine += c;      // add it to the end of the currentLine
        }
      }      
    }
    // Clear the header variable
    header = "";
    // Close the connection
    client.stop();
    Serial.println("Client disconnected.");
    Serial.println("");
  }

  refreshClient = false;
  //Actual time print
  actTimeRefr = millis();
  timeClient.update();
  if(actTimeRefr - prevTimeRefr >=refreshTime)
  {
    Serial.println(timeClient.getFormattedTime());
    formatedTime = timeClient.getFormattedTime();
    prevTimeRefr = actTimeRefr;
    refreshClient = true;
  }
  int hours = timeClient.getHours();
  int minutes = timeClient.getMinutes();
  int seconds = timeClient.getSeconds();

  //Activate wattering in certain UTC time
  if((hours == 5) && (minutes == 0) && (seconds >= 0 && seconds <= 15))
     digitalWrite(outputPump, LOW);
  else
    digitalWrite(outputPump, HIGH);


    
  /*
  wateringActive = true;
  
  if(wateringActive)
  {
    for(uint32_t i = 0; i < 2; ++i)
    {     
      digitalWrite(outputPump, LOW);
      delay(8000);
      digitalWrite(outputPump, HIGH);
      delay(30000);
    }
    wateringActive = false;
  }
  */

 
    
}