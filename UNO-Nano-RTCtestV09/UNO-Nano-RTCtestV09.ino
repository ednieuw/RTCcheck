#include <Wire.h>
#include <RTClib.h>

#define TCA_ADDR 0x70                                                // TCA9548A I2C multiplexer adres (standaard 0x70)
#define MAX_RTC 8                                                    // Maximaal Noof RTC modules
#define FILENAAM (strrchr(__FILE__, '\\') ? strrchr(__FILE__, '\\') + 1 : __FILE__) 

RTC_DS3231 rtc[MAX_RTC];                                             // RTC objecten aanmaken
static uint32_t msTick;                                              // Number of millisecond ticks since we last incremented the second counter
bool rtcActive[MAX_RTC] = {false};                                   // Array om bij te houden welke RTC's werkend zijn
int activeRTCCount     = 0;
uint32_t  Loopcounter  = 0;                                          // Loop speed counter
char sptext[255];                                                    // text print buffer

//--------------------------------------------                       //
// SETUP
//--------------------------------------------
void setup() 
{
  Serial.begin(115200);
  Wire.begin();

  Serial.println("DS3231 RTC Multiplexer Test");
  Serial.println("===========================");
   sprintf(sptext,"Software: %s",FILENAAM); Tekstprintln(sptext);   
  
  Serial.print("TCA9548A search on addres 0x");                      // Test or TCA9548A is available
  Serial.print(TCA_ADDR, HEX);
  Serial.print("... ");
  
  Wire.beginTransmission(TCA_ADDR);
  if (Wire.endTransmission() == 0) {
    Serial.println("FOUND!");
  } else {
    Serial.println("NOT FOUND!");
    Serial.println("check wiring and power");
    while(1);                                                       // 
  }  
  Serial.println("\nScanning DS3231 modules...");                   // Scan which channels a DS3231 has
  for (int i = 0; i < MAX_RTC; i++) 
   {
    selectTCAChannel(i);
    delay(50); 
    Wire.beginTransmission(0x68);                                   // DS3231 address
    uint8_t error = Wire.endTransmission();
    
    if (error == 0) 
     {
      Serial.print("DS3231 found on channel ");
      Serial.println(i);
     } 
     else
     {
      Serial.print("No DS3231 on channel ");
      Serial.print(i);
      Serial.print(" (Error: ");
      Serial.print(error);
      Serial.println(")");
     }
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
    
    if (!rtc[i].begin()) {
      Serial.println("FAILED!");
      Serial.println("Check connection on channel " + String(i));
      rtcActive[i] = false;
    } else {
      Serial.println("OK!");
      rtcActive[i] = true;
      activeRTCCount++;
      
      
      DateTime now = rtc[i].now();                                   // Show current
      sprintf(sptext,"Before update: %02d/%02d/%04d %02d:%02d:%02d ", now.day(),now.month(),now.year(),
                                                  now.hour(),now.minute(),now.second());
      Tekstprintln(sptext);
      
      // // Set time to compilation time
      // DateTime compileTime = DateTime(F(__DATE__), F(__TIME__));
      // Serial.print("  Time set to: ");
      // Serial.println(compileTime.timestamp(DateTime::TIMESTAMP_FULL));
      // rtc[i].adjust(compileTime);
      // delay(100);

      DateTime newTime = rtc[i].now();                               // Check time set
      sprintf(sptext," After update: %02d/%02d/%04d %02d:%02d:%02d ", newTime.day(),newTime.month(),newTime.year(),
                                                  newTime.hour(),newTime.minute(),newTime.second());
      Tekstprintln(sptext);  
    }
    delay(200); 
  }
  
  Serial.print("Setup finished! ");
  Serial.print(activeRTCCount);
  Serial.println(" RTC modules active.");
  Serial.println();
}
//--------------------------------------------                       //
// LOOP
//--------------------------------------------
void loop() 
{
  CheckSerialInput();
  Every5SecondCheck(); 
}

//--------------------------------------------                       //
// COMMON Update routine 
// Performs tasks every 5 seconds
//--------------------------------------------
void Every5SecondCheck(void)
{
 uint32_t msLeap = millis() - msTick;                                // 
 Loopcounter++;
 if (msLeap >4999)                                                   // Every 5 second enter the loop
 {
   msTick = millis();
   Loopcounter /= 5;
   PrintRTCtimes();
   Loopcounter = 0;
 }  
}

//--------------------------------------------                       //
// Check voor seriële input voor commando's
//-------------------------------------------- 
void CheckSerialInput(void)
{
  if (Serial.available() > 0) 
  {
    String command = Serial.readStringUntil('\n');
    command.trim();
    
    if (command == "sync") 
    {
      synchronizeAllRTCs();
      return;
    }
    if (command == "set") {
      setCurrentTime();
      return;
    }
    if (command == "help") {
      Serial.println("available commands:");
      Serial.println("  sync - Synchroniseer all RTC's with RTC 0");
      Serial.println("  set  - Enter specific time");
      Serial.println("  help - this help");
      return;
    }
  }
}
//--------------------------------------------                       //
// Print RTC times
//-------------------------------------------- 
void PrintRTCtimes(void)
{
  for (int i = 0; i < MAX_RTC; i++)                                  // Read time of all ACTIVE RTC modules
  {
    if (!rtcActive[i]) continue;                                     // Skip non-active RTC's
    selectTCAChannel(i);
    delay(10);
    char Temp[16];
    dtostrf(rtc[i].getTemperature(), 0, 2, Temp); 
    DateTime now = rtc[i].now();
    sprintf(sptext,"RTC %d: %02d/%02d/%04d %02d:%02d:%02d T: %s C",
             i, now.day(),now.month(),now.year(), now.hour(),now.minute(),now.second(), Temp);
    Tekstprintln(sptext);    
  }
  Serial.println("-------------------");
  Serial.print(Loopcounter);
  Serial.println(" loop/s. Type 'sync' to synchronise all RTC's");
}

//--------------------------------------------                       //
// COMMON common print routines
//--------------------------------------------
void Tekstprint(char const *tekst)    { if(Serial) Serial.print(tekst); }  //Serial.print(strlen(sptext));
void Tekstprintln(char const *tekst)  { sprintf(sptext,"%s\n",tekst); Tekstprint(sptext); }

//--------------------------------------------                       //
// Function to select TCA9548A channel
//--------------------------------------------
void selectTCAChannel(uint8_t channel) 
{
  if (channel > 7) return;                                           // Maximaal 8 kanalen (0-7)
  Wire.beginTransmission(TCA_ADDR);
  Wire.write(1 << channel);                                          // Select channel by setting bit
  uint8_t error = Wire.endTransmission();
  
  if (error != 0) 
   {
    Serial.print("Error selecting channel ");
    Serial.print(channel);
    Serial.print(", error code: ");
    Serial.println(error);
  }
  delay(10);                                                         // short delay after channel change
}
//--------------------------------------------                       //
// RTC operations
//--------------------------------------------
void setTimeForRTC(int rtcIndex, DateTime newTime) 
{
  if (!rtcActive[rtcIndex]) return;
  selectTCAChannel(rtcIndex);
  rtc[rtcIndex].adjust(newTime);
}

void printTimeForRTC(int rtcIndex) 
{
  if (!rtcActive[rtcIndex]) return;
  selectTCAChannel(rtcIndex);
  DateTime now = rtc[rtcIndex].now();
  
  Serial.print("RTC ");
  Serial.print(rtcIndex);
  Serial.print(": ");
  Serial.print(now.timestamp(DateTime::TIMESTAMP_FULL));
  Serial.println();
}

float getTemperatureForRTC(int rtcIndex) 
{
  if (!rtcActive[rtcIndex]) return -999;
  selectTCAChannel(rtcIndex);
  return rtc[rtcIndex].getTemperature();
}
//--------------------------------------------                                                //
// Synchronize all RTC's with current time
//--------------------------------------------
void synchronizeAllRTCs()   // Zoek eerste actieve RTC als referentie
{
  int referenceRTC = -1;
  for (int i = 0; i < MAX_RTC; i++) 
     {  if (rtcActive[i]) { referenceRTC = i;  break;  }  }
  if (referenceRTC == -1) 
     {  Serial.println("No active RTC's found!");   return;  }
  
  selectTCAChannel(referenceRTC);
  DateTime referenceTime = rtc[referenceRTC].now();
  Serial.println("\n=== RTC SYNCHRONISATION ===");
  Serial.print("Reference time of RTC ");
  Serial.print(referenceRTC);
  Serial.print(": ");
  Serial.println(referenceTime.timestamp(DateTime::TIMESTAMP_FULL));
  
  for (int i = 0; i < MAX_RTC; i++) 
   {
    if (!rtcActive[i]) continue;
    selectTCAChannel(i);
    delay(50);
    DateTime before = rtc[i].now();                                  // Show time before update
    Serial.print("RTC ");
    Serial.print(i);
    Serial.print(" before sync: ");
    Serial.println(before.timestamp(DateTime::TIMESTAMP_FULL));
    rtc[i].adjust(referenceTime);                                    // Update time
    delay(100); 
    DateTime after = rtc[i].now();                                   // Verify
    Serial.print("RTC ");
    Serial.print(i);
    Serial.print(" after sync:  ");
    Serial.println(after.timestamp(DateTime::TIMESTAMP_FULL));
  }
 Serial.println("Synchronisation done!\n");
}

//--------------------------------------------                       //
// Set time by hand in serial monitor
//--------------------------------------------
void setCurrentTime() 
{
  Serial.println("\n=== SET TIME  ===");
  Serial.println("Enter date and time:");
  Serial.println("Format: YYYY MM DD HH MM SS");
  Serial.println("Example: 2024 3 15 14 30 0");
  Serial.print("Input: ");
  
  int n=0;
  while (!Serial.available()) 
  { 
   delay(100); 
   if (++n > 600) return;                                            // Wait for input max 60 seconds
  } 
  String input = Serial.readStringUntil('\n');
  Serial.println(input);

  int year, month, day, hour, minute, second;                        // Parse input met sscanf (simpler than strtok)
  int parsed = sscanf(input.c_str(), "%d %d %d %d %d %d", &year, &month, &day, &hour, &minute, &second);
  
  if (parsed == 6) 
  {
    DateTime newTime(year, month, day, hour, minute, second);
    Serial.print("New time: ");
    Serial.println(newTime.timestamp(DateTime::TIMESTAMP_FULL));
    
    for (int i = 0; i < MAX_RTC; i++) 
    {
      if (!rtcActive[i]) continue;
      selectTCAChannel(i);
      delay(50);
      
      DateTime before = rtc[i].now();                                // Show time before update

      Serial.print("RTC ");
      Serial.print(i);
      Serial.print(" before: ");
      Serial.println(before.timestamp(DateTime::TIMESTAMP_TIME));
      
      rtc[i].adjust(newTime);
      delay(100);
     
      DateTime after = rtc[i].now();                                 // Verify
      Serial.print("RTC ");
      Serial.print(i);
      Serial.print(" na:   ");
      Serial.println(after.timestamp(DateTime::TIMESTAMP_TIME));
    }
    Serial.println("All active RTC's updated!\n");
  } 
  else {Serial.println("Wrong entry format. Try again\n"); }
}