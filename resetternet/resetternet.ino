/*
 * ResetterNET 1.0
 * a router/access point resetter
 * by Giovanni Bernardo (CyB3rn0id)
 * http://www.settorezero.com
 * 
 * 
 * some ideas taken from internet alarm by @3zuli
 * found on (https://www.instructables.com/id/ESP8266-Internet-Alarm/)
 */

#include <ESP8266WiFi.h>

// WiFi settings
const char* ssid = "[YOUR-SSID]";
const char* password = "[YOUR-PASSPHRASE]";
// IP address to be assigned to ResetterNET
IPAddress deviceIP(192, 168, 1, 177);
// IP Address of your WiFi Router (Gateway)
// please note that some router has different IP, such as 192.168.0.1
// so if you don't get host connection, probably you've not entered correctly this parameter!
IPAddress gateway(192, 168, 1, 1);
// subnet mask
IPAddress subnet(255, 255, 255, 0);
// host to ping for detecting internet connection
// httpbin.org is a simple HTTP Request & Response Service
const char* host = "httpbin.org";

// wifi led: used to monitor/debug the wifi connection
// OFF: no connection
// ON: connected
// BLINK: trying to connect
// this led on both NodeMCU ad ESP-01 module is already connected to GPIO2 and is active LOW 
// GPIO2 is normally tied High by led+resistor
// GPIO2 is D4 on NodeMCU devkit
#define WIFILED       2 // D4 on NodeMCU 
#define WIFILED_ON  LOW // led turns on with a low level on the pin
#define WIFILED_OFF HIGH

// relay used to turn on/off the router
// is better connect router power to NC contact of the relay, so less power is consumed during normal operations
// if you follow my schematic using the BS170 for driving the relay, when you put LOW the GPIO relay will turn on
// and then, power will be detached from the router/access point.
// relay is connected to GPIO0. This GPIO is used to put in flash mode the ESP8266 at startup, 
// so normally is tied high by 12K resistor
// GPIO0 is D3 on NodeMCU devkit
#define ROUTER_RELAY        0 // D5 on NodeMCU
#define ROUTER_OFF   HIGH // router will turn off with an high level on the GPIO
#define ROUTER_ON    LOW

#define RETRIES_WIFI 600  // WiFi re-connection retries after a no-connection, at 500mS reconnection rate is 5 minutes
#define RETRIES_HOST 4    // Host ping retries after a "no internet connection" is sure

// routine for WiFi connecting
void WiFi_connect(void)
  {
  static uint16_t retries_wifi_count=0; //re-connection retries counter
  static bool wifiLedStatus=true; // used for flashing the led during connection
  digitalWrite(WIFILED,WIFILED_OFF); // turn off the wifi led
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.config(deviceIP, subnet, gateway);
  WiFi.begin(ssid, password);
  WiFi.mode(WIFI_STA);
  
  wl_status_t wifiStatus = WL_IDLE_STATUS;
  wl_status_t lastWifiStatus=wifiStatus;
    
  while (wifiStatus != WL_CONNECTED) 
    {
    wifiStatus = WiFi.status();
    // printing actual status only once until it not changed
    if (wifiStatus!=lastWifiStatus)
      {
      lastWifiStatus=wifiStatus;
      retries_wifi_count=0; // reset retries counter every different status
      Serial.println();
      switch(wifiStatus)
        {
        // Printing the error status, aids to debugging
        case WL_NO_SSID_AVAIL:
          Serial.println("SSID not available");
          break;
        case WL_CONNECT_FAILED:
          Serial.println("Connection failed");
          break;
        case WL_CONNECTION_LOST:
          Serial.println("Connection lost");
          break;
        case WL_DISCONNECTED:
          Serial.println("WiFi disconnected");
          break;
        }  // \switch
       } // \last status != actual status
    wifiLedStatus^=1; // invert led status
    digitalWrite(WIFILED,wifiLedStatus); // wifi led flashing during access
    Serial.print('.');
    retries_wifi_count++;
    if (retries_wifi_count==RETRIES_WIFI)
      {
      // too many retries with same connection status: something is gone wrong
      Serial.println("Too many retries");
      retries_wifi_count=0;
      routerReset();
      }
    delay(500);
    } // \not connected
    
  digitalWrite(WIFILED,WIFILED_ON); // connected: wifi led on
  Serial.println();
  Serial.println("Connected!");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());  
  }

// routine for router reset
void routerReset(void)
  {
  digitalWrite(WIFILED,WIFILED_OFF); // turn off the wifi led
  Serial.println("Resetting the router, please wait");
  digitalWrite(ROUTER_RELAY,ROUTER_OFF);
  delay(5000); // allow router capacitors discharging
  digitalWrite(ROUTER_RELAY,ROUTER_ON);
  // allow WiFi to turn on in 20 seconds, increase this value if your
  // router/access point get more time to set wifi availability
  delay(20000);
  }
  
void setup() 
  {
  Serial.begin(115200);
  pinMode(ROUTER_RELAY,OUTPUT);
  pinMode(WIFILED,OUTPUT);
  delay(5);
  digitalWrite(WIFILED,WIFILED_OFF);
  digitalWrite(ROUTER_RELAY,ROUTER_ON);
  delay(100);
  WiFi_connect();
  }


void loop() 
  {
  static uint8_t retries_host_count=0;
  static uint16_t delay_ping=10000;
  
  // check host response/connection every 10 or 5 seconds
  delay(delay_ping);
  
  wl_status_t wifiStatus = WiFi.status();
  if(wifiStatus != WL_CONNECTED)
    {
    // in case of No WiFi connection I must NOT restart the router but try to connect to Wifi
    // I must restart the router/ap only in the case of no response from internet
    retries_host_count=0;
    delay_ping=10000;
    Serial.println("No WiFi connection");
    WiFi_connect();
    }
  else 
    {
    // wifi connection Ok does not mean that there is internet connection!
    Serial.println("Wifi connection OK!");
    }

  // Now I'll check the internet connection
  Serial.print("Pinging host: ");
  Serial.println(host);
  
  WiFiClient client;
  if (!client.connect(host, 80)) 
      {
      // Maybe no internet connection?
      retries_host_count++;    
      Serial.print("Host does not respond - ");
      Serial.println(retries_host_count);
      delay_ping=5000;
      // too many tries: reset the router
      if (retries_host_count==RETRIES_HOST)
        {
        retries_host_count=0;
        delay_ping=10000;
        Serial.println("No internet connection");
        Serial.println("Resetting router. Please wait");
        routerReset();
        }
      }
  else // connection to host is ok
    {
    Serial.println("Host responds. Internet Ok");
    retries_host_count=0;
    delay_ping=10000;
    // Request URI to host
    String url = "/get";
    client.print(String("GET ") + url + " HTTP/1.1\r\n" + "Host: " + host + "\r\n" + "Connection: close\r\n\r\n");
    delay(10);
    // reading response
    while(client.available())
      {
      String line = client.readStringUntil('\r');
      //Serial.println(line);
      }
    }
}
