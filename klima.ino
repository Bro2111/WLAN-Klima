/**************************************************************************************/

// Interaktion mit der klima.html
// Kommunikation über UART mit der Klimaanlage

/**************************************************************************************/

//Bitdefinition Settings Byte 7
#define EcoBitSnd     7 
#define BuzzerBitSnd  5
#define DisplayBitSnd 6
#define OnBitSnd      2

//Bitdefinition Byte 7 Empfang
#define EcoBitRcv     6
#define OnBitRcv      4

uint8_t LastSend[70];
uint8_t LastSendCnt = 0;
uint8_t LastRCV[70];
uint8_t LastRCVCnt = 0;
uint8_t serialBuffer[70];
uint8_t serialBufferIndex;
unsigned long Timeout = 0;        //Timer uart timeout protokoll
// uint8_t iTemperatur = 21;
uint8_t iModus = 8;
uint8_t iAir = 0;
uint8_t iSetting = 224;
char sRcvTime[] = "--:--:--";
char topic;


//-------------------------------------------------------------------------------------
void WebsiteUpdate() {          // Funktionsaufruf "WebsiteUpdate();" muss im Setup eingebunden werden
  server.on("/daten", handleWebsiteUpdate);
  server.on("/btnclick", handleWebsiteButton);
  server.on("/btnSettingSend", handleWebsiteButtonSetting);
  server.on("/btnSettingShow", handleWebsiteButtonSettingShow);
}

//-------------------------------------------------------------------------------------
void LoadSetup() {          // Läd sie Einstellungen aus der Datei
  File file = SPIFFS.open("/settings.dat", "r");
  if (file && file.size() == 10) {                          // Einlesen aller Daten falls die Datei im Spiffs vorhanden und deren Größe stimmt.
    char load[10];
    file.read(reinterpret_cast<byte*>(&load), sizeof(load));
    file.close();
    iTemperatur = load[0];
    iModus = load[1];
    iAir = load[2];
    iSetting = load[3];
    cmdSource = load[4];
  } else {                                                   // Sollte die Datei nicht existieren
    iTemperatur = 21;   //21 Grad
    iModus = 1;         //Heizen
    iAir = 0;           //Auto
    iSetting = 96;      //Display, Buzzer, Aus, kein Eco
    cmdSource = 0b00000111;      //Timer, UDP, Website
  }
}

//-------------------------------------------------------------------------------------
// Anzeigen der Button auf der Klimaseite und zrucksenden des Status
void handleWebsiteButtonSetting() {
  
  //if(IsDebug()) Serial.println("\nhandleWebsiteButtonBedien: " +server.arg(0) + " " + server.arg(1) + " " + server.arg(2) + " " + server.arg(3) + " " + server.arg(4) + " " + server.arg(5) + " " + server.arg(6)  + " " +server.arg(7) + " " + server.arg(8) + " " + server.arg(9) + " " + server.arg(10));

  //Temperatur
  uint8_t i = atoi(server.arg(0).c_str());
  if(i > 15 && i<27) iTemperatur = i;
  //Modus
  if(server.arg(1) == "radHeat") iModus = 1;
  else if(server.arg(1) == "radCool") iModus = 3;
  else if(server.arg(1) == "radAuto") iModus = 8;
  if(server.arg(3) == "radAir4") iModus = iModus + 128; //Lüfter in Stellung Mute

  //Lüfterdrehzahl
  if(server.arg(3) == "radAir0") iAir = 0;
  else if(server.arg(3) == "radAir1") iAir = 2;
  else if(server.arg(3) == "radAir2") iAir = 3;
  else if(server.arg(3) == "radAir3") iAir = 5;
  else if(server.arg(3) == "radAir4") iAir = 2;
  
  CheckCheckbox(server.arg(2), &iSetting, EcoBitSnd); //Eco
  CheckCheckbox(server.arg(5), &iSetting, DisplayBitSnd); //Display
  CheckCheckbox(server.arg(6), &iSetting, BuzzerBitSnd); //Buzzer

  //CheckCheckbox(server.arg(7), &cmdSource, 0); //Timer
  //CheckCheckbox(server.arg(8), &cmdSource, 1); //UDP
  //CheckCheckbox(server.arg(9), &cmdSource, 2); //Website
  //CheckCheckbox(server.arg(10), &cmdSource, 3); //Debug

  //Save Settings
  if(server.arg(4) == "on") 
  {
    File file = LittleFS.open("/settings.dat", "w");
    if (file) {
      char save[10];
      save[0] = iTemperatur;
      save[1] = iModus;
      save[2] = iAir;
      save[3] = iSetting;
      save[4] = cmdSource;
      file.write(reinterpret_cast<byte*>(&save), 10);
      file.close();
      iWritecountSettings++;
    }
  }

  handleWebsiteButtonSettingShow();
}

//-------------------------------------------------------------------------------------
// Auswertung einer Checkbox von der Website und Setzen eines Bits
void CheckCheckbox(String arg, uint8_t* wert, uint8_t bit) {
  
  if(arg == "on") *wert |= (1 << bit);
  else if(arg == "off") *wert &= ~(1 << bit);
}

//-------------------------------------------------------------------------------------
//zurücksenden des Status bei Button pressed
void handleWebsiteButtonSettingShow() {
 
char sModus[13] = {0};
  if((iModus == 1) || (iModus == 129)) {
    strcpy(sModus, "radHeat");
    Modus= "Heat";
  }
  else if ((iModus == 3) || (iModus == 131)) {
    strcpy(sModus, "radCool");
    Modus= "Cool";
  }
  else if(iModus == 8 || iModus == 138) {
    strcpy(sModus, "radAuto");
    Modus= "Auto";
  }
 char sAir[] = "radAir_";
  if(iAir == 0) sAir[6] = '0';
  else if(iAir == 2) sAir[6] = '1';
  else if(iAir == 3) sAir[6] = '2';
  else if(iAir == 5) sAir[6] = '3';
  if(iModus > 127) sAir[6] = '4';

char sEco[] = "off";
  Eco = "off";
  if(bitRead(iSetting,EcoBitSnd)) {
    strcpy(sEco, "on");
    Eco = "on";
  }
char sDisp[] = "off";
  Display = "off";
  if(bitRead(iSetting, DisplayBitSnd)) {
    strcpy(sDisp, "on");
    Display = "on";
  }
char sBuzz[] = "off";
  Buzzer = "off";
  if(bitRead(iSetting, BuzzerBitSnd)) {
    strcpy(sBuzz, "on");
    Buzzer = "on";
  }
//  char timer[] = "off";
//  char udp[] = "off";
//  char web[] = "off";
//  char debug[] = "off";
//  if(cmdSource & 1) strcpy(timer, "on");
//  if(cmdSource & 2) strcpy(udp, "on");
//  if(cmdSource & 4) strcpy(web, "on");
//  if(cmdSource & 8) strcpy(debug, "on");

//JSON zusammensetzen
  char temp[255];
  //sprintf(temp, "[\"%d\",\"%s\",\"%s\",\"%s\",\"%s\",\"%s\",\"%s\",\"%s\",\"%s\",\"%s\",\"%s\"]",iTemperatur,sModus, sEco, sAir, "off", sDisp, sBuzz); //, timer, udp, web, debug);
  sprintf(temp, "[\"%d\",\"%s\",\"%s\",\"%s\",\"%s\",\"%s\",\"%s\"]",iTemperatur,sModus, sEco, sAir, "off", sDisp, sBuzz);
  server.send(200, "application/json", temp);
}

//-------------------------------------------------------------------------------------
// Auswerten der Button auf der Klimaseite und zrucksenden des Status
void handleWebsiteButton() {
  char btnResponse[] = "0";
  
  if (server.arg(0) == "btnON") {
    if(cmdSource & 4) 
    {
      SendOn();
      iLastSource = 3;
      btnResponse[0] = '1';
    }
  }
  else if (server.arg(0) == "btnOFF") {
    if(cmdSource & 4) 
    {
      SendOff();
      iLastSource = 3;
      btnResponse[0] = '1';
    }
  }
  server.send(200, "text/plain", btnResponse);    //Rückmeldung ob erfolgreich gesendet
}

//-------------------------------------------------------------------------------------
// Zyklisches Update der Anzeige auf der Website
void handleWebsiteUpdate() {
  //Ziel ist folgender JSON String: ["UDP","187,0,.....,1,0,189","187,1,.....,0,0,182"]

  char sRcv[255] = {0};
  char sSent[255] = {0};

  //Empfangsdatenstring
  for (int i = 0; i < LastRCVCnt; i++ ) 
  {
    sprintf(sRcv, "%s%i,",sRcv,LastRCV[i]);
  }
  if(strlen(sRcv)) sSent[strlen(sRcv)-1] = 0;  //letztes Komma löschen

  //Sendedatenstring
 for (int i = 0; i < LastSendCnt; i++ ) 
  {
    sprintf(sSent, "%s%i,",sSent,LastSend[i]);
  }
  if(strlen(sSent)) sSent[strlen(sSent)-1] = 0;  //letztes Komma löschen
  
  char cLastSource[16] = {0};
  if(iLastSource == 1) strcpy(cLastSource, "Timer");
  //if(iLastSource == 1) strcpy_P(cLastSource, PSTR("Timer"));    //spart RAM Nutzt Flash
  else if(iLastSource == 2) strcpy(cLastSource, "UDP-Raw");
  else if(iLastSource == 3) strcpy(cLastSource, "Web");
  else if(iLastSource == 4) strcpy(cLastSource, "Scheduler");
  else if(iLastSource == 5) strcpy(cLastSource, "UDP-Cmd");
  else strcpy(cLastSource, "keine");

  //Betriebszustand
  char sStatus[50] = {0};
  if(LastRCVCnt == 61 && (LastRCV[3] == 4 || LastRCV[3] == 3))    //kompletter und richtiger  Datensatz empfangen
  {
    if(LastRCV[7] & 0b00010000){ 
      strcpy(sStatus, "Ein");
      Status = "Ein";
    }
    else {
      strcpy(sStatus, "AUS");
      Status = "AUS";
    }
    if(LastRCV[7] & 0b01000000){ 
      strcat(sStatus, ", Eco");      // Eco
      Status = "Eco";
    }

 /*   uint8_t i = (LastRCV[8] & 0b00110000) >> 4;   //Lüfterdrehzahl        
    if(!i){ 
      strcat(sStatus, ", Lüfter Auto");
      Lufter =  "Auto";
    }      
    else if(i == 1){ 
      if (LastRCV[8] >=128) {
        strcat(sStatus, ", Lüfter Mute");
        Lufter =  "Mute";
      }
      else {
        strcat(sStatus, ", Lüfter Low");
        Lufter =  "Low";
      }
    }      
    else if(i == 2){ 
      strcat(sStatus, ", Lüfter Mid");
      Lufter =  "Mid";
    }      
    else if(i == 3){ 
      strcat(sStatus, ", Lüfter High");
      Lufter =  "High";      
    }*/

    uint8_t i = (LastRCV[8] & 0b00110000) >> 4;
    uint8_t i1 = (LastRCV[7] & 0b00000111);

    if(!i1){ 
      strcat(sStatus, ", Modus ???");
      Modus1 =  "Auto";
    }      
    else if(i1 == 1){ 
        strcat(sStatus, ", Modus Cool");
        Modus1 =  "Cool";
      }
    else if(i1 == 2){ 
      strcat(sStatus, ", Modus Fan");
      Modus1 =  "Fan";
    }      
    else if(i1 == 3){ 
      strcat(sStatus, ", Modus dehumid");
      Modus1 =  "High";      
    }
    else if(i1 == 4){ 
      strcat(sStatus, ", Modus Heat");
      Modus1 =  "Heat";
    }      
    else if(i1 == 5){ 
      strcat(sStatus, ", Modus Auto");
      Modus1 =  "Auto";
    } 
    
    if (LastRCV[33] == 128){
        strcat(sStatus, ", Lüfter Mute");
        Lufter =  "Mute";
      }
    else if (LastRCV[34] == 85){
        strcat(sStatus, ", Lüfter Low");
        Lufter =  "MidLow";
      }
    else if(!i){ 
      strcat(sStatus, ", Lüfter Auto");
      Lufter =  "Auto";
    }
    else if (LastRCV[34] == 90){
        strcat(sStatus, ", Lüfter MidLow");
        Lufter =  "MidLow";
      }
    else if (LastRCV[34] == 95){
        strcat(sStatus, ", Lüfter Medium");
        Lufter =  "Medium";
    }
    else if (LastRCV[34] == 100){
        strcat(sStatus, ", Lüfter MidHigh");
        Lufter =  "MidHigh";
    }
    else if (LastRCV[34] == 115){
        strcat(sStatus, ", Lüfter High");
        Lufter =  "High";
    }
    else if (LastRCV[34] == 125){
        strcat(sStatus, ", Lüfter Strong");
        Lufter =  "Strong";
    }

    sprintf(sStatus, "%s, %i°C",sStatus, (LastRCV[8] & 0b00001111) +16);  //Temperatur
    iTemperatur = (LastRCV[8] & 0b00001111) +16;  //Temperatur

    sprintf(sStatus, "%s  (%s)", sStatus, sRcvTime); //Zeitstempel
  } 
  else strcpy(sStatus, "---");
  
  

  Tempinin = (float(LastRCV[17]) - 65)/2;
  Tempinheat = (float(LastRCV[30]) - 65)/2;
  Tempoutin = (LastRCV[35] - 20);
  Tempoutheat = (LastRCV[37] - 20);
  Tempoutout = (LastRCV[36] - 20);
  
  //JSON zusammensetzen
  char temp[255];
  sprintf(temp, "[\"%s\",\"%s\",\"%s\",\"%s\"]",cLastSource, sSent, sRcv, sStatus);
  server.send(200, "application/json", temp);
  
  //Webseite ist aktuell aufgerufen -> zyklisches Abfragen der Daten der Klimaanlage
  cmdSource |= (1 << 4);
}

//-------------------------------------------------------------------------------------
//Klimaanage ausschalten
void SendOff() {
	char cmd[] = {187,0,1,3,29,0,0,96,1,90,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,128,31};
	
  cmd[9] = 111 - iTemperatur;   //Temperatur
  cmd[8] = iModus;              //Modus (Heizen,Kühlen etc.) Byte 7 und 8 ist noch nicht berücksichtigt
  cmd[10] = iAir;               //Lüfterdrehzahl
  cmd[7] = iSetting;            //Eco, Display, Buzzer + EIN
  bitClear(cmd[7], 2);

  //Prüfzeichen XOR
  uint8_t crc = 0;
  for(uint8_t i=0; i<34;i++)
  {
    crc = crc ^ cmd[i];
  }
  cmd[34] = crc;
  
  SendCmd(cmd, 35);
}

//-------------------------------------------------------------------------------------
//Klimaanage einschalten (Basiskommando und gespeicherte Einstellungen)
void SendOn() {
	char cmd[] = {187,0,1,3,29,0,0,96,1,90,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,128,31};

  cmd[9] = 111 - iTemperatur;   //Temperatur
  cmd[8] = iModus;              //Modus (Heizen,Kühlen etc.) Byte 7 und 8 ist noch nicht berücksichtigt
  cmd[10] = iAir;               //Lüfterdrehzahl
  cmd[7] = iSetting | 4;        //Eco, Display, Buzzer + EIN
  cmd[32] = 3;                  //Klappenstellung
  
  //Prüfzeichen XOR
  uint8_t crc = 0;
  for(uint8_t i=0; i<34;i++)
  {
    crc = crc ^ cmd[i];
  }
  cmd[34] = crc;

	SendCmd(cmd, 35);
}

//-------------------------------------------------------------------------------------
//Klimaanage ausschalten
void SendOffMqtt() {
	char cmd[] = {187,0,1,3,29,0,0,96,1,90,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,128,31};
	
  cmd[9] = 111 - iTemperaturMqtt;   //Temperatur
  cmd[8] = iModusMqtt;              //Modus (Heizen,Kühlen etc.) Byte 7 und 8 ist noch nicht berücksichtigt
  cmd[10] = iAirMqtt;               //Lüfterdrehzahl
  cmd[7] = iSettingMqtt;            //Eco, Display, Buzzer + EIN
  bitClear(cmd[7], 2);

  //Prüfzeichen XOR
  uint8_t crc = 0;
  for(uint8_t i=0; i<34;i++)
  {
    crc = crc ^ cmd[i];
  }
  cmd[34] = crc;
  
  SendCmd(cmd, 35);
}

//-------------------------------------------------------------------------------------
//Klimaanage einschalten (Basiskommando und gespeicherte Einstellungen)
void SendOnMqtt() {
	char cmd[] = {187,0,1,3,29,0,0,96,1,90,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,128,31};

  cmd[9] = 111 - iTemperaturMqtt;   //Temperatur
  cmd[8] = iModusMqtt;              //Modus (Heizen,Kühlen etc.) Byte 7 und 8 ist noch nicht berücksichtigt
  cmd[10] = iAirMqtt;               //Lüfterdrehzahl
  cmd[7] = iSettingMqtt | 4;        //Eco, Display, Buzzer + EIN
  cmd[32] = 3;                  //Klappenstellung
  
  //Prüfzeichen XOR
  uint8_t crc = 0;
  for(uint8_t i=0; i<34;i++)
  {
    crc = crc ^ cmd[i];
  }
  cmd[34] = crc;

	SendCmd(cmd, 35);
}

//-------------------------------------------------------------------------------------
//zyklische Abfrage der Klimaanlage wenn Timer aktiviert und Website aufgerufen ist
void SendRequest() {
	  char cmd[] = {187,0,1,4,2,1,0,189};
	  SendCmd(cmd, 8);
    cmdSource &= ~(1 << 4);   //Flag "Website aufgerufen" wieder löschen
}

//-------------------------------------------------------------------------------------
//Wertet den UDP Befehlssatz aus und sendet ein entsprechendes Kommando an die Klima
void ParseUDP(char aUdp[], uint8_t len){

  if(aUdp[0] == 97)
  {
    /*  Byte  0	 	Startzeichen	        97
              1   Befehl                '?'											
              2   Befehl                '?'											
    */
    SendRequest();
  }
  else if(aUdp[0] == 98)
  {
    /*  Byte  0	 	Startzeichen	        98											
              1		Ein/Aus					      0 = Aus		>0 = Ein									
              2		Temperatur					  21											
              3		Modus					        1=Heizen  2=Trocknen  3=Kühlen	7=Lüftung		8=Auto
              4		Buzzer					      0 = Aus		>0 = Ein									
              5		Display					      0 = Aus		>0 = Ein									
              6		Eco					          0 = Aus		>0 = Ein									
              7		Airspeed					    noch nicht definiert											
              8		Klappenstellung				noch nicht definiert											
              9		noch nicht definiert																
              10	noch nicht definiert																
              11	noch nicht definiert																
              12	noch nicht definiert																
              13	noch nicht definiert																
              14	noch nicht definiert																
              15	noch nicht definiert						
    */

    iTemperatur = aUdp[2];              //Temperatur
    iModus = aUdp[3];                   //Modus
    bitWrite(iSetting, BuzzerBitSnd, aUdp[4]);     //Buzzer
    bitWrite(iSetting, DisplayBitSnd, aUdp[5]);    //Display
    bitWrite(iSetting, EcoBitSnd, aUdp[6]);        //Eco
    iAir = aUdp[7];                     //Lüfterdrehzahl

    //Ein/Aus
    if(aUdp[1]) SendOn();
    else SendOff();
  }
  else iLastSource = 0;
}

//-------------------------------------------------------------------------------------
//Sendet den übergebenden Datansatz an die Klima
void SendCmd(char aToSend[], uint8_t len){
  for (int i = 0; i < len; i++ ) 
  {
    LastSend[i] = aToSend[i];
  }
  LastSendCnt = len;

	serialBufferIndex = 0;
  Timeout = millis();
  Serial.write(aToSend, len);    //je nach Transmitbuffergrösse möglicherweise blockierend
}

//-------------------------------------------------------------------------------------
void ReceiveUART() {
  while (Serial.available()>0)
  {
    uint8_t c = Serial.read();
    if (c == 187)       //Framestart
    {
      serialBuffer[0] = c;    
      serialBufferIndex = 1;
    }
    else if (serialBufferIndex < 70 - 1)   //solange noch Platz im Puffer ist
    {
      serialBuffer[serialBufferIndex++] = c;    //Zeichen abspeichern und Index inkrementieren
    }
	  Timeout = millis();
  }


  if ( (serialBufferIndex !=0) && ((millis() - Timeout) >= (500L))) // 200 ms 
  {
	  //200ms keine Daten mehr -> Paket komplett
	  if(serialBuffer[0] == 187)
    {
      // Empfangene Daten sichern
      for (int i = 0; i < serialBufferIndex; i++ ) 
      {
        LastRCV[i] = serialBuffer[i];
      }
      LastRCVCnt = serialBufferIndex;
      
      if(iLastSource == 2) //RAW Paket kam von UDP -> Antwort senden
      {
        SendBackUDP(LastRCV, LastRCVCnt);
      }
      else if(iLastSource == 5) //Befehlspaket kam von UDP -> Antwort senden
      {
        uint8_t aUdp[] = {99,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
        //Datenaufbau siehe ParseUDP()
        aUdp[1] = bitRead(LastRCV[7], OnBitRcv);  //Ein/Aus
        aUdp[2] = (LastRCV[8] & 0b00001111) +16;  //Temperatur
        aUdp[3] = 0;                              //Modus
        aUdp[4] = 0;                              //Buzzer
        aUdp[5] = 0;                              //Display
        aUdp[6] = bitRead(LastRCV[7], EcoBitRcv); //Eco
        aUdp[7] = (LastRCV[8] & 0b00110000);      //Lüfterdrehzahl        
        aUdp[15] = iSchedIsActive;                //Scheduler ist aktiviert 

        SendBackUDP(aUdp, 16);
      }
    }
    serialBufferIndex = 0;
    sprintf(sRcvTime, "%.2d:%.2d:%.2d", tm.tm_hour, tm.tm_min, tm.tm_sec); //aktuelle Uhrzeit
    Timeout = millis();
  }

  /*if ( !serialBufferIndex && (millis() - Timeout) >= (2000L)); // 2 s 
  {
	  //2s keine Daten -> Offline
	  //if(IsDebug()) Serial.println("ccc");
    //serialBufferIndex = 0;
  }*/
}

//-------------------------------------------------------------------------------------
//bool IsDebug() {
//  return (cmdSource & 8);
//}  

