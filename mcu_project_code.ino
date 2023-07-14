#include <HX711_ADC.h>
#include <Arduino.h>
#include <soc/rtc.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "BluetoothSerial.h"

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32


//HX711 pins & settings
const int HX711_datapin = 32;
const int HX711_sckpin = 33;
const int CALIBRATION_FACTOR = 228.23;
float weight;
unsigned long t = 0;

HX711_ADC loadcell(HX711_datapin, HX711_sckpin);

//HC-SR04 pins & settings
const int trigPin = 5;
const int echoPin = 18;
long duration;
volatile float height = 0;
volatile float distance = 0;
float fixedDistance = 200;  //distance to set sensor in, in meters

//OLED pins & settings
//const int OLED_sck = 22;
//const int OLED_sda = 21;
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

//Bluetooth settings
BluetoothSerial espBt;

//bmi calc variables
volatile float bmi = 0;

const int powerLED = 25;

void setup() {
  // put your setup code here, to run once:
  
  //Serial monitor setup
  Serial.begin(115200); 
  delay(10);

  //OLED setup
  display.begin(SSD1306_SWITCHCAPVCC,0x3C);
  delay(500);  //delay for stability
  display.clearDisplay();
  display.setTextColor(WHITE);

  //HX711 setup  
  Serial.println("Initializing scale"); //serial testing
  loadcell.begin();
  unsigned long stabilizingtime = 2000; // preciscion right after power-up can be improved by adding a few seconds of stabilizing time
  boolean _tare = true; //set this to false if you don't want tare to be performed in the next step (at start)
  loadcell.start(stabilizingtime, _tare);
  loadcell.setCalFactor(CALIBRATION_FACTOR);  

  //HC-SR04 setup
  pinMode(trigPin, OUTPUT); // Sets the trigPin as an Output
  pinMode(echoPin, INPUT); // Sets the echoPin as an Input
  
  //bluetooth setup
  espBt.begin("BMI_scale"); //bt name

  //pin setup
  pinMode (powerLED, OUTPUT);
  
}

void loop() {
  // put your main code here, to run repeatedly:
  
  //powerLED
  //digitalWrite(powerLED, HIGH);

  //func to read serial monitor for tare request from app

  //weight data loop
  static boolean newWeight = 0; //false initial boolean argument
  const int serialPrintInterval = 0; //increase value to slow down serial print - testing
  if (loadcell.update()) newWeight = true; //checks for new data
  if (newWeight) {
    if (millis() > t + serialPrintInterval) {
      weight = loadcell.getData();
      if (weight < 0){
        weight = 0;
      }
      Serial.print("Load_cell output val: "); //serial testing
      Serial.println(weight);
      //oledDisplay(weight);  //call to method to update oled display
                                    //send data through bluetooth
      newWeight = 0;
      t = millis();

      //height data loop
      digitalWrite(trigPin, LOW);         //send a pulse
      delayMicroseconds(2);
      digitalWrite(trigPin, HIGH);
      delayMicroseconds(10);
      digitalWrite(trigPin, LOW);

      duration = pulseIn(echoPin, HIGH);  //measure response
      distance = duration*0.034/2;         //D = S x T
      height = (200.0 - distance) / 100;
      Serial.print("Height output val: "); //serial testing
      Serial.println(height);

      oledDisplay(weight, height); //call to method to update oled display
    }
  }

  //bmiCalc(weight, height);
  espBt.println(bmiCalc(weight, height), 1);
  
//  float bmi = bmiWeight / (1.78*1.78);
 // Serial.print(bmi);
  //delay (500);

  //command from serial terminal, send 't' to tare scale
  if (Serial.available() > 0) {
    char inByte = Serial.read();
    if (inByte == 't') loadcell.tareNoDelay();
  }
} 

void oledDisplay (float value1, float value2) {
  display.clearDisplay();
  display.setCursor(0, 16);
  display.setFont();
  display.setTextSize(2);
  display.setTextColor(WHITE);
  if(value1 < 10){
    display.print("0");
    display.print(value1, 1);
    display.print("kg");
  } else {
    display.print(value1, 1);
    display.print("kg");
  }
  display.print(value2, 1);
  display.print("m");
  display.display();
}

float bmiCalc(float weight, float height) {
  bmi = weight / (height*height);
  return bmi;
}
