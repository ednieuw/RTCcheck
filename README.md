# RTCcheck
When using DS3231 time modules some are not functioning perfect.  
Check up to eight DS3231 RTC's using a TCA9548A I2C multiplexer and an Arduino Nano ESP32, ESP32 C3, S3 or similar. 
When a WS2812 or SK6812 LED-strip is attached colours indicate the drift of the RTC.

A small sketch is included for Arduino UNO or similar. 
This UNO compatible script has no NTP time but is perfect to use if three or more DS3231 are tested. The one that drifts can be identified easily.

<img width="900" alt="image" src="https://github.com/user-attachments/assets/9e922c23-0284-4c15-b8a1-e6c1403f3660" />

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
! See RTC times
& Update system time all RTC times
U Update system time in RTC U0-U7
R Reset settings
@ Restart
___________________________________
Slope: 50     Min: 5     Max: 255 
BLE name: RTCtest
IP-address: 192.168.178.94/update
WIFI=On NTP=On BLE=On FastBLE=Off
06/09/2025 17:03:10 
___________________________________

RTC 0: 06/09/2025 17:03:19 T: 23.75 C
RTC 1: 06/09/2025 17:03:19 T: 24.25 C
RTC 2: 06/09/2025 17:03:19 T: 23.50 C
 Clock time: 06/09/2025 17:03:18 
   NTP time: 06/09/2025 17:03:18 

___________________________________
A SSID B Password C BLE beacon name
D Date (D15012021) T Time (T132145)
E Timezone  (E<-02>2 or E<+01>-1)
G Scan WIFI networks
I Info menu, II long menu 
P Status LED toggle On/Off
R Reset settings, @ Reset MCU
W WIFI X NTP& Z WPS CCC BLE + Fast BLE
RTC: ! See, & Update
Ed Nieuwenhuijs Sep 2025
___________________________________
Slope: 50     Min: 5     Max: 255 
SSID: FRITZ!Box
BLE name: RTCtest
IP-address: 192.168.178.94/update
Timezone:CET-1CEST,M3.5.0,M10.5.0/3
WIFI=On NTP=On BLE=On FastBLE=Off
Rotary=Off Membrane=Off  DS3231=Off
SK6812 strip with 1 LEDs (switch %)
Software: ESP32_CommV08_RTC02.ino
ESP32 Arduino core version: 2.0.17
06/09/2025 17:30:46 
```
<img width="600" alt="image" src="https://github.com/user-attachments/assets/78d2f8b0-1e74-4c92-83cf-22d1a3c9b053" />
<br>
Eight RTC's, a ES2812 LED-strip and LDR attached. 
