/******************************************************************
  - UDP Verbindung 
  - Port ist fest einzustellen
/**************************************************************************************/

// The server accepts connections on this port
#define PORT 5444
WiFiUDP udpServer;

// Buffer for incoming UDP messages
char udp_buffer[WIFICLIENT_MAX_PACKET_SIZE+1];


/** Receive UDP messages and send an echo back */
void process_incoming_udp()
{   
    if (udpServer.parsePacket()) 
    {
      // Fetch received message
      int len=udpServer.read(udp_buffer,sizeof(udp_buffer)-1);
              
      //IP checken
      //IPAddress ipadr = udpServer.remoteIP()
      //if(ipadr.operator==(192 + (168<<8) + (2<<16) + (4<<24)) {};
              
      // Display the message
      // Serial.print(F("Received from "));
      // Serial.print(udpServer.remoteIP());
      // Serial.print(":");
      // Serial.print(udpServer.remotePort());
      // Serial.print(": ");
      // Serial.println(udp_buffer);
      
//      if(cmdSource & 2)
//      {
        if(udp_buffer[0] == 187)    //RAW
        {
          SendCmd(udp_buffer, len);
          iLastSource = 2;
        }
        else if((udp_buffer[0] == 98) && (len == 16))   //Befehl
        {
          ParseUDP(udp_buffer, len);
          iLastSource = 5;                               
        }
        else if((udp_buffer[0] == 97) && (udp_buffer[1] == '?') && (len == 3))  //request
        {
          ParseUDP(udp_buffer, len);
          iLastSource = 5;                               
        }
//      }
    }
}

//-------------------------------------------------------------------------------------
// Send Values back
void SendBackUDP(uint8_t aToSend[], uint8_t len){
        udpServer.beginPacket(udpServer.remoteIP(), udpServer.remotePort());
        udpServer.write(aToSend,len);
        udpServer.endPacket();     
}


/** Runs once at startup */
void StartUDP()
{
    // Start the UDP server
    udpServer.begin(PORT);
}

