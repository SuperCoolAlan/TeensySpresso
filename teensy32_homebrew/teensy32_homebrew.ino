// Rancilio Silvia PID controller
// Alan Sandoval
// June 2024

#include <SPI.h>
#include "Adafruit_MAX31855.h"
#include <PID_v1_bc.h>

#define CS 10
#define SCLK 13
#define DIN 12

#define RelayPin 6
#define RedLEDOne 23
#define RedLEDTwo 20 

Adafruit_MAX31855 tc0(SCLK, CS, DIN);

//Define PID vars
double Setpoint, Input, Output;

//Specify the links and initial tuning parameters
PID myPID(&Input, &Output, &Setpoint, 1, 1, 1, DIRECT);

int WindowSize = 1000;
unsigned long windowStartTime;

void setup() {
  // create serial
  Serial.begin(9600);
  long unsigned debug_start = millis ();
  while (!Serial && ((millis () - debug_start) <= 2000)); 

  // configure SPI outputs
  pinMode(CS, OUTPUT);
  digitalWrite(CS, HIGH);

  setupPid();
  
  delay(500);
}

void setupPid() {
    pinMode(RelayPin, OUTPUT);
    windowStartTime = millis();
  
    //initialize the variables we're linked to
    Setpoint = 28;
  
    //tell the PID to range between 0 and the full window size
    myPID.SetOutputLimits(0, WindowSize);
  
    //turn the PID on
    myPID.SetMode(AUTOMATIC);
}

void loop() {
  double Input = tc0.readCelsius();

  calculatePid(Input);

  delay(500);
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
    Serial.print("Relay: ");
    Serial.println(digitalRead(RelayPin));
  }
  
}
