// ArduCAM Mini demo (C)2018 Lee
// Web: http://www.ArduCAM.com
// This program is a demo of how to use the enhanced functions
// This demo was made for ArduCAM_Mini_2MP_Plus.
// It can  continue shooting and store it into the SD card  in JPEG format
// The demo sketch will do the following tasks
// 1. Set the camera to JPEG output mode.
// 2. Capture a JPEG photo and buffer the image to FIFO
// 3.Write the picture data to the SD card
// 5.close the file
//You can change the FRAMES_NUM count to change the number of the picture.
//IF the FRAMES_NUM is 0X00, take one photos
//IF the FRAMES_NUM is 0X01, take two photos
//IF the FRAMES_NUM is 0X02, take three photos
//IF the FRAMES_NUM is 0X03, take four photos
//IF the FRAMES_NUM is 0X04, take five photos
//IF the FRAMES_NUM is 0X05, take six photos
//IF the FRAMES_NUM is 0X06, take seven photos
//IF the FRAMES_NUM is 0XFF, continue shooting until the FIFO is full
//You can see the picture in the SD card.
// This program requires the ArduCAM V4.0.0 (or later) library and ArduCAM_Mini_2MP_Plus
// and use Arduino IDE 1.6.8 compiler or above

#include <Wire.h>
#include <ArduCAM.h>
#include <SPI.h>
#include <SD.h>
#include <RTCZero.h>


#include "memorysaver.h"


#define BMPIMAGEOFFSET 66
const char bmp_header[BMPIMAGEOFFSET] PROGMEM =
{
  0x42, 0x4D, 0x36, 0x58, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x42, 0x00, 0x00, 0x00, 0x28, 0x00,
  0x00, 0x00, 0x40, 0x01, 0x00, 0x00, 0xF0, 0x00, 0x00, 0x00, 0x01, 0x00, 0x10, 0x00, 0x03, 0x00,
  0x00, 0x00, 0x00, 0x58, 0x02, 0x00, 0xC4, 0x0E, 0x00, 0x00, 0xC4, 0x0E, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF8, 0x00, 0x00, 0xE0, 0x07, 0x00, 0x00, 0x1F, 0x00,
  0x00, 0x00
};
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
String fname;

ArduCAM myCAM( OV2640, CS );

RTCZero rtc;

uint8_t read_fifo_burst(ArduCAM myCAM);

void wakeUp(){}

void setup() {
   pinMode(4, INPUT);
	attachInterrupt(digitalPinToInterrupt(4), wakeUp, LOW); 
  // put your setup code here, to run once:
  


  Wire.begin();
  
	rtc.begin();
  
	rtc.setTime(0, 0, 0);
	rtc.setDate(0, 0, 0);  
  

  Serial1.begin(9600);
  Serial1.println(F("R"));
  // set the CS as an output:
  pinMode(CS, OUTPUT);
  digitalWrite(CS, HIGH);

  
  // initialize SPI:
  SPI.begin();
  
  camWake();


  //Initialize SD Card
  while (!SD.begin(SDCARD_SS_PIN))
  {
    Serial1.println(F("SD Card Error!"));
	delay(1000);
  }
  //Serial1.println(F("SD Card detected."));

}

void camWake(){
	uint8_t vid, pid;
	uint8_t temp;
	
	Serial1.println("W");
	pinMode(5, OUTPUT);
	digitalWrite(5, LOW);
	delay(20);
	
	 //myCAM.clear_bit(ARDUCHIP_GPIO,GPIO_PWDN_MASK);
  
	
	//Reset the CPLD
myCAM.write_reg(0x07, 0x80);
delay(100);
myCAM.write_reg(0x07, 0x00);
delay(100);
  while (1) {
    //Check if the ArduCAM SPI bus is OK
    myCAM.write_reg(ARDUCHIP_TEST1, 0x55);
    temp = myCAM.read_reg(ARDUCHIP_TEST1);
    if (temp != 0x55)
    {
      Serial1.println(F("SPI interface Error!"));
      delay(1000); continue;
    } else {
      //Serial1.println(F("SPI interface OK."));
	  break;
    }
  }

  while (1) {
    //Check if the camera module type is OV2640
    myCAM.wrSensorReg8_8(0xff, 0x01);
    myCAM.rdSensorReg8_8(OV2640_CHIPID_HIGH, &vid);
    myCAM.rdSensorReg8_8(OV2640_CHIPID_LOW, &pid);
    if ((vid != 0x26 ) && (( pid != 0x41 ) || ( pid != 0x42 ))) {
      Serial1.println(F("ACK CMD Can't find OV2640 module!"));
      delay(1000); continue;
    }
    else {
      //Serial1.println(F("ACK CMD OV2640 detected."));
	  break;
    }
  }
	  //Change to JPEG capture mode and initialize the OV5640 module
  myCAM.set_format(BMP);
  myCAM.InitCAM();
  myCAM.OV2640_set_Light_Mode(Sunny);
  // myCAM.OV2640_set_Color_Saturation(Saturation0);
  // myCAM.OV2640_set_Brightness(Brightness0);
  // myCAM.OV2640_set_Contrast(Contrast0);
  // myCAM.OV2640_set_Special_effects(Normal);
 //myCAM.write_reg(ARDUCHIP_FRAMES, FRAMES_NUM);
 ////take dummy shot to get exposure right
  // myCAM.flush_fifo();
  // myCAM.clear_fifo_flag();
  // myCAM.start_capture();
  // while ( !myCAM.get_bit(ARDUCHIP_TRIG, CAP_DONE_MASK));
  // myCAM.flush_fifo();
  // myCAM.clear_fifo_flag();
  
  
  
  
  
}

void camSleep(){
	 digitalWrite(5, HIGH);
}

void capture(){
	
	
	
	uint8_t reg;
	camWake();
 for (int iter = 0; iter < 5; iter++){
  
  
	
  total_time = millis();
  
  // put your main code here, to run repeatedly:
  myCAM.flush_fifo();
  myCAM.clear_fifo_flag();

  

  //Start capture
   //Serial1.println(F("start capture"));
  

  myCAM.start_capture();
  while ( !myCAM.get_bit(ARDUCHIP_TRIG, CAP_DONE_MASK));
  
  
  read_fifo_burst(myCAM);
  
  //Serial1.println(F("saved capture"));
 

  //Clear the capture done flag
  myCAM.clear_fifo_flag();
  
  total_time -= millis();
  total_time += 2000;
  if (total_time > 0){
  delay(total_time);
  }else{
  }
  
  // Serial1.print(F("CAM Capture Done: "));
   // Serial1.println(total_time, DEC);
}
  
  Serial1.print(F("S"));  
  camSleep();
	
}

void loop() {
  
  
 

    rtc.attachInterrupt(alarmMatch);
   
    uint32_t now = rtc.getEpoch();
	rtc.setAlarmEpoch(now + 3600);
	rtc.enableAlarm(rtc.MATCH_HHMMSS);


   rtc.standbyMode();
   
   sendimg();
   
  capture();
  
  
}

void sendimg(){
	Serial1.print(fname);
	Serial1.print('T');
}



uint8_t read_fifo_burst(ArduCAM myCAM)
{
  uint32_t length = 0;
  static int i = 0;
  static int k = 0;
  
  char VH, VL;
  int row  = 0;
  int col  = 0;
  char str[16];
  File outFile;
  File logFile;
  byte buf[256];
  length = myCAM.read_fifo_length();
  //Serial1.print(F("The fifo length is :"));
  //Serial1.println(length, DEC);
  if (length >= MAX_FIFO_SIZE) //8M
  {
    //Serial1.println("Over size.");
    return 0;
  }
  if (length == 0 ) //0 kb
  {
    //Serial1.println(F("Size is 0."));
    return 0;
  }
  
	 myCAM.CS_HIGH();
      //Create a avi file
      k = k + 1;
      itoa(k, str, 10);
      strcat(str, ".bmp");
	 
	  fname = str;
	 
      //Open the new file
      outFile = SD.open(str, O_WRITE | O_CREAT | O_TRUNC);
      if (! outFile)
      {
        //Serial1.println(F("File open failed"));
        while (1);
      }
  
    myCAM.CS_LOW();
    myCAM.set_fifo_burst();//Set fifo burst mode
  
  for ( i = 0; i < BMPIMAGEOFFSET; i++){
	  buf[i] = pgm_read_byte(&bmp_header[i]);
  }
  
    
  for (row = 0; row < 240; row++)
      {
        for (col = 0; col < 320; col++)
        {
          VH = SPI.transfer(0x00);
          VL = SPI.transfer(0x00);
          
		  buf[i++] = VL;
		  buf[i++] = VH;
			
		  if (i > 254)
		  {
			myCAM.CS_HIGH();
			outFile.write(buf, i);
			i = 0;
			myCAM.CS_LOW();
			myCAM.set_fifo_burst();
		  }
        }
      }
	myCAM.CS_HIGH();
	outFile.write(buf, i);
	//Close the file
	outFile.close();
	//Serial1.println(F("OK"));

  myCAM.CS_HIGH();
  logFile = SD.open("time.csv", FILE_WRITE);
      if (! logFile)
      {
        //Serial1.println(F("Log File open failed"));
        while (1);
      }
  logFile.print(str);
  logFile.print(',');
 logFile.print(rtc.getSeconds());
  logFile.print(',');
  logFile.print(rtc.getMinutes());
  logFile.print(',');
 logFile.print(rtc.getHours());
  logFile.print(',');
 logFile.print(rtc.getDay());
  logFile.print(',');
 logFile.print(rtc.getMonth());
  logFile.print(',');
 logFile.print(rtc.getYear());
  logFile.println();
  logFile.close();
  //Serial1.println(F("time logged"));

	fname += ',';
	fname += rtc.getSeconds();
	fname += ',';
	fname += rtc.getMinutes();
	fname += ',';
	fname += rtc.getHours();
	fname += ',';
	fname += rtc.getDay();
	fname += ',';
	fname += rtc.getMonth();
	fname += ',';
	fname += rtc.getYear();

  myCAM.CS_LOW();
  
  
  return 1;
}
void alarmMatch()
{
 
}

