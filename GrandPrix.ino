// nsocwx 2022
// @nsocwx
// Digital pressure and RPM counter
// Pressure transducer output conncets to A0
// RPM signal (falling) connects to digital 2
/*****************************/
// includes

/*****************************/
// definitions
#define pressurePin A3
#define rpmPin 2

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
  Serial.begin(9600);           //  setup serial
  //vss
  attachInterrupt(digitalPinToInterrupt(rpmPin),rpmCount,FALLING);//create interrupt on rpmPin to run rpmCount() on each pulse
  timeOld = millis();
  delay(2000);
}

void loop() {
  // put your main code here, to run repeatedly:
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE){  
    rpm = rpmCount();
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
  delay(1000);
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
