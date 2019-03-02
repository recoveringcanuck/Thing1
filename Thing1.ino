#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <FS.h>
//#include <ESP8266WiFiMulti.h>
#include <ESP8266HTTPClient.h>
#include <stdio.h>
#include <ArduinoJson.h>
#include "DHT.h"
#include <Hash.h>
#define DHTPIN D6
#define DHTTYPE DHT11

/* Set these to your desired credentials. */
const char *APSsid = "IOTSetup";
const char *kUUID = "RelayBox1";
const char *secret = "8675309";

String *uuid; 
String ssid;
String password;
bool configureMode = false;
//ESP8266WiFiMulti WiFiMulti;
int green = 512;
int red = 512;
int blue = 512;
String payload;
float change = 0;
int errCnt = 0;





DHT dht(DHTPIN, DHTTYPE);

ESP8266WebServer server(80);

void logInt(int me, String var)
{
  if ((WiFi.status() == WL_CONNECTED)) {

    HTTPClient http;

    static String logIntURL = "http://foc-electronics.com/iot/logIntSecure.php?";
    String argStr = String("uuid=") + *uuid + String("&value=") + String(me) + String("&var=") + var;
    
    
    String myURL = logIntURL + argStr + "&check=" + sha1(argStr + secret);
    Serial.println(myURL);
    http.begin(myURL.c_str()); //HTTP

      Serial.print("[HTTP] GET...\n");
    // start connection and send HTTP header
    Serial.print("beginning get.  Took: ");
    int start = millis();
    int httpCode = http.GET();
    Serial.println(millis()-start);
    // httpCode will be negative on error
    if (httpCode > 0) {
      // file found at server
      if (httpCode == HTTP_CODE_OK) {
        payload = http.getString();
        Serial.println(payload);
      }

    } else {
      Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
    //  WiFi.reconnect(); //getting errors where esp8266 still thinks it's connected, but all get requests are being rejected
                            //(probably not getting through) resetting the module fixes it so try just reconnecting first 
      errCnt++;
    }
    
  }
  else
  {
    Serial.println("can't seem to connect to wifi");
   
  }
  }

void handleConfig()
{
  if (server.hasArg("ssid") == true && server.hasArg("password") == true)
  {
    ssid  = server.arg("ssid");
    password = server.arg("password");
    Serial.println(server.arg("ssid"));
    Serial.println(server.arg("password"));

    File passwordFile = SPIFFS.open("/password.txt", "w");
    File ssidFile = SPIFFS.open("/ssid.txt", "w");
    passwordFile.print(password.c_str());
    passwordFile.print("\n");
    ssidFile.print(ssid.c_str());
    ssidFile.print("\n");
    ssidFile.close();
    passwordFile.close();
    server.send(200, "text/html", "SSID and Password set, device will reset in several seconds.");
    delay(1000);
    while (1 == 1);
  }
  else
  {
    server.send(200, "text/html", "SSID or Password missing, <a href=\"/\">click here</a> to try again.");

  }
}
void handleRoot() {
  //server.send(200, "text/html", "<h1>You are connected</h1>");
  if (SPIFFS.exists("/setup.html")) {
    File file = SPIFFS.open("/setup.html", "r");                 // Open it
    size_t sent = server.streamFile(file, "text/html"); // And send it to the client
    file.close();
  }
  else
  {
    Serial.println("file not found");
  }




}
void setupAP()
{
  WiFi.mode(WIFI_AP);
  Serial.println();
  Serial.print("Configuring access point...");
  /* You can remove the password parameter if you want the AP to be open. */
  WiFi.softAP(APSsid);

  IPAddress myIP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(myIP);
  server.on("/", handleRoot);
  server.on("/config", handleConfig);
  server.begin();
  Serial.println("HTTP server started");
  SPIFFS.begin();
  configureMode = true;
}
void setupClient()
{
  WiFi.mode(WIFI_STA);
  Serial.println("Entering client mode");
  SPIFFS.begin();
  File passwordFile = SPIFFS.open("/password.txt", "r");
  File ssidFile = SPIFFS.open("/ssid.txt", "r");
  password = passwordFile.readStringUntil('\n');
  ssid = ssidFile.readStringUntil('\n');
  passwordFile.close();
  ssidFile.close();
  Serial.print("ssid: ");
  Serial.println(ssid.c_str());
  Serial.print("password: ");
  Serial.println(password.c_str());

  WiFi.begin(ssid.c_str(), password.c_str() );

   while (WiFi.status() != WL_CONNECTED) {
     delay(500);
     Serial.print(".");
    }

    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());

  
  Serial.print("UUID is: ");
  Serial.println(kUUID);
  
}

void setup() {

  uuid = new String(kUUID);


  
  pinMode(D0, OUTPUT);
  pinMode(D1, OUTPUT);
  pinMode(D2, OUTPUT);
  pinMode(D3, OUTPUT);
  pinMode(D4, OUTPUT);
  pinMode(D5, OUTPUT);
  pinMode(D6, OUTPUT);
  pinMode(D7, OUTPUT);
  pinMode(D8, OUTPUT);

  
  digitalWrite(D0, HIGH);

  delay(1000);
  Serial.begin(115200);
  if (!digitalRead(D0))
  {
    setupAP();
  }
  else
  {
    setupClient();
  }
  dht.begin();
}


void loop() {
  if (configureMode)
    server.handleClient();
  else
  {
    clientLoop();
  }
}
void reportInputs()
{
  
  }
void updateOutputs()
{
  String updateURL  = String("http://foc-electronics.com/iot/readState.php?uuid=")+*uuid+String("&check=")+sha1(*uuid+secret);

 if ((WiFi.status() == WL_CONNECTED)) {

    HTTPClient http;

    //Serial.print("[HTTP] begin...\n");
    // configure traged server and url
    //http.begin("https://foc-electronics.com:443/webservices/ethchange.php"); //HTTPS
    http.begin(updateURL.c_str()); //HTTP

    //  Serial.print("[HTTP] GET...\n");
    // start connection and send HTTP header
    int httpCode = http.GET();

    // httpCode will be negative on error
    if (httpCode > 0) {
      // HTTP header has been send and Server response header has been handled
      //Serial.printf("[HTTP] GET... code: %d\n", httpCode);

      // file found at server
      if (httpCode == HTTP_CODE_OK) {
        payload = http.getString();
        Serial.println(payload);
      }

    } else {
      Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
    }
  
  const size_t capacity = JSON_OBJECT_SIZE(10) + 100;
  DynamicJsonBuffer jsonBuffer(capacity);
  const char* json = payload.c_str();

  JsonObject& root = jsonBuffer.parseObject(json);


  int d0 = root["D0"]; // 1
   int d1 = root["D1"]; // 1
    int d2 = root["D2"]; // 1
     int d3 = root["D3"]; // 1
      int d4 = root["D4"]; // 1
       int d5 = root["D5"]; // 1
        int d6 = root["D6"]; // 1
   int d7 = root["D7"]; // 1
    int d8 = root["D8"]; // 1

    Serial.println(d0);
    Serial.println(d1);
    Serial.println(d2);
    Serial.println(d3);
    Serial.println(d4);
    Serial.println(d5);
    Serial.println(d6);
    Serial.println(d7);
    Serial.println(d8);

    digitalWrite(D0, d0);
    digitalWrite(D1, d1);
    digitalWrite(D2, d2);
    digitalWrite(D3, d3);
    digitalWrite(D4, d4);
    digitalWrite(D5, d5);
    digitalWrite(D6, d6);
    digitalWrite(D7, d7);
    digitalWrite(D8, d8);

    delay(1000);

 }
 else
  {
    Serial.println("can't seem to connect to wifi");
    errCnt++;
    delay(1000);
  }
 
 
  }
void clientLoop()
{
  updateOutputs();
   if(errCnt >9)
    WiFi.reconnect();
}
