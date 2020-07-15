
#include <SPI.h>
#include <SD.h>
// #include <LowPower.h>
#include <RTCZero.h>
#include <stdlib.h>

#include "memorysaver.h"
//This demo can only work on OV5640_MINI_5MP_PLUS or OV5642_MINI_5MP_PLUS platform.

// #ifndef SDCARD_SS_PIN
	// #define SDCARD_SS_PIN 28
// #endif

#define   FRAMES_NUM    0x00

// set pin 7 as the slave select for the digital pot:
const int CS = 7;
int capture_count = 0;
bool is_header = false;
int total_time = 0;


RTCZero rtc;




void setup() {
  // put your setup code here, to run once:
  


  
  rtc.begin();
  
 rtc.setTime(0, 0, 0);
 rtc.setDate(0, 0, 0);  
  

  Serial1.begin(115200);
  Serial1.println(F("ArduCAM Start!"));
  // set the CS as an output:
  pinMode(CS, OUTPUT);
  digitalWrite(CS, HIGH);
  // initialize SPI:
  SPI.begin();
  
  camWake();


  //Initialize SD Card
  while (!SD.begin(SDCARD_SS_PIN))
  {
    Serial1.println(F("SD Card Error!")); delay(1000);
  }
  Serial1.println(F("SD Card detected."));
  //Change to JPEG capture mode and initialize the OV5640 module

}

void camWake(){
	uint8_t vid, pid;
	uint8_t temp;
	
	
	
  
	



    if ((vid != 0x26 ) && (( pid != 0x41 ) || ( pid != 0x42 ))) {
      Serial1.println(F("ACK CMD Can't find OV2640 module!"));

    }
    else {
      Serial1.println(F("ACK CMD OV2640 detected."));
    }
  
	
}

void camSleep(){
	 
}

void loop() {
  
  uint8_t reg;
	
  total_time = millis();
  
 

  //Start capture
  Serial1.println(F("start capture"));
  

 
  
  
  
  
  Serial1.println(F("saved capture"));
 

  //Clear the capture done flag
  
  
  total_time -= millis();
  total_time += 1000;
  if (total_time > 0){
  delay(total_time);
  }else{
  }
  
  Serial1.print(F("CAM Capture Done: "));
   Serial1.println(total_time, DEC);

  if(capture_count >= 5){
  Serial1.print(F("Sleep"));  
  camSleep();
 

   rtc.attachInterrupt(alarmMatch);
   
   uint32_t now = rtc.getEpoch();
	rtc.setAlarmEpoch(now + 7);
	rtc.enableAlarm(rtc.MATCH_HHMMSS);

	// USBdetach();
   rtc.standbyMode();
    USBDevice.attach();
    // Serial1.begin(115200);
  
  

  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);
  delay(300);
  digitalWrite(LED_BUILTIN, LOW);
  
  camWake();
  
  
  capture_count = 0;
  }
  capture_count++;
}

void alarmMatch()
{
 
}

// void USBdetach()
// { 
  // // disable the USB
  // USBCON |= _BV(FRZCLK);  //freeze USB clock
  // PLLCSR &= ~_BV(PLLE);   // turn off USB PLL
  // USBCON &= ~_BV(USBE);   // disable USB
// }





