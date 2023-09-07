
// ****************************************************************
// Sketch Esp8266 Zeitschaltuhr Singel Modular(Tab)
// created: Jens Fleischer, 2019-07-20
// last mod: Jens Fleischer, 2020-09-21
// For more information visit: https://fipsok.de
// ****************************************************************
// Hardware: Esp8266, Relais Modul o. Mosfet IRF3708 o. Fotek SSR-40 DA
// für Relais Modul
// GND an GND
// IN1 an D5 = GPIO14
// VCC an VIN -> je nach verwendeten Esp.. möglich
// Jumper JD-VCC VCC
// alternativ ext. 5V Netzteil verwenden
//
// für Mosfet IRF3708
// Source an GND
// Mosfet1 Gate an D5 = GPIO14
//
// für 3V Solid State Relais
// GND an GND
// SSR1 Input + an D5 = GPIO14
//
// Software: Esp8266 Arduino Core 2.4.2 / 2.5.2 / 2.6.3
// Getestet auf: Nodemcu, Wemos D1 Mini Pro
/******************************************************************
  Copyright (c) 2019 Jens Fleischer. All rights reserved.

  This file is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.
  This file is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.
*******************************************************************/
// Diese Version von Zeitschaltuhr sollte als Tab eingebunden werden.
// #include <FS.h> #include <ESP8266WebServer.h> müssen im Haupttab aufgerufen werden
// Die Funktionalität des ESP8266 Webservers ist erforderlich.
// Der Lokalzeit Tab ist zum ausführen der Zeitschaltuhr einzubinden.
// Die Funktion "singleTimerSwitch();" muss im Setup aufgerufen werden.
// Zum schalten muss die Funktion "timerSwitch();" im loop(); aufgerufen werden.
/**************************************************************************************/

#define SPIFFS LittleFS  // Einkommentieren wenn LittleFS als Filesystem genutzt wird

//const auto aktiv = HIGH;                       // LOW für LOW aktive Relais oder HIGH für HIGH aktive (zB. SSR, Mosfet) einstellen
//const uint8_t relPin = LED_BUILTIN;            // Pin für Relais einstellen
const auto count = 7;                        // Anzahl Schaltzeiten (analog Html Dokument) einstellen 1 bis 60
//bool relState{!aktiv};


//-------------------------------------------------------------------------------------
// Zeitstruktur
struct Collection {
  bool fixed;                     //Zeiten deaktiviert
  byte switchActive[count];       //Schaltzeit aktiviert
  byte wday[count];
  char switchTime[count * 2][6];
} times;

//-------------------------------------------------------------------------------------
// Initialisierung, Zeiten aus Datei laden
void singleTimerSwitch() {
  //digitalWrite(relPin, !aktiv);
  //pinMode(relPin, OUTPUT);
  File file = SPIFFS.open("/ctime.dat", "r");
  if (file && file.size() == sizeof(times)) {                          // Einlesen aller Daten falls die Datei im Spiffs vorhanden und deren Größe stimmt.
    file.read(reinterpret_cast<byte*>(&times), sizeof(times));
    file.close();
  } else {                                                             // Sollte die Datei nicht existieren
    for (auto i = 0; i < count; i++) {
      times.switchActive[i] = 1;                                       // werden alle Schaltzeiten
      times.wday[i] = ~times.wday[i];                                  // und alle Wochentage aktiviert.
    }
  }
  server.on("/timer", HTTP_POST, []() {
    if (server.args() == 1) {
      times.switchActive[server.argName(0).toInt()] = server.arg(0).toInt();
      printer();
      String temp = "\"";
      for (auto& elem : times.switchActive) temp += elem;
      temp += "\"";
      server.send(200, "application/json", temp);
    }
    if (server.hasArg("sTime")) {
      byte i {0};
      char str[count * 14];
      strcpy (str, server.arg("sTime").c_str());
      char* ptr = strtok(str, ",");
      while (ptr != NULL) {
        strcpy (times.switchTime[i++], ptr);
        ptr = strtok(NULL, ",");
      }
      if (server.arg("sDay")) {
        i = 0;
        strcpy (str, server.arg("sDay").c_str());
        char* ptr = strtok(str, ",");
        while (ptr != NULL) {
          times.wday[i++] = atoi(ptr);
          ptr = strtok(NULL, ",");
        }
        printer();
      }
    }
    String temp = "[";
    for (auto& elem : times.switchTime) {
      if (temp != "[") temp += ',';
      temp += (String)"\"" + elem + "\"";
    }
    temp += ",\"";
    for (auto& elem : times.switchActive) {
      temp += elem;
    }
    for (auto& elem : times.wday) {
      temp += "\",\"";
      temp += elem;
    }
    temp += "\"]";
    server.send(200, "application/json", temp);
  });
  server.on("/timer", HTTP_GET, []() {
    //if (server.hasArg("tog") && server.arg(0) == "tog") relState = !relState;             // Relais1 Status manuell ändern
    if (server.hasArg("tog") && server.arg(0) == "fix") {
      times.fixed = !times.fixed;                                                         // alle Schalzeiten deaktivieren/aktivieren
      //printer();    //beim Deaktivieren/Aktivieren nicht jedesmal speichern
    }
    server.send(200, "application/json", (String)"[\"" + 0 + "\"," + localTime().substring(1, 11) + ",\"" + times.fixed + "\"]");
    //server.send(200, "application/json", (String)"[[\"" + (relState == aktiv) + "\",\"" + times.fixed + "\"]," + localTime() + "\"]");
  });
}

//-------------------------------------------------------------------------------------
// Zeiten in Datei speichern
void printer() {
  File file = SPIFFS.open("/ctime.dat", "w");
  if (file) {
    file.write(reinterpret_cast<byte*>(&times), sizeof(times));
    file.close();
    iWritecountCtime++;
  }
}

//-------------------------------------------------------------------------------------
// Zyklischer Test auf Schaltzeit aktiv
void timerSwitch() {
  static uint8_t lastmin {CHAR_MAX};
  uint8_t FLEin = 0;
  uint8_t FLAus = 0;
  localTime();                                        // Uhrzeit aktualisieren
  
  if (tm.tm_min != lastmin && !times.fixed) {         // Prüfung nur wenn sich die Uhrzeit geändert hat
    lastmin = tm.tm_min;
    char buf[6];
    sprintf(buf, "%.2d:%.2d", tm.tm_hour, tm.tm_min); //aktuelle Uhrzeit
    for (auto i = 0; i < count * 2; i++) {            // i= 0 bis 13
      if (times.switchActive[i / 2] && !strcmp(times.switchTime[i], buf)) {      //Schaltzeit aktiviert und Uhrzeit (ein oder aus) passt 
        if (times.wday[i / 2] & (1 << (tm.tm_wday ? tm.tm_wday - 1 : 6))) 
        {
          if(i & 1) FLAus = 1;
          else FLEin = 1;
        }
      }
    }
  }

  //Schedulerstatus (de-,aktiviert) zur Verwendung im Programm
  iSchedIsActive = 0;
  for (auto i = 0; i < count * 2; i++) {            // i= 0 bis 13
    if (times.switchActive[i / 2] ){                //Schaltzeit aktiviert
      iSchedIsActive = 1;
    }
  }
  if (times.fixed) iSchedIsActive = 0;
  if (iSchedIsActive) digitalWrite(LED_BUILTIN, 0);
  else digitalWrite(LED_BUILTIN, 1);
  
  //Klimaanlage Ein-, Ausschalten
  if(FLEin) 
  {
    SendOn();
    iLastSource = 4;
  }
  else if(FLAus)
  {
    SendOff();
    iLastSource = 4;
  }
}
