## Objectives

Short story:

1. Detection of a flashing light signal coming from an intercom button (without opening and/or changing the wiring of the intercom)
2. Wireless transmission of the signal to another portable device (receiver)
3. Low power consumption: both sensor-transmitter (implemented in one device) and receiver should run for several months on batteries
4. Ability to send and receive the signal across the whole apartment
5. Implementation via microcontrollers since I wanted to play around with my Arduino board

Click [here for the long story](#long-story).

![Solution](https://github.com/RobertRol/IntercomLightSensor/blob/master/pics/DesignSmall.png)

## Overview
1. [Links](#links)
2. [Sensor](#sensor)
3. [Transmitter](#transmitter)
4. [Receiver](#receiver)
5. [Power Source](#power-source)
5. [Photos](#photos)
5. [Long Story](#long-story)

Sections [Sensor](#sensor), [Transmitter](#transmitter) and [Receiver](#receiver) include rudimentary parts lists.
Sections [Transmitter](#transmitter) and [Receiver](#receiver) also include links to the respective sketchbook code.

## Links
The low power objective turned out to be really tricky. However, the website https://www.gammon.com.au/power was incredibly helpful, especially the part about the low-power temperature monitor.
My main take-aways from this website were that
1. I had to go for a bare ATmega328P-PU board implementation (rather than using an Arduino Nano or similar)
2. a watchdog timer should be used to periodically wake up the sleeping processor
3. I should go as low as possible with processor frequency and voltage 

Descriptions on how to set up a bare microprocessor board can be found on the same web page or on https://www.arduino.cc/en/Main/Standalone.

I also ran into some dead ends with the wireless data transmission. In the end, I got it working with the HC-12 wireless module. Its documentation can be found here: https://statics3.seeedstudio.com/assets/file/bazaar/product/HC-12_english_datasheets.pdf.

The ATmega328P-PU microprocessors I bought did not have a bootloader installed. I used my Elegoo Uno R3 development board to burn the bootloader onto the ATmegas. A description of the procedure and a simple bread board layout can be found here: https://www.arduino.cc/en/Tutorial/ArduinoToBreadboard

I was also using my Elegoo Uno R3 development board to upload the sketches to the microprocessors.

The ATmega328P-PU pin layout is given here: https://www.arduino.cc/en/Hacking/PinMapping168

## Sensor
The main part of the sensor is a voltage divider composed of two light-dependent resistors (LDRs). One of the LDRs is placed right in front of the intercom light ("intercom LDR"), the other one a bit away from it ("ambient LDR"). The voltage drop across the ambient LDR is then used as an analogRead input to the ATmega328P-PU microcontroller.
The reason for using two LDRs is that the sensor has to work independent of the ambient light level. By using a second (ambient) LDR, the sensor will always just measure the difference between intercom light and ambient light level.

Since the sensor will always draw current, it was important that the resistance of the LDRs is rather high in order to minimize power drain. I used two 12mm GL12537 LDRs, which have about 40kOhm/4kOhm in dark/bright state, respectively. Additionally, I have added another 0-10kOhm potentiometer to be able to adjust for any differences in resistances between the two LDRs.

![Sensor schematic](https://github.com/RobertRol/IntercomLightSensor/blob/master/Sensor.svg)

Parts list:
* R1, R2 https://www.alibaba.com/product-detail/GL12537-1-CdS-Photoresistor-GM12537-1_1894530106.html
* R3 standard 10kOhm variable resistor (actually, it turned out that the two LDR resistances are very similar so that adding a  potentiometer is not necessary)
* C1 standard 0.1µF ceramic capacitor for noise reduction

## Transmitter
Wireless transmission of the light-triggered signal is performed via an HC-12 tranceiver module. While its transmission range turned out to be much better than for the cheaper Arduino RF modules, it's still not good enough to receive the signal everywhere in my apartment. However, it works if I leave the boxes with the sensor-transmitter and receiver electronics open.

Important note about digital pins 0 and 1 of the ATmega328P-PU and the SoftwareSerial library: It seems as if you cannot use them as RX/TX pins when setting up a SoftwareSerial object, see http://forum.arduino.cc/index.php?topic=412164.0. At least I did not manage to get the RF communication working when using pins 0 and 1.

Another important note: If the AREF pin is connected to an external voltage source, the source code must include the line
```C
analogReference(EXTERNAL);
```
Otherwise, the microcontroller might be damaged!

In a nutshell, the sensor-transmitter code performs the following steps:

1. Every 256ms, the ATmega328P-PU microcontroller is woken up from state `SLEEP_MODE_PWR_DOWN` via a watchdog timer.
2. The microcontroller takes 5 subsequent analog readings of the voltage drop across the ambient LDR. The readings are filtered using an exponentially-weighted moving average in order to reduce noise and to allow for the internal analogRead circuit to adjust to the sensor impedance.
3. The filtered value is compared to the stored value of the last cycle. If there is a "significant" change, the HC-12 module is woken up from its sleep mode and a signal is sent to the receiver (repeated 2 times with a delay of 2000ms). Otherwise, the HC-12 stays in sleep mode. The HC-12 module is set back to sleeping mode (22µA idle current), after the signal was transmitted.

The trasmission electronics is depicted in the left-hand-side part of the schematic given below; the sensor is given in the right-hand-side.
Pins A4 and A5 can be used to send optional debugging messages via the I2C bus.

![Sensor-Transmitter schematic](https://github.com/RobertRol/IntercomLightSensor/blob/master/SensorTransmitter.svg)

Parts list:
* R1, R2, R3, C1 ... see parts list of [Sensor](#sensor) section above
* R4 ... 10kOhm pullup resistor
* C2, C3 ... 22pF ceramic capacitors for crystal oscillator
* C4, C5 ... 0.1µF ceramic capacitor for noise reduction
* 8MHz crystal oscillator
* HC-12 RF tranceiver module https://statics3.seeedstudio.com/assets/file/bazaar/product/HC-12_english_datasheets.pdf

## Receiver
The receiver module uses another HC-12 module to detect the signals sent from the sensor-transmitter.

1. In order to save energy, the microprocessor of the receiver is put into `SLEEP_MODE_PWR_DOWN`. The HC-12 module is also put into power saving mode `AT+FU2`, where it draws about 90µA.
2. I have noticed that upon incoming RF signals, the HC-12 module changes the voltage on its TXD pin. This voltage change can be used to wake up the ATmega microprocessor via an hardware interrupt. Information on how to use interrupts can be found here: https://www.arduino.cc/reference/en/language/functions/external-interrupts/attachinterrupt/
3. After the ATmega microprocessor is woken up, it flashes a small 3x2 LED array for a few times.
4. Finally, the ATmega328P-PU is put back into mode `SLEEP_MODE_PWR_DOWN`.

Like for the sensor-transmitter, pins A4 and A5 can be used to send optional debugging messages via the I2C bus.

![Receiver schematic](https://github.com/RobertRol/IntercomLightSensor/blob/master/Receiver.svg)

Parts list:
* basically the same as in section [Transmitter](#transmitter), but with the reset pullup resistor now denoted by R7
* R1-R6, 220 Ohm resistors
* L1-L6, red LEDs

## Power Source
I used 3 serially-connected high-capacity AA rechargeable batteries as power source for both the sensor-transmitter and the receiver. This gives a voltage of approx. 3.6V and a capacity of 7500mAh. With the receiver drawing roughly 90-200µA in idle mode, this battery pack has more than enough energy to run the receiver for months. The power consumption of the sensor-transmitter is a bit harder to estimate.

## Photos
| ![SensorTransmitter open](https://github.com/RobertRol/IntercomLightSensor/blob/master/pics/sensorTransmitter400px.png) | ![Receiver open](https://github.com/RobertRol/IntercomLightSensor/blob/master/pics/receiver400px.png) |
|:--:| :--:| 
| *Sensor-Transmitter (open)* | *Receiver (open)* |


## Long story
Working at home comes with several advantages. One of them is that I am usually around when I am expecting a package from a parcel service. However, since I like to listen to music while coding, I cannot hear the doorbell/intercom when my orders arrive at home. Here in Germany, the parcel services sometimes drop the package for pick-up at the local post office in case you are not opening the door. Riding my bike, it takes me about 15mins to get to the post office. Not to mention the long waiting line there...
So I decided to design a small device that detects when someone is using the intercom and that triggers a visual notification that can be seen while I am sitting in front of my computer.
The easiest solution would have been to open my intercom device, find the wires that go to the buzzer and to use this as an interrupt. However, I am renting my place and --cautious as I am-- potentially damaging the intercom was too risky for me. ;) Apart from the acoustic signal from the buzzer, my intercom also flashes a light when someone rings. So, I decided to detect this light signal and to transmit it _wirelessly_ to a small receiver that is just underneath my computer monitors.
