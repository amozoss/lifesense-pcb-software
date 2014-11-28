/*
   Adapted from: October 16, 2013 by Robert Wessels (http://energia.nu)
   Derived from example Sketch by Hans Scharler (http://www.iamshadowlord.com)

*/

#include <SPI.h>
#include <WiFi.h>


#define inputPin A4 
#define outPin1 A2
#define outPin2 A1
#define scale A0

const IPAddress INADDR_NONE(0,0,0,0);

// Prototypes
void printConfig();
void printIndex();

char ssid[] = "cc3000"; 
char pass[] = "coolestever"; 

uint8_t rx_buf[16];

// Settings
IPAddress myIp(192,168,150,4);
IPAddress hostIp(192,168,150,1);

// Variable Setup
long lastConnectionTime = 0; 
boolean lastConnected = false;
int failedCounter = 0;
int PORT = 4033;

// Initialize WiFi Client
WiFiServer server(80);
WiFiClient client;
int statusConfig = 0;
String currentLine = "";
String token = "";
void setup()
{
  // Start Serial for debugging on the Serial Monitor
  Serial.begin(115200);
  pinMode(inputPin, INPUT);  
  pinMode(outPin1, OUTPUT);  
  pinMode(outPin2, OUTPUT);  
  pinMode(RED_LED, OUTPUT);
  pinMode(GREEN_LED, OUTPUT);
  pinMode(BLUE_LED, OUTPUT);
  digitalWrite(BLUE_LED, LOW);
  digitalWrite(RED_LED, LOW);
  digitalWrite(GREEN_LED, LOW);

  // Set communication pins for CC3000
  WiFi.setCSpin(18);  // 18: P2_2 @ F5529, PE_0 @ LM4F/TM4C
  WiFi.setENpin(2);   //  2: P6_5 @ F5529, PB_5 @ LM4F/TM4C
  WiFi.setIRQpin(19); // 19: P2_0 @ F5529, PB_2 @ LM4F/TM4C

  // Start WiFi
  startWiFi();
  server.begin();
}

void loop()
{
  // if (!sentOnce) {
  pushToServer();
 // sentOnce = 1;
  //}
 // startServer();
}

void pushToServer() {
  // Read value from Scale Sensor
  String analogPin0 = String(analogRead(scale), DEC);
  // Read push button S1
  String digitalPin = String(digitalRead(inputPin), DEC);
  if (digitalRead(inputPin) ) {
    digitalWrite(outPin1, HIGH);   

    digitalWrite(outPin2, LOW);   
  }
  else {
    digitalWrite(outPin1, LOW);   

    digitalWrite(outPin2, HIGH); 
  }  

  if (!client.connected()) {
    Serial.println("connecting!");
    Serial.println();

    client.stop();
    if (client.connect(hostIp, PORT)) {
      Serial.println("Connected!");
      digitalWrite(RED_LED, LOW);

      failedCounter=0;
    }
    else {
      Serial.println("Connection failed.");
      digitalWrite(RED_LED, HIGH);
      digitalWrite(BLUE_LED, LOW);
      digitalWrite(GREEN_LED, LOW);

      failedCounter++;
      delay(500);
    }
  }

  // Update 
  if(client.connected()) {
    pushUpdate("{\"weight\":"+analogPin0 + ", \"sensor\":\"" + digitalPin + "\"}\n\n");
  }
  // Print Update Response to Serial Monitor
  //int abc = 0;
  
  while (client.available()) {
   
    char c = client.read();
    Serial.print(c);
  }
  // Check if WiFi needs to be restarted
  if (failedCounter > 3 ) {
    failedCounter = 0;
    Serial.println("Greater than 3");
    startWiFi();
  }
}



void pushUpdate(String tsData)
{
  //Serial.println();

  client.print("POST /scale HTTP/1.1\n");
  client.print("Host: localhost:8080\n");
  client.print("Accept: \*/\*\n");
  client.print("Content-Type: application/json\n");
  client.print("Content-Length: ");

  client.print(tsData.length());
  client.print("\n\n");
  client.print(tsData);

  //Serial.println("Updated!");
}

void startWiFi()
{
  digitalWrite(RED_LED, HIGH);

  WiFi.disconnect();
  client.stop();

  Serial.print("Connecting LaunchPad to SSID:");
  Serial.print(ssid);
  Serial.println();

  // Connect to network and obtain an IP address using DHCP

  if (WiFi.begin(ssid, pass) == 0) {
    Serial.println("Connected to WiFi!");
    digitalWrite(GREEN_LED, HIGH);
    digitalWrite(RED_LED, LOW);
    while ( WiFi.status() != WL_CONNECTED) {
      // print dots while we wait to connect
      blinkLed(GREEN_LED);
      Serial.print("*");
    }

    int failedCountMax = 15;
    int failedCount = 0;
    IPAddress ip = INADDR_NONE;
    while (ip == INADDR_NONE) {
      // print dots while we wait for an ip addresss
      ip = WiFi.localIP();
      delayForTime(1000);
      if (failedCount++ > failedCountMax) {
        startWiFi();   
        return;
      }
      Serial.print(".");
    }

    digitalWrite(BLUE_LED, HIGH);
    digitalWrite(GREEN_LED, HIGH);
    printWifiStatus();
    Serial.println();
  } else {
    Serial.println("LaunchPad connected to network using DHCP");
    Serial.println();
  }

  delay(1000);
}

void delayForTime(int time) {
  int toDelay = time;
  int beenDelayed = 0;
  while (beenDelayed < toDelay) {
    blinkLed(GREEN_LED);
    beenDelayed += 1000;
  }
}


void blinkLed(int LED) {
  digitalWrite(LED, HIGH);
  delay(500);
  digitalWrite(LED, LOW);
  delay(500);
}

void printWifiStatus() {
  // print the SSID of the network you're attached to:
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print your WiFi shield's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);
}

/*
 * Server code
 */
void startServer() {
  client = server.available();

  if (client) {                             // if you get a client,
    Serial.println("new client");           // print a message out the serial port
    String currentLine = "";                // make a String to hold incoming data from the client
    while (client.connected()) {            // loop while the client's connected

      if (client.available()) {             // if there's bytes to read from the client,    
        char c = client.read();             // read a byte, then
        // This lockup is because the recv function is blocking.
        Serial.print(c);
        if (c == '\n') {                    // if the byte is a newline character
          // if the current line is blank, you got two newline characters in a row.
          // that's the end of the client HTTP request, so send a response:
          if (currentLine.length() == 0) {  
            break;         
          } 
          else {      // if you got a newline, then clear currentLine:
            currentLine = "";
          }
        }     
        else if (c != '\r') {    // if you got anything else but a carriage return character,
          currentLine += c;      // add it to the end of the currentLine
        }
        if (currentLine.endsWith("GET / ")) {
          statusConfig = 0;
          printIndex();
        }
        if (currentLine.endsWith("GET /config.html ")) {
          statusConfig = 1;
          printConfig();
        }
        if (currentLine.endsWith("GET /index.html ")) {
          statusConfig = 0;
          printIndex();
        }
        // Check to see if the client request was "GET /H" or "GET /L":
        if (currentLine.endsWith("GET /RED_LED_H")) {digitalWrite(RED_LED, HIGH);printConfig();}         
        if (currentLine.endsWith("GET /RED_LED_L")) {digitalWrite(RED_LED, LOW);printConfig();}     
        if (currentLine.endsWith("GET /GREEN_LED_H")) {digitalWrite(GREEN_LED, HIGH);printConfig();}       
        if (currentLine.endsWith("GET /GREEN_LED_L")) {digitalWrite(GREEN_LED, LOW);printConfig();}
      }
    }
    // close the connection:
    client.stop();
    Serial.println("client disonnected");
  }
}

void printIndex() {
  // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
  // and a content-type so the client knows what's coming, then a blank line:    

  client.println("HTTP/1.1 200 OK");

  client.println("Content-type:text/html");
  client.println();
  client.println("<html><head><title>CC3000 Energia Webpage</title></head><body align=center>");
  client.println("<h1 align=center><font color=\"red\">Welcome To CC3000 Web Server</font></h1>");
  client.println("</br><font size=\"4px\"><table border=\"0\" align=center width=1200px height=590px>");
  client.println("<tr><td align=center width=375></td><td width=450px align=left valign=\"top\">");


  client.println("<p>Using CC3000 WLAN connectivity, Web Server provides ");
  client.println("capability to remotely read and write GPIOs ");
  client.println("on/off.</p></br></br>");
  client.println("<p><a href=\"/config.html\">Click here</a> ");
  client.println("to check status and configure the board</p>");
  client.println("<td align=cneter width=375></td></tr></table></font></body></html>");

  client.println();

}

void printConfig() {
  // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
  // and a content-type so the client knows what's coming, then a blank line:    
  client.println("HTTP/1.1 200 OK");
  client.println("Content-type:text/html");
  client.println();
  client.println("<html><head><title>CC3000 Energia Webpage</title></head><body align=center>");
  client.println("<h1 align=center><font color=\"red\">Welcome To CC3000 Web Server</font></h1>");

  // the content of the HTTP response follows the header:
  // Added: nicer buttons
  client.print("<font color='green'>GREEN_LED</font> <button onclick=\"location.href='/GREEN_LED_H'\">HIGH</button>");
  client.println(" <button onclick=\"location.href='/GREEN_LED_L'\">LOW</button><br>");
  client.print("<font color='red'>RED_LED</font> <button onclick=\"location.href='/RED_LED_H'\">HIGH</button>");
  client.println(" <button onclick=\"location.href='/RED_LED_L'\">LOW</button><br><br>");

  client.println("PUSH1 ");
  if(digitalRead(PUSH1))client.print("is HIGH<br>");
  else client.print("is LOW<br>");
  client.println("PUSH2 ");
  if(digitalRead(PUSH2))client.print("is HIGH<br>");
  else client.print("is LOW<br>");  

  client.println("<a href=\"/config.html\" >refresh</a> <br>");
  client.println("<a href=\"/index.html\" >home</a> <br>");
  client.println("</body></html>");
  // The HTTP response ends with another blank line:
  client.println();
  // break out of the while loop:
}

