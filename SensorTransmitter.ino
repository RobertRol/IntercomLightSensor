#include <avr/sleep.h>
#include <avr/wdt.h>
#include <SoftwareSerial.h>

// toggle debug output via I2C
#define DEBUGGING true

#if DEBUGGING
  // I2C communication
  #include <Wire.h>
#endif

// number of LDR measurements
const int readingSamples=5;

// HC-12 pins (do NOT use digital pins 0 and 1 since they are used by the serial interface)
const int hc12RxPin=2;
const int hc12TxPin=3;
const int hc12SetPin=4;

// repeat RF signal 2 times with a delay of 2000msec
const int messageRepeat=2;
const int messageRepeatWait=2000;

// oldLightLevel is the light level from the previous sleep-wake iteration
int oldLightLevel=500;
// an RF signal will be sent if the new light level is above/below oldLightLevel+/-lightDelta
const int lightDelta=3;

// these values are taken from the HC-12 documentation v2 (+10ms for safety)
const unsigned long hc12setHighTime=90;
const unsigned long hc12setLowTime=50;
const unsigned long hc12cmdTime=100;

// SoftwareSerial interface to HC-12 module
// the datasheet for the HC-12 module can be found here:
// https://statics3.seeedstudio.com/assets/file/bazaar/product/HC-12_english_datasheets.pdf
SoftwareSerial HC12(hc12TxPin,hc12RxPin);

// most of the watchdog-timer and sleep-mode related code/comments taken from https://www.gammon.com.au/power
// watchdog intervals
// sleep bit patterns for WDTCSR
enum {
  WDT_16_MS  =  0b000000,
  WDT_32_MS  =  0b000001,
  WDT_64_MS  =  0b000010,
  WDT_128_MS =  0b000011,
  WDT_256_MS =  0b000100,
  WDT_512_MS =  0b000101,
  WDT_1_SEC  =  0b000110,
  WDT_2_SEC  =  0b000111,
  WDT_4_SEC  =  0b100000,
  WDT_8_SEC  =  0b100001,
 };
 
// watchdog interrupt service routine
ISR (WDT_vect) {
  // disable watchdog
  wdt_disable();
}
 
#if DEBUGGING
  // send data over I2C to other board
  // wire library info can be found here: https://www.arduino.cc/en/Reference/Wire
  void wireSend(char dataDescr[], const char data[]="") {
    Wire.beginTransmission(8);
    Wire.write(dataDescr);
    Wire.write(data);
    Wire.write("\n");
    Wire.endTransmission();
  }
  
  // pipe data from HC-12 to I2C
  void ssToWire() {
    byte incomingByte;
    while (HC12.available()) {
      incomingByte = HC12.read();
      Wire.beginTransmission(8);
      Wire.write(incomingByte);
      Wire.endTransmission();
    }
  }
#endif

// send an AT command to the HC12 module
// AT commands can be used to change the configuration of the HC-12 module
void sendCmd(const char cmd[]) {
  #if DEBUGGING
    wireSend("Sending Cmd:",cmd);
  #endif
  
  // put HC-12 into cmd mode
  digitalWrite(hc12SetPin,LOW);
  delay(hc12setLowTime);
  
  HC12.print(cmd);
  delay(hc12cmdTime);
  
  #if DEBUGGING
    ssToWire();
  #endif
  
  // put HC-12 back into transparent mode
  digitalWrite(hc12SetPin,HIGH);
  delay(hc12setHighTime);
}

// put HC-12 module into sleep mode
void HC12Sleep() {
  #if DEBUGGING
    wireSend("Setting HC12 to sleep");
  #endif
  
  digitalWrite(hc12SetPin,LOW);
  delay(hc12setLowTime);
  
  HC12.print("AT+SLEEP");
  delay(hc12cmdTime);
  
  digitalWrite(hc12SetPin,HIGH);
  delay(hc12setHighTime);
}

// wake HC-12 module from sleep mode
void HC12Wake() {
  #if DEBUGGING
    wireSend("Waking up HC12");
  #endif
  digitalWrite(hc12SetPin,LOW);
  delay(hc12setLowTime);
  digitalWrite(hc12SetPin,HIGH);
  // wait some extra time
  delay(250);
}

// send data via SoftwareSerial
void ssSend(const char *lightLevelChar, const char *oldLightLevelChar) {
  #if DEBUGGING
    // information about the light levels is included in debugging mode
    HC12.print("|r");
    HC12.print("-");
    HC12.print(lightLevelChar);
    HC12.print("-");
    HC12.print(oldLightLevelChar);
  #else
    HC12.print("|r|");
  #endif
}

void doSleep (const byte interval) {
  // code and comments in this function body taken from https://www.gammon.com.au/power
  // clear various "reset" flags
  MCUSR = 0;     
  // allow changes, disable reset
  WDTCSR = bit (WDCE) | bit (WDE);
  // set interrupt mode and an interval 
  WDTCSR = bit (WDIE) | interval;    // set WDIE, and requested delay
  wdt_reset();  // pat the dog
  
  // disable ADC
  byte old_ADCSRA = ADCSRA;
  ADCSRA = 0;
  
  // turn off various modules
  byte old_PRR = PRR;
  PRR = 0xFF; 

  // timed sequence coming up
  noInterrupts ();
  
  // ready to sleep
  set_sleep_mode (SLEEP_MODE_PWR_DOWN);  
  sleep_enable();

  // turn off brown-out enable in software
  MCUCR = bit (BODS) | bit (BODSE);
  MCUCR = bit (BODS); 
  interrupts ();
  sleep_cpu ();  

  // cancel sleep as a precaution
  sleep_disable();
  PRR = old_PRR;
  ADCSRA = old_ADCSRA;
}

// read the voltage of the LDR voltage divider
int readLightLevel() {
  // EWMA decay factor
  float alpha = 0.9;
  // discard initial reading
  float value = float(analogRead(A0));
  
  // perform EWMA for subsequent readings (gives more weight to the later readings)
  // this also allows the internal capacitor to charge up
  for (int i=0;i<readingSamples;++i) {
    delay(1);
    value=alpha*float(analogRead(A0)) + (1.0-alpha)*value;
  }
  return int(value);
}

boolean takeReading () {
  // read the current light level
  int lightLevel = readLightLevel();
  
  // hasSent is actually not used anymore and could be removed from the code
  boolean hasSent=false;
  
  const int buffSize=5;
  char lightLevelChar [buffSize];
  char oldLightLevelChar [buffSize];
    
  // convert ints to char arrays
  snprintf (lightLevelChar,buffSize, "%d", lightLevel);
  snprintf (oldLightLevelChar,buffSize, "%d", oldLightLevel);
  
  #if DEBUGGING
    wireSend("lightLevel = ",lightLevelChar);
    wireSend("oldLightLevel = ",oldLightLevelChar);
  #endif
   
  // send signal if new reading exceeds old light level by a given threshold
  if (lightLevel>oldLightLevel+lightDelta) {
    // first wake HC12 module
    HC12Wake();
    // repeat RF signal
    for (int i=0;i<messageRepeat;++i) {
      // send data via HC-12 module
      ssSend(lightLevelChar,oldLightLevelChar);
      delay(messageRepeatWait);
    }
    delay(100);
    //put HC-12 module back to sleep
    HC12Sleep();
    
    #if DEBUGGING
      wireSend("Ring!");
    #endif

    hasSent=true;
  }
  
  // store current light level for comparison in next iteration
  oldLightLevel = lightLevel;
  return hasSent;
}
 
void setup (void) {
  // important! set to EXTERNAL if voltage source is connected to AREF pin
  // otherwise microcontroller might get damaged
  analogReference(EXTERNAL);
  analogRead(0);
  
  pinMode(A0,INPUT);
  pinMode(A1,INPUT);
  
  #if DEBUGGING
    Wire.begin();
  #endif

  pinMode(hc12SetPin, OUTPUT);
  delay(200);
  
  // Serial connection to HC-12
  // set this to twice the BAUD rate that gets reported by sending "AT+RX" to the HC-12 module
  // I don't know why it has to be twice this BAUD rate but setting it to other values did not work
  HC12.begin(2400);
  
  #if DEBUGGING
    // I2C to other board
    Wire.begin();
    delay(200);
    // print configuration of the HC-12 module
    sendCmd("AT+RX");
  #endif
  
  // actually, one only needs to set the power mode of the HC-12 module once since it remembers settings
  // I left it in the setup anyway
  // AT+FU2 is the power saving mode of the HC-12 module
  sendCmd("AT+FU2");
  delay(1000);
  // high power transmission mode
  sendCmd("AT+P8");
  delay(1000);
  
  // put HC-12 module into sleep mode
  HC12Sleep();
  delay(100);
}

void loop (void) {
  boolean hasSent=takeReading();
    
  // sleep for 256ms
  doSleep(WDT_256_MS);
}
