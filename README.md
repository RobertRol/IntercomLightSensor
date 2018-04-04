## Objectives
1. Detection of a light signal from an intercom (without the need of opening and/or changing the wiring of the intercom)
2. Wireless transmission of the signal to another portable device (receiver)
3. Low power consumption: both sensor-transmitter (implemented in one device) and receiver should run for several months on batteries
4. Ability to send and receive the signal across the whole apartment
5. Implementation via microcontrollers since I wanted to play around with my Arduino board

## Overview
1. [Links](#links)
2. [Sensor](#sensor)
3. [Transmitter](#transmitter)
4. [Receiver](#receiver)
5. [Power Source](#power-source)

## Links
The low power objective turned out to be really tricky. However, the website https://www.gammon.com.au/power was incredibly helpful, especially the part about the low-power temperature monitor.
My main take-aways from this website were that
1. I had to go for a bare ATmega328P-PU board implementation (rather than using an Arduino Nano or similar)
2. a watchdog timer should be used to periodically wake up the sleeping processor
3. I should go as low as possible with processor frequency and voltage 

Descriptions on how to set up a bare microprocessor board can be found on the same web page or on https://www.arduino.cc/en/Main/Standalone.

I also ran into some dead ends with the wireless data transmission. In the end, I got it working with the HC-12 wireless module. Its documentation can be found here: https://statics3.seeedstudio.com/assets/file/bazaar/product/HC-12_english_datasheets.pdf.

The ATmega328P-PU microprocessors I bought did not have a bootloader installed. I used my Elegoo Uno R3 development board to burn the bootloader onto the ATmegas. A description of the procedure and a simple bread board layout can be found here: https://www.arduino.cc/en/Tutorial/ArduinoToBreadboard

The microprocessor pin layout is given here: https://www.arduino.cc/en/Hacking/PinMapping168

## Sensor
The main part of the sensor is a voltage divider composed of two light-dependent resistors (LDRs). One of the LDRs is placed right in front of the intercom light ("intercom LDR"), the other one a bit away from it ("ambient LDR"). The voltage drop across the intercom LDR is then used as an analogRead input to the ATmega328P-PU microcontroller.
The reason for using two LDRs instead of one intercom LDR plus a constant resistor is that the sensor has to work independent of the ambient light level. By using a second (ambient) LDR, the sensor will always just measure the difference between intercom light and ambient light level.

Since the sensor will be always "on" in my design, it was important that the resistance of the LDRs is rather high so that the current through them is small. I used two 12mm GL12537 LDRs, which have about 40kOhm/4kOhm in dark/bright state, respectively. Additionally, I have added another 0-10kOhm potentiometer to be able to adjust for any differences in resistance levels between the two LDRs.

![Sensor schematic](https://github.com/RobertRol/IntercomLightSensor/blob/master/Sensor.svg)

Parts list:
* R1, R2 https://www.alibaba.com/product-detail/GL12537-1-CdS-Photoresistor-GM12537-1_1894530106.html
* C1 standard 0.1µF ceramic capacitor for noise reduction

## Transmitter
Wireless transmission of the light-triggered signal is performed via an HC-12 tranceiver module. While its range turned out to be much better than for the more cheaper Arduino RF modules, it's still not good enough so that I can completely close the boxes where I have put the sensor-transmitter and receiver electronics.

One important thing that I found out is that one should not use digital pins 0 and 1 of the ATmega328P-PU to connect it to the HC-12 module because these are reserved for the serial interface. Connecting via them would interfere with the SoftwareSerial library that I will be using for the RF communication.

Another very important point is that I connected the microcontroller's AREF pin to Vss. If the AREF pin is connected to an external voltage source, the source code must include the analogReference(EXTERNAL) command. Otherwise, the microcontroller might be damaged.

In a nutshell, the sensor-transmitter code performs the following steps:

1. The ATmega328P-PU microcontroller will be woken up from the SLEEP_MODE_PWR_DOWN state via a watchdog timer every 256ms.
2. It will then take 5 subsequent analog readings of the voltage drop across the ambient LDR. The readings are filtered using an exponentially-weighted moving average in order to reduce noise and to allow for the internal analogRead circuit to adjust to the sensor impedance.
3. The filtered value is compared to the last stored value. If there is a "significant" change, the HC-12 module is woken up from its sleep mode and a signal is sent to the receiver (repeated 2 times with a delay of 1000ms). Otherwise, the HC-12 stays in sleep mode. The HC-12 module is sent back to sleeping mode (22µA idle current), after the signal was transmitted.

The trasmission electronics is depicted in the left-hand-side part of the schematic given below.

![Sensor-Transmitter schematic](https://github.com/RobertRol/IntercomLightSensor/blob/master/SensorTransmitter.svg)

## Receiver
The receiver module uses another HC-12 module to detect the signals sent from the sensor-transmitter.

1. In order to save energy, the microprocessor of the receiver is put into SLEEP_MODE_PWR_DOWN and the HC-12 module is put into power saving mode (where it roughly needs 90µA).
2. Upon incoming RF signals the HC-12 module changes the voltage on its TXD pin. This voltage change can be used to wake up the ATmega microprocessor via an hardware interrupt.
3. After the ATmega microprocessor is woken up, it flashes a small 3x2 LED array for a few times.
4. Finally, the ATmega328P-PU is put back into SLEEP_MODE_PWR_DOWN mode.

![Receiver schematic](https://github.com/RobertRol/IntercomLightSensor/blob/master/Receiver.svg)

## Power Source
I used 3 serially-connected high-capacity AA rechargeable batteries as power source for both the sensor-transmitter and the receiver. This gives a voltage of approx. 3.6V and a capacity of 7500mAh. With the receiver needing roughly 90-200µA in idle mode, this battery pack has more than enough energy to run it for months. The power consumption of the sensor-transmitter is a bit harder to estimate.
