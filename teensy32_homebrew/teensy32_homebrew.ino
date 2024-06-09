// Rancilio Silvia PID controller
// Alan Sandoval
// June 2024

#include <SPI.h>
#include <WiFi101.h>
#include "Adafruit_MAX31855.h"
#include <PID_v1_bc.h>
#include "TeensyThreads.h"

#define CS 10
#define SCLK 13
#define DIN 12

#define RelayPin 6
#define RedLEDOne 20 // bright light shows when PID output is on
#define RedLEDTwo 23 // status for system (solid lit while starting, blinking while running)

unsigned long blinkPreviousMillis = 0;  // will store last time LED was updated
const long blinkInterval = 1000;  // interval at which to blink (milliseconds)

#include "arduino_secrets.h" 
///////please enter your sensitive data in the Secret tab/arduino_secrets.h
char ssid[] = SECRET_SSID;        // your network SSID (name)
char pass[] = SECRET_PASS;    // your network password (use for WPA, or use as key for WEP)
int keyIndex = 0;                 // your network key Index number (needed only for WEP)
#define WINC_CS 9
#define WINC_IRQ 8
#define WINC_RST 4
#define WINC_EN -1

int status = WL_IDLE_STATUS;
WiFiServer server(80);

// Connect to Temperature Module
Adafruit_MAX31855 tc0(SCLK, CS, DIN);
double latestTemp;

//Define PID vars
double Setpoint, Input, Output;
int WindowSize = 1000;
unsigned long windowStartTime;

//Specify the links and initial tuning parameters
PID myPID(&Input, &Output, &Setpoint, 1, 1, 1, DIRECT);

void setup() {
  // create serial
  Serial.begin(9600);
  long unsigned debug_start = millis ();
  while (!Serial && ((millis () - debug_start) <= 2000)); 

  // configure SPI outputs
  pinMode(CS, OUTPUT);
  digitalWrite(CS, HIGH);
  pinMode(RedLEDTwo, OUTPUT);
  digitalWrite(RedLEDTwo, HIGH);

  setupPid();
  // Setting pins
  setupWiFi();

  threads.addThread(queryTemp);
  threads.addThread(runWiFiServer);
  threads.addThread(calculatePid);
}

void loop() {
  blink();
  // runWiFiServer();
  delay(1);
}

void queryTemp() {
  while(1) {
    // Serial.println("running queryTemp()");
    latestTemp = tc0.readCelsius();
    threads.yield();
  }
}

void blink() {
  unsigned long currentMillis = millis();
  if (currentMillis - blinkPreviousMillis >= blinkInterval) {
    // save the last time you blinked the LED
    blinkPreviousMillis = currentMillis;

    // set the LED with the ledState of the variable:
    digitalWrite(RedLEDTwo, !digitalRead(RedLEDTwo));
  }
}

//
// PID functions
//

void setupPid() {
    pinMode(RelayPin, OUTPUT);
    pinMode(RedLEDOne, OUTPUT);
    
    windowStartTime = millis();
  
    //initialize the pid variables we're linked to
    Setpoint = 100;
    Input = 0;
  
    //tell the PID to range between 0 and the full window size
    myPID.SetOutputLimits(0, WindowSize);
  
    //turn the PID on
    myPID.SetMode(AUTOMATIC);
}

void calculatePid() {
  while(1) {
    // Serial.println("running calculatePid()");
    if(isnan(latestTemp)) {
      Serial.print("MAX31855 error: ");
      Serial.println(tc0.readError());
    } else{
      // Serial.print("temp: "); 
      // Serial.println(latestTemp);
      
      myPID.Compute();

      unsigned long now = millis();
      if (now - windowStartTime > WindowSize) { //time to shift the Relay Window
        windowStartTime += WindowSize;
      }
      if (Output > now - windowStartTime) {
        digitalWrite(RelayPin, HIGH);
        digitalWrite(RedLEDOne, HIGH);
      }
      else {
        digitalWrite(RelayPin, LOW);
        digitalWrite(RedLEDOne, LOW);
      }
      // Serial.print("Relay: ");
      // Serial.println(digitalRead(RelayPin));
    }
    threads.yield();
  }
}

//
// WiFi Functions
//

void runWiFiServer() {
  while(1) {
    Serial.println("running runWiFiServer");
    Serial.println(status);
    Serial.println(WiFi.status()); // this is where WiFi.status cannot be found
    if (status != WiFi.status()) {
      // it has changed update the variable
      status = WiFi.status();

      if (status == WL_AP_CONNECTED) {
        byte remoteMac[6];

        // a device has connected to the AP
        Serial.print("Device connected to AP, MAC address: ");
        WiFi.APClientMacAddress(remoteMac);
        printMacAddress(remoteMac);
      } else {
        // a device has disconnected from the AP, and we are back in listening mode
        Serial.println("Device disconnected from AP");
      }
    }

    WiFiClient client = server.available();   // listen for incoming clients

    if (client) {                             // if you get a client,
      Serial.println("new client");           // print a message out the serial port
      String currentLine = "";                // make a String to hold incoming data from the client
      while (true) {            // loop while the client's connected
        if (client.available()) {             // if there's bytes to read from the client,
          char c = client.read();             // read a byte, then
          Serial.write(c);                    // print it out the serial monitor
          if (c == '\n') {                    // if the byte is a newline character

            // if the current line is blank, you got two newline characters in a row.
            // that's the end of the client HTTP request, so send a response:
            if (currentLine.length() == 0) {
              // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
              // and a content-type so the client knows what's coming, then a blank line:
              client.println("HTTP/1.1 200 OK");
              client.println("Content-type:text/html");
              client.println();

              // the content of the HTTP response follows the header:
              client.print("Click <a href=\"/H\">here</a> turn the LED on<br>");
              client.print("Click <a href=\"/L\">here</a> turn the LED off<br>");

              // The HTTP response ends with another blank line:
              client.println();
              // break out of the while loop:
              break;
            }
            else {      // if you got a newline, then clear currentLine:
              currentLine = "";
            }
          }
          else if (c != '\r') {    // if you got anything else but a carriage return character,
            currentLine += c;      // add it to the end of the currentLine
          }

          // Check to see if the client request was "GET /H" or "GET /L":
          if (currentLine.endsWith("GET /H")) {
            digitalWrite(RedLEDOne, HIGH);               // GET /H turns the LED on
          }
          if (currentLine.endsWith("GET /L")) {
            digitalWrite(RedLEDOne, LOW);                // GET /L turns the LED off
          }
        }
      }
      // close the connection:
      client.stop();
      Serial.println("client disconnected");
    }
  threads.yield();
  }
}

void setupWiFi() {
  WiFi.setPins(WINC_CS, WINC_IRQ, WINC_RST, WINC_EN);
  // check for the presence of the shield:
  if (WiFi.status() == WL_NO_SHIELD) {
    Serial.println("WiFi shield not present");
    // don't continue
    while (true);
  }

  // by default the local IP address of will be 192.168.1.1
  // you can override it with the following:
  // WiFi.config(IPAddress(10, 0, 0, 1));

  // print the network name (SSID);
  Serial.print("Creating access point named: ");
  Serial.println(ssid);

  // Create open network. Change this line if you want to create an WEP network:
  status = WiFi.beginAP(ssid);
  if (status != WL_AP_LISTENING) {
    Serial.println("Creating access point failed");
    // don't continue
    while (true);
  }

  // wait 5 seconds for connection:
  delay(5000);

  // start the web server on port 80
  server.begin();

  // you're connected now, so print out the status
  printWiFiStatus();
}

void printWiFiStatus() {
  // print the SSID of the network you're attached to:
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print your WiFi shield's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  // print the received signal strength:
  long rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");
  // print where to go in a browser:
  Serial.print("To see this page in action, open a browser to http://");
  Serial.println(ip);

}

void printMacAddress(byte mac[]) {
  for (int i = 5; i >= 0; i--) {
    if (mac[i] < 16) {
      Serial.print("0");
    }
    Serial.print(mac[i], HEX);
    if (i > 0) {
      Serial.print(":");
    }
  }
  Serial.println();
}
