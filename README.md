## Objectives
1. Detection of a light signal from an intercom (without the need of opening and/or changing the wiring of the intercom)
2. Wireless transmission of the signal to another portable device (receiver)
3. Low power consumption: both sensor-transmitter (implemented in one device) and receiver should run for several months on batteries
4. Ability to send and receive the signal across the whole apartment
5. I wanted to implement this using Arduino-like hardware

## Main source
The low power objective turned out to be really tricky.
This website https://www.gammon.com.au/power turned out to be incredibly helpful, especially the part about the low-power temperature monitor.
My main take-aways from this website were that I
1. had to go for a bare Atmel328 board implementation
2. should use the watchdog timer to periodically wake up the sleeping processor
3. should go as low as possible with processor frequency and voltage 

I also ran into some dead ends with the wireless data transmission. In the end I got it working with the HC-12 wireless module. Its documentation can be found here https://statics3.seeedstudio.com/assets/file/bazaar/product/HC-12_english_datasheets.pdf
There are also some tricks about the HC-12 wireless model which I will describe later.

## Sensor
The main part of the sensor is a voltage divider composed of two light-dependent resistors (LDRs). One of the LDRs is placed right in front of the intercom light ("intercom-light LDR"), the other one a bit away from it ("ambient-light LDR"). The voltage drop across the intercom-light LDR is then used as an analogRead input to the Atmel328 microcontroller.
The reason for using two LDRs instead of one intercom-light LDR together with a constant resistor is that the sensor should work independent of the ambient light level. By using the second ambient-light LDR, the sensor will always just measure the difference between doorbell light and ambient light level.

Since the sensor will be always "on" in my design, it was important that the resistance of the LDRs is rather high so that the current through them is small. Actually, it would be rather easy to change my design to also support LDRs with lower resistance but it turned out that this is not really necessary.
I used two 12mm GL12537 LDRs, which have about 40kOhm/4kOhm in dark/bright state, respectively. Additionally, I have added another 0-10kOhm potentiometer to be able to adjust for any differences in resistance levels between the two LDRs. But it turned that this is not necessary.

![Sensor schematic](/IntercomLightSensor/Sensor.svg)

## Transmitter
