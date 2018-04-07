#include <avr/sleep.h>
#include <SoftwareSerial.h>

// toggle debug output via I2C
// I keep debugging on
#define DEBUGGING true

#if DEBUGGING
  // I2C communication
  #include <Wire.h>
#endif

// configuration of flashing LEDs
const int ledArrayPin=8;
const int ledFlashRepeat=5;
const int ledFlashInterval=200;

// HC-12 pins (do NOT use digital pins 0 and 1 since they are used by the serial interface)
const int hc12RxPin=13;
const int hc12TxPin=12;
const int hc12SetPin=11;

// handle interrupt via HC12 module (pin D3); this pin number will be used in pinMode
const int interruptPin=3;
// attachInterrupt expects a different pin number than pinMode
// usually, one we would use digitalPinToInterrupt to transform it into the right format
// however, digitalPinToInterrupt does not work on my computer, so I manually translated (i.e., D3=1)
const int iPin=1;

// flag indicating whether microcontroller is allowed to go into sleep mode
volatile boolean canSleep=true;

// software serial interface to HC12 module
// the datasheet for the HC12 module can be found here:
// https://statics3.seeedstudio.com/assets/file/bazaar/product/HC-12_english_datasheets.pdf
SoftwareSerial HC12(hc12TxPin,hc12RxPin);

// buffer variable
byte incomingByte;

// these values are taken from the HC12 documentation v2 (+10ms for safety)
const unsigned long hc12setHighTime=90;
const unsigned long hc12setLowTime=50;
const unsigned long int hc12cmdTime=100;

#if DEBUGGING
  // send data over I2C to other board
  // information on how to use the wire library can be found here: https://www.arduino.cc/en/Reference/Wire
  void wireSend(char dataDescr[], const char data[]="") {
    Wire.beginTransmission(8);
    Wire.write(dataDescr);
    Wire.write(data);
    Wire.write("\n");
    Wire.endTransmission();
    delay(20);
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
    delay(20);
  }
#endif

void flashLedArray(int pin, int repeat, int period) {
  // flash the LED array for a few times
  for (int i=0;i<repeat;i++) {
    digitalWrite(pin,HIGH);
    delay(period);
    digitalWrite(pin,LOW);
    delay(period);
  }
}

// send an AT command to the HC12 module
// AT commands can be used to change the configuration of the HC12 module
void sendCmd(const char cmd[]) {
  #if DEBUGGING
    wireSend("Sending Cmd:",cmd);
  #endif
  
  // put HC-12 into cmd mode
  digitalWrite(hc12SetPin,LOW);
  delay(hc12setLowTime);
  
  HC12.print(cmd);
  delay(hc12cmdTime);
  
  // be carefule with piping the data from HC12 to wire
  // it may interfere with sender-receiver communication
  // commented out for now
  /*
  #if DEBUGGING
    ssToWire();
  #else
    ssEmptyBuffer();
  #endif
  */
  // put HC-12 back into transparent mode
  digitalWrite(hc12SetPin,HIGH);
  delay(hc12setHighTime);
}

// discard buffer
void ssEmptyBuffer() {
  while (HC12.available()) HC12.read();
}

// wake Atmel chip from PWR_DOWN mode
void atmelWake() {
  // canSleep will be set back to true as soon as we are finished with flashing the LEDs
  canSleep=false;
  sleep_disable();
  // detach interrupt so that we do not get interrupted while we are flashing the LEDs
  detachInterrupt(iPin);
}

// put Atmel chip to sleep and attach interrupt for waking it up
// the interrupt is triggered by the TXD pin of the HC12 module
void atmelSleep() { 
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
  noInterrupts();
  sleep_enable();
  // clear previously set interrupt flag
  EIFR = bit (INTF1);
  // FALLING edge worked best for me
  attachInterrupt(iPin, atmelWake, FALLING);
  interrupts();
  sleep_cpu();
}

void setup() { 
  pinMode(ledArrayPin, OUTPUT);
  pinMode(hc12SetPin, OUTPUT);
  pinMode(interruptPin, INPUT);
  
  digitalWrite(ledArrayPin,LOW);
  
  // turn off ADC and comparator (we don't need them for the receiver)
  ADCSRA = ADCSRA & B01111111;
  ACSR = B10000000;
   
  // flash LED array once to signal that we started up
  flashLedArray(ledArrayPin,1,100);
  
  // Open serial port to HC12
  // set this to twice the BAUD rate that gets reported by sending "AT+RX" to the HC-12 module
  // I don't know why it has to be twice this BAUD rate but setting it to other values did not work
  HC12.begin(2400);
  
  #if DEBUGGING
    // I2C to other board
    Wire.begin();
    delay(200);
    // print configuration of HC-12 module
    sendCmd("AT+RX");
  #endif
  
  // actually, only need to set that once since HC12 remembers settings
  // I left it in the setup anyway
  // AT+FU2 is the power saving mode of the HC12 module
  sendCmd("AT+FU2");
  delay(1000);
  // high power transmission mode
  sendCmd("AT+P8");
  delay(1000);
  
  // flash LED array twice so that we know that setup has finished
  flashLedArray(ledArrayPin,2,100);
}


void loop() {
  if (canSleep) {
    atmelSleep();
  } else {
    flashLedArray(ledArrayPin,ledFlashRepeat,ledFlashInterval);
    canSleep=true;
  }
}
