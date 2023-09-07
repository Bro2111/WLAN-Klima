
// ****************************************************************
// WLAN-Modul für Klimaanlage 
// mit ESP8285-M3
// Lochraster Platine: WLAN-Klima 
// Auf Basis der Sketche von Jens Fleischer https://fipsok.de
//
/******************************************************************
  
  MODUL
------------  
  - ESP8285-M3: 75kb RAM for Heap and Data 
	  1MB Flash, 128k FS, 438k OTA

  WLAN
------------
  - das Modul verbindet sich mit dem WLAN (eingestellt im Sketch "Connect"
  - eigener Webserver 
  - Dateiup- und download über die Filesystem HTML Seite

  UART
------------
  - Verbindung mit der Klimaanlage über UART 5V
  
  UDP
------------
  - werden Daten über UDP Port 5444 empfangen:
  - ist das erste Zeichen eine 187 werden die RAW Daten direkt über UART an die Klimaanlage weitergeleitet
    die Antwort wird über UDP zurück gesendet 
  - ist das erste Zeichen eine 98 gilt folgendes Format:
    Byte  0	 	Startzeichen	        98											
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

  - ist das erste Zeichen eine 97 wird nur der Status abgefragt, es gilt folgendes Format:
    Byte  0	 	Startzeichen	        97
          1   Befehl                '?'											
          2   Befehl                '?'											

    Als Antwort mit Startbyte 99 kommt der aktuelle Zustand im selben Format

  FLASHEN
------------
  - Programmierung über Rx/Tx und FTDI Adapter mit 5V Jumper-Einstellung
  - Programmiermodus wird aufgerufen wenn während des Starts DIO0 (Jumper) auf GND liegt
    anschliessend DIO0 wieder öffnen
  - Programmierung auch über OTA ( es darf nur der halbe Flash belegt sein)


----------------------------------------------------------------------------------------------------------------------
todo:
Anzeige aktueller Zustand erweitern (Modus)
HTML Debug in andere HTML Datei
"Last Source" ändern in Last On/Off
Aktuelle Tempereatureinstellung (Zahl) wird auf der Webseite nicht aktualisiert, nur der Schieberegler

V1.6 
19.4.2023
- Temperaturanzeige korrigiert

V1.5
5.2.2023
- Debug entfernt
- Einstellmöglichkeit cmdSource entfernt

V1.4
4.2.2023
- Lüfterstärke lässt sich von SPS einstellen und ebenfalls im Web anzeigen
- Temperaturanzeige (Status) korrigiert
- Scheduler Status zur Meldung an SPS und blaue LED
- Beim Ausschalten ebenfalls Display, Eco und Buzzer berücksichtigen
- Scheduler Activ-Status wird beim Seitenaufruf angezeigt, nicht erst beim Buttondruck
- Schreibzyklen der Setting.dat und ctime.dat wird auf der Admin Seite angezeigt 

V1.3
- NTP auf Router umgestellt
- UDP Befehlssatz eingefügt zusätzlich zu den RAW Daten

V1.2
24.1.23
- Anzeige aktueller Zustand
- Timer UART Abfrage nur wenn Webseite auch gerade angezeigt wird
- Zeitstempel letzter Befehl  
- etc.

V1.1
23.1.23
- viele Änderungen/Erweiterungen

V1.0
20.1.23
- Erste Version


/**************************************************************************************/

//#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ArduinoOTA.h>         // https://arduino-esp8266.readthedocs.io/en/latest/ota_updates/readme.html
#include <LittleFS.h>           // Library für Dateisystem LittleFS
//#include <FS.h>               // Library für Dateisystem Spiffs einkommentieren wenn erforderlich
#include <WiFiUDP.h>
#include <PubSubClient.h>
#include <iostream>
#include <string>

#define MSG_BUFFER_SIZE (50)
const char* mqtt_server = "192.168.99.9"; // eg. your-demo.cedalo.cloud or 192.168.1.11
const uint16_t mqtt_server_port = 1883; // or 8883 most common for tls transport
const char* mqttUser = "";
const char* mqttPassword = "";
const char* mqttTopicIn = "Klima/Wohnzimmer/klima-in";
const char* mqttTopicOut = "Klima/Wohnzimmer/klima-out/";
const char* mqttTempOut = "Klima/Wohnzimmer/klima-out/Temperatur";
const char* mqttStateOut = "Klima/Wohnzimmer/klima-out/state";
const char* mqttLuefterOut = "Klima/Wohnzimmer/klima-out/Luefter";
const char* mqttModusOut = "Klima/Wohnzimmer/klima-out/Status";
const char* mqttModus1Out = "Klima/Wohnzimmer/klima-out/Modus";
const char* mqttEcoOut = "Klima/Wohnzimmer/klima-out/Eco";
const char* mqttDispOut = "Klima/Wohnzimmer/klima-out/Display";
const char* mqttBuzzOut = "Klima/Wohnzimmer/klima-out/Buzzer";
const char* mqttAntOut = "Klima/Wohnzimmer/klima-out/Antwort";
const char* mqttAntOut1 = "Klima/Wohnzimmer/klima-out/Antwort1";
const char* mqttTempin = "Klima/Wohnzimmer/klima-out/Tempinin";
const char* mqttTempheatin = "Klima/Wohnzimmer/klima-out/Tempinheat";
const char* mqttTempoutin = "Klima/Wohnzimmer/klima-out/Tempoutin";
const char* mqttTempout = "Klima/Wohnzimmer/klima-out/Tempoutin";
const char* mqttTempheatout = "Klima/Wohnzimmer/klima-out/Tempoutheat";
const char* mqttTempoutout = "Klima/Wohnzimmer/klima-out/Tempoutout";
const char* mqttAnt1Out = "Klima/Wohnzimmer/klima-out/Antwort1";
uint8_t iTemperatur = 21;
const char* Status = "Aus";
const char* Lufter = "Auto";
const char* Modus = "Auto";
const char* Eco = "off";
const char* Display = "off";
const char* Buzzer = "off";
const char* Modus1 = "Auto";
float Tempinin;
float Tempinheat;
uint8_t Tempoutin;
uint8_t Tempoutheat;
uint8_t Tempoutout;
uint8_t iTemperaturMqtt = 21;   //Temperatur
uint8_t iModusMqtt = 131;              //Modus (Heizen,Kühlen etc.) Byte 7 und 8 ist noch nicht berücksichtigt
uint8_t iAirMqtt = 4;               //Lüfterdrehzahl
uint8_t iSettingMqtt = 132;        //Eco, Display, Buzzer + EIN
uint8_t iStart = 10;
uint8_t i1=0;

ESP8266WebServer server(80);

WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

unsigned long Timer1 = 0;		    //Timer für Klima Send   //falls es gleich beim Start losgehen soll: Timer1 = 0 - INTERVAL;
unsigned long Timer2 = 20000;		//Timer für MQTT Send   //falls es gleich beim Start losgehen soll: Timer1 = 0 - INTERVAL;
uint8_t iLastSource = 0;        //Letzte Befehlsquelle: 0 = keine, 1 = Timer, 2 = UDP RAW, 3 = Web, 4 = Scheduler, 5 = UDP Befehl
uint8_t cmdSource = 7;          //Bit 0 = Intervall; 
                                //Bit 1 = UDP; 
                                //Bit 2 = Website, 
                                //Bit 3 = RS232debug, 
                                //Bit 4 = Webseite ist online (zusammen mit Bit 0 Abfragen), 
uint8_t iSchedIsActive;         //Scheduler ist aktiviert 
uint16_t iWritecountSettings = 0;
uint16_t iWritecountCtime = 0;


String sketchName() {           // Dateiname für den Admin Tab ab EspCoreVersion 2.6.0
  char file[sizeof(__FILE__)] = __FILE__;
  char * pos = strrchr(file, '.'); *pos = '\0';
  return file;
}

void setup() {
  Serial.begin(9600,SERIAL_8E1);
  delay(100);
  
  setupFS();                    // setupFS(); oder spiffs(); je nach Dateisystem
  connectWifi();
  admin(); 
  WebsiteUpdate();					    // Update der Klimawebseite Initialisieren
  LoadSetup();                  //Konfiguration aus Datei laden
  //bme280();
  //bme280Duo();
  //dht22();
  //ds18b20();
  //espboardLed();
  //onboardLed();               // Die Tabs die du nutzen mÃ¶chtest, musst du Einkommentieren
  setupTime();                  //NTP
  //ds18b20list();
  //bh1750();
  //dualRelais();
  singleTimerSwitch();          //Scheduler

  ArduinoOTA.onStart([]() {
    //save();                   // Wenn Werte vor dem Neustart gespeichert werden sollen
  });
  ArduinoOTA.begin();
  server.begin();
  
  StartUDP();
  mqttClient.setServer(mqtt_server, mqtt_server_port);
  mqttClient.setCallback(callback);
  if (!mqttClient.connected()) {
    connect();
  }
  mqttClient.subscribe(mqttTopicIn);
}

void loop() {
  ArduinoOTA.handle();
  server.handleClient();
  process_incoming_udp();
  timerSwitch();            //Scheduler (aktualisiert auch die Uhrzeit)
  
  if ( millis() - Timer1 >= (1000L*4)) // 4 Sekunden )
  {
  	if((cmdSource & 1) && (cmdSource & (1 << 4)))     //Timer aktiv und Website aufgerufen
  	{
      iLastSource = 1;
      SendRequest();		      //UART Klimakommunikation
    }
    Timer1 = millis();
  }
  ReceiveUART();
  
  if (millis() < 0x2FFF || millis() > 0xFFFFF0FF) {    // Die Funktion "runtime()" wird nur für den Admin Tab gebraucht.
    runtime();                                         // Auskommentieren falls du den Admin Tab nicht nutzen möchtest.
  }
  if (!mqttClient.connected()) {
    connect();
  }
  mqttClient.loop(); 
  if ( millis() - Timer2 >= (1000L*20)) // 20 Sekunden )
  {
//    if (!mqttClient.connected()) {
//      connect();
//    }
//    mqttClient.publish( mqttTempOut , "Hallo Welt");
    handleWebsiteUpdate();
    mqttClient.publish( mqttTempOut , String(iTemperatur).c_str());
    mqttClient.publish( mqttStateOut, (char *) Status);
    mqttClient.publish( mqttLuefterOut, (char *) Lufter);
    mqttClient.publish( mqttModusOut, (char *) Modus);
    mqttClient.publish( mqttModus1Out, (char *) Modus1);
    mqttClient.publish( mqttEcoOut, (char *) Eco);
    mqttClient.publish( mqttDispOut, (char *) Display);
    mqttClient.publish( mqttBuzzOut, (char *) Buzzer);
    mqttClient.publish( mqttTempin , String(Tempinin).c_str());
    mqttClient.publish( mqttTempheatin , String(Tempinheat).c_str());
    mqttClient.publish( mqttTempout , String(Tempoutin).c_str());
    mqttClient.publish( mqttTempheatout , String(Tempoutheat).c_str());
    mqttClient.publish( mqttTempoutout , String(Tempoutout).c_str());
    mqttClient.publish( mqttTempOut , String(iTemperatur).c_str());
    Timer2 = millis(); 
  }

}



