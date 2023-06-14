
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
  }

  void DoubleClicked(){

    Serial.println("Double Click        tac tac");
    UpdateSunElevation();
  }

  void LongPressed(){
    Serial.println("Long Click          taaaaaaac");
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
  Serial.print(F(" UTC on "));
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

  //check outputBroadCastState
  if (broadcast==false){BroadcastOnOff(OFF);}
  else {BroadcastOnOff(ON);}

  //input Serial Monitor
  if (Serial.available()) {
        delayMilisec(50);
        // read string from serial
        String comMsg;

        while (Serial.available()) {
            comMsg = comMsg + (char)(Serial.read());
        }; // read data from serial
    
        if (comMsg == "on") {
            BroadcastOnOff(ON);
        }; // broadcast, 100%

        if (comMsg == "off") {
                BroadcastOnOff(OFF);
        }; // broadcast, 0%
  }
}
//------------------------Loop-------------------End

void HandleClient() {
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
                				     webpage +=  "<txt>Lamp is</txt>";
                            if (outputBroadCastState=="off") {
                                webpage +=  "<button class=\"button\" input type='submit'>ON</button>";
                            } else {
                                webpage +=  "<a href=\"/broadcast/off\"><button class=\"button button2\">OFF</button></a>";
                            }     
                	    webpage +=  "</d1>";
                    
                      webpage += "<form action='http://"+IPaddress+"' method='POST'>";
                      webpage +=  "<d2>";
                        webpage +=  "<d3>";
                	        webpage +=  "<subtitle>Time Zone</subtitle>";
                            webpage +=  "<c1>";
								                webpage +=  "<select name=\"utc\" required>";
                                  webpage +=  "<option value=\"\">---UTC---</option>";
                                  webpage +=  "<option value=\"0\">UTC+0</option>";
                                  webpage +=  "<option value=\"1\">UTC+1</option>";
                                  webpage +=  "<option value=\"2\">UTC+2</option>";
                                  webpage +=  "<option value=\"3\">UTC+3</option>";
                                  webpage +=  "<option value=\"4\">UTC+4</option>";
                                  webpage +=  "<option value=\"5\">UTC+5</option>";
                                  webpage +=  "<option value=\"6\">UTC+6</option>";
                                  webpage +=  "<option value=\"7\">UTC+7</option>";
                                  webpage +=  "<option value=\"8\">UTC+8</option>";
                                  webpage +=  "<option value=\"9\">UTC+9</option>";
                                  webpage +=  "<option value=\"10\">UTC+10</option>";
                                  webpage +=  "<option value=\"11\">UTC+11</option>";
                                  webpage +=  "<option value=\"12\">UTC+12</option>";
                                  webpage +=  "<option value=\"13\">UTC+13</option>";
                                  webpage +=  "<option value=\"14\">UTC+14</option>";
                                  webpage +=  "<option value=\"15\">UTC+15</option>";
                                  webpage +=  "<option value=\"16\">UTC+16</option>";
                                  webpage +=  "<option value=\"17\">UTC+17</option>";
                                  webpage +=  "<option value=\"18\">UTC+18</option>";
                                  webpage +=  "<option value=\"19\">UTC+19</option>";
                                  webpage +=  "<option value=\"20\">UTC+20</option>";
                                  webpage +=  "<option value=\"21\">UTC+21</option>";
                                  webpage +=  "<option value=\"22\">UTC+22</option>";
                                  webpage +=  "<option value=\"23\">UTC+23</option>";
                                  webpage +=  "</select>";
                    	      webpage +=  "</c1>";
                            webpage +=  "<c1>";
                            webpage +=  "</c1>";
                          
                	      webpage +=  "</d3>";
                    
                        webpage +=  "<d3>";
                	        webpage +=  "<subtitle>Location Settings</subtitle>";
                            webpage +=  "<c1>";
                              webpage +=  "<txt>Latitude</txt>";
								              webpage +=  "<input type=\"number\" step=0.01 placeholder=\"Latitude\" name=\"latitude\"min=\"-90\" max=\"90\" required>";
                    	      webpage +=  "</c1>";
                            webpage +=  "<c1>";
                              webpage +=  "<txt>Longitude</txt>";
								              webpage +=  "<input type=\"number\" step=0.01 placeholder=\"Longitude\" name=\"longitude\"min=\"-180\" max=\"180\" required>";
                    	      webpage +=  "</c1>";
                        webpage +=  "</d3>";
                        webpage +=  "<button class=\"button\" input type='submit'>Set</button>";
                	    webpage +=  "</d2>";
                      webpage += "</form>";
                    webpage +=  "</h2>";
   webpage += "</body>";
  webpage += "</html>";
  server.send(200, "text/html", webpage); // Send a response to the client asking for input

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
    }
  }
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