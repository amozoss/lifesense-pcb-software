#include <SPI.h>
#include <WiFi.h>

#define DEBUG 0

const IPAddress INADDR_NONE(0,0,0,0);
char ssid[] = "cc3000"; 
char pass[] = "coolestever"; 

// Settings
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

int INPUT_PINS_SIZE = 7;
// These are the names we will send to the server
char* INPUT_PINS_NAME[] = {"A0","A1","A2","A4","A5","A6","A7"};
int INPUT_PINS[] = {A0,A1,A2,A4,A5,A6,A7};

int   OUTPUT_PINS_SIZE   = 8;
// These are the names we are expecting the server to give us.
char* OUTPUT_PINS_NAME[] = {"PF_1","PF_2","PF_3","PB_3","PC_4","PC_5","PC_6","PC_7","PD_6","PD_7","PF_4"};
int   OUTPUT_PINS[]      = { PF_1 , PF_2 , PF_3 , PB_3 , PC_4 , PC_5 , PC_6 , PC_7 , PD_6 , PD_7 , PF_4 };

void setInputPins() {
  if (DEBUG) Serial.println("Input Pins:  ");
  for (int i=0;i<INPUT_PINS_SIZE;i++) {
    if (DEBUG) Serial.println(INPUT_PINS[i]);
    pinMode(INPUT_PINS[i], INPUT);
  }
  if (DEBUG) Serial.println("");
}

void setOutputPins() {
}

void setup()
{
  // Start Serial for debugging on the Serial Monitor
  Serial.begin(115200);
  
  setInputPins();
  
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
}

void loop()
{
  if (client.connected()) {
    // Start building our JSON data payload
    String data = "{";
    readInputPins(data);
    data += "}\n\n";
    pushUpdate(data);

    // Read the response  
    parseResponse();
  }
  else {
    reconnect();    
  }

  // Check if WiFi needs to be restarted
  if (failedCounter > 3 ) {
    failedCounter = 0;
    Serial.println("Greater than 3");
    startWiFi();
  }
}

// Read the input pins specified in INPUT_PIN and append the json to
// the result string.
//
// Adding to the result string like this is not the most efficient, but it
// is probably fast enough for 8 values.  
void readInputPins(String &result) {
  for (int i=0;i<INPUT_PINS_SIZE;i++) {
    result = result + "\"" + INPUT_PINS_NAME[i] + "\":";
    String value = String(analogRead(INPUT_PINS[i]), DEC);
    //String value = String(i * 10000);
    result += value;
    if (i!=(INPUT_PINS_SIZE-1)) {
      result += ",";
    }
  }
}

void reconnect() {
  Serial.println("Connecting!");
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

void pushUpdate(String tsData)
{
  if (DEBUG) Serial.println("Trying to push: ");
  
  if (DEBUG) Serial.println(tsData);

  client.print("POST /scale HTTP/1.1\n");
  client.print("Host: localhost:8080\n");
  client.print("Accept: \*/\*\n");
  client.print("Content-Type: application/json\n");
  client.print("Content-Length: ");

  client.print(tsData.length());
  client.print("\n\n");
  client.print(tsData);

  if (DEBUG) Serial.println("pushed an update!");
}

void parseResponse() {
  String currentLine = "";
  while (client.available()) {
    char c = client.read();             // read a byte, then
    // This lockup is because the recv function is blocking.
    Serial.print(c);
    if (c == '\n') {                    // if the byte is a newline character
      // if the current line is blank, you got two newline characters in a row.
      // that's the end of the client HTTP request, so send a response:
      if (currentLine.length() == 0) {  
       // return;         
      } 
      else {      // if you got a newline, then clear currentLine:
        currentLine = "";
      }
    }     
    else if (c != '\r') {    // if you got anything else but a carriage return character,
      currentLine += c;      // add it to the end of the currentLine
    }

    if (currentLine.endsWith("RED ON")) {digitalWrite(RED_LED, HIGH);}         
    if (currentLine.endsWith("RED OFF")) {digitalWrite(RED_LED, LOW);}
    if (currentLine.endsWith("GREEN ON")) {digitalWrite(GREEN_LED, HIGH);}       
    if (currentLine.endsWith("GREEN OFF")) {digitalWrite(GREEN_LED, LOW);}
  }
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
