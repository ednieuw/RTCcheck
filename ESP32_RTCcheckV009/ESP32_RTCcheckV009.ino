/* 
 Author      : Ed Nieuwenhuijs ednieuw.nl
 Changes V008: Equal to ESP32Arduino_WordClockV112.ino
 Changes V001: It runs
 Changes V002: Added functions in menu
 Changes V003: RGB LEDs added, CalcDifRTCtimes
 Changes V004: 
 Changes V005: Corrected extra output option F from menu 
 Changes V006: changed second deviation LED colours
 Changes V007: Added Title in WebPage() 
 Changes V008: Tekstprint ipv Serial.print
 Changes V009: Time functions updated. You can compile now By Arduino pin (default) 
*
********************
How to compile: 
Install Arduino ESP32 boards
Board: Arduino Nano ESP32 version 2.0.17 or ESP32 core >3.2.0 
Partition Scheme: With FAT
Pin Numbering: By Arduino pin (default)                    //By GPIO number (legacy).  'By Arduino pin (default)'
USB mode: Normal (Tiny USB)
*********************

ESP32-S3-WROOM-DevKitC-1
Option	    Value
Board:      ESP32-S3 DEV Module on ESP32 core 3.2.0 
USB         CDC on Boot	Enabled
USB Mode	  Hardware CDC and JTAG (or just Hardware CDC)
Upload Mode	UART0 / Hardware Serial
Flash Mode	QIO 80MHz (default, or leave unchanged)
Port	      Select the UART COM port (the one with a USB-to-Serial chip)

*/
// =============================================================================================================================

//------------------------------------------------------------------------------              //
// ESP32 Includes defines and initialisations
//--------------------------------------------
                      #if ESP_ARDUINO_VERSION >= ESP_ARDUINO_VERSION_VAL (3, 2, 0)            // Use EdSoftLED with ESP32 compiler until V3.2.0. Neopixel crashes>
#define USEEDSOFTLED
                      #endif
                      #ifdef USEEDSOFTLED
#include <EdSoftLED.h>         // https://github.com/ednieuw/EdSoftLED for LED strip WS2812 or SK6812 
                      #else
#include <Adafruit_NeoPixel.h> // https://github.com/adafruit/Adafruit_NeoPixel   for LED strip WS2812 or SK6812
                      #endif
#include <Preferences.h>
#include <NimBLEDevice.h>      // For BLE communication. !!!Use NimBLE version 2.x.x  https://github.com/h2zero/NimBLE-Arduino
#include <WiFi.h>              // Used for web page 
#include <WebServer.h>         // Used for web page 
#include "esp_sntp.h"          // for NTP
#include "esp_wps.h"           // For WPS
#include <Update.h>            // For Over-the-Air (OTA)
#include <ESPmDNS.h>           // To show BLEname in router
#include <DNSServer.h>         // For the web page to enter SSID and password of the WIFI router 
#include <Wire.h>              // 
#include <RTClib.h>            // Used for connected DS3231 RTC // Reference https://adafruit.github.io/RTClib/html/class_r_t_c___d_s3231.html
#include <Encoder.h>           // For rotary encoder
#include <Keypad.h>            // For 3x1 membrane keypad instead of rotary encoder by Mark Stanley & Alexander Brevig 

//------------------------------------------------------------------------------              //
// SPIFFS storage
//--------------------------------------------
Preferences FLASHSTOR;

#if defined(ARDUINO_NANO_ESP32)
//------------------------------------------------------------------------------              //
// PIN Assigments for Arduino Nano ESP32 S3
//------------------------------------------------------------------------------
 
enum DigitalPinAssignments {      // Digital hardware constants ATMEGA 328 ----
 SERRX        = D0,               // Connects to Bluetooth TX
 SERTX        = D1,               // Connects to Bluetooth RX
 encoderPinB  = D2,               //5,  // D2 left (labeled CLK on decoder)no interrupt pin (Use GPIO pin numbering for rotary encoder lib)  
 encoderPinA  = D3,               //6,  // D3 right (labeled DT on decoder)on interrupt pin
 clearButton  = D4,               //7,  // D4 switch (labeled SW on decoder)
 LED_PIN      = D5,               //8,  // D5 / GPIO 8 Pin to control colour SK6812/WS2812 LEDs (replace D5 with 8 for NeoPixel lib)
 PCB_LED_D09  = D9,               // D9
 PCB_LED_D10  = D10,              // D10
 secondsPin   = D13               //48, // D13  GPIO48 (#ifdef LED_BUILTIN  #undef LED_BUILTIN #define LED_BUILTIN 48 #endif)
 };
 
enum AnaloguePinAssignments {     // Analogue hardware constants ----
 EmptyA0      = A0,               // Empty
 EmptyA1      = A1,               // Empty
 PhotoCellPin = A2,               // LDR pin
 OneWirePin   = A3,               // OneWirePin
 SDA_pin      = A4,               // SDA pin
 SCL_pin      = A5,               // SCL pin
 EmptyA6      = A6,               // Empty
 EmptyA7      = A7};              // Empty
 #else
//------------------------------------------------------------------------------              //
// PIN Assigments for ESP32 S3 DevKitC 1
//------------------------------------------------------------------------------  
enum DigitalPinAssignments {      // Digital hardware constants ATMEGA 328 ----
 SERRX        = 44,               // D1 Connects to Bluetooth TX
 SERTX        = 43,               // D0 Connects to Bluetooth RX
 encoderPinB  = 4,                // D2 left (labeled CLK on decoder)no interrupt pin (Use GPIO pin numbering for rotary encoder lib)  
 encoderPinA  = 5,                // D3 right (labeled DT on decoder)on interrupt pin
 clearButton  = 6,                // D4 switch (labeled SW on decoder)
 LED_PIN      = 48,               // D5 / GPIO 8 Pin to control colour SK6812/WS2812 LEDs (replace D5 with 8 for NeoPixel lib)
 PCB_LED_D09  = 10,               // D9
 PCB_LED_D10  = 11,               // D10
 secondsPin   = 13,               // D13  GPIO48 (#ifdef LED_BUILTIN  #undef LED_BUILTIN #define LED_BUILTIN 48 #endif)
 };
 
enum AnaloguePinAssignments {     // Analogue hardware constants ----
 EmptyA0      = 10,               // Empty
 EmptyA1      = 11,               // Empty
 PhotoCellPin = 12,               // LDR pin
 OneWirePin   = 13,               // OneWirePin
 SDA_pin      = 8,                // SDA pin
 SCL_pin      = 9,                // SCL pin
 EmptyA6      = 16,               // Empty
 EmptyA7     =  17};              // Empty
                          #endif //defined(ARDUINO_NANO_ESP32)
//------------------------------------------------------------------------------              //
// LED
//------------------------------------------------------------------------------
// 

const uint32_t  NUM_LEDS = 14;  // Used strip is 14 LEDs long. Will now clear all the LEDs at startup

                     #ifdef USEEDSOFTLED
EdSoftLED LEDstrip ;//    = EdSoftLED();                                                         // Use EdSoftLED with ESP32 compiler V3.x.x. Neopixel crashes
EdSoftLED LED6812strip = EdSoftLED(NUM_LEDS, LED_PIN, SK6812WRGB);
EdSoftLED LED2812strip = EdSoftLED(NUM_LEDS, LED_PIN, WS2812RGB);
                      #else

Adafruit_NeoPixel LEDstrip;
Adafruit_NeoPixel LED6812strip = Adafruit_NeoPixel(NUM_LEDS, LED_PIN, NEO_GRBW + NEO_KHZ800); // NEO_RGBW
Adafruit_NeoPixel LED2812strip = Adafruit_NeoPixel(NUM_LEDS, LED_PIN, NEO_GRB  + NEO_KHZ800); // NEO_RGB NEO_GRB
                      #endif

//------------------------------------------------------------------------------              //
const uint32_t black    = 0x000000, orangered     = 0xFF4000, red        = 0xFF0000, chartreuse   = 0x7FFF00;
const uint32_t brown    = 0x503000, cyberyellow   = 0xFFD300, orange     = 0xFF8000; 
const uint32_t yellow   = 0xFFFF00, cadmiumyellow = 0xFFF600, dyellow    = 0xFFAA00, chromeyellow = 0xFFA700;
const uint32_t green    = 0x00FF00, brightgreen   = 0x66FF00, apple      = 0x80FF00, grass        = 0x00FF80;  
const uint32_t amaranth = 0xE52B50, edamaranth    = 0xFF0050, amber      = 0xFF7E00;
const uint32_t marine   = 0x0080FF, darkviolet    = 0x800080, pink       = 0xFF0080, purple       = 0xFF00FF; 
const uint32_t blue     = 0x0000FF, cerulean      = 0x007BA7, sky        = 0x00FFFF, capri        = 0x00BFFF;
const uint32_t edviolet = 0X7500BC, frenchviolet  = 0X8806CE, coquelicot = 0xFF3800;
const uint32_t greenblue= 0x00F2A0, hotmagenta    = 0xFF00BF, dodgerblue = 0x0073FF, screamingreen= 0x70FF70;
      uint32_t white    = 0xFFFFFF, lgray         = 0x666666, wgray      = 0xAAAAAA;
      uint32_t gray     = 0x333333, dgray         = 0x222222;  

//--------------------------------------------
// DS3231 CLOCK MODULE
//--------------------------------------------
#define DS3231_I2C_ADDRESS          0x68
#define DS3231_TEMPERATURE_MSB      0x11
#define DS3231_TEMPERATURE_LSB      0x12
#define TCA_ADDR 0x70                                                                         // TCA9548A I2C multiplexer adres (standaard 0x70)
#define MAX_RTC 8                                                                             // Maximaal Noof RTC modules

RTC_DS3231 RTCklok; 
bool DS3231Installed = false;                                                                 // True if the DS3231 is detected
uint32_t RTCdiffSec[MAX_RTC];                                                                 // Seconds deviating from NTP time
//------------------------------------------------------------------------------              //
// KY-040 ROTARY
//------------------------------------------------------------------------------                       
Encoder myEnc(encoderPinA, encoderPinB);                                                      // Use digital pin  for encoder    
long     Looptime             = 0;
byte     RotaryPress          = 0;                                                            // Keeps track display choice and how often the rotary is pressed.
bool     ChangeLightIntensity = false;                                                        // Increase or decrease slope light intensity        
uint32_t RotaryPressTimer     = 0;
byte     NoofRotaryPressed    = 0;

//--------------------------------------------                                                //
// One-wire keypad
//--------------------------------------------
bool     ChangeTime           = false;                                                        // Flag to change time within 60 seconds       
uint64_t KeyLooptime          = 0;
String   KeypadString         ="";

//------------------------------------------------------------------------------              //
// KEYPAD 3x1
//          -------- GND
//  R Y G   -------- Pin D8
//          -------- Pin D3
//          -------- Pin D4
// COLPIN is used as dummy pin that must  be LOW when there is input from keypad 
//--------------------------------------------
const byte ROWS   = 3; 
const byte COLS   = 1; 
const byte COLPIN = 12;                                                                       // Column that is always LOW. Mimic with a not used pin
char keys[ROWS][COLS] = {{'R'}, {'Y'}, {'G'}};
byte rowPins[ROWS] = { 2, 3, 4};                                                              // Connect to the row pinouts of the keypad
byte colPins[COLS] = {COLPIN};                                                                // Connect to the column pinouts of the keypad
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS );

//------------------------------------------------------------------------------              //
// LDR PHOTOCELL
//------------------------------------------------------------------------------
//                                                                                            //
const byte SLOPEBRIGHTNESS    = 50;                                                           // Steepness of with luminosity of the LED increases
const int  MAXBRIGHTNESS      = 255;                                                          // Maximum value in bits  for luminosity of the LEDs (1 - 255)
const byte LOWBRIGHTNESS      = 5;                                                            // Lower limit in bits of Brightness ( 0 - 255)   
byte       TestLDR            = 0;                                                            // If true LDR info is printed every second in serial monitor
int        OutPhotocell;                                                                      // stores reading of photocell;
int        MinPhotocell       = 999;                                                          // stores minimum reading of photocell;
int        MaxPhotocell       = 1;                                                            // stores maximum reading of photocell;
uint32_t   SumLDRreadshour    = 0;
uint32_t   NoofLDRreadshour   = 0;

//--------------------------------------------                                                //
// BLE   //#include <NimBLEDevice.h>
//--------------------------------------------
BLEServer *pServer      = NULL;
BLECharacteristic * pTxCharacteristic;
bool deviceConnected    = false;
bool oldDeviceConnected = false;
std::string ReceivedMessageBLE = "";
unsigned long BLEConnectedSince = 0;

#define SERVICE_UUID           "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"                         // UART service UUID
#define CHARACTERISTIC_UUID_RX "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_TX "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"

//------------------------------------------------------------------------------              //
// WIFI and webserver
//----------------------------------------
#define NOT_SET       0
#define SET           1
#define SET_AND_OK    2
#define IN_AP_NOT_SET 3
#include "Webpage.h"                                                                          // The Clock web page
#include "SoftAP.h"                                                                           // The web page to enter SSID and password of the WIFI router 
#include "OTAhtml.h"                                                                          // OTA update page
WiFiEventId_t wifiEventHandler;                                                               // To stop the interrupts or callbacks triggered by WiFi.onEvent(WiFiEvent);, you need to deregister the event handler.
bool WIFIwasConnected      = false;                                                           // Is WIFI connected?
bool apMode                = false;
bool apModeTrigger         = false;
const char* AP_SSID        = "StartEdSoft";
const char* AP_PASSWORD    = "startedsoft";
WebServer server(80);                                                                         // For OTA Over the air uploading
DNSServer dnsServer;
bool shouldReboot          = false;

//----------------------------------------                                                    //
// WPS
//----------------------------------------
#define ESP_WPS_MODE      WPS_TYPE_PBC
#define ESP_MANUFACTURER  "ESPRESSIF"
#define ESP_MODEL_NUMBER  "ESP32"
#define ESP_MODEL_NAME    "ESPRESSIF IOT"
#define ESP_DEVICE_NAME   "ESP STATION"
static esp_wps_config_t config;

//------------------------------------------------------------------------------              //
// Common
//----------------------------------------
#define   MAXTEXT 255
char      sptext[MAXTEXT];                                                                    // For common print use 
bool      LEDsAreOff        = false;                                                          // If true LEDs are off except time display
bool      NoTextInLeds      = false;                                                          // Flag to control printing of the text in function ColorLeds()
byte      lastminute = 0, lasthour = 0, lastday = 0;
int       Previous_LDR_read = 512;                                                            // The actual reading from the LDR + 4x this value /5
uint32_t  Loopcounter       = 0;
static    uint32_t msTick;                                                                    // Number of millisecond ticks since we last incremented the second counter
struct    tm timeinfo;                                                                        // Storage of time 
struct    EEPROMstorage {                                                                     // Data storage in EEPROM to maintain them after power loss
  byte DisplayChoice    = 0;
  byte TurnOffLEDsAtHH  = 0;
  byte TurnOnLEDsAtHH   = 0;
  byte LanguageChoice   = 0;
  byte LightReducer     = 0;
  int  LowerBrightness  = 0;
  int  UpperBrightness  = 0;
  int  NVRAMmem[24];                                                                          // LDR readings
  byte BLEOn            = 1;
  byte NTPOn            = 1;
  byte WIFIOn           = 1;  
  byte StatusLEDOn      = 1;
  int  MCUrestarted     = 0;                                                                  // No of times WIFI reconnected 
  byte ByteFuture5      = 0;
  byte UseRotary        = 0;                                                                  // Use coding for Rotary encoder ==1 or 3x1 membrane keypad ==2
  byte UseDS3231        = 0;                                                                  // Use the DS3231 time module 
  byte LEDstrip         = 0;                                                                  // 0 = SK6812 LED strip. 1 = WS2812 LED strip
  byte ByteFuture7      = 0;                                                                  // true = Fibonacci, false = chrono clock display
  byte ByteFuture8      = 0;                                                                  // 0 = Normal, 1 = Extreme, 2 = Ultimate display of colours
  int  WIFIcredentials  = 0;                                                                  // Status of the WIFI connection. SSID&PWD set or in AP mode
  int  IntFuture2       = 0;                                                                  // For future use
  int  IntFuture3       = 0;                                                                  // For future use
  byte ByteFuture1      = 0;                                                                  // Turn On or Off HET IS WAS   
  byte ByteFuture2      = 0;                                                                  // EdSoft text on/off   
  byte ByteFuture3      = 0;                                                                  // For future use 
  byte ByteFuture4      = 0;                                                                  // For future use   
  byte UseBLELongString = 0;                                                                  // Send strings longer than 20 bytes per message. Possible in IOS app BLEserial Pro 
  uint32_t OwnColour    = 0;                                                                  // Self defined colour for clock display
  uint32_t DimmedLetter = 0;
  uint32_t BackGround   = 0;
  char SSID[30];                                                                              // 
  char Password[40];                                                                          // 
  char BLEbroadcastName[30];                                                                  // Name of the BLE beacon
  char Timezone[50];
  int  Checksum        = 0;
}  Mem; 
//--------------------------------------------                                                //
// Menu
//0        1         2         3         4
//1234567890123456789012345678901234567890----  
bool LastMenuformat = true;                                                                   // Small=true of full=false menu
char menu[][40] = {
 "A SSID B Password C BLE beacon name",
 "D Date (D15012021) T Time (T132145)",
 "E Timezone  (E<-02>2 or E<+01>-1)",
 "F Print time differences from NTP",
 "G Scan WIFI networks",
 //"H H001 rotary, H002 membrane (H000)", 
 "I Info menu, II long menu ",
 //"J Toggle use DS3231 RTC module",
 "K LDR reads/sec toggle On/Off", 
 "N Display off between Nhhhh (N2208)",
 "P Status LED toggle On/Off", 
 "R Reset settings, @ Reset MCU",
 "U Update system time in RTC U0-U7",
 "--Light intensity settings (1-250)--",
 "S Slope, L Min, M Max  (S50 L5 M200)",
 "W WIFI X NTP& Z WPS CCC BLE + Fast BLE",
 "RTC: ! See, & Update all RTC's",
 "Ed Nieuwenhuijs Sep 2025" };
 
 char menusmall[][40] = {
 "I Menu, II long menu",
 "F Print time differences from NTP",
 "! See RTC times",
 "& Update system time all RTC times",
 "U Update system time in RTC U0-U7",
 "R Reset settings",
 "@ Restart" };
//  -------------------------------------   End Definitions  ---------------------------------------

//--------------------------------------------                                                //
// ARDUINO Setup
//--------------------------------------------
void setup() 
{
 Serial.begin(115200);                                                                        // Setup the serial port to 115200 baud
 Wire.begin();
 int32_t Tick = millis(); 
 SetStatusLED(10,0,0);                                                                        // Set the status LED to red
 InitStorage();                                                                               // Load settings from storage and check validity   
 StartLeds();                                                                                 // LED RainbowCycle  
 while (!Serial) {if((millis()-Tick)>5000) break; LEDstartup(orangered); delay(1000); }      // Wait max 5 second to establish serial connection
 LEDstartup(capri); Tekstprintln("Serial started\nStored settings loaded\nLED strip started");// InitStorage and StartLEDs must be called first
 Mem.MCUrestarted++;                                                                          // MCU Restart counter     
 StoreStructInFlashMemory();   
 Tekstprint("Sketch compiled for board: ");
 Tekstprintln(ARDUINO_BOARD);                                                               // 
                      #ifdef USEEDSOFTLED
 Tekstprintln("Using EDSOFTLED library");
                      #else
 Tekstprintln("Using NEOPIXEL library");
                      #endif
 if(Mem.MCUrestarted>5) { Reset();  ResetCredentials(); }                                     // If the MCU restarts so often Reset all 
 if(Mem.UseRotary==1) {LEDstartup(pink);  InitRotaryMod(); Tekstprintln("Rotary available"); }// Start the Rotary encoder
 if(Mem.UseRotary==2) {LEDstartup(grass); InitKeypad3x1(); Tekstprintln("Keypad available"); }// Start the Keypad 3x1 
 InitDS3231Mod();     LEDstartup(dyellow);                 Tekstprintln("DS3231 RTC started");// Start the DS3231 RTC-module even if not installed. It can be turned it on later in the menu
 if(Mem.BLEOn)     { LEDstartup(blue); StartBLEService();  Tekstprintln("BLE started"); }     // Start BLE service // Set the status LED to blue
 if(Mem.WIFIOn)    { LEDstartup(darkviolet); ConnectWIFI();Tekstprintln("WIFI started");}     // Start WIFI and optional NTP if Mem.WIFIOn = 1 
 Previous_LDR_read = ReadLDR();                                                               // Set the initial LDR reading
 GetTijd(true); Tekstprintln("");                                                             // Get the time and print it
 setupRTCs();                                                                                 // RTC8 function
 SWversion();                                                                                 // Print the menu + version 
 LEDstartup(green);                                                                           // Set the status LED to green                                  
 Mem.MCUrestarted = 0;                                                                        // Startup went well; Set MCUrestart counter to 0    
 StoreStructInFlashMemory();                                                                  // 
 msTick = millis();                                                                           // start the seconds loop counter
}

//--------------------------------------------                                                //
// ARDUINO Loop
//--------------------------------------------
void loop() 
{
 Loopcounter++;                                                                               // a counter to check the speed of the loop
 CheckDevices();                                                                              // Check input from devices
 EverySecondCheck();                                                                          // Enter the command structure
// Every5SecondCheck(); 
}

//--------------------------------------------                                                //
// COMMON Check connected input devices
//--------------------------------------------
void CheckDevices(void)
{
 CheckBLE();                                                                                  // Something with BLE to do?
 SerialCheck();                                                                               // Check serial port every second 
 if (Mem.UseRotary==1) RotaryEncoderCheck(); 
 if (Mem.UseRotary==2) Keypad3x1Check();
}
//--------------------------------------------                                                //
// COMMON Update routine 
// Performs tasks every second
//--------------------------------------------
void EverySecondCheck(void)
{
 static int Toggle = 0;
 uint32_t msLeap = millis() - msTick;                                                         // 
 if (msLeap >999)                                                                             // Every second enter the loop
 {
  msTick = millis();
  GetTijd(false);                                                                             // Get the time for the seconds 
  Toggle = 1-Toggle;                                                                          // Used to turn On or Off Leds
  UpdateStatusLEDs(Toggle);
  DimLeds(TestLDR);                                                                           // Every second an intensity check and update from LDR reading 
  if (shouldReboot) { delay(1000);   ESP.restart(); }                                         // After finish OTA update restart
  if (timeinfo.tm_min != lastminute) EveryMinuteUpdate();                                     // Enter the every minute routine after one minute; 
  Loopcounter=0;
 }  
}
//--------------------------------------------                                                //
// COMMON Update routine done every minute
//-------------------------------------------- 
void EveryMinuteUpdate(void)
{   
lastminute = timeinfo.tm_min;  
 CheckRestoreWIFIconnectivity();                                                              // Check if WIFI is sill connected and if not restore it
 GetTijd(false);
 Displaytime();  
 DimLeds(true);   
 if(timeinfo.tm_hour != lasthour) EveryHourUpdate(); 
}
//--------------------------------------------                                                //
// COMMON Update routine done every hour
//--------------------------------------------
void EveryHourUpdate(void)
{
 lasthour = timeinfo.tm_hour;
 if (!Mem.StatusLEDOn) SetStatusLED(0,0,0);                                                   // If for some reason the LEDs are ON and after a MCU restart turn them off.  
 if( (timeinfo.tm_hour == Mem.TurnOffLEDsAtHH) && (Mem.TurnOffLEDsAtHH != Mem.TurnOnLEDsAtHH))
       { LEDsAreOff = true;  ClearScreen(); }                                                 // Is it time to turn off the LEDs?
 if(timeinfo.tm_hour == Mem.TurnOnLEDsAtHH)
   { LEDsAreOff = false;   lastminute = 99;     Displaytime(); }                              // Force a minute update
 Mem.NVRAMmem[lasthour] =(byte)((SumLDRreadshour / NoofLDRreadshour?NoofLDRreadshour:1));     // Update the average LDR readings per hour
 SumLDRreadshour  = 0;
 NoofLDRreadshour = 0;
 if (timeinfo.tm_mday != lastday) EveryDayUpdate();  
}
//--------------------------------------------                                                //
// COMMON Update routine done every day
//--------------------------------------------
void EveryDayUpdate(void)
{
 lastday           = timeinfo.tm_mday; 
 Previous_LDR_read = ReadLDR();                                                            // to have a start value and reset the Min Max measurements 
 MinPhotocell      = Previous_LDR_read;                                                    // Stores minimum reading of photocell;
 MaxPhotocell      = Previous_LDR_read;                                                    // Stores maximum reading of photocell;
 if(DS3231Installed && !Mem.UseDS3231)                                                        // If on NTP time and DS3231 installed set the correct the time
   {
    sprintf(sptext, "Time set in external RTC module");
    SetDS3231Time();
    PrintDS3231Time();
  } 
 StoreStructInFlashMemory();                                                                 // Update Mem struct once a day to store Mem.NVRAM measurementd
}


//--------------------------------------------                                                //
// RTC8 
//-------------------------------------------- 

#define FILENAAM (strrchr(__FILE__, '\\') ? strrchr(__FILE__, '\\') + 1 : __FILE__) 

RTC_DS3231 rtc[MAX_RTC];                                             // RTC objecten aanmaken
bool rtcActive[MAX_RTC] = {false};                                   // Array om bij te houden welke RTC's werkend zijn
int activeRTCCount     = 0;
//uint32_t  Loopcounter  = 0;                                          // Loop speed counter
//char sptext[255];                                                    // text print buffer

//--------------------------------------------                       //
// RTC8 Initialyse all connected RTC's
//--------------------------------------------
void setupRTCs(void) 
{
  Serial.println("DS3231 RTC Multiplexer Test");
  Serial.println("===========================");
  sprintf(sptext,"Software: %s",FILENAAM); Tekstprintln(sptext);   
  
  Serial.print("TCA9548A search on addres 0x");                      // Test or TCA9548A is available
  Serial.print(TCA_ADDR, HEX);
  Serial.print("... ");
  
  Wire.beginTransmission(TCA_ADDR);
  if (Wire.endTransmission() == 0) {
    Serial.println("FOUND!");
  } else 
  {
    Serial.println("NOT FOUND!\ncheck wiring and power");
    while(1);                                                       // 
  }  
  Serial.println("\nScanning DS3231 modules...");                   // Scan which channels a DS3231 has
  for (int i = 0; i < MAX_RTC; i++) 
   {
    selectTCAChannel(i);
    delay(50); 
    Wire.beginTransmission(0x68);                                   // DS3231 address
    uint8_t error = Wire.endTransmission();
    
    if (error == 0) {Serial.print("DS3231 found on channel "); Serial.println(i);  } 
    else {sprintf(sptext,"No DS3231 on channel %d Error : %d", i,error); Tekstprintln(sptext); }
  }
  Serial.println("\nInitialising RTC modules...");                   // Initialiseonly found RTC modules
  for (int i = 0; i < MAX_RTC; i++) 
   {
    selectTCAChannel(i);                                             // Select multiplexer channel
    delay(50);
    
    Serial.print("Initialising RTC ");
    Serial.print(i);
    Serial.print("... ");
    
    // Test eerst of er een device is
    Wire.beginTransmission(0x68);
    if (Wire.endTransmission() != 0) 
    {
      Serial.println("NO DEVICE");
      continue;
    }
    
    if (!rtc[i].begin()) 
     {
      Serial.println("FAILED!");
      Serial.println("Check connection on channel " + String(i));
      rtcActive[i] = false;
     } 
    else 
     {
      Serial.println("OK!");
      rtcActive[i] = true;
      activeRTCCount++;
      DateTime newTime = rtc[i].now();                               // Check time set
      sprintf(sptext,"It's current time: %02d/%02d/%04d %02d:%02d:%02d ", newTime.day(),newTime.month(),newTime.year(),
                                                                      newTime.hour(),newTime.minute(),newTime.second());
      Tekstprintln(sptext);  
     }
    delay(100); 
   }
  sprintf(sptext,"Setup finished! %d  RTC modules active.", activeRTCCount);
  Tekstprintln(sptext); 
}

//--------------------------------------------                       //
// RTC8 Print RTC times
//-------------------------------------------- 
void PrintRTCtimes(void)
{
  for (int i = 0; i < MAX_RTC; i++)                                  // Read time of all ACTIVE RTC modules
  {
    if (!rtcActive[i]) continue;                                     // Skip non-active RTC's
    selectTCAChannel(i);
    delay(10);
    char ff[16];
    dtostrf(rtc[i].getTemperature(), 0, 2, ff); 
    DateTime now = rtc[i].now();
    sprintf(sptext,"RTC %d: %02d/%02d/%04d %02d:%02d:%02d T: %s C",
             i, now.day(),now.month(),now.year(), now.hour(),now.minute(),now.second(), ff);
    Tekstprintln(sptext);    
  }
}
//--------------------------------------------                       //
// RTC8 Calculate difference in sec from NTP
//-------------------------------------------- 
void CalcDifRTCtimes(void)
{
for (int i = 0; i < MAX_RTC; i++) 
 {  
  if (!rtcActive[i]) {RTCdiffSec[i] = 26; continue; }
  selectTCAChannel(i);
  delay(10);
  DateTime now = rtc[i].now();

  // Build a tm struct from RTC (local time)
  struct tm rtc_tm;
  rtc_tm.tm_year = now.year() - 1900;  // tm year starts at 1900
  rtc_tm.tm_mon  = now.month() - 1;    // tm months 0-11
  rtc_tm.tm_mday = now.day();
  rtc_tm.tm_hour = now.hour();
  rtc_tm.tm_min  = now.minute();
  rtc_tm.tm_sec  = now.second();

  // Convert to Unix seconds in *local time*
  time_t tRTC = mktime(&rtc_tm);

  // NTP time already in local time
  time_t tNTP = mktime(&timeinfo);

  RTCdiffSec[i] = abs(tNTP - tRTC);
 }
} 

//--------------------------------------------                       //
// RTC8 Update the time difference in the LEDs
// 1 second sec * -10 GREEN sec * 10 RED
// BLUE is empty
// max time difference = 25 seconds. 26 = NA
//--------------------------------------------
void UpdateRTCTimeLEDs(void)
{
 CalcDifRTCtimes();                                                 // Calculate difference in sec from NTP
 for (int i = 0; i < MAX_RTC; i++)                                  // Read time of all ACTIVE RTC modules
  {
   int c;
   if (!rtcActive[i]) c = 26;                                     // Skip non-active RTC's 
   else c = RTCdiffSec[i]; 
   switch (c)
    {
      case  0 ...  1: ColorLed(i,green);        break;
      case  2 ...  4: ColorLed(i,orange);       break;
      case  5 ...  7: ColorLed(i,greenblue);    break;
      case  8 ... 10: ColorLed(i,darkviolet);   break;
      case 11 ... 17: ColorLed(i,pink);         break;
      case 18 ... 25: ColorLed(i,purple);       break;
      case 26:        ColorLed(i,blue);         break;
      default:        ColorLed(i,red); 
    }
  }
ShowLeds(); 
}

//--------------------------------------------                       //
// RTC8 Print RTC differences from NTP
//--------------------------------------------
void PrintDifRTCtimes(void)
{
 Tekstprint(" ");
 CalcDifRTCtimes();
 for (int i = 0; i < MAX_RTC; i++) 
  {  
   if (!rtcActive[i]) { continue; } 
   sprintf(sptext,"|T%d:%2ld",i,RTCdiffSec[i]);
   Tekstprint(sptext);
  }
 Tekstprint("| ");
}
//--------------------------------------------                       //
// RTC8 Function to select TCA9548A channel
//--------------------------------------------
void selectTCAChannel(uint8_t channel) 
{
  if (channel > 7) return;                                           // Maximaal 8 kanalen (0-7)
  Wire.beginTransmission(TCA_ADDR);
  Wire.write(1 << channel);                                          // Select channel by setting bit
  uint8_t error = Wire.endTransmission();
  
  if (error != 0) 
   {
    sprintf(sptext,"Error selecting channel %d Error code : %d", channel,error);
    Tekstprintln(sptext);  
  }
  delay(10);                                                         // short delay after channel change
}
//--------------------------------------------                       //
// RTC8 set Time For RTC 
//--------------------------------------------
void setTimeForRTC(int rtcIndex, DateTime newTime) 
{
  if (!rtcActive[rtcIndex]) return;
  selectTCAChannel(rtcIndex);
  rtc[rtcIndex].adjust(newTime);
}
//--------------------------------------------                       //
// RTC8 Print Time For RTC 
//--------------------------------------------
void printTimeForRTC(int rtcIndex) 
{
  if (!rtcActive[rtcIndex]) return;
  selectTCAChannel(rtcIndex);
  DateTime now = rtc[rtcIndex].now();

  sprintf(sptext,"RTC %d: %s", rtcIndex, now.timestamp(DateTime::TIMESTAMP_FULL).c_str());
  Tekstprintln(sptext);  
}
//--------------------------------------------                       //
// RTC8 get Temperature For RTC
//--------------------------------------------
float getTemperatureForRTC(int rtcIndex) 
{
  if (!rtcActive[rtcIndex]) return -999;
  selectTCAChannel(rtcIndex);
  return rtc[rtcIndex].getTemperature();
}
//--------------------------------------------                                                //
// RTC8 Synchronize all RTC's with current time
//--------------------------------------------
void synchronizeAllRTCs()   // Zoek eerste actieve RTC als referentie
{
  int referenceRTC = -1;
  for (int i = 0; i < MAX_RTC; i++) { if (rtcActive[i]) { referenceRTC = i;   break;  }  }
  if (referenceRTC == -1)           { Tekstprintln("No active RTC's found!"); return;  }
  
  selectTCAChannel(referenceRTC);
  DateTime referenceTime = getLocalDateTime() + 1;                                               // Get the current time and add 1 sec setting time  
  
  Tekstprintln("\n=== RTC SYNCHRONISATION ===");
  sprintf(sptext,"Reference time of NTP RTC %d: %s", referenceRTC, referenceTime.timestamp(DateTime::TIMESTAMP_FULL).c_str());
  Tekstprintln(sptext);

  for (int i = 0; i < MAX_RTC; i++) 
   {
    if (!rtcActive[i]) continue;
    selectTCAChannel(i);
    delay(50);
    DateTime before = rtc[i].now();                                  // Show time before update
    sprintf(sptext,"RTC %d: before sync %s", i, before.timestamp(DateTime::TIMESTAMP_FULL).c_str());
    Tekstprintln(sptext);
    rtc[i].adjust(referenceTime);                                    // Update time
    delay(50); 
    DateTime after = rtc[i].now();                                   // Verify
    sprintf(sptext,"RTC %d:  after sync %s", i, after.timestamp(DateTime::TIMESTAMP_FULL).c_str());
    Tekstprintln(sptext);
  }
 Tekstprintln("Synchronisation done!\n");
}

//--------------------------------------------                       //
// RTC8 Set time by hand in serial monitor
//--------------------------------------------
void setCurrentTime() 
{
 int year, month, day, hour, minute, second;                        //
 hour   = timeinfo.tm_hour;
 minute = timeinfo.tm_min;
 second = timeinfo.tm_sec;
 year   = timeinfo.tm_year + 1900;                                                      // Inow.year() is years since 2000 tm_year is years since 1900
 month  = timeinfo.tm_mon  + 1;
 day    = timeinfo.tm_mday;
 DateTime newTime(year, month, day, hour, minute, second);
 Tekstprint("New time: ");
 Tekstprintln(newTime.timestamp(DateTime::TIMESTAMP_FULL).c_str());
 for (int i = 0; i < MAX_RTC; i++) 
    {
     if (!rtcActive[i]) continue;
     selectTCAChannel(i);
     delay(50);      
     DateTime before = rtc[i].now();                                // Show time before update
     sprintf(sptext,"RTC %d: before sync %s", i, before.timestamp(DateTime::TIMESTAMP_FULL).c_str());
     Tekstprintln(sptext);
     rtc[i].adjust(newTime);
     delay(50);
     DateTime after = rtc[i].now();                                 // Verify
     sprintf(sptext,"RTC %d: after sync %s", i, after.timestamp(DateTime::TIMESTAMP_FULL).c_str());
     Tekstprintln(sptext);
    }
 Tekstprintln("All active RTC's updated!\n");
}

//--------------------------------------------                                                //
// COMMON Update routine for the status LEDs
//-------------------------------------------- 
void UpdateStatusLEDs(int Toggle)
{
 if(Mem.StatusLEDOn)   
   {
    SetStatusLED((Toggle && WiFi.localIP()[0]==0) * 20, 
                 (Toggle && WiFi.localIP()[0]!=0) * 20 , 
                 (Toggle &&      deviceConnected) * 20);
    SetPCBLED09( Toggle * 10);                                                                // Left LED
    SetPCBLED10((1-Toggle) * 10);                                                             // Right LED
    SetNanoLED13((1-Toggle) * 50);                                                            // LED on ESP32 board
   }
   else
   {
    SetStatusLED(0, 0, 0); 
    SetPCBLED09(0);                                                                           //
    SetPCBLED10(0);                                                                           //
    SetNanoLED13(0);      
   }
}
//--------------------------------------------                                                //
// COMMON Control the RGB LEDs on the Nano ESP32
// Analog range 0 - 512. 0 is LED On max intensity
// 512 is LED off. Therefore the value is subtracted from 512 
//--------------------------------------------
void SetStatusLED(uint16_t Red, uint16_t Green, uint16_t Blue)
{
// For ESP32 with WS2812 LED   
// ColorLed(0,FuncCRGBW( Red,  Green,  Blue, 0)); 
// ShowLeds(); 

// For Arduino Nano ESP32 with  RGB LED 
 analogWrite(LED_RED,   512 - Red);                                                           // !Red (not Red) because 1 or HIGH is LED off
 analogWrite(LED_GREEN, 512 - Green);
 analogWrite(LED_BLUE,  512 - Blue);

}
//--------------------------------------------                                                //
// COMMON Control orange LED D13 on the Arduino 
//--------------------------------------------
void SetNanoLED13(int intensity) {analogWrite(secondsPin, intensity);}
//--------------------------------------------                                                //
// COMMON Control the RGB LED on the PCB
//--------------------------------------------
void SetPCBLED09(int intensity) {analogWrite(PCB_LED_D09, intensity);}
void SetPCBLED10(int intensity) {analogWrite(PCB_LED_D10, intensity);}

//--------------------------------------------                                                //
// COMMON check for serial input
//--------------------------------------------
void SerialCheck(void)
{
 String SerialString; 
 while (Serial.available())
    { 
     char c = Serial.read();                                                                  // Serial.write(c);
     if (c>31 && c<128) SerialString += c;                                                    // Allow input from Space - Del
     else c = 0;                                                                              // Delete a CR
    }
 if (SerialString.length()>0) 
    {
     ReworkInputString(SerialString);                                                         // Rework ReworkInputString();
     SerialString = "";
    }
}

//--------------------------------------------                                                //
// COMMON Reset to default settings. BLE On, WIFI NTP Off
//--------------------------------------------
void Reset(void)
{
 Mem.Checksum         = 25065;                                                                //
 Mem.LightReducer     = SLOPEBRIGHTNESS;                                                      // Factor to dim ledintensity with. Between 0.1 and 1 in steps of 0.05
 Mem.UpperBrightness  = MAXBRIGHTNESS;                                                        // Upper limit of Brightness in bits ( 1 - 1023)
 Mem.LowerBrightness  = LOWBRIGHTNESS;                                                        // Lower limit of Brightness in bits ( 0 - 255)
 Mem.BLEOn            = 1;                                                                    // default BLE On
 Mem.UseBLELongString = 0;                                                                    // Default off. works only with iPhone/iPad with BLEserial app
 Mem.NTPOn            = 0;                                                                    // NTP default off
 Mem.WIFIOn           = 0;                                                                    // WIFI default off
 Mem.MCUrestarted     = 0;                                                                    // MCU Restart counter
 Mem.WIFIcredentials  = NOT_SET;                                                              // Status of the WIFI connection
 //Mem.UseRotary      = 0;    // Do not erase this setting with a reset                       // Use the rotary coding
 Mem.UseDS3231        = 0;                                                                    // Default off
 //Mem.LEDstrip       = 0;    // Do not erase this setting with a reset                       // 0 = SK6812, 1=WS2812
 Previous_LDR_read    = ReadLDR();                                                            // Read LDR to have a start value. max = 4096/8 = 255
 MinPhotocell         = Previous_LDR_read;                                                    // Stores minimum reading of photocell;
 MaxPhotocell         = Previous_LDR_read;                                                    // Stores maximum reading of photocell;                                            
 TestLDR              = 0;                                                                    // If true LDR display is printed every second
 Tekstprintln("**** Reset of preferences ****"); 
 StoreStructInFlashMemory();                                                                  // Update Mem struct       
 GetTijd(false);                                                                              // Get the time and store it in the proper variables
 SWversion();                                                                                 // Display the version number of the software
 Displaytime();
}
//--------------------------------------------                                                //
// COMMON Reset to empty credential settings WIFI, NTP, BLE ON
//--------------------------------------------
void ResetCredentials(void)
{
 strcpy(Mem.SSID,"");                                                                         // Default SSID
 strcpy(Mem.Password,"");                                                                     // Default password
 strcpy(Mem.BLEbroadcastName,"ESP32Nano");
 strcpy(Mem.Timezone,"CET-1CEST,M3.5.0,M10.5.0/3");                                           // Central Europe, Amsterdam, Berlin etc.
 Mem.WIFIcredentials  = 0;                                                                    // Status of the WIFI connection  
 Mem.WIFIOn           = 1;                                                                    // WIFI on
 Mem.NTPOn            = 1;
 Mem.BLEOn            = 1;                                                                    // default BLE On
 StoreStructInFlashMemory();                                                                  // Update Mem struct   
}
//--------------------------------------------                                                //
// COMMON common print routines
//--------------------------------------------
void Tekstprint(char const *tekst)    { if(Serial) Serial.print(tekst);  SendMessageBLE(tekst); } //sptext[0]=0; } 
void Tekstprintln(char const *tekst)  { sprintf(sptext,"%s\n",tekst); Tekstprint(sptext); }
void TekstSprint(char const *tekst)   { printf(tekst); } //sptext[0]=0;}                      // printing for Debugging purposes in serial monitor 
void TekstSprintln(char const *tekst) { sprintf(sptext,"%s\n",tekst); TekstSprint(sptext); }
//--------------------------------------------                                                //
// COMMON Print web menu page and BLE menu
// 0 = text to print, 1 = header of web page with menu, 2 = footer of web page
//  html_info but be empty before starting: --> html_info[0] = 0; 
//--------------------------------------------
void WTekstappend(char const *tekst, char const *prefixtekst, char const *suffixtekst, bool newline) 
{
    if (newline) { sprintf(sptext, "%s\n", tekst); } 
    else {         sprintf(sptext, "%s", tekst);   }
    Tekstprint(sptext);
    size_t needed = strlen(prefixtekst) + strlen(tekst) + strlen(suffixtekst) + strlen("<br>");// Estimate how much space will be added
    if (strlen(html_info) + needed > MAXSIZE_HTML_INFO - 1) 
       { strcat(html_info, "<br> *** Increase MAXSIZE_HTML_INFO ***<br>");   return;  }
    strcat(html_info, prefixtekst);                                             
    strcat(html_info, tekst);
    strcat(html_info, suffixtekst);
    if (newline) { strcat(html_info, "<br>"); }   // Append to html_info
}

void WTekstprintln(char const *tekst) { WTekstappend(tekst, "", "", true);}
void WTekstprintln(char const *tekst, char const *prefixtekst, char const *suffixtekst) 
                                      { WTekstappend(tekst, prefixtekst, suffixtekst, true); }

void WTekstprint(char const *tekst)   { WTekstappend(tekst, "", "", false);}
void WTekstprint(char const *tekst, char const *prefixtekst, char const *suffixtekst) 
                                      { WTekstappend(tekst, prefixtekst, suffixtekst, false);}

//--------------------------------------------                                                //
// COMMON Constrain a string with integers
// The value between the first and last character in a string is returned between the low and up bounderies
//--------------------------------------------
int SConstrainInt(String s,byte first,byte last,int low,int up){return constrain(s.substring(first, last).toInt(), low, up);}
int SConstrainInt(String s,byte first,          int low,int up){return constrain(s.substring(first).toInt(), low, up);}
//--------------------------------------------                                                //
// COMMON Init and check contents of EEPROM
//--------------------------------------------
void InitStorage(void)
{
 // if(!SPIFFS.begin(FORMAT_SPIFFS_IF_FAILED)){ Tekstprintln("Card Mount Failed");   return;}
 // else Tekstprintln("SPIFFS mounted"); 

 GetStructFromFlashMemory();
 if( Mem.Checksum != 25065)
   {
    sprintf(sptext,"Checksum (25065) invalid: %d\n Resetting to default values",Mem.Checksum); 
    Tekstprintln(sptext); 
    Reset();                                                                                  // If the checksum is NOK the Settings were not set
   }
 Mem.LightReducer    = constrain(Mem.LightReducer,1,250);                                     // 
 Mem.LowerBrightness = constrain(Mem.LowerBrightness, 1, 250);                                // 
 Mem.UpperBrightness = _min(Mem.UpperBrightness, 255); 
 if (strlen(Mem.BLEbroadcastName)<5) strcpy(Mem.BLEbroadcastName,"ESP32test");
 if(Mem.LEDstrip  > 1) Mem.LEDstrip = 0;                                                      // Default SK6812 
 StoreStructInFlashMemory();
}
//--------------------------------------------                                                //
// COMMON Store mem.struct in FlashStorage or SD
// Preferences.h  
//--------------------------------------------
void StoreStructInFlashMemory(void)
{
  FLASHSTOR.begin("Mem",false);       //  delay(100);
  FLASHSTOR.putBytes("Mem", &Mem , sizeof(Mem) );
  FLASHSTOR.end();          
  
// Can be used as alternative
//  SPIFFS
//  File myFile = SPIFFS.open("/MemStore.txt", FILE_WRITE);
//  myFile.write((byte *)&Mem, sizeof(Mem));
//  myFile.close();
 }
//--------------------------------------------                                                //
// COMMON Get data from FlashStorage
// Preferences.h
//--------------------------------------------
void GetStructFromFlashMemory(void)
{
 FLASHSTOR.begin("Mem", false);
 FLASHSTOR.getBytes("Mem", &Mem, sizeof(Mem) );
 FLASHSTOR.end(); 

// Can be used as alternative if no SD card
//  File myFile = SPIFFS.open("/MemStore.txt");  FILE_WRITE); myFile.read((byte *)&Mem, sizeof(Mem));  myFile.close();

 sprintf(sptext,"Mem.Checksum = %d",Mem.Checksum);Tekstprintln(sptext); 
}

//--------------------------------------------                                                //
// COMMON Version info
//--------------------------------------------
void SWversion(void) {SWversion(LastMenuformat);}                                             // LastMenuformat is default after startup true (= small)   
void SWversion(bool Small) 
{ 
 #define FILENAAM (strrchr(__FILE__, '\\') ? strrchr(__FILE__, '\\') + 1 : __FILE__)    
 html_info[0] = 0;                                                                           // Empty the info web page to be used in void WTekstprintln() 
 LastMenuformat = Small;
 PrintLine(35);
 if(Small) {for (uint8_t i = 0; i < sizeof(menusmall) / sizeof(menusmall[0]); WTekstprintln(menusmall[i++]) ); }
 else      {for (uint8_t i = 0; i < sizeof(menu) / sizeof(menu[0]);           WTekstprintln(menu[i++]) ); }                                     
 PrintLine(35);
 sprintf(sptext,"Slope: %d     Min: %d     Max: %d ",
                 Mem.LightReducer, Mem.LowerBrightness,Mem.UpperBrightness);                    WTekstprintln(sptext);
 if(!Small) {sprintf(sptext,"SSID: %s", Mem.SSID);                                               WTekstprintln(sptext); }
// sprintf(sptext,"Password: %s", Mem.Password);                                                WTekstprintln(sptext);
 sprintf(sptext,"BLE name: %s", Mem.BLEbroadcastName);                                          WTekstprintln(sptext,"<span class=\"verdana-red\">","</span>");
 sprintf(sptext,"IP-address: %d.%d.%d.%d", WiFi.localIP()[0], WiFi.localIP()[1], 
                                           WiFi.localIP()[2], WiFi.localIP()[3] );              WTekstprint(sptext);
 sprintf(sptext,"/update");                                                                     WTekstprintln(sptext," <a href=" , "> /update</a>");                                            
 if(!Small) {sprintf(sptext,"Timezone:%s", Mem.Timezone);                                       WTekstprintln(sptext); }
 sprintf(sptext,"%s %s %s %s", Mem.WIFIOn?"WIFI=On":"WIFI=Off", 
                               Mem.NTPOn? "NTP=On":"NTP=Off",
                               Mem.BLEOn? "BLE=On":"BLE=Off",
                               Mem.UseBLELongString? "FastBLE=On":"FastBLE=Off" );              WTekstprintln(sptext);
 char fftext[20];              
 if(!Small) {sprintf(fftext,"%s", Mem.UseDS3231?" DS3231=On":" DS3231=Off"); }
 if(!Small) {sprintf(sptext,"%s %s",Mem.UseRotary==0 ?"Rotary=Off Membrane=Off":
                        Mem.UseRotary==1 ?"Rotary=On Membrane=Off":
                        Mem.UseRotary==2 ?"Rotary=Off Membrane On":"NOP",fftext);               WTekstprintln(sptext); }                           
if(!Small) { sprintf(sptext,"%s strip with %d LEDs (switch %%)", 
                 Mem.LEDstrip==0?"SK6812":Mem.LEDstrip==1?"WS2812":"NOP",(int) NUM_LEDS);       WTekstprintln(sptext); }
  if(!Small) {sprintf(sptext,"Software: %s",FILENAAM);                                          WTekstprintln(sptext);}  // VERSION);
  if(!Small) {sprintf(sptext,"ESP32 Arduino core version: %d.%d.%d", 
          ESP_ARDUINO_VERSION_MAJOR,ESP_ARDUINO_VERSION_MINOR,ESP_ARDUINO_VERSION_PATCH);       WTekstprintln(sptext); }
 GetTijd(false);                                                                              // Get the time and store it in the proper variables
 PrintRTCTime();                                                                                
 PrintLine(35);                                                                               //
 
}
//--------------------------------------------                                                //
// COMMON PrintLine
//--------------------------------------------
void PrintLine(byte Lengte)
{
 for(int n=0; n<Lengte; n++) sptext[n]='_';
 sptext[Lengte] = 0;
 WTekstprintln(sptext);
 sptext[0] = 0;
}

//--------------------------------------------                                                //
// COMMON Input from Bluetooth, Serial or HTML page
//--------------------------------------------
void ReworkInputString(String InputString)
{
 if(InputString.length()> 40){Serial.printf("Input string too long (max40)\n"); return;}      // If garbage return
 InputString.trim();                                                                          // Remove CR, LF etc.
 sptext[0] = 0;   
if(InputString[0] > 31 && InputString[0] < 127)                                               // Does the string start with a letter?
  {
  char cmd = toupper(InputString[0]);                                                         // Convert to uppercase once
  bool validLength = false;
  int len = InputString.length();
  switch(cmd) 
   {
    case 'A':                                                                                 // SSID setting
      validLength = (len > 4 && len < 30);
      if(validLength) 
        {
        InputString.substring(1).toCharArray(Mem.SSID, len);
        sprintf(sptext, "SSID set: %s", Mem.SSID);
        Mem.WIFIcredentials = NOT_SET;    
        WIFIwasConnected = false;           
        } 
      else sprintf(sptext, "**** Length fault. Use between 4 and 30 characters ****");
      break;
      
    case 'B':                                                                                 // Password setting
      if(InputString.equals("BBBB")) 
      {
        sprintf(sptext, "%s,**** Length fault. Use between 5 and 40 characters ****", Mem.Password);
        break;
      }
      validLength = (len > 4 && len < 40);
      if(validLength) 
        {
        InputString.substring(1).toCharArray(Mem.Password, len);
        sprintf(sptext, "Password set: %s\n Enter @ to reset ESP32 and connect to WIFI and NTP\n WIFI and NTP are turned ON", Mem.Password);
        Mem.NTPOn = Mem.WIFIOn = 1;                                                          // Turn both on
        Mem.WIFIcredentials = NOT_SET;    
        WIFIwasConnected = false;   
        } 
      else sprintf(sptext, "**** Length fault. Use between 5 and 40 characters ****");
      break;

    case 'C':                                                                                // BLE settings
      if(InputString.equals("CCC")) 
       {
        Mem.BLEOn = 1 - Mem.BLEOn;
        sprintf(sptext, "BLE is %s after restart", Mem.BLEOn ? "ON" : "OFF");
        break;
       }
      validLength = (len > 4 && len < 30);
      if(validLength) 
       {
        InputString.substring(1).toCharArray(Mem.BLEbroadcastName, len);
        sprintf(sptext, "BLE broadcast name set: %s", Mem.BLEbroadcastName);
        Mem.BLEOn = 1;
       } 
      else sprintf(sptext, "**** Length fault. Use between 4 and 30 characters ****");
      break;
      
    case 'D':                                                                                 // Date entry
      if(len == 9) 
       {
        timeinfo.tm_mday = (int)SConstrainInt(InputString, 1, 3, 0, 31);
        timeinfo.tm_mon = (int)SConstrainInt(InputString, 3, 5, 0, 12) - 1;
        timeinfo.tm_year = (int)SConstrainInt(InputString, 5, 9, 2000, 9999) - 1900;
        if(DS3231Installed) 
         {
          sprintf(sptext, "Time set in external RTC module");
          SetDS3231Time();
          PrintDS3231Time();
         } 
        else sprintf(sptext, "No external RTC module detected");
       } 
      else sprintf(sptext, "****\nLength fault. Enter Dddmmyyyy\n****");
      break;
      
    case 'E':                                                                                 // Time zone setting
      validLength = (len > 2);
      if(validLength) 
       {
        InputString.substring(1).toCharArray(Mem.Timezone, len);
        sprintf(sptext, "Timezone set: %s", Mem.Timezone);
       } 
      else sprintf(sptext, "**** Length fault. Use more than 2 characters ****");
      break;

    case 'F':                                                                                 // Scan WIFI stations
      if(len == 1) 
       {
        CalcDifRTCtimes();
        PrintDifRTCtimes();
        sptext[0]=0;                                                                         // Clear last input in sptext
       } 
      else sprintf(sptext, "**** Length fault. Enter F ****");
      break;

    case 'G':                                                                                 // Scan WIFI stations
      if(len == 1) 
       {
        ScanWIFI();
        if(WIFIwasConnected) WiFi.reconnect();
       } 
      else sprintf(sptext, "**** Length fault. Enter G ****");
      break;
      
    case 'H':                                                                                 // Use rotary encoder
      if(len == 4) 
       {
        Mem.UseRotary = (byte)SConstrainInt(InputString, 1, 0, 2);
        if(Mem.UseRotary > 2) Mem.UseRotary = 0;
        sprintf(sptext, "\nUse of rotary encoder is %s\nUse of membrane keypad is %s", 
                Mem.UseRotary == 1 ? "ON" : "OFF", Mem.UseRotary == 2 ? "ON" : "OFF");
        Tekstprintln(sptext);
        if(Mem.UseRotary > 0)  {Mem.NTPOn = 0;              Mem.UseDS3231 = 1;  }             // Configure related settings based on rotary use 
        else                   {Mem.WIFIOn = Mem.NTPOn = 1; Mem.UseDS3231 = 0;}
        sprintf(sptext, "Use DS3231 is %s, WIFI is %s, NTP is %s\n *** Restart clock with @ ***", 
                Mem.UseDS3231 ? "ON" : "OFF", Mem.WIFIOn ? "ON" : "OFF", Mem.NTPOn ? "ON" : "OFF");
       } 
      else sprintf(sptext, "**** Fault. Enter H000 (none), H001 (Rotary) or H002 (Membrane) ****\nUse rotary encoder is %s\nUse membrane keypad %s",
                    Mem.UseRotary == 1 ? "ON" : "OFF", Mem.UseRotary == 2 ? "ON" : "OFF");
      break;
      
    case 'I':                                                                                 // Menu
      SWversion(len == 1);                                                                    // true for small menu, false for full menu
      break;
      
    case 'J':                                                                                 // Use DS3231 RTC module
      if(len == 1) 
      {
        Mem.UseDS3231 = 1 - Mem.UseDS3231;
        Mem.NTPOn = (1 - Mem.UseDS3231);
        if(Mem.WIFIOn == 0) Mem.NTPOn = 0;                                                    // If WIFI is Off then No NTP
        sprintf(sptext, "Use DS3231 is %s, WIFI is %s, NTP is %s", 
                Mem.UseDS3231 ? "ON" : "OFF", Mem.WIFIOn ? "ON" : "OFF", Mem.NTPOn ? "ON" : "OFF");
       } 
      else sprintf(sptext, "**** Length fault. Enter J ****");
      break;
      
    case 'K':                                                                                 // Test LDR
      TestLDR = 1 - TestLDR;
      sprintf(sptext, "TestLDR: %s", TestLDR ? "On\n   Bits, Out, loops per second and time" : "Off\n");
      break;

    case 'L':                                                                                 // Lower brightness
      validLength = (len > 1 && len < 5);
      if(validLength) 
       {
        Mem.LowerBrightness = (byte)SConstrainInt(InputString, 1, 0, 255);
        sprintf(sptext, "Lower brightness: %d bits", Mem.LowerBrightness);
       } 
      else sprintf(sptext, "**** Input fault. \nEnter Lnnn where n between 1 and 255");
      break;
      
    case 'M':                                                                                 // Max brightness
      validLength = (len > 1 && len < 5);
      if(validLength) 
       {
        Mem.UpperBrightness = SConstrainInt(InputString, 1, 1, 255);
        sprintf(sptext, "Upper brightness changed to: %d bits", Mem.UpperBrightness);
       } 
      else sprintf(sptext, "**** Input fault. \nEnter Mnnn where n between 1 and 255");
      break;

    case 'N':                                                                                 // Turn off display between hours
      sprintf(sptext, "**** Length fault N. ****");
      if(len == 1) { Mem.TurnOffLEDsAtHH = Mem.TurnOnLEDsAtHH = 0;  } 
      else if(len == 5) 
       {
        Mem.TurnOffLEDsAtHH = (byte)InputString.substring(1, 3).toInt();
        Mem.TurnOnLEDsAtHH  = (byte)InputString.substring(3, 5).toInt();
       }
      Mem.TurnOffLEDsAtHH = _min(Mem.TurnOffLEDsAtHH, 23);
      Mem.TurnOnLEDsAtHH  = _min(Mem.TurnOnLEDsAtHH, 23);
      sprintf(sptext, "Display is OFF between %2d:00 and %2d:00", Mem.TurnOffLEDsAtHH, Mem.TurnOnLEDsAtHH);
      break;
      
    case 'O':                                                                                 // Turn On/Off Display
      if(len == 1) 
       {
        LEDsAreOff = !LEDsAreOff;
        sprintf(sptext, "Display is %s", LEDsAreOff ? "OFF" : "ON");
        if(LEDsAreOff) { ClearScreen(); }
        else 
         {
          Tekstprintln(sptext);
          lastminute = 99;                                                                    // Force display update
          sptext[0]=0;
         }
       } 
      else sprintf(sptext, "**** Length fault O. ****");
      break;
          
    case 'P':                                                                                 // Status LEDs On/Off
      if(len == 1) {
        Mem.StatusLEDOn = !Mem.StatusLEDOn;
        UpdateStatusLEDs(0);
        sprintf(sptext, "StatusLEDs are %s", Mem.StatusLEDOn ? "ON" : "OFF");
      } else sprintf(sptext, "**** Length fault P. ****");
      break;

    case 'R':                                                                                 // Reset to default settings
      if(InputString.equals("RRRRR"))                                                                                  // Delete WIFI settings and set defaults
      {
        Reset();
        ResetCredentials();
        ESP.restart();
        break;
      }
      if(InputString.equals("RRR"))                                                                                  // Delete WIFI settings only
       { 
        ResetCredentials();
        sprintf(sptext, "\nSSID and password deleted. \nWIFI, NTP and BLE is On\n Enter @ to restart");
        break;
       }
      if(len == 1)                                                                                  // Set to default settings
        {
        Reset();
        sprintf(sptext, "\nReset to default values: Done");
        lastminute = 99;                                                                        // Force a minute update
        Displaytime();
       } 
      else sprintf(sptext, "**** Length fault R. ****");
      break;
      
    case 'S':                                                                                 // Slope factor for brightness
      validLength = (len > 1 && len < 5);
      if(validLength) 
       {
        Mem.LightReducer = (byte)SConstrainInt(InputString, 1, 1, 255);
        sprintf(sptext, "Slope brightness changed to: %d%%", Mem.LightReducer);
       } 
      else sprintf(sptext, "**** Input fault. \nEnter Snnn where n between 1 and 255");
      break;
      
    case 'T':                                                                                 // Time setting
      if(len == 7) 
       {
        timeinfo.tm_hour = (int)SConstrainInt(InputString, 1, 3, 0, 23);
        timeinfo.tm_min = (int)SConstrainInt(InputString, 3, 5, 0, 59);
        timeinfo.tm_sec = (int)SConstrainInt(InputString, 5, 7, 0, 59);
        if(DS3231Installed) 
         {
          sprintf(sptext, "Time set in external RTC module");
          SetDS3231Time();
          PrintDS3231Time();
         } 
        else sprintf(sptext, "No external RTC module detected");
       } 
      else sprintf(sptext, "**** Length fault. Enter Thhmmss ****");
      break;

     case 'U':      
      if(len == 1) 
       {
        synchronizeAllRTCs(); 
        sprintf(sptext, "All RTC's updated with system time");
       }                                                                                   // Slope factor for brightness
      else if(len == 2)  
       {
        int rtcIndex = (int)SConstrainInt(InputString, 1, 2, 0, 7);  
        DateTime referenceTime(
           timeinfo.tm_year + 1900,                                                         // tm_year = years since 1900
           timeinfo.tm_mon + 1,                                                             // tm_mon = months since January [0–11]
           timeinfo.tm_mday,                                                                // day of the month [1–31]
           timeinfo.tm_hour,
           timeinfo.tm_min,
           timeinfo.tm_sec      );
        setTimeForRTC(rtcIndex, referenceTime) ;
        sprintf(sptext, "RTC %d updated with system time",rtcIndex);
       } 
      else sprintf(sptext, "**** Input fault. \nEnter U0 till U7");
      lastminute = 99;                                                                        // Force a minute update
      break;


    case 'W':                                                                                 // WIFI toggle
      if(len == 1) 
       {
        Mem.WIFIOn = 1 - Mem.WIFIOn;
        Mem.NTPOn = Mem.WIFIOn;                                                               // If WIFI is off turn NTP also off
        sprintf(sptext, "WIFI is %s after restart", Mem.WIFIOn ? "ON" : "OFF");
       } 
      else sprintf(sptext, "**** Length fault. Enter W ****");
      break;
      
    case 'X':                                                                                 // NTP toggle
      if(len == 1) 
       {
        Mem.NTPOn = 1 - Mem.NTPOn;
        sprintf(sptext, "NTP is %s after restart", Mem.NTPOn ? "ON" : "OFF");
       } 
      else sprintf(sptext, "**** Length fault. Enter X ****");
      break;
      
    case 'Z':                                                                                 // Start WPS
      sprintf(sptext, "**** Start WPS on your router");
      WiFi.onEvent(WiFiEvent);
      WiFi.mode(WIFI_STA);
      Serial.println("Starting WPS");
      wpsInitConfig();
      wpsStart();
      break;
      
    case '!':                                                                                 // Print times
      if(len == 1) 
        {
         PrintRTCtimes();      //RTC8
         PrintAllClockTimes();
        } 
      sptext[0]=0;
      break;
      
    case '@':                                                                                 // Reset ESP
      if(len == 1) 
       {
        Tekstprintln("\n*********\n ESP restarting\n*********\n");
        ESP.restart();
       } 
      else sprintf(sptext, "**** Length fault. Enter @ ****");
      break;
  
    case '&':                                                                                 // Force NTP update
      if(len == 1) 
       {
        SetDS3231Time();
        SetRTCTime();
        synchronizeAllRTCs();
        PrintAllClockTimes();
        sprintf(sptext,"\n");
       } 
      else sprintf(sptext, "**** Length fault &. ****");
      break;
 
     case '%':                                                                                 // LED strip type
      if(len == 1) 
       {
        Mem.LEDstrip = 1 - Mem.LEDstrip;
        sprintf(sptext, "LED strip is %s after restart", Mem.LEDstrip ? "WS2812" : "SK6812");
       } 
      else sprintf(sptext, "**** Length fault . ****");
      break;     
 
    case '+':                                                                                 // BLE string toggle
      if(len == 1) 
       {
        Mem.UseBLELongString = 1 - Mem.UseBLELongString;
        sprintf(sptext, "Fast BLE is %s", Mem.UseBLELongString ? "ON" : "OFF");
       } 
      else sprintf(sptext, "**** Length fault. Enter + ****");
      break;
      

    case '=':                                                                                 // Print permanent Mem memory
      if(len == 1)  PrintMem(); 
      else sprintf(sptext, "**** Length fault. Enter = ****");
      break;     

    case '0': case '1': case '2':                                                             // Time entry compatibility mode
      if(len == 6) 
       {
        timeinfo.tm_hour = (int)SConstrainInt(InputString, 0, 2, 0, 23);
        timeinfo.tm_min = (int)SConstrainInt(InputString, 2, 4, 0, 59);
        timeinfo.tm_sec = (int)SConstrainInt(InputString, 4, 6, 0, 59);
        if(DS3231Installed) 
         {
          sprintf(sptext, "Time set in external RTC module");
          SetDS3231Time();
          PrintDS3231Time();
         } 
        else sprintf(sptext, "No external RTC module detected");
       }   
      else sprintf(sptext, "**** Length fault. Enter Thhmmss ****");
      break;
  }
  Tekstprintln(sptext);
  StoreStructInFlashMemory();                                                                 // Update EEPROM
}
InputString = "";
}

//--------------------------------------------                                                //
// COMMON Print Display choice
// Default print the text
//--------------------------------------------
void PrintDisplayChoice() {PrintDisplayChoice(true);}
void PrintDisplayChoice(bool PrintIt)
{
 byte dp = Mem.DisplayChoice;
 sprintf(sptext,"Display choice: %s",
        dp==0?"Yellow+":dp==1?"Hourly":dp==2?"White"  :dp==3?"All Own":
        dp==4?"Own+"   :dp==5?"Wheel" :dp==6?"Digital":dp==7?"Hourly+":
        dp==8?"Rainbow":      "NOP");        
 if (PrintIt) Tekstprintln(sptext);
}
//--------------------------------------------                                                //
// COMMON Print permanent Mem memory
//--------------------------------------------
void PrintMem(void)
{
 //PrintDisplayChoice();
 sprintf(sptext," TurnOffLEDsAtHH: %d",Mem.TurnOffLEDsAtHH);                Tekstprintln(sptext);
 sprintf(sptext,"  TurnOnLEDsAtHH: %d",Mem.TurnOnLEDsAtHH);                 Tekstprintln(sptext);
 sprintf(sptext,"  LanguageChoice: %d",Mem.LanguageChoice);                 Tekstprintln(sptext);
 sprintf(sptext,"    LightReducer: %d",Mem.LightReducer);                   Tekstprintln(sptext);
 sprintf(sptext," LowerBrightness: %d",Mem.LowerBrightness);                Tekstprintln(sptext);
 sprintf(sptext," UpperBrightness: %d",Mem.UpperBrightness);                Tekstprintln(sptext);
 for (int i=0;i<12;i++) { sprintf(sptext,"%03d ",Mem.NVRAMmem[i]);          Tekstprint(sptext); }   Tekstprintln("");
 for (int i=12;i<24;i++){ sprintf(sptext,"%03d ",Mem.NVRAMmem[i]);          Tekstprint(sptext); }   Tekstprintln("");
 sprintf(sptext,"   DisplayChoice: %d",Mem.DisplayChoice);                  Tekstprintln(sptext);                                                                   
 sprintf(sptext,"           BLEOn: %s",Mem.BLEOn ? "ON" : "OFF");           Tekstprintln(sptext);
 sprintf(sptext,"           NTPOn: %s",Mem.NTPOn ? "ON" : "OFF");           Tekstprintln(sptext);
 sprintf(sptext,"          WIFIOn: %s",Mem.WIFIOn ? "ON" : "OFF");          Tekstprintln(sptext);
 sprintf(sptext,"     StatusLEDOn: %s",Mem.StatusLEDOn ? "ON" : "OFF");     Tekstprintln(sptext);
 sprintf(sptext,"    MCUrestarted: %d",Mem.MCUrestarted );                  Tekstprintln(sptext);                                              
// sprintf(sptext,"         DCF77On: %s",Mem.DCF77On  ? "ON" : "OFF");        Tekstprintln(sptext);
 sprintf(sptext,"       UseRotary: %s",Mem.UseRotary ? "ON" : "OFF");       Tekstprintln(sptext);
 sprintf(sptext,"       UseDS3231: %s",Mem.UseDS3231 ? "ON" : "OFF");       Tekstprintln(sptext);
 sprintf(sptext,"       LED strip: %s", Mem.LEDstrip?"WS2812":"SK6812" );   Tekstprintln(sptext);
// sprintf(sptext,"      FiboChrono: %s",Mem.FiboChrono ? "FIBO" : "CHRONO"); Tekstprintln(sptext);
// sprintf(sptext,"          NoExUl: %s", Mem.NoExUl ? "ON" : "OFF");         Tekstprintln(sptext);
 byte wc=Mem.WIFIcredentials; 
 sprintf(sptext," WIFIcredentials: %s", wc==0? "Not SET" : wc==1? "SET" : wc==2? "SET&OK": 
                                   wc==3? "in AP not SET":"Unknown code");  Tekstprintln(sptext);
 sprintf(sptext,"      IntFuture2: %d",Mem.IntFuture2 );                    Tekstprintln(sptext);
 sprintf(sptext,"      IntFuture3: %d",Mem.IntFuture3 );                    Tekstprintln(sptext);
 sprintf(sptext,"     ByteFuture1: %d", Mem.ByteFuture1);                   Tekstprintln(sptext);
 sprintf(sptext,"     ByteFuture2: %d", Mem.ByteFuture2);                   Tekstprintln(sptext);
 sprintf(sptext,"     ByteFuture3: %d", Mem.ByteFuture3);                   Tekstprintln(sptext);
 sprintf(sptext,"     ByteFuture4: %d", Mem.ByteFuture4);                   Tekstprintln(sptext);
 sprintf(sptext,"UseBLELongString: %s",Mem.UseBLELongString?"ON":"OFF");    Tekstprintln(sptext);
 sprintf(sptext,"     Font colour: 0X%08" PRIX32, Mem.OwnColour);           Tekstprintln(sptext);
 sprintf(sptext,"    DimmedLetter: 0X%08" PRIX32,Mem.DimmedLetter );        Tekstprintln(sptext);
 //sprintf(sptext,"      BackGround: 0X%08" PRIX32,Mem.BackGround);           Tekstprintln(sptext);
 sprintf(sptext,"            SSID: %s",Mem.SSID);                           Tekstprintln(sptext);
 // sprintf(sptext,"BackGround: %s",Mem.Password);                          Tekstprintln(sptext);
 sprintf(sptext,"BLEbroadcastName: %s",Mem.BLEbroadcastName);               Tekstprintln(sptext);
 sprintf(sptext,"        Timezone: %s",Mem.Timezone);                       Tekstprintln(sptext);
 sprintf(sptext,"        Checksum: %d",Mem.Checksum);                       Tekstprintln(sptext);
 sptext[0]=0;
}
//--------------------------------------------                                                //
// LDR reading are between 0 and 255. 
// ESP32 analogue read is between 0 - 4096 --   is: 4096 / 8
//--------------------------------------------
int ReadLDR(void)
{
 return analogRead(PhotoCellPin)/16;
}
//--------------------------------------------                                                //
// CLOCK Say the time and load the LEDs 
// with the proper colour and intensity
//--------------------------------------------
void Displaytime()
{ 
 ClearScreen();                                                                               // Start by clearing the display to a known state   
// GetTijd(false);  //Tekstprintln("");
 PrintDifRTCtimes(); 
 UpdateRTCTimeLEDs();
}
//--------------------------------------------                                                //
// CLOCK Dim the leds measured by the LDR and print values
// LDR reading are between 0 and 255. The Brightness send to the LEDs is between 0 and 255
//--------------------------------------------
void DimLeds(bool print) 
{ 
  int LDRread = ReadLDR();                                                                    // ESP32 analoge read is between 0 - 4096, reduce it to 0-1024                                                                                                   
  int LDRavgread = (4 * Previous_LDR_read + LDRread ) / 5;                                    // Read lightsensor and avoid rapid light intensity changes
  Previous_LDR_read = LDRavgread;                                                             // by using the previous reads
  OutPhotocell = (uint32_t)((Mem.LightReducer * sqrt(255*LDRavgread))/100);                   // Linear --> hyperbolic with sqrt. Result is between 0-255
  MinPhotocell = min(MinPhotocell, LDRavgread);                                               // Lowest LDR measurement
  MaxPhotocell = max(MaxPhotocell, LDRavgread);                                               // Highest LDR measurement
  OutPhotocell = constrain(OutPhotocell, Mem.LowerBrightness, Mem.UpperBrightness);           // Keep result between lower and upper boundery en calc percentage
  SumLDRreadshour += LDRavgread;    NoofLDRreadshour++;                                       // For statistics LDR readings per hour
  if(print)
  {
  // sprintf(sptext,"LDR:%3d Avg:%3d (%3d-%3d) Out:%3d=%2d%% Loop(%ld) ",
  //      LDRread,LDRavgread,MinPhotocell,MaxPhotocell,OutPhotocell,(int)(OutPhotocell/2.55),Loopcounter);    
 if (Mem.UseDS3231)   sprintf(sptext,"LDR:%3d=%2d%% %5ld l/s %0.0fC ",
               LDRread,(int)(OutPhotocell*100/255),Loopcounter,RTCklok.getTemperature()); 
 else                 sprintf(sptext,"LDR:%3d=%2d%% %5ld l/s ",
               LDRread,(int)(OutPhotocell*100/255),Loopcounter);   
   Tekstprint(sptext);
   PrintTimeHMS();    
  }
 if(LEDsAreOff) OutPhotocell = 0;
 SetBrightnessLeds(OutPhotocell);     // values between 0 and 255
}

// --------------------Colour Clock Light functions -----------------------------------
//--------------------------------------------                                                //
// LED Set color for LEDs in strip and print tekst
//---------------------------------------------
void ColorLeds(char const *Tekst, int FirstLed, int LastLed, uint32_t RGBWColor)
{ 
 Stripfill(RGBWColor, FirstLed, ++LastLed - FirstLed );                                        //
 if (!NoTextInLeds && strlen(Tekst) > 0 )
     {sprintf(sptext,"%s ",Tekst); Tekstprint(sptext); }                                      // Print the text  
}
//--------------------------------------------
// LED Set color for one LED
//--------------------------------------------
void ColorLed(int Lednr, uint32_t RGBWColor)
{   
 Stripfill(RGBWColor, Lednr, 1 );
}
//--------------------------------------------                                                //
// LED Clear display settings of the LEDs
//--------------------------------------------
void LedsOff(void) 
{ 
 Stripfill(0, 0, NUM_LEDS );                                                                  // 
}
//--------------------------------------------                                                //
// LED Turn On and the LEDs off after Delaymsec milliseconds
//--------------------------------------------
void Laatzien(int Delaymsec) 
{ 
 ShowLeds(); 
 delay(Delaymsec);
 LedsOff(); 
 CheckDevices();                                                                              // Check for input from input devices
}

//--------------------------------------------                                                //
// LED Push data in LED strip to commit the changes
//--------------------------------------------
void ShowLeds(void)
{
//    if (xSemaphoreTake(ledMutex, portMAX_DELAY))                                           // A semaphore is not needed when using Neopixel
    { // Lock the mutex
      LEDstrip.show();
//    xSemaphoreGive(ledMutex); // Release the mutex
    }
}
//--------------------------------------------                                                //
// LED Set brighness of LEDs
//--------------------------------------------
void SetBrightnessLeds(byte Bright)
{
 LEDstrip.setBrightness(Bright);                                                              // Set brightness of LEDs   
 ShowLeds();
}
//--------------------------------------------
// LED Fill the strip array
//--------------------------------------------
void Stripfill(uint32_t RGBWColor, int FirstLed, int NoofLEDs)
{   
 LEDstrip.fill(RGBWColor, FirstLed, NoofLEDs);
}
//--------------------------------------------
// LED Strip Get Pixel Color 
//--------------------------------------------
uint32_t StripGetPixelColor(int Lednr)
{
  return (0);
return(LEDstrip.getPixelColor(Lednr));
}
//--------------------------------------------                                                //
// LED Synchronize the colour of the LEDstrip with the Status LED
// Used during Setup()
//--------------------------------------------
void LEDstartup(uint32_t LEDColour)
{
 static uint32_t ProgressLedNr = 0;
 ColorLed(ProgressLedNr++,LEDColour); 
 ShowLeds();   
 SetStatusLED(Cred(LEDColour),Cgreen(LEDColour),Cblue(LEDColour));  
}

//--------------------------------------------                                                //
// LED convert HSV to RGB  h is from 0-360, s,v values are 0-1
// r,g,b values are 0-255
//--------------------------------------------
/**
 * @brief Convert HSV values to packed RGBW format (white = 0).
 * @param H Hue angle (0–360)
 * @param S Saturation (0–1)
 * @param V Value/Brightness (0–1)
 * @return Packed RGBW uint32_t value
 */
uint32_t HSVToRGB(double H, double S, double V) 
{
 int i;
 double r, g, b, f, p, q, t;
 if (S == 0)  {r = V;  g = V;  b = V; }
 else
  {
   H >= 360 ? H = 0 : H /= 60;
   i = (int) H;
   f = H - i;
   p = V * (1.0 -  S);
   q = V * (1.0 - (S * f));
   t = V * (1.0 - (S * (1.0 - f)));
   switch (i) 
    {
     case 0:  r = V;  g = t;  b = p;  break;
     case 1:  r = q;  g = V;  b = p;  break;
     case 2:  r = p;  g = V;  b = t;  break;
     case 3:  r = p;  g = q;  b = V;  break;
     case 4:  r = t;  g = p;  b = V;  break;
     default: r = V;  g = p;  b = q;  break;
    }
  }
return FuncCRGBW((int)(r*255), (int)(g*255), (int)(b*255), 0 );                                // R, G, B, W 
}
//--------------------------------------------                                                //
// LED function to make RGBW colour
//--------------------------------------------
uint32_t FuncCRGBW( uint32_t Red, uint32_t Green, uint32_t Blue, uint32_t White)
{ 
 return ( (White<<24) + (Red << 16) + (Green << 8) + Blue );
}
//--------------------------------------------                                                //
// LED functions to extract RGBW colours
//--------------------------------------------
 uint8_t Cwhite(uint32_t c) { return (c >> 24);}
 uint8_t Cred(  uint32_t c) { return (c >> 16);}
 uint8_t Cgreen(uint32_t c) { return (c >> 8); }
 uint8_t Cblue( uint32_t c) { return (c);      }


//--------------------------------------------                                                //
//  DISPLAY Clear the display
//--------------------------------------------
void ClearScreen(void)
{
 LedsOff();
}

//--------------------------------------------                                                //
// DISPLAY Initialyse the LEDstrip for WS2812 or SK6812 LEDs
//--------------------------------------------
void StartLeds(void) 
{
  
 switch (Mem.LEDstrip)
    {
      case 0: LEDstrip = LED6812strip; 
              white  = 0xFF000000; 
              lgray  = 0x66000000;  
              gray   = 0x33000000;                                                            // The SK6812 LED has a white LED that is pure white
              dgray  = 0x22000000;
              wgray  = 0xAA000000; 

      break;
      case 1: LEDstrip = LED2812strip; 
              white  = 0xFFFFFF;
              lgray  = 0x666666;                                                              // R, G and B on together gives white light
              gray   = 0x333333;
              dgray  = 0x222222;
              wgray  = 0xAAAAAA;         
      break;
     default: LEDstrip = LED6812strip;
              white  = 0xFF000000; 
              lgray  = 0x66000000;  
              gray   = 0x33000000;                                                            // The SK6812 LED has a white LED that is pure white
              dgray  = 0x22000000; 
              wgray  = 0xAA000000;     
    }
 sprintf(sptext,"LED strip is %s", Mem.LEDstrip?"WS2812":"SK6812" ); Tekstprintln(sptext);

 LEDstrip.begin();
 LEDstrip.setBrightness(16);  
 LedsOff();                                                                // Set initial brightness of LEDs  (0-255)  
 ShowLeds();
}


//--------------------------------------------                                                //
// CLOCK In- or decrease light intensity value i.e. Slope
//--------------------------------------------
void WriteLightReducer(int amount)
{
 int value = Mem.LightReducer + amount;                                                       // Prevent byte overflow by making it an integer before adding
 Mem.LightReducer = (byte) constrain(value,5, 250);                                           // Between 5 and 250
 sprintf(sptext,"Max brightness: %3d%%",Mem.LightReducer);
 Tekstprintln(sptext);
}
//--------------------------- Time functions --------------------------
    
//--------------------------------------------                                                //
// RTC Get time from NTP cq internal ESP32 RTC 
// and store it in timeinfo struct
// return local time in unix time format
//--------------------------------------------
time_t GetTijd(bool printit)
{
 time_t now;
 if (Mem.UseDS3231) GetDS3231Time(false);                                                     // If the DS3231 is attached and used get its time in timeinfo struct
 else
    { 
     if(Mem.NTPOn)  getLocalTime(&timeinfo);                                                  // If NTP is running get the local time
     else { time(&now); localtime_r(&now, &timeinfo);}                                        // Else get the time from the internal RTC and place it timeinfo
    }
 if (printit)  PrintRTCTime();                                                                // 
 localtime(&now);                                                                             // Get the actual time and
 return now;                                                                                  // Return the unixtime in seconds
}

//--------------------------------------------                                                //
// RTC return local time in a RTClib Datetime 
//--------------------------------------------
DateTime getLocalDateTime() 
{
 struct tm timeinfo;
 getLocalTime(&timeinfo);

return DateTime(
  timeinfo.tm_year + 1900,  // tm_year is years since 1900
  timeinfo.tm_mon + 1,       // tm_mon is 0-11, most libraries expect 1-12
  timeinfo.tm_mday,
  timeinfo.tm_hour,
  timeinfo.tm_min,
  timeinfo.tm_sec
); 
}
//--------------------------------------------                                                //
// NTP print the NTP time for the timezone set 
//--------------------------------------------
void GetNTPtime(bool printit)
{
 wait4SNTP();                                                                                 // Force a NTP time update 
 if(printit) PrintNTPtime();
}

//--------------------------------------------                                                //
// NTP print the NTP time for the timezone set 
//--------------------------------------------
void PrintNTPtime(void)
{
  getLocalTime(&timeinfo); //   Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
  PrintRTCTime(); 
}
//--------------------------------------------                                                //
// NTP print the NTP UTC time 
//--------------------------------------------
void PrintUTCtime(void)
{
 time_t tmi = time(nullptr);
 struct tm* UTCtime = gmtime(&tmi);
 sprintf(sptext,"UTC: %02d:%02d:%02d %02d-%02d-%04d  ", 
     UTCtime->tm_hour,UTCtime->tm_min,UTCtime->tm_sec,
     UTCtime->tm_mday,UTCtime->tm_mon+1,UTCtime->tm_year+1900);
 Tekstprint(sptext);   
}

//--------------------------------------------                                                //
// DS3231 Init module
//--------------------------------------------
void InitDS3231Mod(void)
{
 DS3231Installed = IsDS3231I2Cconnected();                                                    // Check if DS3231 is connected and working   
 sprintf(sptext,"External RTC module %s found", DS3231Installed?"IS":"NOT");
 RTCklok.begin();     
 Tekstprintln(sptext);                                                             
}
//--------------------------------------------                                                //
// DS3231 check for I2C connection
// DS3231_I2C_ADDRESS (= often 0X68) = DS3231 module
//--------------------------------------------
bool IsDS3231I2Cconnected(void)
 {
  bool DS3231Found = false;
  for (byte i = 1; i < 120; i++)
  {
   Wire.beginTransmission (i);
   if (Wire.endTransmission () == 0)                                                       
      {
      sprintf(sptext,"Found I2C address: 0X%02X", i); Tekstprintln(sptext);  
      if( i== DS3231_I2C_ADDRESS) DS3231Found = true;
      } 
  }
  return DS3231Found;   
  }
//--------------------------------------------                                                //
// DS3231 Get temperature from DS3231 module
//--------------------------------------------
float GetDS3231Temp(void)
{
 byte tMSB, tLSB;
 float temp3231;
 
  Wire.beginTransmission(DS3231_I2C_ADDRESS);                                                 // Temp registers (11h-12h) get updated automatically every 64s
  Wire.write(0x11);
  Wire.endTransmission();
  Wire.requestFrom(DS3231_I2C_ADDRESS, 2);
 
  if(Wire.available()) 
  {
    tMSB = Wire.read();                                                                       // 2's complement int portion
    tLSB = Wire.read();                                                                       // fraction portion 
    temp3231 = (tMSB & 0b01111111);                                                           // do 2's math on Tmsb
    temp3231 += ( (tLSB >> 6) * 0.25 ) + 0.5;                                                 // only care about bits 7 & 8 and add 0.5 to round off to integer   
  }
  else   {temp3231 = -273; }  
  return (temp3231);
}

//--------------------------------------------                                                //
// DS3231 Set time in module DS3231
//--------------------------------------------
void SetDS3231Time(void)
{
 return; //RTC8 
RTCklok.adjust(DateTime(timeinfo.tm_year+1900, timeinfo.tm_mon+1, timeinfo.tm_mday, 
                        timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec));
}

//--------------------------------------------                                                //
// DS3231 reads time in module DS3231
// and store it in Word clock time structure
//--------------------------------------------
void GetDS3231Time(bool printit)
{
 DateTime Inow    = RTCklok.now();                                                           // Be sure to get the latest DS3231 RTC clock time
 timeinfo.tm_hour = Inow.hour();
 timeinfo.tm_min  = Inow.minute();
 timeinfo.tm_sec  = Inow.second();
 timeinfo.tm_year = Inow.year() - 1900;                                                      // Inow.year() is years since 2000 tm_year is years since 1900
 timeinfo.tm_mon  = Inow.month() - 1;
 timeinfo.tm_mday = Inow.day();
 if (printit) PrintRTCTime(); 
}

//--------------------------------------------                                                //
// DS3231 prints time to serial
// reference https://adafruit.github.io/RTClib/html/class_r_t_c___d_s3231.html
//--------------------------------------------
void PrintDS3231Time(void)
{
 DateTime Inow = RTCklok.now();                                                                        // Be sure to get the lates DS3231 RTC clock time
 sprintf(sptext,"%02d/%02d/%04d %02d:%02d:%02d ", Inow.day(),Inow.month(),Inow.year(),
                                                  Inow.hour(),Inow.minute(),Inow.second());
 Tekstprint(sptext);
}

//--------------------------------------------                                                //
// RTC prints the ESP32 internal RTC time to serial
//--------------------------------------------
void PrintRTCTime(void)
{
 sprintf(sptext,"%02d/%02d/%04d %02d:%02d:%02d ", 
     timeinfo.tm_mday,timeinfo.tm_mon+1,timeinfo.tm_year+1900,
     timeinfo.tm_hour,timeinfo.tm_min,timeinfo.tm_sec);
 WTekstprint(sptext,"<!--"," -->");                                                         // Hide the time on the HTML page <!-- This text won't appear at all -->
 Tekstprintln("");
}
//--------------------------------------------                                                //
// RTC Fill sptext with time
//--------------------------------------------
void PrintTimeHMS(){ PrintTimeHMS(2);}                                                        // print with linefeed
void PrintTimeHMS(byte format)
{
 sprintf(sptext,"%02d:%02d:%02d",timeinfo.tm_hour,timeinfo.tm_min,timeinfo.tm_sec);
 switch (format)
 {
  case 0: break;
  case 1: Tekstprint(sptext); break;
  case 2: Tekstprintln(sptext); break;  
 }
}
//--------------------------------------------                                                //
// RTC Set time using global timeinfo struct
//--------------------------------------------
void SetRTCTime(void) 
{ 
 time_t t = mktime(&timeinfo);                                                                // t= unixtime
 SetRTCTime(t);
}  
//--------------------------------------------                                                //
// RTC Set RTC time using Unix timestamp
//--------------------------------------------
void SetRTCTime(time_t t)
{ 
 snprintf(sptext, sizeof(sptext), "Setting time: %s", asctime(&timeinfo));                    // Safer than sprintf to avoid buffer overflows
 Tekstprintln(sptext); 
 struct timeval now = { .tv_sec = t , .tv_usec = 0};
 settimeofday(&now, NULL);
 GetTijd(false);                                                                              // Sync global timeinfo with RTC                                                                    // Synchronize time with RTC clock
 Displaytime();                                                                               // Show time on clock
 PrintTimeHMS();                                                                              // Print time in HH:MM:SS format
}
//--------------------------------------------                                                //
// Print all the times available 
//--------------------------------------------
void PrintAllClockTimes(void)
{
 Tekstprint(" Clock time: ");
 PrintRTCTime();
 if(WiFi.localIP()[0] != 0)                                                                   // If no WIFI then no NTP time printed
   {
    Tekstprint("   NTP time: ");
    PrintNTPtime();
   }
 if(DS3231Installed)
   {
    Tekstprint("DS3231 time: ");
    PrintDS3231Time();
   }
}
//                                                                                            //
// ------------------- End  Time functions 

//--------------------------------------------                                                //
// BLE SendMessage by BLE Slow in packets of 20 chars
// or fast in one long string.
// Fast can be used in IOS app BLESerial Pro
//------------------------------
void SendMessageBLE(std::string Message)
{
 if(deviceConnected) 
   {
    if (Mem.UseBLELongString)                                                                 // If Fast transmission is possible
     {
      pTxCharacteristic->setValue(Message); 
      pTxCharacteristic->notify();
      delay(10);                                                                              // Bluetooth stack will go into congestion, if too many packets are sent
     } 
   else                                                                                       // Packets of max 20 bytes
     {   
      int parts = (Message.length()/20) + 1;
      for(int n=0;n<parts;n++)
        {   
         pTxCharacteristic->setValue(Message.substr(n*20, 20)); 
         pTxCharacteristic->notify();
         delay(10);                                                                           // Bluetooth stack will go into congestion, if too many packets are sent
        }
     }
   } 
}

//--------------------------------------------                                                //
// BLE Start BLE Classes NimBLE Version 2.x.x
//------------------------------
class MyServerCallbacks: public NimBLEServerCallbacks 
{
 void onConnect(NimBLEServer* pServer, NimBLEConnInfo& connInfo) override 
    {deviceConnected = true; Serial.println("BLE Connected"      );}
 void onDisconnect(NimBLEServer* pServer, NimBLEConnInfo& connInfo, int reason) override 
    {deviceConnected = false; Serial.println("BLE NOT Connected" );}
};
  
class MyCallbacks: public NimBLECharacteristicCallbacks 
{
 void onWrite(NimBLECharacteristic *pCharacteristic, NimBLEConnInfo& connInfo) override  
  {
   std::string rxValue = pCharacteristic->getValue();
   ReceivedMessageBLE = rxValue + "\n";
  //  if (rxValue.length() > 0) {for (int i = 0; i < rxValue.length(); i++) printf("%c",rxValue[i]); }
  //  printf("\n");
  }  
};

//--------------------------------------------                                                //
// BLE Start BLE Service
//------------------------------
void StartBLEService(void)
{
 NimBLEDevice::init(Mem.BLEbroadcastName);                                                    // Create the BLE Device
 pServer = NimBLEDevice::createServer();                                                      // Create the BLE Server
 pServer->setCallbacks(new MyServerCallbacks());
 BLEService *pService = pServer->createService(SERVICE_UUID);                                 // Create the BLE Service
 pTxCharacteristic                     =                                                      // Create a BLE Characteristic 
     pService->createCharacteristic(CHARACTERISTIC_UUID_TX, NIMBLE_PROPERTY::NOTIFY);                 
 BLECharacteristic * pRxCharacteristic = 
     pService->createCharacteristic(CHARACTERISTIC_UUID_RX, NIMBLE_PROPERTY::WRITE);
 pRxCharacteristic->setCallbacks(new MyCallbacks());
 pService->start(); 
 BLEAdvertising *pAdvertising = NimBLEDevice::getAdvertising();
 pAdvertising->addServiceUUID(SERVICE_UUID); 
 pAdvertising->setName(Mem.BLEbroadcastName);
 pServer->start();                                                                            // Start the server  Nodig??
 pServer->getAdvertising()->start();                                                          // Start advertising
 TekstSprint("BLE Waiting a client connection to notify ...\n"); 
}
//                                                                                            //
//-----------------------------
// BLE CheckBLE input and rework string
// After one hour the BLE connection is disconnected
//------------------------------
void CheckBLE(void)
{
if (deviceConnected && !oldDeviceConnected) 
  {
   oldDeviceConnected = deviceConnected;
   BLEConnectedSince = millis();                                                              // Mark time of last connection 1 hour after this time the BLE connection will disconnect
  }

if (!deviceConnected && oldDeviceConnected)                                                   // If device is disconnected start advertising
  {
   delay(300);
   pServer->startAdvertising();
   TekstSprint("Start advertising\n");
   oldDeviceConnected = deviceConnected;
   BLEConnectedSince = 0;                                                                     // Reset BLE connection timer
  }

if (deviceConnected && BLEConnectedSince > 0 && (millis() - BLEConnectedSince > 900000))      // Disconnect if connected longer than 15 minutes inactivity 
  {
    auto connHandles = NimBLEDevice::getServer()->getPeerDevices();                           // Vector of uint16_t handles
    if (!connHandles.empty()) 
      {
        uint16_t conn_handle = connHandles[0];
        int rc = ble_gap_terminate(conn_handle, BLE_ERR_REM_USER_CONN_TERM);                  // Reason = 0x13 = user terminated
        if (rc == 0) { TekstSprint("Disconnected BLE client after 1 hour\n"); } 
        else         
           { 
            sprintf(sptext,"BLE disconnect failed, error code: %d\n", rc);
            TekstSprint(sptext);  
           }
        BLEConnectedSince = 0;
      }
  }
 if(ReceivedMessageBLE.length()>0)
   {
    SendMessageBLE(ReceivedMessageBLE);
    String BLEtext = ReceivedMessageBLE.c_str();
    ReceivedMessageBLE = "";
    ReworkInputString(BLEtext); 
    BLEConnectedSince = millis();                                                             // Mark time of last connection 1 hour after thsi time the BLE connection will disconnect
   }
}


//--------------------------------------------                                                //
// WIFI WIFIEvents
//--------------------------------------------
void WiFiEvent(WiFiEvent_t event)
{
  // sprintf(sptext,"[WiFi-event] event: %d  : ", event); 
  // Tekstprint(sptext);
  WiFiEventInfo_t info;
  static bool LostPrinted = false;
    switch (event) 
     {
        case ARDUINO_EVENT_WIFI_READY: 
            Tekstprintln("WiFi interface ready");
            break;
        case ARDUINO_EVENT_WIFI_SCAN_DONE:
            Tekstprintln("Completed scan for access points");
            break;
        case ARDUINO_EVENT_WIFI_STA_START:
            Tekstprintln("WiFi client started");
            break;
        case ARDUINO_EVENT_WIFI_STA_STOP:
            Tekstprintln("WiFi clients stopped");
            break;
        case ARDUINO_EVENT_WIFI_STA_CONNECTED:
            Tekstprintln("Connected to access point");
            LostPrinted = false;
            break;
       case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
            if(!LostPrinted)
             {
              sprintf(sptext,"WiFi lost connection.");                                          // Reason: %d",info.wifi_sta_disconnected.reason); 
              Tekstprintln(sptext);
              LostPrinted = true;
             }
 //           WiFi.reconnect(); is checked in EveryMinuteUpdate()
            break;
        case ARDUINO_EVENT_WIFI_STA_AUTHMODE_CHANGE:
            Tekstprintln("Authentication mode of access point has changed");
            break;
        case ARDUINO_EVENT_WIFI_STA_GOT_IP:
//            sprintf(sptext,"Connected to : %s %s",WiFi.SSID().c_str(), WiFi.psk().c_str()  );
            sprintf(sptext,"Connected to : %s",WiFi.SSID().c_str());
            Tekstprintln(sptext);
            sprintf(sptext, "Obtained IP address: %d.%d.%d.%d", WiFi.localIP()[0], WiFi.localIP()[1], WiFi.localIP()[2], WiFi.localIP()[3] );
            Tekstprintln(sptext);         
            strcpy(Mem.SSID,      WiFi.SSID().c_str());
            strcpy(Mem.Password , WiFi.psk().c_str());                                         // Store SSID and password
            Mem.NTPOn        = 1;                                                              // NTP On
            Mem.WIFIOn       = 1;                                                              // WIFI On  
            StoreStructInFlashMemory();  
            break;
        case ARDUINO_EVENT_WIFI_STA_LOST_IP:
            Tekstprintln("Lost IP address and IP address is reset to 0");
            break;
        case ARDUINO_EVENT_WPS_ER_SUCCESS:
            wpsStop();
            delay(100);
            WiFi.begin();
            delay(200);
             sprintf(sptext, "WPS Successfull, stopping WPS and connecting to: %s: ", WiFi.SSID().c_str());
            Tekstprintln(sptext);       
            break;
        case ARDUINO_EVENT_WPS_ER_FAILED:
            Tekstprintln("WPS Failed, retrying");
            wpsStop();
            wpsStart();
            break;
        case ARDUINO_EVENT_WPS_ER_TIMEOUT:
            Tekstprintln("WPS Timedout, Start WPS again");
            wpsStop();
            // wpsStart();
            break;
        case ARDUINO_EVENT_WPS_ER_PIN:
            sprintf(sptext,"WPS_PIN = %s",wpspin2string(info.wps_er_pin.pin_code).c_str());
            Tekstprintln(sptext);
            break;
        case ARDUINO_EVENT_WIFI_AP_START:
            Tekstprintln("WiFi access point started");
            break;
        case ARDUINO_EVENT_WIFI_AP_STOP:
            Tekstprintln("WiFi access point  stopped");
            break;
        case ARDUINO_EVENT_WIFI_AP_STACONNECTED:
            Tekstprintln("Client connected");
            break;
        case ARDUINO_EVENT_WIFI_AP_STADISCONNECTED:
            sprintf(sptext,"Client disconnected.");                                            // Reason: %d",info.wifi_ap_stadisconnected.reason); 
            Tekstprintln(sptext);
            break;
        case ARDUINO_EVENT_WIFI_AP_STAIPASSIGNED:
            Tekstprintln("Assigned IP address to client");
            break;
        case ARDUINO_EVENT_WIFI_AP_PROBEREQRECVED:
            Tekstprintln("Received probe request");
            break;
        case ARDUINO_EVENT_WIFI_AP_GOT_IP6:
            Tekstprintln("AP IPv6 is preferred");
            break;
        case ARDUINO_EVENT_WIFI_STA_GOT_IP6:
            Tekstprintln("STA IPv6 is preferred");
            break;
        case ARDUINO_EVENT_ETH_GOT_IP6:
            Tekstprintln("Ethernet IPv6 is preferred");
            break;
        case ARDUINO_EVENT_ETH_START:
            Tekstprintln("Ethernet started");
            break;
        case ARDUINO_EVENT_ETH_STOP:
            Tekstprintln("Ethernet stopped");
            break;
        case ARDUINO_EVENT_ETH_CONNECTED:
            Tekstprintln("Ethernet connected");
            break;
        case ARDUINO_EVENT_ETH_DISCONNECTED:
            // WiFi.scanNetworks will return the number of networks found("Ethernet disconnected");
            break;
        case ARDUINO_EVENT_ETH_GOT_IP:
            Tekstprintln("Obtained IP address");
            break;
        default: break;
    }
}
//--------------------------------------------                                                //
// WIFI Check for WIFI Network 
// Check if WIFI network to connect to is available
//--------------------------------------------
 bool CheckforWIFINetwork(void)         { return CheckforWIFINetwork(true);}
 bool CheckforWIFINetwork(bool PrintIt)
 {
  // WiFi.mode(WIFI_STA);                                                                        // Set WiFi to station mode and disconnect from an AP if it was previously connected.
  // WiFi.disconnect(true); 
  Tekstprintln("Scanning for networks");
  int n = WiFi.scanNetworks();                                                                // WiFi.scanNetworks will return the number of networks found
  if (n == 0) { Tekstprintln("no networks found"); return false;} 
  else 
    { 
     sprintf(sptext,"%d: networks found",n);    Tekstprintln(sptext);
     for (int i = 0; i < n; ++i)                                                              // Print SSID and RSSI for each network found
      {
        sprintf(sptext,"%2d: %20s %3d %1s",i+1,WiFi.SSID(i).c_str(),(int)WiFi.RSSI(i),(WiFi.encryptionType(i) == WIFI_AUTH_OPEN)?" ":"*");
        if (strcmp(WiFi.SSID(i).c_str(), Mem.SSID)==0) { strcat(sptext, " -- Will connect to"); }
        if(PrintIt) Tekstprintln(sptext);
      }
    }
 return true;                                                                                  //  If no SSID an web page at 192.168.4.1 will be started to enter the credentials
 }

//--------------------------------------------                                                //
// WIFI Scan for WIFI stations
//--------------------------------------------
void ScanWIFI(void)
{
 WiFi.mode(WIFI_STA);                                                                         // Set WiFi to station mode and disconnect from an AP if it was previously connected.
 WiFi.disconnect(true);
 delay(100);
 int n = WiFi.scanNetworks();                                                                 // WiFi.scanNetworks will return the number of networks found.
 if (n == 0)  { Tekstprintln("no networks found");  } 
 else 
   {
    sprintf(sptext,"%d networks found",n);   Tekstprintln(sptext);
    Tekstprintln("Nr | SSID                             | RSSI | CH | Encryption");
    for(int i = 0; i < n; ++i) 
      {
       sprintf(sptext,"%2d | %-32.32s | %4d | %2d | ",i + 1, 
                       WiFi.SSID(i).c_str(), (int)WiFi.RSSI(i), (int)WiFi.channel(i));        // Print SSID and RSSI for each network found
       Tekstprint(sptext);
       switch (WiFi.encryptionType(i))
           {
             case WIFI_AUTH_OPEN:            Tekstprint("open");      break;
             case WIFI_AUTH_WEP:             Tekstprint("WEP");       break;
             case WIFI_AUTH_WPA_PSK:         Tekstprint("WPA");       break;
             case WIFI_AUTH_WPA2_PSK:        Tekstprint("WPA2");      break;
             case WIFI_AUTH_WPA_WPA2_PSK:    Tekstprint("WPA+WPA2");  break;
             case WIFI_AUTH_WPA2_ENTERPRISE: Tekstprint("WPA2-EAP");  break;
             case WIFI_AUTH_WPA3_PSK:        Tekstprint("WPA3");      break;
             case WIFI_AUTH_WPA2_WPA3_PSK:   Tekstprint("WPA2+WPA3"); break;
             case WIFI_AUTH_WAPI_PSK:        Tekstprint("WAPI");      break;
             default:                        Tekstprint("unknown");
            }
            Tekstprintln("");
            delay(10);
        }
   }
Tekstprintln("");
WiFi.scanDelete();                                                                            // Delete the scan result to free memory for code below.
}
//--------------------------------------------                                                //
// WIFI Check for WIFI router SSID and password 
// If not valid then start webpage to enter the credentials
//--------------------------------------------
void ConnectWIFI(void)
{
if( (strlen(Mem.Password)<5 || strlen(Mem.SSID)<3))   // If WIFI required and no SSID or password
   {
     Tekstprintln("Starting AP mode to enter SSID and password of the WIFI router");
     StartAPMode();
   }  
 else 
  { 
    Tekstprintln("Starting WIFI/NTP");
    StartWIFI_NTP();
  }
 }
//--------------------------------------------                                                //
// WIFI Check if WIFI is sill connected and if not restore it
//-------------------------------------------- 
void CheckRestoreWIFIconnectivity(void)
 {
 if(Mem.WIFIOn && WIFIwasConnected)                                                           // If WIFI switch is On and there was a connection.
   {
    if(WiFi.localIP()[0] == 0) 
       {
        if(Mem.WIFIcredentials == SET_AND_OK)  WiFi.reconnect();                              // If connection lost and WIFI is used reconnect
        if(CheckforWIFINetwork(false) && !WIFIwasConnected) StartWIFI_NTP();                  // If there was no WIFI at start up start a WIFI connection 
        if(WiFi.localIP()[0] != 0)                                                            // WIFI connection is established
          {
          sprintf(sptext, "Reconnected to IP address: %d.%d.%d.%d", WiFi.localIP()[0], WiFi.localIP()[1], WiFi.localIP()[2], WiFi.localIP()[3] );
          Tekstprintln(sptext);
          }
       }
    }
 }

//--------------------------------------------                                                //
// WIFI Start WIFI connection and NTP service
//--------------------------------------------
void StartWIFI_NTP(void)
{
 WiFi.disconnect(true,true);                                                                  // remove all previous settings and entered SSID and password
 delay(100);
 WiFi.setHostname(Mem.BLEbroadcastName);                                                      // Set the host name in the WIFI router instead of a cryptic esp32 name
 WiFi.mode(WIFI_STA);  
 WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE, INADDR_NONE);
 WIFIwasConnected = false;
 wifiEventHandler = WiFi.onEvent(WiFiEvent);                                                  // Using WiFi.onEvent interrupts and crashes IL9341 screen display while writing the screen
 WiFi.begin(Mem.SSID, Mem.Password);
 MDNS.begin(Mem.BLEbroadcastName);                                                            // After reset http://ESP32Nano.local 
 // Task function, Name, Stack size, Parameter, Priority, Task handle, Core 1
 xTaskCreatePinnedToCore(WebServerTask,"WebServerTask", 4096, NULL, 1, NULL, 1 );             // Start handleClient loop on core 1
 int tryDelay = 5000;                                                                         // Will try for about 50 seconds (20x 10,000ms)
 int numberOfTries = 10;
 while (true)                                                                                 // Wait for the WiFi event
  {
    switch(WiFi.status()) 
    {
     case WL_NO_SSID_AVAIL:
          Tekstprintln("[WiFi] SSID not found (Unexpected error)\n Is the router turned off?");
          return;
     case WL_CONNECT_FAILED:
          Tekstprint("[WiFi] Failed - WiFi not connected! Reason:? \n Reset the clock with option R and re-enter SSID and Password.");
          return;
     case WL_CONNECTION_LOST:
          Tekstprintln("[WiFi] Connection was lost");
          break;
     case WL_SCAN_COMPLETED:
          Tekstprintln("[WiFi] Scan is completed");
          break;
     case WL_DISCONNECTED:
          Tekstprintln("[WiFi] WiFi is disconnected, will reconnect");
          WiFi.reconnect();
          break;
     case WL_CONNECTED:
          sprintf(sptext, "IP Address: %d.%d.%d.%d", WiFi.localIP()[0], WiFi.localIP()[1], WiFi.localIP()[2], WiFi.localIP()[3] );
          Tekstprintln(sptext); 
          WIFIwasConnected = true;                                                            // No we know the SSID ans password are correct and we can reconnect
          Mem.WIFIcredentials = SET_AND_OK;
          StoreStructInFlashMemory();
          break;
     default:
          Serial.print("[WiFi] WiFi Status: ");
          Serial.println(WiFi.status());
          break;
    } 
  LEDstartup(orange);                                                                      // white colour in WS2812 and SK6812 LEDs
  if (WIFIwasConnected) break;       
  if(numberOfTries <= 0)
    {
     Tekstprintln("[WiFi] Failed to connect to WiFi!");
     WiFi.disconnect();                                                                     // Use disconnect function to force stop trying to connect
     WIFIwasConnected = false;
     switch(Mem.WIFIcredentials)
        {    
         case NOT_SET:
             Tekstprintln("Check SSID and password or turn WIFI in menu off with option W");
             break;               
         case SET:
             Tekstprintln("Check your SSID name and password.\nRe-enter your password with option B in the menu. Password is now deleted");    
             strcpy(Mem.Password,"");                                                               // This will force a restart is AP mode. PWD can not be checked in menu. SSID can be checked
             break;
         case SET_AND_OK:
             Tekstprintln("Check WIFI router. The router is probably turned off");                  
             break;       
         case IN_AP_NOT_SET:    
         default:    
             Tekstprintln("Unknown condition. Re-enter SSID and password. They are deleted.\nOr turn WIFI in the menu with option W off");      
             strcpy(Mem.SSID,"");
             strcpy(Mem.Password,"");  
             break;
        }
    return;
    } 
  else 
      { 
       delay(tryDelay);  
       numberOfTries--;    
      }
  }
if ( !WIFIwasConnected) return;                                                                  // If WIFI connection fails -> returm
//  sprintf(sptext, "IP Address: %d.%d.%d.%d", WiFi.localIP()[0], WiFi.localIP()[1], WiFi.localIP()[2], WiFi.localIP()[3] );  Tekstprintln(sptext); 
if(Mem.NTPOn )
  { 
   initSNTP();
   if(wait4SNTP())  Tekstprintln("NTP is On and synced");
   else             Tekstprintln("NTP is On but NOT synced");
   }   
 if(Mem.WIFIOn) WebPage();                                                                    // Show the web page if WIFI is on
 Tekstprint("Web page started\n");
// WiFi.removeEvent(wifiEventHandler);                                                        // You can leave it off because it undertakes no actions Remove the WIFI event handler
}
//--------------------------------------------                                                //
// WIFI WebServerTask on Core 1
// instead of 1000 loops/s it is now 210,000 loops/s 
//--------------------------------------------
void WebServerTask(void *pvParameters) 
{
  for (;;) 
  {
    server.handleClient();                                                                    // This is normally blocking
    vTaskDelay(1);                                                                            // Small delay to yield control
  }
}
//--------------------------------------------                                                //
// NTP functions
//--------------------------------------------
void NTPnotify(struct timeval* t) { Tekstprint("NTPtime synchronized\n");}
void setTimezone()                { setenv("TZ", Mem.Timezone, 1);  tzset(); }
void initSNTP() 
{  
 sntp_set_sync_interval(1 * 60 * 60 * 1000UL);  // 1 hour
 sntp_set_time_sync_notification_cb(NTPnotify);
 esp_sntp_setoperatingmode(ESP_SNTP_OPMODE_POLL);
 esp_sntp_setservername(0, "pool.ntp.org");
 esp_sntp_init();
 setTimezone();
}
//--------------------------------------------                                                //
// NTP Get a NTP time and wait max 5 sec 
//--------------------------------------------
bool wait4SNTP() 
{
 int32_t Tick = millis(); 
 bool SNTPtimeValid = true;
 while (sntp_get_sync_status() != SNTP_SYNC_STATUS_COMPLETED) 
  { if ((millis() - Tick) >5000) {SNTPtimeValid = false; break;}   }                          // Wait max 5 seconds 
return  SNTPtimeValid;
}

//--------------------------------------------                                                //
// WIFI WEBPAGE 
//--------------------------------------------
void WebPage(void)
{
  int i = 0, n;

  SWversion();                                                                                // Update version/menu info used in html_info

  for (n = 0; n < strlen(index_html_top); n++) HTML_page[i++] = index_html_top[n];             // Start building the HTML page
  if (i > MAXSIZE_HTML_PAGE - 999) 
    { strcat(HTML_page, "<br> *** INCREASE MAXSIZE_HTML_PAGE in Webpage.h ***<br><br><br>"); } 
  else 
    {
     sprintf(sptext, "<head><title>%s</title></head>",Mem.BLEbroadcastName);                  // insert the BLE name to the title of the web page
     for (n = 0; n < strlen(sptext); n++)            HTML_page[i++] = sptext[n];
     for (n = 0; n < strlen(html_info); n++)         HTML_page[i++] = html_info[n];
     for (n = 0; n < strlen(index_html_footer); n++) HTML_page[i++] = index_html_footer[n];
     HTML_page[i] = 0;                                                                        // Null-terminate
    }
  
  server.on("/", HTTP_GET, []() { server.send(200, "text/html", HTML_page);  });               // Root page handler

  server.on("/get", HTTP_GET, []()                                                             // GET handler for input parameter
    {
     String inputMessage;
     if (server.hasArg(PARAM_INPUT_1)) { inputMessage = server.arg(PARAM_INPUT_1); } 
     else { inputMessage = ""; }
     ReworkInputString(inputMessage);
     SWversion();
     int i = 0, n;
     for (n = 0; n < strlen(index_html_top); n++)    HTML_page[i++] = index_html_top[n];
     for (n = 0; n < strlen(html_info); n++)         HTML_page[i++] = html_info[n];
     for (n = 0; n < strlen(index_html_footer); n++) HTML_page[i++] = index_html_footer[n];
     HTML_page[i] = 0;
     server.send(200, "text/html", HTML_page);
    });

  server.on("/update", HTTP_GET, []() { server.send(200, "text/html", OTA_html); });             // OTA update page
  server.on("/update", HTTP_POST, []() 
     {                                                                                           // OTA upload handler
         shouldReboot = true;
         server.send(200, "text/html",
        "<!DOCTYPE html><html><body>"
        "<h2>Update successful. Rebooting...</h2>"
        "<p>You will be redirected shortly.</p>"
        "<script>setTimeout(()=>{location.href='/'}, 7000);</script>"
        "</body></html>");  
     }, []() 
     {
      HTTPUpload& upload = server.upload();
      if (upload.status == UPLOAD_FILE_START) 
        {
         Serial.printf("OTA Start: %s\n", upload.filename.c_str());
         if (!Update.begin(UPDATE_SIZE_UNKNOWN)) { Update.printError(Serial); }
        } 
      else if (upload.status == UPLOAD_FILE_WRITE) 
        {
        size_t written = Update.write(upload.buf, upload.currentSize);
        if (written != upload.currentSize) 
          {
           Serial.printf("OTA Write Failed: written %u != %u\n", written, upload.currentSize);
           Update.printError(Serial);
          }
        } 
      else if (upload.status == UPLOAD_FILE_END) 
        {
         if (Update.end(true)) { Serial.printf("OTA Success: %u bytes\n", upload.totalSize);} 
         else { Serial.printf("OTA End Failed\n");      Update.printError(Serial);   }
        } 
      else if (upload.status == UPLOAD_FILE_ABORTED) 
        {  Update.end();    Serial.println("OTA Aborted");      }
      }  );                                                                                   // END OTA upload handler
 server.onNotFound([]() {  server.send(404, "text/plain", "Not found");  });                  // WIFI WEBPAGE 404 handler
 server.begin();
}
//--------------------------------------------                                                //
// WIFI WEBPAGE Login credentials Access Point page with 192.168.4.1
//--------------------------------------------
void StartAPMode(void) 
{
 WiFi.disconnect(true);                                                                      // Disconnect and erase old config
 apMode = true;
 Mem.WIFIcredentials = IN_AP_NOT_SET;
 StoreStructInFlashMemory(); 
 WiFi.softAP(AP_SSID, AP_PASSWORD);
 delay(100);
 xTaskCreatePinnedToCore(WebServerTask,"WebServerTask", 4096, NULL, 1, NULL, 1 );            // Start handleClient loop on core 1
 dnsServer.start(53, "*", WiFi.softAPIP());
 Tekstprintln("\nConnect to StartWordcock in WIFI on your mobile.\nEnter password: wordclock\nThen in URL: 192.168.4.1 and enter router credentials");   
 server.on("/", HTTP_GET, []() {  server.send(200, "text/html", SoftAP_html); });            // Serve AP HTML form
 server.on("/", HTTP_POST, []()                                                              // Handle AP form POST
  {
    int params = server.args();
    for (int i = 0; i < params; i++) 
    {
      String name = server.argName(i);
      String value = server.arg(i);
      if (name == "ssid") strcpy(Mem.SSID, value.c_str());
      if (name == "pass") strcpy(Mem.Password, value.c_str());
    }
    StoreStructInFlashMemory();
    Mem.WIFIcredentials = SET;
    StoreStructInFlashMemory();  
    server.send(200, "text/plain", "Credentials saved. Restarting..."); 
    delay(300);
    ESP.restart();
  } );

 server.begin();

 Serial.println("AP Mode Started");
 Serial.print("AP SSID: ");       Serial.println(AP_SSID);
 Serial.print("AP IP Address: "); Serial.println(WiFi.softAPIP()); 
}

//--------------------------------------------                                                //
// WIFI WPS functions
//--------------------------------------------
void wpsInitConfig()
{
 config.wps_type = ESP_WPS_MODE;
 strcpy(config.factory_info.manufacturer, ESP_MANUFACTURER);
 strcpy(config.factory_info.model_number, ESP_MODEL_NUMBER);
 strcpy(config.factory_info.model_name, ESP_MODEL_NAME);
 strcpy(config.factory_info.device_name, ESP_DEVICE_NAME);
}

void wpsStart()
{
 if(esp_wifi_wps_enable(&config))  Serial.println("WPS Enable Failed");
 else if(esp_wifi_wps_start(0)) 	 Serial.println("WPS Start Failed");
}

void wpsStop()
{
 if(esp_wifi_wps_disable()) 	     Serial.println("WPS Disable Failed");
}

String wpspin2string(uint8_t a[])
{
 char wps_pin[9];
 for(int i=0;i<8;i++){ wps_pin[i] = a[i]; }
 wps_pin[8] = '\0';
 return (String)wps_pin;
}
//--------------------------------------------                                                //
// End WPS
//--------------------------------------------


//--------------------------------------------                                                //
// KEYPAD 3x1 Init 
//--------------------------------------------
 void InitKeypad3x1(void)
 {
 digitalWrite(COLPIN,LOW);
 sprintf(sptext,"3*1 keypad %s used", Mem.UseRotary==2?"IS":"NOT");
 Tekstprintln(sptext);
 }
//--------------------------------------------
// KEYPAD check for Keypad input
//--------------------------------------------                           
void Keypad3x1Check(void)
{ 
 digitalWrite(COLPIN,LOW);                                                                    // Mimic a key press on pin 6 in order to select the first column
 char Key = keypad.getKey();
 if(Key)
  {
   Serial.println(Key);
   if (Key == 'Y')    ProcessKeyPressTurn(0);                                                 // Pressing Middle button Yellow activates the keyboard input.   
   else if (ChangeTime)    
     { 
      if (Key == 'R') ProcessKeyPressTurn(1);                                                 // Pressing Red increases hour or minute. 
      if (Key == 'G') ProcessKeyPressTurn(-1);                                                // Pressing Green decreases hour or minute. 
     }
   delay(200);
  }
} 
//--------------------------------------------                                                //
// KY-040 ROTARY encoder Init 
//--------------------------------------------
 void InitRotaryMod(void)
 {
 pinMode(encoderPinA,  INPUT_PULLUP);
 pinMode(encoderPinB,  INPUT_PULLUP);  
 pinMode(clearButton,  INPUT_PULLUP); 
 myEnc.write(0);                                                                              // Clear Rotary encoder buffer
 sprintf(sptext,"Rotary %s used", Mem.UseRotary==1?"IS":"NOT");
 Tekstprintln(sptext);   
 } 
//--------------------------------------------                                                //
// KY-040 ROTARY check if the rotary is moving
//--------------------------------------------
void RotaryEncoderCheck(void)
{
 int ActionPress = 999;
 if (digitalRead(clearButton) == LOW )          ProcessKeyPressTurn(0);                       // Set the time by pressing rotary button
 else if (ChangeTime || ChangeLightIntensity)    
  {   
   ActionPress = myEnc.read();                                                                // If the knob is turned store the direction (-1 or 1)
   if (ActionPress == 0) {  ActionPress = 999;  ProcessKeyPressTurn(ActionPress);  }          // Sent 999 = nop (no operation) 
   if (ActionPress == 1 || ActionPress == -1 )  ProcessKeyPressTurn(ActionPress);             // Process the ActionPress
  } 
 myEnc.write(0);                                                                              // Set encoder pos back to 0

if ((unsigned long) (millis() - RotaryPressTimer) > 60000)                                    // After 60 sec after shaft is pressed time of light intensity can not be changed 
   {
    if (ChangeTime || ChangeLightIntensity)                         
      {
        Tekstprintln("<-- Changing time is over -->");
        NoofRotaryPressed = 0;
      }
    ChangeTime            = false;
    ChangeLightIntensity  = false;
   }   
}

//--------------------------------------------                                                //
// CLOCK KY-040 Rotary or Membrane 3x1 processing input
// encoderPos < 1 left minus 
// encoderPos = 0 attention and selection choice
// encoderPos > 1 right plus
//--------------------------------------------
void ProcessKeyPressTurn(int encoderPos)
{
 if (ChangeTime || ChangeLightIntensity)                                                      // If shaft is pressed time of light intensity can be changed
   {
    if ( encoderPos!=999 && ( (millis() - Looptime) > 250))                                   // If rotary turned avoid debounce within 0.25 sec
     {   
     Serial.print(F("----> Index:"));   Serial.println(encoderPos);
     if (encoderPos == 1)                                                                     // Increase  
       {     
        if (ChangeLightIntensity)  { WriteLightReducer(5); }                                  // If time < 60 sec then adjust light intensity factor
        if (ChangeTime) 
          {
           if (NoofRotaryPressed == 1)                                                        // Change hours
              {if( ++timeinfo.tm_hour >23) { timeinfo.tm_hour = 0; } }      
           if (NoofRotaryPressed == 2)                                                        // Change minutes
              {  timeinfo.tm_sec = 0;
               if( ++timeinfo.tm_min  >59) 
                 { timeinfo.tm_min  = 0; if( timeinfo.tm_hour >=23) { timeinfo.tm_hour = 0; } }   
              }
           } 
        }    
      if (encoderPos == -1)                                                                   // Decrease
       {
       if (ChangeLightIntensity)   { WriteLightReducer(-5); }    // If time < 60 sec then adjust light intensity factor
       if (ChangeTime)     
          {
           if (NoofRotaryPressed == 1)                                                        // Change hours
             {if( timeinfo.tm_hour-- ==0)  { timeinfo.tm_hour = 23; }  }      
           if (NoofRotaryPressed == 2)                                                        // Change minutes
             { 
              timeinfo.tm_sec = 0;
              if( timeinfo.tm_min-- == 0) 
                { timeinfo.tm_min  = 59; if( timeinfo.tm_hour  == 0) { timeinfo.tm_hour = 23; } }
             } 
           }          
        } 
      SetDS3231Time();  
      PrintDS3231Time();
      Displaytime();      
      Looptime = millis();    
     }                                                     
   }
 if (encoderPos == 0 )                                                                        // Set the time by pressing rotary button
   { 
    delay(250);
    ChangeTime            = false;
    ChangeLightIntensity  = false;
    RotaryPressTimer      = millis();                                                         // Record the time the shaft was pressed.
    if(++NoofRotaryPressed > 9) NoofRotaryPressed = 0;
    switch (NoofRotaryPressed)                                                                // No of times the rotary is pressed
      {
       case 1:  ChangeTime = true;                                  break;                    // 
       case 2:  ChangeTime = true;                                  break;                    //        
       case 3:  ChangeLightIntensity = true;                        break;                    // 
       case 4:                                                      break;                    // 
       case 5:                                                      break;                    // 
       case 6:                                                      break;                    // 
       case 7:                                                      break;                    //                                
       case 8:                                                      break;
       case 9:  Reset();                                            break;                    // Reset all settings                                                                  
      default:                                                      break;                     
      }
    Serial.print(F("NoofRotaryPressed: "));   Serial.println(NoofRotaryPressed);   
    Looptime = millis();
    Displaytime(); 
   }
 }

