//--------------------------------------
// config (edit here before compiling)
//--------------------------------------
//#define MQTT_TLS // uncomment this define to enable TLS transport
//#define MQTT_TLS_VERIFY // uncomment this define to enable broker certificate verification
//--------------------------------------
// function callback called everytime 
// if a mqtt message arrives from the broker
//--------------------------------------

void callback(char* topic, byte* payload, unsigned int length) {
  //Serial.print("Message arrived on topic: '");
  //Serial.print(topic);
  //Serial.print("' with payload: ");
#define MSG_BUFFER_SIZE	(50)
char msg[MSG_BUFFER_SIZE];
char delimiter[] = ",;";
char* ptr;
char msg1[MSG_BUFFER_SIZE];
  //mqttClient.publish( mqttAntOut, (char *) payload);
  

  iTemperaturMqtt = 21;   //Temperatur
  iModusMqtt = 131;              //Modus (Heizen,Kühlen etc.) Byte 7 und 8 ist noch nicht berücksichtigt
  iAirMqtt = 4;               //Lüfterdrehzahl
  iSettingMqtt = 132;        //Eco, Display, Buzzer + EIN
  iStart = 10;

  for (int i = 0; i < length; i++) {
    //Serial.print((char)payload[i]);
    msg1[i] = ((char)payload[i]);
  }
  mqttClient.publish( mqttAntOut, (char *) msg1);
  ptr = strtok(msg1, delimiter);
  i1 = 0;
  while(ptr != NULL) {
	  uint8_t zahl = (uint8_t) atoi(ptr);;
    if (i1 == 0) iStart = zahl;
    else if (i1 == 1) iTemperaturMqtt = zahl;
    else if (i1 == 2) iModusMqtt = zahl;
    else if (i1 == 3) iAirMqtt = zahl;
    else if (i1 == 4) iSettingMqtt = zahl;    
    // naechsten Abschnitt erstellen
 	  ptr = strtok(NULL, delimiter);
    i1 = i1 + 1;
  }
  
  mqttClient.publish( mqttAnt1Out, String(iStart).c_str());

  if (iStart == 0) {
    SendOffMqtt();
  }
  if (iStart == 1) {
    SendOnMqtt();
  } 
//  mqttClient.publish(mqttTopicOut,("ESP8266: Cedalo Mosquitto is awesome. ESP8266-Time: " ));
}

//--------------------------------------
// function connect called to (re)connect
// to the broker
//--------------------------------------
void connect() {
  while (!mqttClient.connected()) {
    //Serial.print("Attempting MQTT connection...");
    String mqttClientId = "Klimaanlage_1";
    if (mqttClient.connect(mqttClientId.c_str(), mqttUser, mqttPassword)) {
      //Serial.println("connected");
      mqttClient.subscribe(mqttTopicIn);
    } //else {
      //Serial.print("failed, rc=");
      //Serial.print(mqttClient.state());
      //Serial.println(" will try again in 5 seconds");
      //delay(5000);
    //}
  }
}



