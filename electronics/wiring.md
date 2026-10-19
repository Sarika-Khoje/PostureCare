# Wiring and pin map 

|Part|Connection|
|-|-|
|MPU6050 VCC / GND|ESP32 3V3 / GND|
|MPU6050 SDA / SCL|GPIO21 / GPIO22|
|Vibration motor|GPIO25 -> 1k resistor -> base of NPN (2N2222 or BC547); motor between battery 3.7V and collector; 1N4007 flyback diode across the motor|
|Buzzer (active, 3.3-5V)|GPIO26 -> 1k resistor -> base of NPN; buzzer between supply and collector|
|Calibrate button|GPIO27 to GND (internal pull-up)|
|Status LED|GPIO2 (on-board LED, or external LED + 330 ohm)|
|Power|3.7V Li-ion -> TP4056 module with protection (USB-C) -> boost to 5V into ESP32 VIN, or a 3.3V LDO|



