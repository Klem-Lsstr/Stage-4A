//-----------ESP-8266 LED Controler
//-----------ESIREM / CVUT
//-----------12/04/2023---12/07/23

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <WebSocketsServer.h>

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

ESP8266WebServer server;
WebSocketsServer webSocket = WebSocketsServer(81);

//WIFI
const char* ssid = "C4-460";
const char* password = "bez_dratu460";

//const char* ssid = "Sergioleone";
//const char* password = "Sergioleone";

//Output states
String outputBroadCastState = "off";
String outputs00 = "off";
String outputs01 = "off";
String outputs02 = "off";
String outputs03 = "off";
String outputGradMinute = "off";
String outputGradSecond = "off";

String header;

//Busy
bool running = false;

char webpage[] PROGMEM = R"=====(
<!DOCTYPE html>
<html>
<head>
  <script>
    var Socket;
    function init() {
      Socket = new WebSocket('ws://' + window.location.hostname + ':81/');
      Socket.onmessage = function(event){
        document.getElementById("rxConsole").value += event.data;
      }
    }
    function sendText(){
      Socket.send(document.getElementById("txBar").value);
      document.getElementById("txBar").value = "";
    }
    function sendPowerOn(){
      Socket.send("O"+document.getElementById("poweron").value);
    }
    function sendPowerMax(){
      Socket.send("M"+document.getElementById("powermax").value);
    }
    function fctClicBtn(x) {
      var xhr = new XMLHttpRequest();
      xhr.open("GET", "/" + x, true);
      xhr.send();
    }
    function fctassignGroup(){
      var numGroup = document.getElementById("numGroup").value;
      var fromshort = document.getElementById("fromshort").value;
      var toshort = document.getElementById("toshort").value;
      var xhr = new XMLHttpRequest();
      xhr.open("GET", "/assignGroup?numGroup=" + numGroup + "&fromshort=" + fromshort + "&toshort=" + toshort, true);
      xhr.send();
    } 
  </script>
      <meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">
      <link rel=\"icon\" href=\"data:,\">
      <title>ESP-8266 LED Controler</title>
        <style>
          html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;background-color:#dce7f7;}
          body {border-width:2px;border:1rem solid;border-color:#141b6b;margin:5% 15% 5% 15%;border-radius: 25px;background-color:#ffffff;} 
          button {background-color: #6976ff; border: none; color: white; padding: 10px 30px;border-width:1px;border:solid;border-color:#141b6b;
          text-decoration: none; font-size: 20px; margin:2% 2% 2% 2%; cursor: pointer;border-radius: 25px;}
          .button2 {background-color:#949494;border-width:1px;border:solid;border-color:#141b6b;}
          .button:hover {color: #141b6b;}
          .button:active{background-color: #141b6b;}
          h1{display:grid;background-color:#ffffff;}
          h2{border-width:1px;border:solid;border-color:#141b6b;z-index: 1;margin:1% 1% 1% 1%;border-radius: 5px;display:grid;background-color:#dce7f7;}
          d1{border-width:1px;border:solid;border-color:#141b6b;margin:1% 1% 1% 1%;border-radius: 5px;display:grid;background-color:#ffffff;}
          c1{display:flex;margin:2% 2% 2% 2%;display: inline-block;}
          c2{display:flex;margin:2% 2% 2% 2%;display: inline-block;}
          e1{display:flex;justify-content: space-between;margin:2% 2% 2% 2%;}
          txttitle{color:#141b6b;}
          subtitle{color:#6976ff;}
          p{margin:25px;}
          text-decoration: none; font-size: 30px; margin: 0px auto; cursor: pointer;border-radius: 25px;}
          input,label {margin: 0.4rem 0;}
         </style>

</head>
<body onload="javascript:init()">
<h1>  
  <txttitle>  
    DALI Control Software  
  </txttitle>  
</h1>   

                    <h2>  
                      <d1>  
                		  <subtitle>Settings</subtitle>  
                          <e1>  
                			      <c2>  
                				        <txt>Init All</txt>      
                                  <a><button class="button" onclick="fctClicBtn('init')">INIT</button></a> 
                            </c2>  
                            <c2>  
                				        <txt>Clear All</txt>   
                                  <a><button class="button" onclick="fctClicBtn('clear')">CLEAR</button></a>
                            </c2>  
                            <c2>  
                				        <txt>Reset All</txt>    
                                  <a><button class="button" onclick="fctClicBtn('reset')">RESET</button></a>  
                			      </c2>  
                		      </e1>  
                          <e1>  
                        	  <c2>  
                				        <txt>Power On</txt>  
                                <input type="range" min="0" max="254" id="poweron" oninput="sendPowerOn()" value="select a value" step="1" /> 
                			      </c2>  
                            <c2>  
                				        <txt>Power Max</txt>  
                                <input type="range" min="0" max="254" id="powermax" oninput="sendPowerMax()" value="select a value" step="1" /> 
                			      </c2>  
                	        </e1>  
                      </d1>  
                    	
                      <d1>  
                      <subtitle>Basics</subtitle>  
                          <c1> 
                          <c2> 
                				    <txt>On/Off</txt>  
                                <a><button class="button" onclick="fctClicBtn('broadcastOn')">ON</button></a>  
                			          <a><button class="button button2" onclick="fctClicBtn('broadcastOff')">OFF</button></a>  
                			    </c2>
                          <c2>
                            <txt>LED 1</txt>  
                                <a><button class="button" onclick="fctClicBtn('s00on')">ON</button></a>  
                			          <a><button class="button button2" onclick="fctClicBtn('s00off')">OFF</button></a>  
                			    </c2>
                          <c2>
                            <txt>LED 2</txt>  
                                <a><button class="button" onclick="fctClicBtn('s01on')">ON</button></a>  
                			          <a><button class="button button2" onclick="fctClicBtn('s01off')">OFF</button></a>  
                			    </c2>
                          <c2>
                            <txt>LED 3</txt>  
                                <a><button class="button" onclick="fctClicBtn('s02on')">ON</button></a>  
                			          <a><button class="button button2" onclick="fctClicBtn('s02off')">OFF</button></a>  
                			    </c2>
                          <c2>
                            <txt>LED 4</txt>  
                                <a><button class="button" onclick="fctClicBtn('s03on')">ON</button></a>  
                			          <a><button class="button button2" onclick="fctClicBtn('s03off')">OFF</button></a>  
                			    </c2>
                          </c1>  
                	    </d1>  
                    
                      <d1>  
                	      <subtitle>Groups Options</subtitle>  
                          <c1>  
                            <txt>Group's Num</txt>
								          <input type="number" placeholder="group num" id="numGroup" name="numG"min="0" max="63">
                          <txt>From</txt>
								          <input type="number" placeholder="from" id="fromshort" name="fromshort"min="0" max="999">
                          <txt>To</txt>
                          <input type="number" placeholder="to" id="toshort" name="toshort"min="0" max="999">
                            
                          <a><button class="button" onclick="fctassignGroup()">ASSIGN</button></a> 
                    		</c1>
                	  </d1>  
                    
                      <d1>  
                        <subtitle>Special Mods</subtitle>  
                        <c1>
                				  <txt>Gradient slow</txt>     
                            <a><button class="button" onclick="fctClicBtn('gradslow')">SLOW</button></a>  
                			  </c1>
                        <c1>
                				  <txt>Gradient medium</txt>     
                            <a><button class="button" onclick="fctClicBtn('gradmedium')">MEDIUM</button></a>
                			  </c1>
                        <c1>
                				  <txt>Gradient fast</txt>
                          <a><button class="button" onclick="fctClicBtn('gradfast')">FAST</button></a>
                			  </c1>
                        <c1>
                				  <txt>Small Day simulation</txt>
                            <a><button class="button" onclick="fctClicBtn('smallday')">WAKE UP</button></a>
                			  </c1>

                	  </d1>  

                    </h2>
</body>
</html>
)=====";

void setup()
{
  WiFi.begin(ssid,password);
  Serial.print("Connecting to ");
  Serial.println(ssid);
  Serial.begin(115200);

  //Dali settings
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);
    pinMode(DALI_TX_PIN, OUTPUT);
    digitalWrite(DALI_TX_PIN, HIGH);
    pinMode(DALI_RX_PIN, INPUT);

  

  while(WiFi.status()!=WL_CONNECTED)
  {
    Serial.print(".");
    delayMilisec(500);
  }
  Serial.println("");
  Serial.println("//////////////////////////////////////////");
  Serial.println("///  Welcome to DALI Control Software  ///");
  Serial.println("//////////////////////////////////////////");
  Serial.println("WiFi connected.");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
  //send web page
  server.on("/",[](){server.send_P(200, "text/html", webpage);});
  
  //BBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBB
  //-----------Actions of Buttons--------------BEGIN--------------

  //---------------------------------Settings

  server.on("/clear", HTTP_GET, [](){
            Serial.println("Clear short addresses from all devices");
            DaliClear();
            });

  server.on("/init", HTTP_GET, [](){
            Serial.println("Initialize all devices, assigning the short adresses");
            DaliInit(0, 0);
            });

  server.on("/reset", HTTP_GET, [](){
            Serial.println("Reset all devices");
            DaliReset();
            });

  //---------------------------------Basics

  server.on("/broadcastOn", HTTP_GET, [](){//-----------------ON Broadcast
            outputBroadCastState = "on";
            outputs00 = "on";
            outputs01 = "on";
            outputs02 = "on";
            outputs03 = "on";
            BroadcastOnOff(ON); 
            }); 

  server.on("/broadcastOff", HTTP_GET, [](){//-----------------OFF Broadcast
            outputBroadCastState = "off";
            outputs00 = "off";
            outputs01 = "off";
            outputs02 = "off";
            outputs03 = "off";
            BroadcastOnOff(OFF);
            }); 

  server.on("/s00on", HTTP_GET, [](){//--------------------------s00
              String s ="s00";
              outputs00 = "on";
              int shortAddress = s.substring(1, 3).toInt();
              ShortOnOff(shortAddress, ON); 
            });
  server.on("/s00off", HTTP_GET, [](){
              String s ="s00";
              outputs00 = "off";
              int shortAddress = s.substring(1, 3).toInt();
              ShortOnOff(shortAddress, OFF); 
            });
server.on("/s01on", HTTP_GET, [](){//---------------------------s01
              String s ="s01";
              outputs01 = "on";
              int shortAddress = s.substring(1, 3).toInt();
              ShortOnOff(shortAddress, ON);
            });   
server.on("/s01off", HTTP_GET, [](){
              String s ="s01";
              outputs01 = "off";
              int shortAddress = s.substring(1, 3).toInt();
              ShortOnOff(shortAddress, OFF); 
            });
server.on("/s02on", HTTP_GET, [](){//---------------------------s02
              String s ="s02";
              outputs02 = "on";
              int shortAddress = s.substring(1, 3).toInt();
              ShortOnOff(shortAddress, ON); 
            });
server.on("/s02off", HTTP_GET, [](){
              String s ="s02";
              outputs02 = "off";
              int shortAddress = s.substring(1, 3).toInt();
              ShortOnOff(shortAddress, OFF); 
            });
server.on("/s03on", HTTP_GET, [](){//---------------------------s03
              String s ="s03";
              outputs03 = "on";
              int shortAddress = s.substring(1, 3).toInt();
              ShortOnOff(shortAddress, ON);
              }); 
server.on("/s03off", HTTP_GET, [](){
              String s ="s03";
              outputs03 = "off";
              int shortAddress = s.substring(1, 3).toInt();
              ShortOnOff(shortAddress, OFF); 
            });
            
  //---------------------------------Groups Options
server.on("/assignGroup", HTTP_GET, [](){
              String numGroup = server.arg("numGroup");
              String fromshort = server.arg("fromshort");
              String toshort = server.arg("toshort");
              int numGroupInt = numGroup.toInt();
              int fromshortInt = fromshort.toInt();
              int toshortInt = toshort.toInt();
              //mettre en int pour la suite
              for (int i = fromshortInt; i <= toshortInt; i++) {
                Serial.print("Assigning short address ");
                Serial.print(i);
                Serial.print(" to the group ");
                Serial.println(numGroupInt);

                DaliTransmitCMD((i << 1) + 1, (0x6 << 4) + numGroupInt);
                delayMilisec(DALI_TWO_PACKET_DELAY);
                DaliTransmitCMD((i << 1) + 1, (0x6 << 4) + numGroupInt);
                delayMilisec(DALI_TWO_PACKET_DELAY);
              } 
              Serial.println("Group "+numGroup +" will contain ShortAddresses from "+fromshort+" to "+toshort);
            });
  //---------------------------------Special Mods


server.on("/gradslow", HTTP_GET, [](){
              Serial.println("Gradientslow Sequence starting ");
              Gradient(1); 
              Serial.println("Gradientslow Sequence done ");
            });
server.on("/gradmedium", HTTP_GET, [](){
              Serial.println("Gradientmedium Sequence starting ");
              Gradient(5); 
              Serial.println("Gradientmedium Sequence done ");
            });
server.on("/gradfast", HTTP_GET, [](){
              Serial.println("Gradientfast Sequence starting ");
              Gradient(15); 
              Serial.println("Gradientfast Sequence done ");
            });
server.on("/smallday", HTTP_GET, [](){
              Serial.println("Sun is rising...");
              SmallDay(); 
              Serial.println("Good night ! ");
            });
  
  //-----------Actions of Buttons--------------END----------------
  //BBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBB

  server.begin();
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);
  PrintHelp();
}

void loop()
{
  webSocket.loop();
  server.handleClient();

  if(Serial.available() > 0){

      //Serial Monitor -----Begin
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

        if (comMsg.startsWith("0x")) // direct command
            {
                String cmd1_string = comMsg.substring(2, 4);
                String cmd2_string = comMsg.substring(4, 6);
                int cmd1 = (int)strtol(&cmd1_string[0], NULL, 16);
                int cmd2 = (int)strtol(&cmd2_string[0], NULL, 16);

                DaliTransmitCMD(cmd1, cmd2);
                delayMilisec(DALI_TWO_PACKET_DELAY);

                DaliTransmitCMD(cmd1, cmd2);
                delayMilisec(DALI_TWO_PACKET_DELAY);

                if (cmd1 < 0xa0) {
                    Serial.print("Direct Command: ");
                    Serial.print(cmd1_string);
                    Serial.print(cmd2_string);
                    Serial.print(" (command ");
                    Serial.print(cmd2);
                    Serial.println(")");
                }

                if ((cmd1 >= 0xa1) && (cmd1 != 0xfe) && (cmd1 != 0xff)) {
                    Serial.print("Special Command: ");
                    Serial.print(cmd1_string);
                    Serial.print(cmd2_string);
                    Serial.print(" (command ");
                    Serial.print((cmd1 - 0xa0 + 1)/2 + 0xff);
                    Serial.println(")");
                }

                if (cmd1 == 0xfe) {
                    Serial.print("Broadcast direct arc power control: ");
                    Serial.println(cmd2);
                }

                if (cmd1 == 0xff) {
                    Serial.print("Broadcast command: ");
                    Serial.println(cmd2);
                }

        };

        comMsg = "";
    //Serial Monitor -----End

    char c[] = {(char)Serial.read()};
    webSocket.broadcastTXT(c, sizeof(c));
  }
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


void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length){
  if(type == WStype_TEXT){
    if(payload[0] == 'O'){
      uint16_t poweron = (uint16_t) strtol((const char *) &payload[1], NULL, 10);
      Serial.print("Power On = ");
      Serial.println(poweron);
      SetPowerOnLevel(poweron);
    }

    else if(payload[0] == 'M'){
      uint16_t powermax = (uint16_t) strtol((const char *) &payload[1], NULL, 10);
      Serial.print("Power Max = ");
      Serial.println(powermax);
      SetMaxLevel(powermax);
    }

    else{
      for(int i = 0; i < length; i++)
        Serial.print((char) payload[i]);
      Serial.println();
    }
  }
  
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