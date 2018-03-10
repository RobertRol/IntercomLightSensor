# DoorbellSensor
Arduino-based sensor for doorbell light

## The problem
Working at home comes with several advantages. One of them is that I am usually around when I am expecting a package from a parcel service. However, since I like to listen to music while coding, I cannot hear the doorbell/intercom when my orders arrive at home. Here in Germany, the parcel services sometimes drop the package at the local post office in case you are not at home (or are not opening the door because you don't hear them ringing). Riding my bike, it takes me about 15mins to get to the post office. Not to mention the long waiting line there...

## The solution
A small device that detects when someone is using the doorbell and that notifies me in front of my computer.

The easiest solution would have been to open my intercom device, find the wires that either go to the buzzer and use this as some sort of interrupt. However, I am renting my place and --cautious as I am-- potentially damaging the intercom was too much risk for me.
Apart from an acoustic signal, my doorbell also flashes a light when someone rings. So, I decided to detect this light signal and to transmit it _wirelessly_ to a small receiver that is just underneath my computer monitors. Another important requirement was that both devices (sensor-transmitter and receiver) should run on batteries and that those shouldn't need replacement more than once every few months.

### Sensor-Transmitter
Sensor and signal transmitter are implemented in one device.

The main part of the sensor is a voltage divider composed of two light-dependent resistors (LDRs). One of the LDRs is placed right in front of the doorbell light ("doorbell-light LDR"), the other one a bit away from it ("ambient-light LDR"). The voltage drop across the doorbell-light LDR is then used as an analogRead input to an Atmel microcontroller. The reason for using two LDRs instead of one doorbell-light LDR together with a constant resistor is that the sensor should work independent of the ambient light level. By using the second ambient-light LDR, the sensor will always just measure the difference between doorbell light and ambient light level.

Since the sensor will be always "on" in my design, it was important that the resistance of the LDRs is rather high so that the current through it is small. Actually, it would be rather easy to change my design to also support LDRs with lower resistance but it turned out that this is not really necessary.

As mentioned above, low power consumption was a key requirement. My initial research on the internet quickly revealed that I would have to go for a bare Atmel board rather than, for instance, an Arduino Nano.
