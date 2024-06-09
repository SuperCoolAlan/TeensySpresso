// Rancilio Silvia PID controller
// Alan Sandoval
// June 2024

#include <SPI.h>
#include <WiFi101.h>
#include "Adafruit_MAX31855.h"
#include <PID_v1_bc.h>

#define CS 10
#define SCLK 13
#define DIN 12

#define RelayPin 6
#define RedLEDOne 20 // bright light shows when PID is on
#define RedLEDTwo 23 // status for system

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

  setupPid();
  
  delay(500);
}

void loop() {
  double Input = tc0.readCelsius();

  calculatePid(Input);
  delay(500);
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

void calculatePid(double temp) {
  if(isnan(temp)){
    Serial.print("MAX31855 error: ");
    Serial.println(tc0.readError());
  }else{
    Serial.print("Current temperature: "); 
    Serial.println(temp);
    
    myPID.Compute();

    unsigned long now = millis();
    if (now - windowStartTime > WindowSize)
    { //time to shift the Relay Window
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
    digitalWrite(RedLEDTwo, !digitalRead(RedLEDTwo));
    Serial.print("Relay: ");
    Serial.println(digitalRead(RelayPin));
  }
}

//
// WiFi Functions
//

