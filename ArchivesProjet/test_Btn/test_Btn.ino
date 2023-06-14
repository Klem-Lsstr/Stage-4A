//-----------ESP-8266 LED Controler
//-----------ESIREM / CVUT
//-----------12/04/2023---12/07/23

//Code structure
//----------------------------
//Define
//Include
//
//Setup
//
//Loop
//    Read command from serial -> CCCCCCCCCCCCCCCCCC
//    Web Page                 -> WWWWWWWWWWWWWWWWWW
//        Action on Led        -> LLLLLLLLLLLLLLLLLL
//        CSS                  -> SSSSSSSSSSSSSSSSSS
//        HTML
//
//Fonctions ffffffffffffffff
//      delayMilisec
//      delayMicrosec
//      PrintHelp
//      BroadcastOnOff
//      ShortOnOff
//      GroupOnOff
//      ShortDAPC
//      GroupDAPC
//      SetMaxLevel
//      SetPowerOnLevel
//      DaliInit
//      DaliReset
//      DaliClear
//      SearchAndCompare
//      DaliTransmitCMD
//      SetFadeTime
//      SetDimingCurve
//----------------------------

//for Wemos D1 mini
#define DALI_TX_PIN 14    //D5 --> 14
#define DALI_RX_PIN 16    //D0 --> 16
#define LED_PIN 2         //D4 --> 2

#define ON 1
#define OFF 0
//btn
#define BUTTON_PIN 0

#define BROADCAST_DP 0b11111110
#define BROADCAST_C 0b11111111
#define ON_DP 0b11111110
#define OFF_DP 0b00000000
#define ON_C 0b00000101
#define OFF_C 0b00000000
#define QUERY_STATUS 0b10010000
#define RESET 0b00100000

#define INITIALISE          0xA5
#define RANDOMISE           0xA7
#define SEARCHADDRH         0xB1
#define SEARCHADDRM         0xB3
#define SEARCHADDRL         0xB5
#define PRG_SHORT_ADDR      0xB7
#define COMPARE             0xA9
#define WITHDRAW            0xAB
#define TERMINATE           0xA1

#define START_SHORT_ADDR    0

#define DOWN                0b00000010
#define UP                  0b00000001

#define DALI_HALF_BIT_TIME      416 //microseconds
#define DALI_TWO_PACKET_DELAY   10 //miliseconds
#define DALI_RESPONSE_DELAY_COUNT  15 //maximal number of halfbites

#define DALI_ANALOG_LEVEL   650

// Load Wi-Fi library
#include <NTPClient.h>
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <WifiServer.h>
#include <ESPAsyncTCP.h>
#include "time.h"

//For hour and date
WiFiUDP ntpUDP;
NTPClient timeUTC(ntpUDP, "pool.ntp.org");
String currentHour;

//Output states
String outputBroadCastState = "off";
String outputs00 = "off";
String outputs01 = "off";
String outputs02 = "off";
String outputs03 = "off";
String outputGradMinute = "off";
String outputGradSecond = "off";
bool gradtest = false;

//test
int sliderPowerOnValue = 0;

// CVUT network
const char* ssid     = "C4-460";        //network name
const char* password = "bez_dratu460";  //network password

// Appart klem network
//const char* ssid     = "Sergioleone";
//const char* password = "Sergioleone";

// Set web server port number to 80
WiFiServer server(80);
// Variable to store the HTTP request
String header;

// Current time
unsigned long currentTime = millis();
// Previous time
unsigned long previousTime = 0; 
// Define timeout time in milliseconds
const long timeoutTime = 2000;

//btn
const unsigned long DOUBLE_CLICK_TIME = 300;
const unsigned long LONG_PRESS_TIME = 1000;

int currentState = HIGH;
int lastState = HIGH;
unsigned long pressedTime = 0;
unsigned long releasedTime = 0;
bool clickDetected = false;
bool doubleClickDetected = false;
unsigned long lastClickTime = 0;
bool waitForSecondClick = false;

void setup() {
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  Serial.begin(115200);
}

void loop() {
  currentState = digitalRead(BUTTON_PIN);

  if (lastState == HIGH && currentState == LOW) {     // Button is pressed
    pressedTime = millis();

    if (clickDetected && !doubleClickDetected && (millis() - lastClickTime) < DOUBLE_CLICK_TIME) {
      doubleClickDetected = true;
      clickDetected = false;
      waitForSecondClick = false;
      Serial.println("Double Click");
    } else {
      clickDetected = true;
      lastClickTime = millis();
      waitForSecondClick = true;
    }
  } else if (lastState == LOW && currentState == HIGH) {  // Button is released
    releasedTime = millis();

    long pressDuration = releasedTime - pressedTime;

    if (pressDuration >= LONG_PRESS_TIME) {
      // Long press
      Serial.println("Long Press");
      clickDetected = false;
      doubleClickDetected = false;
      waitForSecondClick = false;
    } else if (waitForSecondClick) {
      waitForSecondClick = false;
    } else {
      // Single click
      if (clickDetected && !doubleClickDetected) {
        clickDetected = false;
        Serial.println("Single Click");
      }
    }
  }

  lastState = currentState;
}





//fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff
//_________________________________________________________________________
//-------Begin------------------------Fonctions----------------------------

// new specification of delay functions
void delayMilisec(unsigned long t) {
  uint32_t s = millis();
  while(millis() - s < t) {
    yield();
  }
  return;
}

void delayMicrosec(unsigned int t) {
  uint32_t s = micros();
  while(micros() - s < t) {
    yield();
  }
  return;
}

// help for serial terminal
void PrintHelp() {
    Serial.println();
    Serial.println("Help");
    Serial.println("------------------");
    Serial.println("on/off : broadcast on/off");
    Serial.println("sAAon/sAAoff : short address on/off, where AA is a short address (dec)");
    Serial.println("sAAdapcDDD : short address DAPC, where AA is a short address (dec) and DDD is DAPC level (dec)");
    Serial.println("gAAon/gAAoff : group address on/off, where AA is a group address (dec)");
    Serial.println("gAAdapcDDD : group address DAPC, where AA is a group address (dec) and DDD is DAPC level (dec)");
    Serial.println("maxDDD : set max level DDD (dec)");
    Serial.println("fadeDDD : set Fade Time DDD (dec)");
    Serial.println("dimingB : set Diming Curve (bin), where 0 = logaritmic diming curve, 1 = linear diming curve");
    Serial.println("power_onDDD : set Power On level DDD (dec)");
    Serial.println("init_all : assign short adresses to all devices");
    Serial.println("init_all_fromDD : assign short adresses to all devices, where DD is the lowest address (dec)");
    Serial.println("init_unsigned_fromDD : assign short adresses to unsigned devices, where DD is the lowest address (dec)");
    Serial.println("add_grDD_fromAA_toAA : add short adresses from AA (dec) to AA (dec) to group DD (dec)");
    Serial.println("reset_all : reset all devices");
    Serial.println("clear_all : clear short adresses from all devices");
    Serial.println("0xHHHH : direct command, where HHHH is command code (hex)");
    Serial.println();
}
//-------------------------------------------------
// control commands

void BroadcastOnOff(bool state) {
    if (state) {
        DaliTransmitCMD(BROADCAST_C, ON_C);
        delayMilisec(DALI_TWO_PACKET_DELAY);
        Serial.println("Broadcast On");
    } // broadcast, 100%
    else {
        DaliTransmitCMD(BROADCAST_C, OFF_C);
        delayMilisec(DALI_TWO_PACKET_DELAY);
        Serial.println("Broadcast Off");
    }; // broadcast, 0%}

}

void ShortOnOff(int shortAddress, bool state) {
    Serial.print("Short ");
    Serial.print(shortAddress);
    if (state) {
        DaliTransmitCMD(((shortAddress << 1) | 0x01), ON_C);
        delayMilisec(DALI_TWO_PACKET_DELAY);
        Serial.println(" on");
    } //short address on
    else {
        DaliTransmitCMD(((shortAddress << 1) | 0x01), OFF_C);
        delayMilisec(DALI_TWO_PACKET_DELAY);
        Serial.println(" off");
    }; //short address off
}

void Gradient(int speed) {
  int i=20;
  int a=speed;
  while(i>18){
    i += a;
    if (i >= 240) {
      a = -a;
    }
    if(a>0){
      SetMaxLevel(i);
      DaliTransmitCMD(BROADCAST_C, ON_C);
      delayMilisec(DALI_TWO_PACKET_DELAY);
    }
    else{
      SetMaxLevel(i);
      DaliTransmitCMD(BROADCAST_C, ON_C);
      delayMilisec(DALI_TWO_PACKET_DELAY);
    }
  }
  DaliTransmitCMD(BROADCAST_C, OFF_C);
}

void Simulation(int second) {

  int pDayUp=1;
  int pEveningUp=0;
  int s0=00;
  int s1=01;
  int s2=02;
  int s3=03;
  int pDay=0;
  int pEvening=0;
  int pNight=0;
    if (pDayUp==1) {pDay++;}
    else if (pDayUp==(-1)) {pDay--;}
    if (pDay==0) {pDayUp=0;}
    if (pEveningUp==1) {pEvening++;}
    else if (pEveningUp==(-1)) {pEvening--;}
    if (pEvening==0) {pEveningUp=0;}

  if (second<10){
    Serial.println("Sunlight increase");
    pDayUp=1;
    pEveningUp=0;
    pNight=0;
  }
  if (second>=10&&second<20){
    Serial.println("Sunlight");
    pDayUp=0;
  }
  if (second>=20&&second<30){
    Serial.println("Sunlight decrease");
    pDayUp=(-1);
  }
  if (second>=30&&second<40){
    Serial.println("Sunset increase");
    pEveningUp=1;
  }
  if (second>=40&&second<50){
    Serial.println("Sunset decrease");
    pEveningUp=(-1);
  }
  if (second>=50){
    Serial.println("Night light");
    pDayUp=0;
    pEveningUp=0;
    pDay=0;
    pEvening=0;
    pNight=192;
  }

  delayMilisec(DALI_TWO_PACKET_DELAY);
  ShortDAPC(s0,pNight);
  delayMilisec(DALI_TWO_PACKET_DELAY);
  ShortDAPC(s1,pDay);
  delayMilisec(DALI_TWO_PACKET_DELAY);
  ShortDAPC(s2,pDay);
  delayMilisec(DALI_TWO_PACKET_DELAY);
  ShortDAPC(s3,pEvening);
  delayMilisec(DALI_TWO_PACKET_DELAY);
}

void SmallDay() {
  DaliTransmitCMD(BROADCAST_C, OFF_C);//setup
  SetMaxLevel(180);
  int i=0;
  int pDayUp=1;
  int pEveningUp=0;
  int s0=00;
  int s1=01;
  int s2=02;
  int s3=03;
  int pDay=0;
  int pEvening=0;
  int pNight=0;
  for (i = 0; i<=700; i++) {

    if (pDayUp==1) {pDay++;}
    else if (pDayUp==(-1)) {pDay--;}
    if (pEveningUp==1) {pEvening++;}
    else if (pEveningUp==(-1)) {pEvening--;}
    if (i==240) {pDayUp=(-1);}
    if (i==400) {pEveningUp=1;ShortOnOff(03,ON);}
    if (i==480) {pEveningUp=(-1);pDay=0;pDayUp=0;ShortOnOff(01,OFF);ShortOnOff(02,OFF);}
    if (i==560) {pEvening=0;pEveningUp=0;pNight=240;ShortOnOff(03,OFF);}

    
    delayMilisec(DALI_TWO_PACKET_DELAY);
    ShortDAPC(s0,pNight);
    delayMilisec(DALI_TWO_PACKET_DELAY);
    ShortDAPC(s1,pDay);
    delayMilisec(DALI_TWO_PACKET_DELAY);
    ShortDAPC(s2,pDay);
    delayMilisec(DALI_TWO_PACKET_DELAY);
    ShortDAPC(s3,pEvening);
    delayMilisec(DALI_TWO_PACKET_DELAY);
  }
  DaliTransmitCMD(BROADCAST_C, OFF_C);
  delayMilisec(DALI_TWO_PACKET_DELAY);
}


void GroupOnOff(int groupAddress, bool state) {
    Serial.print("Group ");
    Serial.print(groupAddress);
    if (state) {
        DaliTransmitCMD(((groupAddress << 1) | 0x81), ON_C);
        delayMilisec(DALI_TWO_PACKET_DELAY);
        Serial.println(" on");
    } //group address on
    else {
        DaliTransmitCMD(((groupAddress << 1) | 0x81), OFF_C);
        delayMilisec(DALI_TWO_PACKET_DELAY);
        Serial.println(" off");
    }; //group address off
 }

void ShortDAPC(int shortAddress, int value) {
    Serial.print("Short ");
    Serial.print(shortAddress);
    DaliTransmitCMD(((shortAddress << 1) | 0x00), value);
    delayMilisec(DALI_TWO_PACKET_DELAY);
    Serial.print(" level is ");
    Serial.println(value);
}; //short address DAPC

void GroupDAPC(int groupAddress, int value) {
    Serial.print("Group ");
    Serial.print(groupAddress);
        DaliTransmitCMD(((groupAddress << 1) | 0x80), value);
        delayMilisec(DALI_TWO_PACKET_DELAY);
        Serial.print(" level is ");
        Serial.println(value);
}; //group address DAPC

void SetMaxLevel(int maxLevel) {
    DaliTransmitCMD(0xA5, 0x00); // Initialize
    delayMilisec(DALI_TWO_PACKET_DELAY);

    DaliTransmitCMD(0xA5, 0x00); // Initialize
    delayMilisec(DALI_TWO_PACKET_DELAY);

    DaliTransmitCMD(0xA3, maxLevel); // DataTransferRegister
    delayMilisec(DALI_TWO_PACKET_DELAY);

    DaliTransmitCMD(0xA3, maxLevel); // DataTransferRegister
    delayMilisec(DALI_TWO_PACKET_DELAY);

    DaliTransmitCMD(BROADCAST_C, 0x2A); // StoreDTRasMaxLevel
    delayMilisec(DALI_TWO_PACKET_DELAY);

    DaliTransmitCMD(BROADCAST_C, 0x2A); // StoreDTRasMaxLevel
    delayMilisec(DALI_TWO_PACKET_DELAY);

    DaliTransmitCMD(0xA1, 0x00); // Terminate
    delayMilisec(DALI_TWO_PACKET_DELAY);

    DaliTransmitCMD(0xA1, 0x00); // Terminate
    delayMilisec(DALI_TWO_PACKET_DELAY);
} // broadcast set max level

void SetFadeTime(int fadeTime) {
    DaliTransmitCMD(0xA5, 0x00); // Initialize
    delayMilisec(DALI_TWO_PACKET_DELAY);

    DaliTransmitCMD(0xA5, 0x00); // Initialize
    delayMilisec(DALI_TWO_PACKET_DELAY);

    DaliTransmitCMD(0xA3, fadeTime); // DataTransferRegister
    delayMilisec(DALI_TWO_PACKET_DELAY);

    DaliTransmitCMD(0xA3, fadeTime); // DataTransferRegister
    delayMilisec(DALI_TWO_PACKET_DELAY);

    DaliTransmitCMD(BROADCAST_C, 0x2E); // StoreStoreDTRasFadeTime
    delayMilisec(DALI_TWO_PACKET_DELAY);

    DaliTransmitCMD(BROADCAST_C, 0x2E); // StoreStoreDTRasFadeTime
    delayMilisec(DALI_TWO_PACKET_DELAY);

    DaliTransmitCMD(0xA1, 0x00); // Terminate
    delayMilisec(DALI_TWO_PACKET_DELAY);

    DaliTransmitCMD(0xA1, 0x00); // Terminate
    delayMilisec(DALI_TWO_PACKET_DELAY);
} // broadcast Fade Time

void SetPowerOnLevel(int powerOnLevel) {
    DaliTransmitCMD(0xA5, 0x00); // Initialize
    delayMilisec(DALI_TWO_PACKET_DELAY);

    DaliTransmitCMD(0xA5, 0x00); // Initialize
    delayMilisec(DALI_TWO_PACKET_DELAY);

    DaliTransmitCMD(0xA3, powerOnLevel); // DataTransferRegister
    delayMilisec(DALI_TWO_PACKET_DELAY);

    DaliTransmitCMD(0xA3, powerOnLevel); // DataTransferRegister
    delayMilisec(DALI_TWO_PACKET_DELAY);

    DaliTransmitCMD(BROADCAST_C, 0x2D); // StoreDTRasPowerOnLevel
    delayMilisec(DALI_TWO_PACKET_DELAY);

    DaliTransmitCMD(BROADCAST_C, 0x2D); // StoreDTRasPowerOnLevel
    delayMilisec(DALI_TWO_PACKET_DELAY);

    DaliTransmitCMD(0xA1, 0x00); // Terminate
    delayMilisec(DALI_TWO_PACKET_DELAY);

    DaliTransmitCMD(0xA1, 0x00); // Terminate
    delayMilisec(DALI_TWO_PACKET_DELAY);
} // broadcast Power On level

void SetDimingCurve(int value) {
    DaliTransmitCMD(0xA5, 0x00); // Initialize
    delayMilisec(DALI_TWO_PACKET_DELAY);

    DaliTransmitCMD(0xA5, 0x00); // Initialize
    delayMilisec(DALI_TWO_PACKET_DELAY);

    DaliTransmitCMD(0xA3, value); // DataTransferRegister
    delayMilisec(DALI_TWO_PACKET_DELAY);

    DaliTransmitCMD(0xA3, value); // DataTransferRegister
    delayMilisec(DALI_TWO_PACKET_DELAY);

    DaliTransmitCMD(BROADCAST_C, 0xE3); // StoreDTRasFadeTime
    delayMilisec(DALI_TWO_PACKET_DELAY);

    DaliTransmitCMD(BROADCAST_C, 0xE); // StoreDTRasFadeTime
    delayMilisec(DALI_TWO_PACKET_DELAY);

    DaliTransmitCMD(0xA1, 0x00); // Terminate
    delayMilisec(DALI_TWO_PACKET_DELAY);

    DaliTransmitCMD(0xA1, 0x00); // Terminate
    delayMilisec(DALI_TWO_PACKET_DELAY);
} // broadcast Set Diming Curve

void DaliInit(bool unsignedDevice, uint8_t ShortAddr)
{
    Serial.println("Initialization...");

    if (unsignedDevice == 0) {
        DaliTransmitCMD(INITIALISE, 0x00); //initialize all devices
        delayMilisec(DALI_TWO_PACKET_DELAY);
        DaliTransmitCMD(INITIALISE, 0x00);
        delayMilisec(DALI_TWO_PACKET_DELAY);
        delayMilisec(100);
    }
    else
    {
        DaliTransmitCMD(INITIALISE, 0xFF); //initialize only devices without short address
        delayMilisec(DALI_TWO_PACKET_DELAY);
        DaliTransmitCMD(INITIALISE, 0xFF);
        delayMilisec(DALI_TWO_PACKET_DELAY);
        delayMilisec(100);
    }

    DaliTransmitCMD(RANDOMISE, 0x00);
    delayMilisec(DALI_TWO_PACKET_DELAY);
    DaliTransmitCMD(RANDOMISE, 0x00);
    delayMilisec(DALI_TWO_PACKET_DELAY);
    delayMilisec(100);

    while (ShortAddr < 64)
    {
        long SearchAddr = 0xFFFFFF;
        bool Response = 0;
        long LowLimit = 0;
        long HighLimit = 0x1000000;

        Response = SearchAndCompare(SearchAddr);
        delayMilisec(DALI_TWO_PACKET_DELAY);

        if (Response)
        {
            digitalWrite(LED_PIN, LOW);
            Serial.println("Device detected, address searching...");

            if (!SearchAndCompare(SearchAddr - 1))
            {
                delayMilisec(DALI_TWO_PACKET_DELAY);
                SearchAndCompare(SearchAddr);
                delayMilisec(DALI_TWO_PACKET_DELAY);
                DaliTransmitCMD(PRG_SHORT_ADDR, ((ShortAddr << 1) | 1));
                delayMilisec(3 * DALI_TWO_PACKET_DELAY);
                DaliTransmitCMD(WITHDRAW, 0x00);
                Serial.print("24-bit address found: 0x");
                Serial.println(SearchAddr, HEX);
                Serial.print("Assigning short address ");
                Serial.println(ShortAddr);
                break;
            }
        }
        else
        {
            Serial.println("No devices detected");
            break;
        }

        while (1)
        {
            SearchAddr = (long)((LowLimit + HighLimit) / 2);

            Response = SearchAndCompare(SearchAddr);
            delayMilisec(DALI_TWO_PACKET_DELAY);

            if (Response)
            {
                digitalWrite(LED_PIN, LOW);

                if ((SearchAddr == 0) || (!SearchAndCompare(SearchAddr - 1)))
                    break;

                HighLimit = SearchAddr;
            }
            else
                LowLimit = SearchAddr;
        }

        delayMilisec(DALI_TWO_PACKET_DELAY);
        SearchAndCompare(SearchAddr);
        delayMilisec(DALI_TWO_PACKET_DELAY);
        DaliTransmitCMD(PRG_SHORT_ADDR, ((ShortAddr << 1) | 1));
        delayMilisec(5 * DALI_TWO_PACKET_DELAY);
        DaliTransmitCMD(WITHDRAW, 0x00);
        delayMilisec(DALI_TWO_PACKET_DELAY);

        Serial.print("24-bit address found: 0x");
        Serial.println(SearchAddr, HEX);
        Serial.print("Assigning short address ");
        Serial.println(ShortAddr);

        ShortAddr++;
    }

    delayMilisec(DALI_TWO_PACKET_DELAY);
    DaliTransmitCMD(TERMINATE, 0x00);
    delayMilisec(DALI_TWO_PACKET_DELAY);
    Serial.println("Init complete");
}

void DaliReset()
{
    DaliTransmitCMD(BROADCAST_C, RESET);
    delayMilisec(DALI_TWO_PACKET_DELAY);
    DaliTransmitCMD(BROADCAST_C, RESET);
    delayMilisec(DALI_TWO_PACKET_DELAY);

    Serial.println("Reset completed");
}

void DaliClear() //clear all short addresses
{
    DaliTransmitCMD(0xA3, 0xFF); //save 0xFF do DTR
    delayMilisec(DALI_TWO_PACKET_DELAY);
    DaliTransmitCMD(0xA3, 0xFF); //save 0xFF do DTR
    delayMilisec(DALI_TWO_PACKET_DELAY);

    DaliTransmitCMD(BROADCAST_C, 0x80); //set DTR as Short Address
    delayMilisec(DALI_TWO_PACKET_DELAY);
    DaliTransmitCMD(BROADCAST_C, 0x80); //set DTR as Short Address
    delayMilisec(DALI_TWO_PACKET_DELAY);

    Serial.println("Clearing completed");
}


//-------------------------------------------------
// physical layer commands

bool SearchAndCompare(long SearchAddr)
{
    bool Response = 0;

    uint8_t HighByte = SearchAddr >> 16;
    uint8_t MiddleByte = SearchAddr >> 8;
    uint8_t LowByte = SearchAddr;

    for (uint8_t i = 0; i < 3; i++)
    {
        DaliTransmitCMD(SEARCHADDRH, HighByte);
        delayMilisec(DALI_TWO_PACKET_DELAY);
        DaliTransmitCMD(SEARCHADDRM, MiddleByte);
        delayMilisec(DALI_TWO_PACKET_DELAY);
        DaliTransmitCMD(SEARCHADDRL, LowByte);
        delayMilisec(DALI_TWO_PACKET_DELAY);
    }
    DaliTransmitCMD(COMPARE, 0x00);
    delayMicrosec(7 * DALI_HALF_BIT_TIME);

    for (uint8_t i = 0; i < DALI_RESPONSE_DELAY_COUNT; i++)
    {
        if (analogRead(DALI_RX_PIN) < DALI_ANALOG_LEVEL)
        {
            Response = 1;
            digitalWrite(LED_PIN, HIGH);
            break;
        }

        delayMicrosec(DALI_HALF_BIT_TIME);
    }

    return Response;
}

void DaliTransmitCMD(uint8_t Part1, uint8_t Part2)
{
    uint8_t DALI_CMD[] = { Part1, Part2 };

    digitalWrite(DALI_TX_PIN, LOW);
    delayMicrosec(DALI_HALF_BIT_TIME);
    digitalWrite(DALI_TX_PIN, HIGH);
    delayMicrosec(DALI_HALF_BIT_TIME);
    
    for (uint8_t CmdPart = 0; CmdPart < 2; CmdPart++)
    {
        for (int i = 7; i >= 0; i--)
        {
            bool BitToSend = false;

            if ((DALI_CMD[CmdPart] >> i) & 1)
                BitToSend = true;

            if (BitToSend)
                digitalWrite(DALI_TX_PIN, LOW);
            else
                digitalWrite(DALI_TX_PIN, HIGH);

            delayMicrosec(DALI_HALF_BIT_TIME);

            if (BitToSend)
                digitalWrite(DALI_TX_PIN, HIGH);
            else
                digitalWrite(DALI_TX_PIN, LOW);

            delayMicrosec(DALI_HALF_BIT_TIME);
        }
    }

    digitalWrite(DALI_TX_PIN, HIGH);
}

//-------End------------------------Fonctions------------------------------
//_________________________________________________________________________
//fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff