# RTC check
When using DS3231 time modules some are not functioning perfect.  
Check up to eight DS3231 RTC's using a TCA9548A I2C multiplexer and an Arduino UNO/Nano or Arduino Nano ESP32, ESP32 C3, S3 or similar.
<img width="750" alt="Screenshot 2025-12-28 at 23 06 35" src="https://github.com/user-attachments/assets/a7ce8147-e0d5-4eff-9c21-c87657b476f4" />

When a WS2812 or SK6812 LED-strip is attached colours indicate the drift of the RTC.
An optional LDR + 10kOhm resistor can be used to regulate the LED-intensity 

This ESP32-sketch is a little overkill. Start a simple menu with 'i' from a (BLE) serial terminal.<br>
For functionality info see here: https://github.com/ednieuw/Arduino-ESP32-Nano-Wordclock

A smaller sketch is included for Arduino UNO, Nano or similar. 
This UNO compatible script has no NTP time but is perfect to use if three or more DS3231 are tested. The one that drifts can be identified easily.

![image](https://github.com/user-attachments/assets/7b3b1c57-4fd4-4453-9dec-0626fe9eb665)<br>
And that some can drift after one month can be seen above.<br>
T5 was and not temperature controlled DS1307 AT24C32<br> 
T7 was a Keyestudio DS3231<br>
The others were AT24C32 DS3231 modules.<br>
T0 was drifting seriously.<br>
<br>
<img width="900" alt="image" src="https://github.com/user-attachments/assets/bb552ae6-f178-4347-9184-caaee65aa5e3" /><br>
Breadboard design for three RTC's but can easily be expanded to eight RTC's. See pictures below.


<img width="1396" height="670" alt="image" src="https://github.com/user-attachments/assets/709c617a-a76a-4291-8e3d-0929d2effe45" /><br>
Fritzing design with Gerber files in this respority.  
Five PCB's [can be ordered from **PCBway**](https://www.pcbway.com) for ~25€ / $. 

The PCB is designed for an Arduino Nano ESP32. 

![RTCPCBv04](https://github.com/user-attachments/assets/34c2d917-9052-478f-aa89-b0baa56b681c)


With this sketch and some wiring up to eight DS3231 can be tested using the NTP time server as reference.
The sketch connect with WIFI to internet and can be controlled with a BLE terminal app or the Arduino IDE serial terminal. 
```
RTC 0: 06/09/2025 17:39:30 T: 23.75 C
RTC 1: 06/09/2025 17:39:30 T: 24.25 C
RTC 2: 06/09/2025 17:39:30 T: 23.75 C
 Clock time: 06/09/2025 17:39:29 
   NTP time: 06/09/2025 17:39:30 
```

![IMG_4759(1)](https://github.com/user-attachments/assets/9259d584-c785-4fab-98e2-f8d25f585b47)
Running on Arduino Nano ESP32 with DS3231 connected to 3.3V pin.

![IMG_4748(1)](https://github.com/user-attachments/assets/eb6db1a6-a77b-453d-bb0d-57268f4ecc56)

Running on Arduino Nano Every with DS3231 connected to 5V pin. (But the DS3231's also run with the 3.3V pin)

```
___________________________________
I Menu, II long menu
F Print time differences from NTP
! See RTC times
& Update system time all RTC times
U Update system time in RTC U0-U7
R Reset settings
@ Restart
___________________________________
Slope: 1     Min: 1     Max: 255 
BLE name: RTCtest
IP-address: 192.168.178.94/update
WIFI=On NTP=On BLE=On FastBLE=Off
26/12/2025 16:03:10 
___________________________________

___________________________________
A SSID B Password C BLE beacon name
D Date (D15012021) T Time (T132145)
E Timezone  (E<-02>2 or E<+01>-1)
F Print time differences from NTP
G Scan WIFI networks
I Info menu, II long menu 
K LDR reads/sec toggle On/Off
N Display off between Nhhhh (N2208)
P Status LED toggle On/Off
R Reset settings, @ Reset MCU
U Update system time in RTC U0-U7
--Light intensity settings (1-250)--
S Slope, L Min, M Max  (S50 L5 M200)
W WIFI X NTP& Z WPS CCC BLE + Fast BLE
RTC: ! See, & Update all RTC's
Ed Nieuwenhuijs Nov 2025
___________________________________
Slope: 1     Min: 1     Max: 255 
SSID: FRITZ!BoxEd
BLE name: RTCtest
IP-address: 192.168.178.94/update
Timezone:CET-1CEST,M3.5.0,M10.5.0/3
WIFI=On NTP=On BLE=On FastBLE=Off
Rotary=Off Membrane=Off  DS3231=Off
WS2812 strip with 14 LEDs (switch %)
Software: ESP32_RTCcheckV011.ino
ESP32 Arduino core version: 3.3.5
26/12/2025 16:03:14 
___________________________________
```
<img width="600" alt="image" src="https://github.com/user-attachments/assets/78d2f8b0-1e74-4c92-83cf-22d1a3c9b053" />
<br>
Eight RTC's, a WS2812 LED-strip and LDR to control the LED intensity. 
