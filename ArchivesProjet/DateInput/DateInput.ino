
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
int currentHour,hourUTC0,minute,second;

String outputBroadCastState = "off";

WiFiClient client;
ESP8266WebServer server(80);

String Argument_Name, Clients_Response1, Clients_Response2;
String current_day,current_month,current_year;
int utc_delay;

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
  }

  void LongPressed(){
    Serial.println("Long Click          taaaaaaac");
  }

  void handle(){
    button.tick();
  }
};

Button button(0);


//------------------------Setup-------------------Begin
void setup() {
  Serial.begin(115200);

  //hour
  timeClient.begin();

  //WiFiManager intialisation
  WiFiManager wifiManager;
  wifiManager.setTimeout(180);
  if(!wifiManager.autoConnect("ESP8266_AP")) {
    Serial.println("failed to connect and timeout occurred");
    delay(3000);
    ESP.reset();
    delay(5000);
  }
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

  //hour
  timeClient.update();
  hourUTC0 = timeClient.getHours();
  minute = timeClient.getMinutes();
  second = timeClient.getSeconds();
  UpdateUTC();
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
                  webpage +=  "body {border-width:2px;border:1rem solid;border-color:#141b6b;margin:5% 15% 5% 15%;border-radius: 25px;background-color:#ffffff;} ";
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
                    
                      webpage +=  "<d2>";
                        webpage +=  "<d3>";
                	        webpage +=  "<subtitle>Time Settings</subtitle>";

                          webpage += "<form action='http://"+IPaddress+"' method='POST'>";
                            webpage +=  "<c1>";
                                webpage +=  "<txt>Time Zone</txt>";
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
								                webpage +=  "<input type=\"number\" placeholder=\"---Day---\" name=\"day\"min=\"1\" max=\"31\" required>";
                                  webpage +=  "<select name=\"month\" required>";
                                  webpage +=  "<option value=\"\">---Month---</option>";
                                  webpage +=  "<option value=\"1\">January</option>";
                                  webpage +=  "<option value=\"2\">February</option>";
                                  webpage +=  "<option value=\"3\">March</option>";
                                  webpage +=  "<option value=\"4\">April</option>";
                                  webpage +=  "<option value=\"5\">May</option>";
                                  webpage +=  "<option value=\"6\">June</option>";
                                  webpage +=  "<option value=\"7\">July</option>";
                                  webpage +=  "<option value=\"8\">August</option>";
                                  webpage +=  "<option value=\"9\">September</option>";
                                  webpage +=  "<option value=\"10\">October</option>";
                                  webpage +=  "<option value=\"11\">November</option>";
                                  webpage +=  "<option value=\"12\">December</option>";
                                  webpage +=  "</select>";
                                webpage +=  "<input type=\"number\" placeholder=\"---Year---\" name=\"year\"min=\"2020\" max=\"2300\" required>";
                            webpage +=  "</c1>";
                            webpage +=  "<c1>";
                                webpage +=  "<button class=\"button\" input type='submit'>Set</button>";
                            webpage +=  "</c1>";
                          webpage += "</form>";
                	      webpage +=  "</d3>";
                    
                        webpage +=  "<d3>";
                	        webpage +=  "<subtitle>Location Settings</subtitle>";
                          webpage +=  "<c1>";
                            webpage +=  "<txt>Latitude</txt>";
								            webpage +=  "<input type=\"number\" placeholder=\"Latitude\" id=\"latitude\" name=\"latitude\"min=\"-90\" max=\"90\">";
                    	    webpage +=  "</c1>";
                          webpage +=  "<c1>";
                            webpage +=  "<txt>Longitude</txt>";
								            webpage +=  "<input type=\"number\" placeholder=\"Longitude\" id=\"longitude\" name=\"longitude\"min=\"-180\" max=\"180\">";
                            webpage +=  "<button class=\"button\" input type='submit'>Set</button>";
                    	    webpage +=  "</c1>";
                        webpage +=  "</d3>";
                	    webpage +=  "</d2>";
                    webpage +=  "</h2>";
   webpage += "</body>";
  webpage += "</html>";
  server.send(200, "text/html", webpage); // Send a response to the client asking for input

  if (server.args() > 0 ) { // Arguments were received
    for ( uint8_t i = 0; i < server.args(); i++ ) {
      //Serial.print(server.argName(i)); // Display the argument
      Argument_Name = server.argName(i);
      if (server.argName(i) == "name_input") {
        Serial.print(" Input received was: ");
        Serial.println(server.arg(i));
        Clients_Response1 = server.arg(i);
        // e.g. range_maximum = server.arg(i).toInt();   // use string.toInt()   if you wanted to convert the input to an integer number
        // e.g. range_maximum = server.arg(i).toFloat(); // use string.toFloat() if you wanted to convert the input to a floating point number
      }
      if (server.argName(i) == "address_input") {
        Serial.print(" Input received was: ");
        Serial.println(server.arg(i));
        Clients_Response2 = server.arg(i);
      }
      if (server.argName(i) == "day") {
        Serial.print("Day num = ");
        Serial.println(server.arg(i));
        current_day = server.arg(i);
      }
      if (server.argName(i) == "month") {
        Serial.print("Month = ");
        Serial.println(server.arg(i));
        current_month = server.arg(i);
      }
      if (server.argName(i) == "year") {
        Serial.print("Year = ");
        Serial.println(server.arg(i));
        current_year = server.arg(i);
      }
      if (server.argName(i) == "utc") {
        Serial.print("UTC delay = ");
        Serial.println(server.arg(i));
        utc_delay = server.arg(i).toInt();
      }
    }
  }
}

void UpdateUTC(){
  switch (utc_delay) {
    case 0: currentHour=hourUTC0;break;
    case 1: currentHour=(hourUTC0+1)%24;break;
    case 2: currentHour=(hourUTC0+2)%24;break;
    case 3: currentHour=(hourUTC0+3)%24;break;
    case 4: currentHour=(hourUTC0+4)%24;break;
    case 5: currentHour=(hourUTC0+5)%24;break;
    case 6: currentHour=(hourUTC0+6)%24;break;
    case 7: currentHour=(hourUTC0+7)%24;break;
    case 8: currentHour=(hourUTC0+8)%24;break;
    case 9: currentHour=(hourUTC0+9)%24;break;
    case 10: currentHour=(hourUTC0+10)%24;break;
    case 11: currentHour=(hourUTC0+11)%24;break;
    case 12: currentHour=(hourUTC0+12)%24;break;
    case 13: currentHour=(hourUTC0+13)%24;break;
    case 14: currentHour=(hourUTC0+14)%24;break;
    case 15: currentHour=(hourUTC0+15)%24;break;
    case 16: currentHour=(hourUTC0+16)%24;break;
    case 17: currentHour=(hourUTC0+17)%24;break;
    case 18: currentHour=(hourUTC0+18)%24;break;
    case 19: currentHour=(hourUTC0+19)%24;break;
    case 20: currentHour=(hourUTC0+20)%24;break;
    case 21: currentHour=(hourUTC0+21)%24;break;
    case 22: currentHour=(hourUTC0+22)%24;break;
    case 23: currentHour=(hourUTC0+23)%24;break;
    default:currentHour=hourUTC0;break;
  }
}