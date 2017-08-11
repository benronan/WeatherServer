
#include <Wire.h>
#include <BME280.h>
#include <WifiApServer.h>
#include <OTAService.h>
#include <ESP8266mDNS.h>

//global variables
String BSSID = "esp_weather";
String OTA_PASSWORD = "password";
String Networks[][] = {
};
WifiApServer *server;
TBME280 weather;
bool weatherInitialized = false;

// the setup function runs once when you press reset or power the board
void setup() {
  Serial.begin(115200);  
  pinMode(BUILTIN_LED,OUTPUT);
  
  Serial.println("Connecting to network");
  InitServer();
  Serial.println("Starting OTA Service");
  OTAService.begin(8266,BSSID,OTA_PASSWORD);
  Serial.println("OTA Service running");

  blink(5,100);
  digitalWrite(BUILTIN_LED,LOW);
}

// the loop function runs over and over again until power down or reset
void loop() {
  
  yield();
  server->HandleClient();
  yield();
  OTAService.handle();
  yield();
}

void blink(int _count, int _delay){
  for(int i=0;i<_count;i++) {
    digitalWrite(BUILTIN_LED,HIGH);
    delay(200);
    digitalWrite(BUILTIN_LED,LOW);
    delay(_delay);
  }
}


void OnEvent(System_Event_t*se) {
  server->OnEvent(se);
}

void InitServer() {
  Serial.println("InitServer()");

  server =  new WifiApServer();
  
  //register callback events
  Serial.println("registering callback events");
  wifi_set_event_handler_cb(OnEvent);
  
  server->OnConnect = [](){Serial.println("We are Connected"); };
  server->OnDisconnect =  [](){Serial.println("We have Disconnected"); };
  server->OnClientConnect = [](){Serial.println("Client Connected"); };
  server->OnClientDisconnect = [](){Serial.println("Client Disconnected"); };
  server->OnIpAssigned = [](IPAddress ip){Serial.println("Local IP Address: " + ipToString(ip));};

  server->RegisterPath("/",HandleRequest);

  // connect to network
  Serial.println("connecting to network");
  int i=0;
  int iMax = sizeof Networks / sizeof Networks[0];
  bool connected = false;
  String ssid,psk;
  while(!connected) {
    ssid= Networks[i][0];
    psk= Networks[i][1];
    Serial.println("connecting to \"" + ssid + "\" with psk \"" + psk + "\"");
    connected = server->Connect(ssid,psk,BSSID);
    if(!connected) {
      Serial.println("Failed to connect to " + ssid);
      if(++i >= iMax) {
        Serial.println("Restarting");
        ESP.restart();
      }
    }
  }
  Serial.println("Connected to \"" + ssid + "\" with psk \"" + psk + "\"");

  Serial.println("InitServer() complete");
}

void HandleRequest() {
    if(!weatherInitialized) {    
      Serial.println("Initializing weather");
      Serial.flush();
      /*
      Serial.end();
      pinMode(RX,OUTPUT);
      pinMode(TX,OUTPUT);
      digitalWrite(RX,LOW);
      digitalWrite(TX,HIGH);
      */
      weather.Initialize();
      weatherInitialized = true;
    }
    weather.Read();
    double t = weather.Temperature();
    double p = weather.Pressure();
    double h = weather.Humidity();
    int a = weather.Altitude(1019.0);
    String s = "{\"Units\":\"Imperial\",\"Temperature\":\"" + String(t) + "\",\"Pressure\":\"" + String(p) + "\",\"Humidity\":\"" + String(h) + "\",\"Altitude\":\"" + String(a) + "\"}";
    server->Send(200, "application/json", s);
}
