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


//make time stamping system
//sleep
//set exposure

#include <avr/power.h>
#include <SoftwareSerial.h>
#include <Wire.h>
#include <ArduCAM.h>
#include <SPI.h>
#include <SD.h>
#include "memorysaver.h"



#define BAUDRATE 9600 // MUST be below 19200 (for stability) but 9600 is more stable

#define CHARBUFF 128 //SIM7000 serial response buffer //longer than 255 will cause issues
#define MAXTRASMITINTERVAL 18000//milli seconds

// For SIM7000 BoSL board
#define PWRKEY 4
#define DTR 5 // Connect with solder jumper
#define BOSL_RX 3 // Microcontroller RX
#define BOSL_TX 2 // Microcontroller TX

#define SITEID "TURB_TEST"

#define   FRAMES_NUM    0x03
// set pin 7 as the slave select for the digital pot:
const int CS = 7;
#define SD_CS 10
bool is_header = false;
int total_time = 0;
int last_file = 0;

extern volatile unsigned long timer0_millis;

char CBC[5];

char response[CHARBUFF]; //sim7000 serial response buffer

String dataStr; //Transmit URL

//SIM7000 serial object
SoftwareSerial simCom = SoftwareSerial(BOSL_RX, BOSL_TX);

ArduCAM myCAM( OV2640, CS );

uint8_t read_fifo_burst(ArduCAM myCAM);


void setup() {
	
 simCom.begin(BAUDRATE);
   Serial.begin(9600);

  Serial.println("Initialising SIM 7000");
  //initialise sim (on arduino startup only)
 simOn();
 simInit();
	
 netReg();
 ///
 netUnreg();

 simOff();
	
  // put your setup code here, to run once:
  uint8_t vid, pid;
  uint8_t temp;


  Serial.println(F("ArduCAM Start!"));
  // set the CS as an output:
  pinMode(CS, OUTPUT);
  digitalWrite(CS, HIGH);
  // initialize SPI:
  SPI.begin();
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
      Serial.println(F("SPI interface Error!"));
      delay(1000);
	  continue;
    } else {
      Serial.println(F("SPI interface OK."));
	  break;
    }
  }

  while (1) {
    //Check if the camera module type is OV2640
    myCAM.wrSensorReg8_8(0xff, 0x01);
    myCAM.rdSensorReg8_8(OV2640_CHIPID_HIGH, &vid);
    myCAM.rdSensorReg8_8(OV2640_CHIPID_LOW, &pid);
    if ((vid != 0x26 ) && (( pid != 0x41 ) || ( pid != 0x42 ))) {
      Serial.println(F("ACK CMD Can't find OV2640 module!"));
      delay(1000); continue;
    }
    else {
      Serial.println(F("ACK CMD OV2640 detected.")); break;
    }
  }

  //Initialize SD Card
  while (!SD.begin(SD_CS))
  {
    Serial.println(F("SD Card Error!")); delay(1000);
  }
  Serial.println(F("SD Card detected."));
  //Change to JPEG capture mode and initialize the OV5640 module
  myCAM.set_format(JPEG);
  myCAM.InitCAM();
  myCAM.clear_fifo_flag();
  myCAM.write_reg(ARDUCHIP_FRAMES, FRAMES_NUM);
}

void loop() {
  // put your main code here, to run repeatedly:
  myCAM.flush_fifo();
  myCAM.clear_fifo_flag();

  myCAM.OV2640_set_JPEG_size(OV2640_160x120);

  //Start capture
  myCAM.start_capture();
  Serial.println(F("start capture."));

  
  while ( !myCAM.get_bit(ARDUCHIP_TRIG, CAP_DONE_MASK));
  Serial.println(F("CAM Capture Done."));


  read_fifo_burst(myCAM);

  Serial.print(F("saved"));

  //Clear the capture done flag
  myCAM.clear_fifo_flag();
  
	simOn();
            
	netUnreg();
	CBCread();

	transmit(); 

	simOff();
  
  delay(5000);
  
  
}

////reads battery voltage from responce char array////
void storeCBCresponse(){
    
    bool end = 0;
    uint8_t x = 0;
    uint8_t j = 0;
    
    memset(CBC, '\0', 5);
    
    //loop through reponce to extract data
    for (uint8_t i=0; i < CHARBUFF; i++){

        //string splitting cases
        switch(response[i]){
        case ':':
            x = 9;
            j=0;
            i += 2;
            break;

        case ',':
            x++;
            j=0;
            break;

        case '\0':
            end = 1;
            break;
        case '\r':
            x++;
            j=0;
            break;
        }
        //write to char arrays
        if (response[i] != ','){
            switch(x){
                case 11:
                    CBC[j] = response[i];
                break;             
            }
            //increment char array counter
            j++;
        }
        //break loop when end flag is high
        if (end){
            i = CHARBUFF; 
        }
    }

}

void transmit(){
    
    dataStr = "AT+HTTPPARA=\"URL\",\"http://www.bosl.com.au/IoT/testing/scripts/WriteMe.php?SiteName=";
    
    dataStr += SITEID;
    dataStr += ".csv&T=";
    dataStr += CBC;
    dataStr += "&D=";
    dataStr += last_file;
    dataStr += "\"";
    
    netReg();
    
    
    ///***check logic
   //set CSTT - if it is already set, then no need to do again...
        sendATcmd(F("AT+CSTT?"), "OK",1000);   
        if (strstr(response, "mdata.net.au") != NULL){
            //this means the cstt has been set, so no need to set again!
       } else {
            sendATcmd(F("AT+CSTT=\"mdata.net.au\""), "OK",1000);
        }
    
    
    //close open bearer
    sendATcmd(F("AT+SAPBR=2,1"), "OK",1000);
    if (strstr(response, "1,1") == NULL){
        if (strstr(response, "1,3") == NULL){
        sendATcmd(F("AT+SAPBR=0,1"), "OK",1000);
        }
        sendATcmd(F("AT+SAPBR=1,1"), "OK",1000);
    }
    
    sendATcmd(F("AT+HTTPINIT"), "OK",1000);
    sendATcmd(F("AT+HTTPPARA=\"CID\",1"), "OK",1000);
   
    sendATcmd(dataStr, "OK",1000);
   
   sendATcmd(F("AT+HTTPACTION=0"), "200",2000);
    sendATcmd(F("AT+HTTPTERM"), "OK",1000);
  //close the bearer connection
    sendATcmd(F("AT+SAPBR=0,1"), "OK",1000);
    
    netUnreg();
    

}

void netUnreg(){
    sendATcmd(F("AT+CFUN=0"), "OK", 1000);
}

////register to network////
void netReg(){
    sendATcmd(F("AT+CFUN=0"), "OK", 1000);
    
    if(sendATcmd(F("AT+CFUN=1"), "+CPIN: READY", 1000) == 0){
        sendATcmd(F("AT+CFUN=6"), "OK", 10000);
        xDelay(10000);
        
        sendATcmd(F("AT+CFUN=1"), "OK", 1000);
    }
    xDelay(2000);
    sendATcmd(F("AT+CREG?"), "+CREG: 0,1", 2000);
}

////sends at command, checks for reply////
bool sendATcmd(String ATcommand, char* expctAns, unsigned int timeout){
    uint32_t timeStart;
    bool answer;
    uint8_t a=0;
    
    do{a++;
    
    Serial.println(ATcommand);
    
    answer=0;
    
    timeStart = 0;


    delay(100);

    while( simCom.available() > 0) {
        simCom.read();    // Clean the input buffer
    }
    
    simCom.println(ATcommand);    // Send the AT command 


    uint8_t i = 0;
    timeStart = millis();
    memset(response, '\0', CHARBUFF);    // Initialize the string

    // this loop waits for the answer

    do{
        if(simCom.available() != 0){    
            response[i] = simCom.read();
            i++;
            // check if the desired answer is in the response of the module
            if (strstr(response, expctAns) != NULL)    
            {
                answer = 1;
            }
        }    
            
            
        
        // Waits for the asnwer with time out
    }
    while((answer == 0) && ((millis() - timeStart) < timeout)); 

    if (expctAns == "0"){
                answer = 1;
            }
    Serial.println(response);
    
    }while(answer == 0 && a < 5);
    
     a = 0;
     return answer;
}



////initialises sim on arduino startup////
void simInit(){
   
      sendATcmd(F("AT+IPR=9600"),"OK",1000);
      
      sendATcmd(F("ATE0"),"OK",1000);
      
      sendATcmd(F("AT&W0"),"OK",1000);
  
}


////powers on SIM7000////
void simOn() {
    //do check for if sim is on
	pinMode(PWRKEY, OUTPUT);
	pinMode(BOSL_TX, OUTPUT);
	digitalWrite(BOSL_TX, HIGH);
	pinMode(BOSL_RX, INPUT_PULLUP);


	digitalWrite(PWRKEY, LOW);
	// See spec sheets for your particular module
	xDelay(1000); // For SIM7000

	digitalWrite(PWRKEY, HIGH);
    xDelay(4000);
}

////powers off SIM7000////
void simOff() {
	//  TX / RX pins off to save power

	digitalWrite(BOSL_TX, LOW);
	digitalWrite(BOSL_RX, LOW);

	digitalWrite(PWRKEY, LOW);

	// See spec sheets for your particular module
	xDelay(1200); // For SIM7000

	digitalWrite(PWRKEY, HIGH);
    xDelay(2000);
}

void CBCread(){
    //get GNSS data
    if (sendATcmd(F("AT+CBC"), "OK",1000)){
        
        storeCBCresponse();
        
    }
}

//like delay but lower power and more dodgy//
void xDelay(uint32_t tmz){
	
	delay(tmz);
	
	// uint32_t tmzslc = tmz/64;
	// //64 chosen as ballance between power savings and time spent in full clock mode
	// clock_prescale_set(clock_div_64);
		// delay(tmzslc);
	// clock_prescale_set(clock_div_1);
	
	// cli();
	// timer0_millis += 63*tmzslc;	
	// sei();
	
	// delay(tmz-64*tmzslc);
}

uint8_t read_fifo_burst(ArduCAM myCAM)
{
  uint8_t temp = 0, temp_last = 0;
  uint32_t length = 0;
  static int i = 0;
  static int k = 0;
  char str[16];
  File outFile;
  byte buf[256];
  length = myCAM.read_fifo_length();
  Serial.print(F("The fifo length is :"));
  Serial.println(length, DEC);
  if (length >= MAX_FIFO_SIZE) //8M
  {
    Serial.println("Over size.");
    return 0;
  }
  if (length == 0 ) //0 kb
  {
    Serial.println(F("Size is 0."));
    return 0;
  }
  myCAM.CS_LOW();
  myCAM.set_fifo_burst();//Set fifo burst mode
  i = 0;
  while ( length-- )
  {
    temp_last = temp;
    temp =  SPI.transfer(0x00);
    //Read JPEG data from FIFO
    if ( (temp == 0xD9) && (temp_last == 0xFF) ) //If find the end ,break while,
    {
      buf[i++] = temp;  //save the last  0XD9
      //Write the remain bytes in the buffer
      myCAM.CS_HIGH();
      outFile.write(buf, i);
      //Close the file
      outFile.close();
      Serial.println(F("OK"));
      is_header = false;
      myCAM.CS_LOW();
      myCAM.set_fifo_burst();
      i = 0;
    }
    if (is_header == true)
    {
      //Write image data to buffer if not full
      if (i < 256)
        buf[i++] = temp;
      else
      {
        //Write 256 bytes image data to file
        myCAM.CS_HIGH();
        outFile.write(buf, 256);
        i = 0;
        buf[i++] = temp;
        myCAM.CS_LOW();
        myCAM.set_fifo_burst();
      }
    }
    else if ((temp == 0xD8) & (temp_last == 0xFF))
    {
      is_header = true;
      myCAM.CS_HIGH();
      //Create a avi file
      k = k + 1;
	  last_file = k;
      itoa(k, str, 10);
      strcat(str, ".jpg");
      //Open the new file
      outFile = SD.open(str, O_WRITE | O_CREAT | O_TRUNC);
      if (! outFile)
      {
        Serial.println(F("File open failed"));
        while (1);
      }
      myCAM.CS_LOW();
      myCAM.set_fifo_burst();
      buf[i++] = temp_last;
      buf[i++] = temp;
    }
  }
  myCAM.CS_HIGH();
  return 1;
}