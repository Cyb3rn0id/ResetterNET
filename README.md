# ResetterNET
ESP8266 Router/Access Point Resetter if no Internet Connection
Source code is for the NodeMCU Devkit (ESP8266 based) or ESP-01 programmed with Arduino IDE.

Connect a relay, thorugh a mosfet, to GPIO0 pin (D5 on NodeMCU).
Cut a wire from router PSU cable and then connect it to NC contacts of the relay.
Change SSID/Password/IP addresses in the code and then upload the firmware on the NodeMCU or ESP-01.

Firmware will attempt to connect to you WiFi. If in 5 minutes it can't connect, router will be resetted.
Once Wi-Fi is connected, device will attempt to ping an host to check internet connectivity every 10 seconds.
If host does not respond for 5 times, router will be resetted.

If you like this project, please condider to make a little gift (not only money, I collect also a lot of garbage!): http://www.settorezero.com/wordpress/donazioni/
