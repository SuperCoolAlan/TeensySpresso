// Rancilio Silvia PID controller
// Alan Sandoval
// June 2024

#include <SPI.h>
#include <WiFi101.h>
#include "Adafruit_MAX31855.h"
#include <PID_v1_bc.h>
#include "TeensyThreads.h"
#include <MQTT.h>

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
int status = WL_IDLE_STATUS;
#define WINC_CS 9
#define WINC_IRQ 8
#define WINC_RST 4
#define WINC_EN -1


WiFiClient net;
MQTTClient client;
unsigned long mqttLastMillis = 0;

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
  if ( status == WL_CONNECTED ) {
    Serial.println("WiFi connection successful... attempting to connect to MQTT");
    // threads.addThread(connectMqtt);
    connectMqtt();
  }

  // threads.addThread(queryTemp);
  // threads.addThread(calculatePid);
}

void loop() {
  blink();

  // publish a message roughly every second.
  if (millis() - mqttLastMillis > 1000) {
    mqttLastMillis = millis();
    client.publish("/teensyspresso", "ilikebeef");
  }
}

void queryTemp() {
  while(1) {
    digitalWrite(CS, LOW);
    latestTemp = tc0.readCelsius();
    digitalWrite(CS, HIGH);
    Serial.println(latestTemp);
    threads.delay(WindowSize / 2);
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
    threads.delay(WindowSize / 2);
  }
}

//
// MQTT
//

// void setupMqtt() {

// }

void connectMqtt() {
  Serial.print("subscribing to MQTT server...");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print("WiFi did not stabilize.");
    threads.delay(1000);
  }

  // Note: Local domain names (e.g. "Computer.local" on OSX) are not supported
  // by Arduino. You need to set the IP address directly.
  client.begin("3.13.75.214", net); //teensyspresso.asandov.com
  client.onMessage(mqttMessageReceived);

    while (!client.connect("teensyspresso")) {
      Serial.print("\n(re)connecting to MQTT...");
      Serial.print(".");
      threads.delay(500);
    }

    Serial.println("\nconnected!");

    client.subscribe("/teensyspresso");
    threads.delay (5000);
    client.unsubscribe("/teensyspresso");
}

void mqttMessageReceived(String &topic, String &payload) {
  Serial.println("incoming: " + topic + " - " + payload);

  // Note: Do not use the client in the callback to publish, subscribe or
  // unsubscribe as it may cause deadlocks when other things arrive while
  // sending and receiving acknowledgments. Instead, change a global variable,
  // or push to a queue and handle it in the loop after calling `client.loop()`.
}

//
// WiFi Functions
//

void setupWiFi() {
  WiFi.setPins(WINC_CS, WINC_IRQ, WINC_RST, WINC_EN);

  // check for the presence of the shield:
  if (WiFi.status() == WL_NO_SHIELD) {
    Serial.println("WiFi shield not present");
    // don't continue
    while (true);
  }

  // attempt to connect to WiFi network:
  int count = 0;
  while ( status != WL_CONNECTED && count < 5 ) {
    count++;
    Serial.print(String("Attempt number ") + count + String(" of 5 to connect to Network named: "));
    Serial.println(ssid);                   // print the network name (SSID);
    // Connect to WPA/WPA2 network. Change this line if using open or WEP network:
    status = WiFi.begin(ssid, pass);
    // wait 7 seconds for connection:
    threads.delay(7000);
  }
  if ( status != WL_CONNECTED ) {
    Serial.println("Failed to connect to WiFi!");
  } else {
    // you're connected now, so print out the status
    printWiFiStatus();
  }
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
}
