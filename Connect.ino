
/******************************************************************
  - Startet die WLAN Verbindung
  - Einstellunge Zugangsdaten bzw. eventuelle feste IP in diesem Sketch 
/**************************************************************************************/


// ****************************************************************
// Sketch Esp8266 Connect STA Modular(Tab) mit optischer Anzeige
// created: Jens Fleischer, 2018-04-08
// last mod: Jens Fleischer, 2020-12-28
// For more information visit: https://fipsok.de
// ****************************************************************
// Hardware: Esp8266
// Software: Esp8266 Arduino Core 2.4.2 - 3.0.2
// Getestet auf: Nodemcu, Wemos D1 Mini Pro, Sonoff Dual
/******************************************************************
  Copyright (c) 2018 Jens Fleischer. All rights reserved.

  This file is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.
  This file is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.
*******************************************************************/
// Diese Version von Connect STA sollte als Tab eingebunden werden.
// #include <ESP8266WebServer.h> oder #include <ESP8266WiFi.h> muss im Haupttab aufgerufen werden
// Die Funktion "connectWifi();" muss im Setup eingebunden werden.
/**************************************************************************************/

#define CONFIG                                // Einkommentieren wenn der ESP dem Router die IP mitteilen soll.
#define NO_SLEEP                              // Auskommentieren wenn der Nodemcu den deep sleep Modus nutzt.

const char* ssid = "FRITZ!Asterisk";              // Darf bis zu 32 Zeichen haben.
const char* password = "7460651262901570";       // Mindestens 8 Zeichen jedoch nicht lÃ¤nger als 64 Zeichen.

#ifdef CONFIG
IPAddress staticIP(192, 168, 99, 15);         // Statische IP des NodeMCU ESP8266
IPAddress gateway(192, 168, 99, 10);            // IP-Adresse des Router
IPAddress subnet(255, 255, 255, 0);           // Subnetzmaske des Netzwerkes
IPAddress dns(192, 168, 99, 10);                // DNS Server
#endif

void connectWifi() {                            // Funktionsaufruf "connectWifi();" muss im Setup eingebunden werden.
  byte i = 0;
  //WiFi.disconnect();                          // Nur erforderlich wenn Esp den AP Modus nicht verlassen will.
  WiFi.persistent(false);                       // Auskommentieren wenn Netzwerkname und Passwort in den Flash geschrieben werden sollen.
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
#ifdef CONFIG
  WiFi.config(staticIP, gateway, subnet, dns);
#endif
  while (WiFi.status() != WL_CONNECTED) {
#ifdef NO_SLEEP
    pinMode(LED_BUILTIN, OUTPUT);               // OnBoardLed Nodemcu, Wemos D1 Mini Pro
    digitalWrite(LED_BUILTIN, 1);
#endif
    delay(500);
    digitalWrite(LED_BUILTIN, 0);
    delay(500);
    i++;
    //Serial.printf(" %d sek\n", i);
    if (i > 9) {
      //Serial.print("\nVerbindung zum AP fehlgeschlagen !\n\n");
      ESP.restart();
    }
  }
  //Serial.println("\nVerbunden mit: " + WiFi.SSID());
  //Serial.println("Esp8285 IP: " + WiFi.localIP().toString());
}
