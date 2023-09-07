
// ****************************************************************
// Sketch Esp8266 SNTP Lokalzeit Modular(Tab)
// created: Jens Fleischer, 2020-10-10
// last mod: Jens Fleischer, 2020-12-25
// For more information visit: https://fipsok.de
// ****************************************************************
// Hardware: Esp8266
// Software: Esp8266 Arduino Core 2.6.0 - 3.0.0
// Getestet auf: Nodemcu, Wemos D1 Mini Pro, Sonoff Switch, Sonoff Dual
/******************************************************************
  Copyright (c) 2020 Jens Fleischer. All rights reserved.

  This file is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.
  This file is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.
*******************************************************************/
// Diese Version von Lokalzeit sollte als Tab eingebunden werden.
// #include <ESP8266WebServer.h> muss im Haupttab aufgerufen werden.
// Funktion "setupTime();" muss im setup() nach dem Verbindungsaufbau aufgerufen werden.
// Automatische Umstellung zwischen Sommer- und Normalzeit anhand der Zeitzone.
/**************************************************************************************/

#include <time.h>

const uint32_t SYNC_INTERVAL = 5;                              // NTP Sync Interval in Stunden einstellen

struct tm tm;

const char* const PROGMEM NTP_SERVER[] = {"fritz.box", "de.pool.ntp.org", "at.pool.ntp.org", "ch.pool.ntp.org", "ptbtime1.ptb.de", "europe.pool.ntp.org"};
const char* const PROGMEM DAY_NAMES[] = {"Sonntag", "Montag", "Dienstag", "Mittwoch", "Donnerstag", "Freitag", "Samstag"};
const char* const PROGMEM DAY_SHORT[] = {"So", "Mo", "Di", "Mi", "Do", "Fr", "Sa"};
const char* const PROGMEM MONTH_NAMES[] = {"Januar", "Februar", "März", "April", "Mai", "Juni", "Juli", "August", "September", "Oktober", "November", "Dezember"};
const char* const PROGMEM MONTH_SHORT[] = {"Jan", "Feb", "Mrz", "Apr", "Mai", "Jun", "Jul", "Aug", "Sep", "Okt", "Nov", "Dez"};

void setupTime() {      // deinen NTP Server einstellen (von 0 - 5 aus obiger Liste) alternativ lassen sich durch Komma getrennt bis zu 3 Server angeben
  configTime("CET-1CEST,M3.5.0,M10.5.0/3", NTP_SERVER[0]);      // Zeitzone einstellen https://github.com/nayarsystems/posix_tz_db/blob/master/zones.csv
  server.on("/time", []() {
    server.send(200, "application/json", localTime());
  });
}

uint32_t sntp_update_delay_MS_rfc_not_less_than_15000() {       // Optionale Funktion, für den Individuellen SNTP Update Intervall. Standart ist jede Stunde.
  return SYNC_INTERVAL * 36e5;                                  // SNTP-Aktualisierungsverzögerung ändern.
}

String localTime() {
  static char buf[26];                                          // je nach Format von "strftime" eventuell anpassen
  static time_t previous;
  time_t now = time(&now);
  if (now != previous) {
    previous = now;
    localtime_r(&now, &tm);
    /* Verwendungbeispiele
      Serial.println(DAY_NAMES[tm.tm_wday]);                      // druckt den aktuellen Tag
      Serial.println(MONTH_NAMES[tm.tm_mon]);                     // druckt den aktuellen Monat
      Serial.println(DAY_SHORT[tm.tm_wday]);                      // druckt den aktuellen Tag (Abk.)
      Serial.println(MONTH_SHORT[tm.tm_mon]);                     // druckt den aktuellen Monat (Abk.)
    */
    strftime (buf, sizeof(buf), R"(["%T","%d.%m.%Y"])", &tm);   // http://www.cplusplus.com/reference/ctime/strftime/
  }
  return buf;
}
