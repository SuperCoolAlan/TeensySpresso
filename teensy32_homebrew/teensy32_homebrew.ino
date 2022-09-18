// Rancilio Silvia PID controller
// Alan Sandoval
// September 2022

#include <SPI.h>
#include "Adafruit_MAX31855.h"
#include <PID_v1.h>

#define CS 10
#define SCLK 13
#define DIN 12
#define RelayPin 6

Adafruit_MAX31855 tc0(SCLK, CS, DIN);

//Define PID vars
double Setpoint, Input, Output;

//Specify the links and initial tuning parameters
PID myPID(&Input, &Output, &Setpoint, 1, 1, 1, DIRECT);

int WindowSize = 1000;
unsigned long windowStartTime;

void setup() {
  // debug log
  Serial.begin(9600);
  long unsigned debug_start = millis ();
  while (!Serial && ((millis () - debug_start) <= 5000)); 
  
  pinMode(CS, OUTPUT);
  digitalWrite(CS, HIGH);

  pinMode(RelayPin, OUTPUT);

  windowStartTime = millis();

  //initialize the variables we're linked to
  Setpoint = 28;

  //tell the PID to range between 0 and the full window size
  myPID.SetOutputLimits(0, WindowSize);

  //turn the PID on
  myPID.SetMode(AUTOMATIC);
  
  delay(500);
}

void loop() {
  double Input = tc0.readCelsius();
  if(isnan(Input)){
    Serial.print("MAX31855 error code: ");
    Serial.println(tc0.readError());
  }else{
    Serial.print("Temperature is "); 
    Serial.println(Input);
    myPID.Compute();

    unsigned long now = millis();
    if (now - windowStartTime > WindowSize)
    { //time to shift the Relay Window
      windowStartTime += WindowSize;
    }
    if (Output > now - windowStartTime) digitalWrite(RelayPin, HIGH);
    else digitalWrite(RelayPin, LOW);
    Serial.print("Relay mode is: ");
    Serial.println(digitalRead(RelayPin));
  }
  delay(500);
}
