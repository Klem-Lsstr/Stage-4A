//-----------ESP-8266 LED Controler
//-----------ESIREM / CVUT
//-----------Lesestre Clément
//-----------25/04/2023

//----------------------
//Include
//define
//Fonctions ffffffffffff
//      delayMilisec ~ 118
//      delayMicrosec
//      PrintHelp
//      BroadcastOnOff
//      turnOffLED
//      turnOnLED
//      ShortOnOff ~ 188
//      GroupOnOff
//      ShortDAPC
//      GroupDAPC
//      SetMaxLevel
//      SetPowerOnLevel ~ 263
//      DaliInit ~ 289
//      DaliReset
//      DaliClear
//      SearchAndCompare
//      DaliTransmitCMD ~ 455
//      SetFadeTime
//      SetDimingCurve
//Setup  ~ 555
//Loop
//    CCCCCCCCCCCCCCCC -> input Serial Monitor  ~ 600 / 740
//    Actions on LEDs
//    CSS
//    HTML
//----------------------



// Load Wi-Fi library
#include <ESP8266WiFi.h>

//get hour and date
#include <WiFiUdp.h>
#include <NTPClient.h>
#include "time.h"

//for Wemos D1 mini
#define DALI_TX_PIN 14    //D5 --> 14
#define DALI_RX_PIN 16    //D0 --> 16
#define LED_PIN 2         //D4 --> 2

#define ON 1
#define OFF 0

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


WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org");
String hourandDate;

// CVUT network
const char* ssid     = "C4-460";
const char* password = "bez_dratu460";

// Appart klem network
//const char* ssid     = "Sergioleone";
//const char* password = "Sergioleone";

// Set web server port number to 80
WiFiServer server(80);

// Variable to store the HTTP request
String header;

// Auxiliar variables to store the current output state
String outputLEDState = "off";

// Assign output variables for the blue LED
#define LED 2 //Define blinking LED pin

// Current time
unsigned long currentTime = millis();
// Previous time
unsigned long previousTime = 0; 
// Define timeout time in milliseconds
const long timeoutTime = 2000;

//fffffffffffffffffffffffffffffffffffffffffffffffffffffffff
//_________________________________________________________
//-------Begin------------------------Fonctions------------

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


void turnOffLED() {
  Serial.print("Led eteinte");
  outputLEDState = "off";
  digitalWrite(LED, HIGH);
  BroadcastOnOff(OFF);
}

void turnOnLED() {
  Serial.print("Led allumee");
  outputLEDState = "on";
  digitalWrite(LED, LOW);
  BroadcastOnOff(ON);
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

//-- physical layer commands

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

//-------End------------------------Fonctions--------------
//_________________________________________________________
//fffffffffffffffffffffffffffffffffffffffffffffffffffffffff


//___________________________________________________________________________________________
//-------Begin----------------------------------------------------Setup----------------------
void setup() {
/*
  Serial.begin(115200);
  // Initialize the output variables as outputs
  pinMode(LED, OUTPUT); // Initialize the LED pin as an output
  // Set outputs to LOW
  digitalWrite(LED, LOW); // Turn the LED on */
           
  //Hour and date
  WiFi.begin("SSID", "password");
  timeClient.begin();

  // Connect to Wi-Fi network with SSID and password
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  // Print local IP address and start web server
  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  server.begin();

  //DALI
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  pinMode(DALI_TX_PIN, OUTPUT);
  digitalWrite(DALI_TX_PIN, HIGH);
  pinMode(DALI_RX_PIN, INPUT);

    

}
//-------End----------------------------------------------------Setup----------------------
//___________________________________________________________________________________________



//___________________________________________________________________________________________
//-------Begin----------------------------------------------------Loop-----------------------
void loop(){

    //CCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC
    // ------------------------Read command from serial
    if (Serial.available()) {
        delayMilisec(50);
        // read string from serial
        String comMsg;

        while (Serial.available()) {
            comMsg = comMsg + (char)(Serial.read());
        }; // read data from serial
    
        // handle string from serial
        if (comMsg == "help") {
            PrintHelp();
        }

        if (comMsg == "on") {
            BroadcastOnOff(ON);
        }; // broadcast, 100%

        if (comMsg == "off") {
                BroadcastOnOff(OFF);
        }; // broadcast, 0%

        if (comMsg.startsWith("s")) // short address on/off/DAPC
            {
                int shortAddress = comMsg.substring(1, 3).toInt();
                if (comMsg.substring(3, 5) == "on") {
                    ShortOnOff(shortAddress, ON);
                }; //short address on
                if (comMsg.substring(3, 6) == "off") {
                    ShortOnOff(shortAddress, OFF);
                }; //short address off
                if (comMsg.substring(3, 7) == "dapc") {
                    int value = comMsg.substring(7, 10).toInt();
                    ShortDAPC(shortAddress, value);
                }; //short address DAPC
        };

        if (comMsg.startsWith("g")) // group address on/off/DAPC
            {
                int groupAddress = comMsg.substring(1, 3).toInt();
                if (comMsg.substring(3, 5) == "on") {
                    GroupOnOff(groupAddress, ON);
                }; //short address on
                if (comMsg.substring(3, 6) == "off") {
                    GroupOnOff(groupAddress, OFF);
                }; //short address off
                if (comMsg.substring(3, 7) == "dapc") {
                    int value = comMsg.substring(7, 10).toInt();
                    GroupDAPC(groupAddress, value);
                }; //group address DAPC
        };

        if (comMsg.startsWith("max")) // broadcast set max level
            {
                int maxLevel = comMsg.substring(3, 6).toInt();
                Serial.print("Max Level ");
                Serial.println(maxLevel);
                SetMaxLevel(maxLevel);
        };// broadcast set max level

        if (comMsg.startsWith("fade")) // broadcast set fade time
        {
            int fadeTime = comMsg.substring(4, 7).toInt();
            Serial.print("Fade Time ");
            Serial.println(fadeTime);
            SetFadeTime(fadeTime);
        };// broadcast set Fade Time
        
        if (comMsg.startsWith("diming")) // broadcast set diming curve
        {
            bool dimingCurve = comMsg.substring(6, 7).toInt();
            Serial.print("Diming Curve ");
            Serial.println(dimingCurve);
            SetDimingCurve(dimingCurve);
        };// broadcast set diming curve

        if (comMsg.startsWith("power_on")) // broadcast Power On level
        {
            int powerOnLevel = comMsg.substring(8, 11).toInt();
            Serial.print("Power On Level ");
            Serial.println(powerOnLevel);
            SetPowerOnLevel(powerOnLevel);
        };// broadcast Power On level

        if (comMsg == "init_all") // initialize all devices
        {
            Serial.println("Initialize all devices, assigning the short adresses");
            DaliInit(0, 0);
        }

        if (comMsg.startsWith("init_all_from")) //initialize all devices, use short adresses from specified one
        {
            int shortAddress = comMsg.substring(13, 15).toInt();
            Serial.print("Initialize all devices, assigning the short adresses from ");
            Serial.println(shortAddress);
            DaliInit(0, shortAddress);
        };

        if (comMsg.startsWith("init_unsigned_from")) //initialize unsigned devices, use short adresses from specified one
        {
            int shortAddress = comMsg.substring(18, 20).toInt();
            Serial.print("Initialize unsigned devices, assigning the short adresses from ");
            Serial.println(shortAddress);
            DaliInit(1, shortAddress);
        };

        if (comMsg.startsWith("add_gr")) //add devices to group
        {
            int shortAddressFrom = comMsg.substring(13, 15).toInt();
            int shortAddressTo = comMsg.substring(18, 20).toInt();
            int groupAddress = comMsg.substring(6, 8).toInt();
            Serial.print("Assigning short adresses from ");
            Serial.print(shortAddressFrom);
            Serial.print(" to ");
            Serial.print(shortAddressTo);
            Serial.print(" to the group ");
            Serial.println(groupAddress);
            for (int i = shortAddressFrom; i <= shortAddressTo; i++) {
                // command 96+g (0x6g) 0aaaaaa1 0110gggg
                Serial.print("Assigning short address ");
                Serial.print(i);
                Serial.print(" to the group ");
                Serial.println(groupAddress);

                DaliTransmitCMD((i << 1) + 1, (0x6 << 4) + groupAddress);
                delayMilisec(DALI_TWO_PACKET_DELAY);
                DaliTransmitCMD((i << 1) + 1, (0x6 << 4) + groupAddress);
                delayMilisec(DALI_TWO_PACKET_DELAY);
            }
            Serial.println("Assigning completed");
        };

        if (comMsg == "reset_all") // reset all devices
        {
            Serial.println("Reset all devices");
            DaliReset();
        }

        if (comMsg == "clear_all") // clear short addresses from all devices
        {
            Serial.println("Clear short addresses from all devices");
            DaliClear();
        }
        comMsg = "";
    }
    //CCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC



  WiFiClient client = server.available();   // Listen for incoming clients

  if (client) {                             // If a new client connects,
    Serial.println("New Client.");          // print a message out in the serial port
    String currentLine = "";                // make a String to hold incoming data from the client
    currentTime = millis();
    previousTime = currentTime;
    while (client.connected() && currentTime - previousTime <= timeoutTime) { // loop while the client's connected
      currentTime = millis();         
      if (client.available()) {             // if there's bytes to read from the client,
        char c = client.read();             // read a byte, then
        Serial.write(c);                    // print it out the serial monitor
        header += c;
        if (c == '\n') {                    // if the byte is a newline character
          // if the current line is blank, you got two newline characters in a row.
          // that's the end of the client HTTP request, so send a response:
          if (currentLine.length() == 0) {
            // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
            // and a content-type so the client knows what's coming, then a blank line:
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println("Connection: close");
            client.println();
            
            //LLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLL
            //-----------Begin--------------------Actions on the LED--------
            if (header.indexOf("GET /5/on") >= 0) {
              turnOnLED(); // Turn the LED off by making the voltage HIGH
            } else if (header.indexOf("GET /5/off") >= 0) {
              turnOffLED(); // Turn the LED on
            } 
            

            //-----------End----------------------Actions on the LED--------
            //LLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLL





            //------------------------------------------------------------------------------------------------
            // -----------Begin----------------------------------------Display the HTML web page--------------
            client.println("<!DOCTYPE html>");
            client.println("<html>");
              // Web Page Heading
              client.println("<head>");
                client.println("<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
                client.println("<link rel=\"icon\" href=\"data:,\">");
                client.println("<title>ESP-8266 LED Controler</title>");

                //SSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSS
                //----Begin------------------------CSS--------------
                client.println("<style>");
                  client.println("html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}");
                  //CSS for body
                  client.println("body{border-width:2px;border:3rem solid;border-color:#4682B4FF;margin:5% 30% 5% 30%;border-radius: 25px;}");
                  //CSS for title
                  client.println("h1{color:#4682B4FF;}");
                  //CSS for interface
                  client.println("h2{display:flex;justify-content: center;flex-direction: column;}");
                  // CSS to style the on/off Led button 
                  client.println(".button { background-color: #4682B4FF; border: none; color: white; padding: 16px 40px;");
                  client.println("text-decoration: none; font-size: 30px; margin: 0px auto; cursor: pointer;border-radius: 25px;}");
                  client.println(".button2 {background-color: #77878A;}");
                  // CSS to style the One clic Sos button 
                  client.println(".sosbutton { background-color: #4682B4FF; border: none; color: white; padding: 16px 40px;");
                  client.println("text-decoration: none; font-size: 30px; margin: 0px auto; cursor: pointer;border-radius: 25px;}");
                  //CSS for b bloc
                  client.println("b{margin:25px;}");
                  //CSS for txt
                  client.println("txt{margin:25px;font-family: Helvetica;}");
                  
                client.println("</style>");
                //----End--------------------------CSS--------------
                //SSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSS
              client.println("</head>");



            
              client.println("<body>");
                //Title -> h1
                  client.println("<h1>______________________</h1>");
                  client.println("<h1>ESP-8266 LED Controler</h1>");
                  client.println("<h1>_________04/23________</h1>");
                  client.println("<h1> </h1>");

                //Interface -> h2
                client.println("<h2>");
                  //1st Bloc -> c1
                  // Display current state, and ON/OFF buttons for Led  
                  client.println("<txt>LED state = " + outputLEDState + "</txt>");
                  // If the outputLEDState is off, it displays the ON button       
                  if (outputLEDState=="off") {
                    client.println("<b><a href=\"/5/on\"><button class=\"button\">ON</button></a></b>");
                  } else {
                    client.println("<b><a href=\"/5/off\"><button class=\"button button2\">OFF</button></a></b>");
                  } 
               
                  //2nd Bloc -> c2
                  //Display SOS Button
                  client.println("<txt>Send Alert</txt>");     
                  client.println("<b><a href=\"/clic\"><sosbutton class=\"sosbutton\">SOS</sosbutton></a></b>");

                  //3rd Bloc -> c3
                  //Disply Hour and date
                  client.println("<txt>Current Hour <div id=\"strHour\"></div> </txt><script>function updateHour() {var currentDate = new Date();");
                  client.println("var currentHour = currentDate.getHours() + \" h \" + currentDate.getMinutes() + \" min \" + currentDate.getSeconds()+ \" sec\";");
                  client.println("document.getElementById(\"strHour\").innerHTML = currentHour;}setInterval(updateHour, 1000);</script>");

                client.println("</h2>");
            
              client.println("</body>");
            client.println("</html>");

            //-----------End-------------------------------------------Display the HTML web page--------------
            //------------------------------------------------------------------------------------------------
            
            // The HTTP response ends with another blank line
            client.println();
            // Break out of the while loop
            break;
          } else { // if you got a newline, then clear currentLine
            currentLine = "";
          }
        } else if (c != '\r') {  // if you got anything else but a carriage return character,
          currentLine += c;      // add it to the end of the currentLine
        }
      }
    }
    // Clear the header variable
    header = "";
    // Close the connection
    client.stop();
    Serial.println("Client disconnected.");
    Serial.println("");
  }
}
//-------End----------------------------------------------------Loop----------------------
//___________________________________________________________________________________________
