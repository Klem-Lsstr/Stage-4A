//for Wemos D1 mini
#define DALI_TX_PIN  14
#define DALI_RX_PIN  16
#define LED_PIN      2

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

// new specification of delay functions
// using delayMilisec() and delayMicrosec() may interfere with web interface functions
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

void setup()
{
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);

    pinMode(DALI_TX_PIN, OUTPUT);
    digitalWrite(DALI_TX_PIN, HIGH);

    pinMode(DALI_RX_PIN, INPUT);

    Serial.begin(115200);
    //delayMilisec(500);
    Serial.println(" ");
    Serial.println("Welcome to DALI Control Software");
    Serial.println("Running ...");
    PrintHelp();
}

void loop()
{    
    // Read command from serial
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
