// nsocwx 2022
// @nsocwx
// Digital pressure and RPM counter
// Pressure transducer output conncets to A0
// RPM signal (falling) connects to digital 2
/*****************************/
// includes
#include <SPI.h>
#include "Adafruit_GFX.h"
#include "Adafruit_HX8357.h"
#include <util/atomic.h>
/*****************************/
// definitions
#define pressurePin A3
#define rpmPin 2
#define TFT_CS 10
#define TFT_DC 9
#define TFT_RST -1 // RST can be set to -1 if you tie it to Arduino's reset
Adafruit_HX8357 tft = Adafruit_HX8357(TFT_CS, TFT_DC, TFT_RST);
// Color definitions
#define BLACK    0x0000
#define BLUE     0x001F
#define RED      0xF800
#define GREEN    0x07E0
#define CYAN     0x07FF
#define MAGENTA  0xF81F
#define YELLOW   0xFFE0 
#define WHITE    0xFFFF
/*****************************/
// variables
unsigned int rpm,RPM;//varaible to store rpm
volatile unsigned int PULSE;//counter for pulses 
unsigned int delta; //holds the delta time
unsigned long timeNew; //variable to store current check time
unsigned long timeOld;  //variable to store previous check time
unsigned int analogVolts; //variable to store the voltage reading from pressurePin
unsigned int oilPressure; //varible to store readable oil pressure

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);           //  setup serial
  //vss
  attachInterrupt(digitalPinToInterrupt(rpmPin),rpmCount,FALLING);//create interrupt on rpmPin to run rpmCount() on each pulse
  timeOld = millis();
  //screen 
  tft.begin();
  // read diagnostics (optional but can help debug problems)
  uint8_t x = tft.readcommand8(HX8357_RDPOWMODE);
  Serial.print("Display Power Mode: 0x"); Serial.println(x, HEX);
  x = tft.readcommand8(HX8357_RDMADCTL);
  Serial.print("MADCTL Mode: 0x"); Serial.println(x, HEX);
  x = tft.readcommand8(HX8357_RDCOLMOD);
  Serial.print("Pixel Format: 0x"); Serial.println(x, HEX);
  x = tft.readcommand8(HX8357_RDDIM);
  Serial.print("Image Format: 0x"); Serial.println(x, HEX);
  x = tft.readcommand8(HX8357_RDDSDR);
  Serial.print("Self Diagnostic: 0x"); Serial.println(x, HEX); 

  tft.setRotation(3);
  tft.fillScreen(HX8357_BLACK);

  
  delay(2000);
}

void loop() {
  // put your main code here, to run repeatedly:
  
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE){  
    rpm = readRpm();
  }
  //Serial.print("RPM:");
  //Serial.println(rpm);
  analogVolts = analogRead(pressurePin); //read in the analog voltage
  if(analogVolts < 102){
    Serial.println("Oil pressure low input");
  }
  else if(analogVolts > 922){
    Serial.println("Oil pressure high input");
  }
  else{
    oilPressure = map(analogVolts, 102, 922, 0, 100);
    Serial.print("Oil Pressure:");
    Serial.println(oilPressure);
  }

  // screen
  tft.fillRect(340, 0, 100, 60, BLACK);
  tft.setCursor(0, 0);
  tft.setTextColor(HX8357_WHITE);  tft.setTextSize(4);
  tft.print("Oil Pressure:   ");
  tft.println(oilPressure);
  tft.print("Engine Speed:   ");
  tft.println(rpm);  
  
  delay(200);
}

/*****************************/
void rpmCount() //runs when rpmPin sees a falling signal
{
  PULSE++;//increment pulse counter by 1;
}
/*****************************/
unsigned int readRpm()//runs when called
{
  unsigned int result;
  timeNew = millis();
  delta = (timeNew-timeOld);
  result = (PULSE/delta)*60000;//adjust revolution to per minute= 60k millis 
  timeOld = timeNew;
  PULSE = 0;
  return result;
}
