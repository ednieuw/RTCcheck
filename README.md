# RTCcheck
When using DS3231 time modules some are not functioning perfect.  
Check up to eight DS3231 RTC's using a TCA9548A I2C multiplexer and an Arduino Nano ESP32, ESP32 C3, S3 or similar. 
A small sketch is included for Arduino UNO or similar. This is perfect to use if three or more DS3231 are tested. The one that drifts can be identified easily

With this sketch and some wiring up to eight DS3231 can be tested using the NTP time server as reference.
The sketch connect with WIFI to internet and can be controlled with a BLE terminal app or the Arduino IDE serial terminal. 

![IMG_4759(1)](https://github.com/user-attachments/assets/9259d584-c785-4fab-98e2-f8d25f585b47)
Running on Arduino Nano ESP32 with DS3231 connected to 3.3V pin.

![IMG_4748(1)](https://github.com/user-attachments/assets/eb6db1a6-a77b-453d-bb0d-57268f4ecc56)

Running on Arduino Nano Every with DS3231 connected to 5V pin. (But the DS3231's also run with the 3.3V pin)
