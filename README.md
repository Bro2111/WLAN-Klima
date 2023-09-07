# WLAN-Klima
ESP8266  control of split air conditioning

Tested devices:
- Bomann CL6045QC
- Hantech Split-Klimaanlagen mit RS232 Abschluss

I have the basic settings and feedback
found out and the existing WLAN module of the air conditioning system
replaced an ESP8285.

The ESP offers the following options for interacting with the
Air conditioner:
- direct control via a website
- Schedulers
- UDP: direct forwarding of the org. Commands (gateways)
- UDP: Control and status query via a separate command set
- MQTT-connection and controll with a home automatisation system (f.e. fhem)

Thus, the system in the local network manually or via a
home automation can be controlled.

Ich habe die grundlegenden Einstellungen und Rückmeldungen 
herausgefunden und das vorhandene WLAN Modul der Klimamanlage durch 
einen ESP8285 ersetzt.

Der ESP bietet folgende Möglichkeiten der Interaktion mit der 
Klimaanlage:
- direkte Steuerung über eine Website
- Scheduler
- UDP: direkte Weiterleitung der org. Befehle (Gateway)
- UDP: Steuerung und Statusabfrage über einen eigenen Befehlssatz
- MQTT-Anbindung und Steuerung über Home-Automation (z.B. fhem)
  
Somit kann die Anlage im lokalen Netz manuell oder über eine 
Hausautomation gesteuert werden.
