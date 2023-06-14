
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <DNSServer.h>
#include <WiFiManager.h> 

//physical button
#include <Arduino.h>
#include <OneButton.h>

//hour
#include <WiFiUdp.h>
#include <NTPClient.h>
#include "time.h"
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org");
int utc_delay;
time_t currentTime;

//Sun elevation
#include <SolarPosition.h>
SolarPosition_t savedPosition;// A solar position structure
float Latitude;
float Longitude;

//Dali ctrl
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

int dayLightGroup = 00;
int eveningLight = 01;
int nightLight = 00;

unsigned long lastExecutionTime = 0;  

bool dayRunning = false;
bool broadcast = false;
String outputBroadCastState = "off";

WiFiClient client;
ESP8266WebServer server(80);

String Argument_Name, Clients_Response1, Clients_Response2;
String current_day,current_month,current_year;

class Button{
private:
  OneButton button;
  int value;
public:
  explicit Button(uint8_t pin):button(pin) {
    button.attachClick([](void *scope) { ((Button *) scope)->Clicked();}, this);
    button.attachDoubleClick([](void *scope) { ((Button *) scope)->DoubleClicked();}, this);
    button.attachLongPressStart([](void *scope) { ((Button *) scope)->LongPressed();}, this);
  }

  void Clicked(){
    Serial.println("Simple Click        tac");
    if (outputBroadCastState=="off") {
            outputBroadCastState="on";
            broadcast=true;
          } else {
            outputBroadCastState="off";
            broadcast=false;
          }
  }

  void DoubleClicked(){

    Serial.println("Double Click        tac tac");
    UpdateSunElevation();
  }

  void LongPressed(){
    Serial.println("Long Click          taaaaaaac");
    dayRunning=!dayRunning;
  }

  void handle(){
    button.tick();
  }

  void UpdateSunElevation(){
    savedPosition = calculateSolarPosition(currentTime, Latitude * DEG_TO_RAD, Longitude * DEG_TO_RAD);
    Serial.print(F("The sun was at an elevation of "));
    Serial.println(savedPosition.elevation * RAD_TO_DEG, 1);
    Serial.print("At ");
    printTime(savedPosition.time);

    int elev = (int)(savedPosition.elevation * RAD_TO_DEG);
    if(elev<=5){
      Serial.println("The elevation is less than 5°");
    }else{
      Serial.println("The elevation is more than 5°");
    }

    Serial.println(" ");
}

void printTime(time_t t)
{
  tmElements_t someTime;
  breakTime(t, someTime);

  Serial.print(someTime.Hour);
  Serial.print(F(":"));
  Serial.print(someTime.Minute);
  Serial.print(F(":"));
  Serial.print(someTime.Second);
  Serial.print(F(" on "));
  Serial.print(dayStr(someTime.Wday));
  Serial.print(F(", "));
  Serial.print(monthStr(someTime.Month));
  Serial.print(F(" "));
  Serial.print(someTime.Day);
  Serial.print(F(", "));
  Serial.println(tmYearToCalendar(someTime.Year));
}
};

Button button(0);


//------------------------Setup-------------------Begin
void setup() {
  Serial.begin(115200);

  //hour
  timeClient.begin();

  //Dali settings
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  pinMode(DALI_TX_PIN, OUTPUT);
  digitalWrite(DALI_TX_PIN, HIGH);
  pinMode(DALI_RX_PIN, INPUT);
  SetPowerOnLevel(0);
  SetMaxLevel(255);

  //WiFiManager intialisation
  WiFiManager wifiManager;
  wifiManager.setTimeout(180);
  if(!wifiManager.autoConnect("Connect the Lamp")) {
    Serial.println("failed to connect and timeout occurred");
    delay(3000);
    ESP.reset();
    delay(5000);
  }
  timeClient.update();
  Serial.println("WiFi connected..");
  server.begin(); Serial.println("Webserver started..."); // Start the webserver
  Serial.print("Use this URL to connect: http://");// Print the IP address
  Serial.print(WiFi.localIP());Serial.println("/");
  // NOTE: You must use the IP address assigned to YOUR Board when printed/displayed here on your serial port
  // that's the address you must use, not the one I used !
  
  // Next define what the server should do when a client connects
  server.on("/", HandleClient); 
}
//------------------------Setup-------------------End

//------------------------Loop-------------------Begin
void loop() {

  server.handleClient();
  button.handle();

  //Get current hour
  UpdateHour();

  //check booleans
  unsigned long currentTime = millis();
  if(dayRunning==true){
    if (currentTime - lastExecutionTime >= 1000) {
      lastExecutionTime = currentTime;
      LittleDay();
    }
  }
  if(broadcast==true){BroadcastOnOff(ON);}
  if(dayRunning==false&&broadcast==false){BroadcastOnOff(OFF);}
  
}
//------------------------Loop-------------------End

void HandleClient() {

  if (server.args() > 0 ) { // Arguments were received
    for ( uint8_t i = 0; i < server.args(); i++ ) {
      //Serial.print(server.argName(i)); // Display the argument
      Argument_Name = server.argName(i);
      if (server.argName(i) == "utc") {
        Serial.print("UTC delay = ");
        Serial.println(server.arg(i));
        utc_delay = server.arg(i).toInt();
      }
      if (server.argName(i) == "latitude") {
        Serial.print("latitude = ");
        Serial.println(server.arg(i));
        Latitude = server.arg(i).toInt();
      }
      if (server.argName(i) == "longitude") {
        Serial.print("longitude = ");
        Serial.println(server.arg(i));
        Longitude = server.arg(i).toInt();
      }
      if (server.argName(i) == "onoff") {
        Serial.print("clic btn : ");
        if (outputBroadCastState=="off") {
            outputBroadCastState="on";
            broadcast=true;
          } else {
            outputBroadCastState="off";
            broadcast=false;
          }
        Serial.println(outputBroadCastState);
      }
    }

  }

  String webpage;
  webpage +=  "<!DOCTYPE html>";
  webpage +=  "<html>";
   webpage += "<head>";
   webpage +=  "<title>Lamp Controler</title>";
    webpage +=  "<style>";
                  webpage +=  "html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;background-color:#dce7f7;}";
                  webpage +=  "body {border-width:2px;border:1rem solid;border-color:#141b6b;margin:5% 25% 5% 25%;border-radius: 25px;background-color:#ffffff;} ";
                  webpage +=  "button {background-color: #6976ff; border: none; color: white; padding: 10px 30px;border-width:1px;border:solid;border-color:#141b6b;";
                  webpage +=  "text-decoration: none; font-size: 20px; margin:2% 2% 2% 2%; cursor: pointer;border-radius: 25px;}";
                  webpage +=  ".button2 {background-color:#949494;border-width:1px;border:solid;border-color:#141b6b;}";
                  webpage +=  ".button:hover {color: #141b6b;}";
                  webpage +=  ".button:active{background-color: #141b6b;}";
                  webpage +=  "h1{display:grid;background-color:#ffffff;}";
                  webpage +=  "h2{border-width:1px;border:solid;border-color:#141b6b;z-index: 1;margin:1% 1% 1% 1%;border-radius: 5px;display:grid;background-color:#dce7f7;}";
                  webpage +=  "d1{border-width:1px;border:solid;border-color:#141b6b;margin:1% 1% 1% 1%;border-radius: 5px;display:grid;background-color:#ffffff;}";
                  webpage +=  "d2{display:flex;justify-content: center;}";
                  webpage +=  "d3{border-width:1px;border:solid;border-color:#141b6b;margin:1% 1% 1% 1%;border-radius: 5px;display:grid;background-color:#ffffff;width:50%}";
                  webpage +=  "c1{display:flex;margin:2% 2% 2% 2%;display: inline-block;}";
                  webpage +=  "c2{display:flex;margin:2% 2% 2% 2%;display: inline-block;}";
                  webpage +=  "e1{display:flex;justify-content: space-between;margin:2% 2% 2% 2%;}";
                  webpage +=  "txttitle{color:#141b6b;}";
                  webpage +=  "subtitle{color:#6976ff;}";
                  webpage +=  "p{margin:25px;}";
                  webpage +=  "text-decoration: none; font-size: 30px; margin: 0px auto; cursor: pointer;border-radius: 25px;}";

                  webpage +=  "input,label {margin: 0.4rem 0;}";
                  webpage +=  "</style>";
   webpage += "</head>";
   webpage += "<body>"; 
    String IPaddress = WiFi.localIP().toString();

    webpage +=  "<h1>";
                      webpage +=  "<txttitle>";
                        webpage +=  "Lamp Controler";
                      webpage +=  "</txttitle>";
                    webpage +=  "</h1>";

                    webpage +=  "<h2>";

                      webpage +=  "<d1>";
                        webpage += "<form action='http://"+IPaddress+"' method='POST'>";
                            if (outputBroadCastState=="off") {
                                webpage +=  "<button class=\"button\" name=\"onoff\" input type='submit'>ON</button>";
                            } else {
                                webpage +=  "<a href=\"/broadcast/off\"><button name=\"onoff\" class=\"button button2\">OFF</button></a>";
                            }
                            
                        webpage += "</form>";     
                	    webpage +=  "</d1>";
                    
                      webpage += "<form action='http://"+IPaddress+"' method='POST'>";
                      webpage +=  "<d2>";
                        webpage +=  "<d3>";
                	        webpage +=  "<subtitle>Time Zone</subtitle>";
                            webpage +=  "<c1>";
								                webpage +=  "<select name=\"utc\" required>";
                                  webpage +=  "<option value=\"\">---UTC---</option>";
                                  webpage += "<option value=\"0\"" + String(utc_delay == 0 ? " selected" : "") + ">UTC+0</option>";
                                  webpage += "<option value=\"1\"" + String(utc_delay == 1 ? " selected" : "") + ">UTC+1</option>";
                                  webpage += "<option value=\"2\"" + String(utc_delay == 2 ? " selected" : "") + ">UTC+2</option>";
                                  webpage += "<option value=\"3\"" + String(utc_delay == 3 ? " selected" : "") + ">UTC+3</option>";
                                  webpage += "<option value=\"4\"" + String(utc_delay == 4 ? " selected" : "") + ">UTC+4</option>";
                                  webpage += "<option value=\"5\"" + String(utc_delay == 5 ? " selected" : "") + ">UTC+5</option>";
                                  webpage += "<option value=\"6\"" + String(utc_delay == 6 ? " selected" : "") + ">UTC+6</option>";
                                  webpage += "<option value=\"7\"" + String(utc_delay == 7 ? " selected" : "") + ">UTC+7</option>";
                                  webpage += "<option value=\"8\"" + String(utc_delay == 8 ? " selected" : "") + ">UTC+8</option>";
                                  webpage += "<option value=\"9\"" + String(utc_delay == 9 ? " selected" : "") + ">UTC+9</option>";
                                  webpage += "<option value=\"10\"" + String(utc_delay == 10 ? " selected" : "") + ">UTC+10</option>";
                                  webpage += "<option value=\"11\"" + String(utc_delay == 11 ? " selected" : "") + ">UTC+11</option>";
                                  webpage += "<option value=\"12\"" + String(utc_delay == 12 ? " selected" : "") + ">UTC+12</option>";
                                  webpage += "<option value=\"13\"" + String(utc_delay == 13 ? " selected" : "") + ">UTC+13</option>";
                                  webpage += "<option value=\"14\"" + String(utc_delay == 14 ? " selected" : "") + ">UTC+14</option>";
                                  webpage += "<option value=\"15\"" + String(utc_delay == 15 ? " selected" : "") + ">UTC+15</option>";
                                  webpage += "<option value=\"16\"" + String(utc_delay == 16 ? " selected" : "") + ">UTC+16</option>";
                                  webpage += "<option value=\"17\"" + String(utc_delay == 17 ? " selected" : "") + ">UTC+17</option>";
                                  webpage += "<option value=\"18\"" + String(utc_delay == 18 ? " selected" : "") + ">UTC+18</option>";
                                  webpage += "<option value=\"19\"" + String(utc_delay == 19 ? " selected" : "") + ">UTC+19</option>";
                                  webpage += "<option value=\"20\"" + String(utc_delay == 20 ? " selected" : "") + ">UTC+20</option>";
                                  webpage += "<option value=\"21\"" + String(utc_delay == 21 ? " selected" : "") + ">UTC+21</option>";
                                  webpage += "<option value=\"22\"" + String(utc_delay == 22 ? " selected" : "") + ">UTC+22</option>";
                                  webpage += "<option value=\"23\"" + String(utc_delay == 23 ? " selected" : "") + ">UTC+23</option>";
                                  webpage +=  "</select>";
                    	      webpage +=  "</c1>";
                            webpage +=  "<c1>";
                            webpage +=  "</c1>";
                          
                	      webpage +=  "</d3>";
                    
                        webpage +=  "<d3>";
                	        webpage +=  "<subtitle>Location Settings</subtitle>";
                            webpage +=  "<c1>";
                              webpage +=  "<txt>Latitude</txt>";
								              webpage +=  "<input type=\"number\" step=0.01 placeholder=\"Latitude\" value='" + String(Latitude, 2) + "' name=\"latitude\"min=\"-90\" max=\"90\" required>";
                    	      webpage +=  "</c1>";
                            webpage +=  "<c1>";
                              webpage +=  "<txt>Longitude</txt>";
								              webpage +=  "<input type=\"number\" step=0.01 placeholder=\"Longitude\" value='" + String(Longitude, 2) + "' name=\"longitude\"min=\"-180\" max=\"180\" required>";
                    	      webpage +=  "</c1>";
                        webpage +=  "</d3>";
                        webpage +=  "<button class=\"button\" input type='submit'>Set</button>";
                	    webpage +=  "</d2>";
                      webpage += "</form>";
                    webpage +=  "</h2>";
   webpage += "</body>";
  webpage += "</html>";

  server.send(200, "text/html", webpage); // Send a response to the client asking for input

}

void UpdateHour(){
  timeClient.update();
  time_t hourUTC0 = timeClient.getEpochTime();
  struct tm* timeinfo;
  timeinfo = localtime(&hourUTC0);
  // Modifier les composantes de l'heure
  timeinfo->tm_hour+=utc_delay;
  if (timeinfo->tm_hour > 23) {
    timeinfo->tm_hour %= 24;  // Ramener la valeur à l'intérieur de la plage des heures
  }
  
  currentTime = mktime(timeinfo);
}

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

void BroadcastOnOff(bool state) {
    if (state) {
        DaliTransmitCMD(BROADCAST_C, ON_C);
        delayMilisec(DALI_TWO_PACKET_DELAY);
        //Serial.println("Broadcast On");
    } // broadcast, 100%
    else {
        DaliTransmitCMD(BROADCAST_C, OFF_C);
        delayMilisec(DALI_TWO_PACKET_DELAY);
        //Serial.println("Broadcast Off");
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

void LittleDay() {
  
    int second = timeClient.getSeconds();
    int pDay;
    int pEvening;
    int pNight;
    Serial.println(second);

    
      if(second>=0&&second<10){
        pDay=second*20;
        pEvening=0;
        pNight=0;
        ShortOnOff(nightLight,OFF);
        ShortOnOff(eveningLight,OFF);
        GroupDAPC(dayLightGroup,pDay);
      }
      else if(second>=10&&second<20){
        pDay=255;
        pEvening=0;
        pNight=0;
        ShortOnOff(nightLight,OFF);
        ShortOnOff(eveningLight,OFF);
        GroupDAPC(dayLightGroup,pDay);
      }
      else if(second>=20&&second<30){
        pDay=0;
        pEvening=150;
        pNight=0;
        ShortOnOff(nightLight,OFF);
        ShortDAPC(eveningLight,pEvening);
        GroupOnOff(dayLightGroup,OFF);
      }
      else if(second>=30&&second<40){
        pDay=0;
        pEvening=255;
        pNight=0;
        ShortOnOff(nightLight,OFF);
        ShortDAPC(eveningLight,pEvening);
        GroupOnOff(dayLightGroup,OFF);
      }
      else if(second>=30&&second<40){
        pDay=0;
        pEvening=0;
        pNight=100;
        ShortDAPC(nightLight,pNight);
        ShortOnOff(eveningLight,OFF);
        GroupOnOff(dayLightGroup,OFF);
      }
      else if(second>=40&&second<50){
        pDay=0;
        pEvening=0;
        pNight=192;
        ShortDAPC(nightLight,pNight);
        ShortOnOff(eveningLight,OFF);
        GroupOnOff(dayLightGroup,OFF);
      }
      else if(second==50){
        pDay=0;
        pEvening=0;
        pNight=0;;
        ShortOnOff(nightLight,OFF);
        ShortOnOff(eveningLight,OFF);
        GroupOnOff(dayLightGroup,OFF);
      }
}