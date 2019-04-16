
/*Thing1 is the first prototype device designed to work with the create-a-thing.com platform.
 * it contains a relay, an RGB led, two buttons, and a humidity/temperature sensor
 */

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

//base URL of webservice
String baseURL = "http://foc-electronics.com/iotv2/";

/* Set these to your desired credentials. */
const char *APSsid = "IOTSetup";
const char *kUUID = "relayBox1"; //change this to be configurable
const char *secret = "8675309"; //change this to be configurable

String *uuid; 
String ssid;
String password;

String payload;
float change = 0;
int errCnt = 0;
int pinNumbers[] = {D0, D1, D2, D3, D4, D5, D6, D7, D8};
long pinCmds[9];
long pinStates[9];
int pinModes[9];
bool modeSet = false;


struct httpResponse
{
  int code;
  String payload;
  };

ESP8266WebServer server(80);


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
//  WiFi.mode(WIFI_AP);
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
}
void setupClient()
{
//  WiFi.mode(WIFI_STA);
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

  /* while (WiFi.status() != WL_CONNECTED) {
     delay(500);
     Serial.print(".");
    }*/

    


  
  Serial.print("UUID is: ");
  Serial.println(kUUID);
  
}

void setup() {

   //setup all the IOs (power up with pullups off, everything set to input)
   for(int i = 0; i < 9; i++)
   {
    digitalWrite(pinNumbers[i], LOW);
    pinMode(pinNumbers[i], INPUT);
    }

  uuid = new String(kUUID);

  
  WiFi.mode(WIFI_AP_STA);

  delay(1000);
  Serial.begin(115200);

 
  
  
  setupAP();
  
  setupClient();

  runConfigureMode();
}

void runConfigureMode()
{
  long configTime = 1000*60*5;
  long startTime = millis();
  bool doneConnection = false;
  while(millis() < configTime || modeSet == false)
  {
    server.handleClient();
    
    if(!modeSet)
    {
      modeSet = fetchMode();
      }
      else
      clientLoop();
      if(WiFi.status() == WL_CONNECTED && doneConnection == false)
      {
        Serial.println("WiFi connected"); 
        Serial.println("IP address: ");
        Serial.println(WiFi.localIP());
        doneConnection = true;
        }
    } 
  Serial.println("shutting down access point");
  WiFi.mode(WIFI_STA);
  }
  
void loop() {
    clientLoop();
    if(errCnt >9)
    {
    WiFi.reconnect();
    errCnt = 0;
    }
}

httpResponse getReq(String myURL)
{
    HTTPClient http;
    httpResponse retVal;
    
    Serial.print("[HTTP] begin...\n");
    long start = millis();
    http.begin(myURL.c_str()); //HTTP

    //  Serial.print("[HTTP] GET...\n");
    // start connection and send HTTP header
    retVal.code = http.GET();

    Serial.println(millis()-start);
    
    if (retVal.code > 0) {
      // HTTP header has been send and Server response header has been handled
      Serial.printf("[HTTP] GET... code: %d\n", retVal.code);

      // file found at server
      if (retVal.code == HTTP_CODE_OK) {
        retVal.payload = http.getString();
        Serial.println(retVal.payload);
        errCnt = 0;
      }
    } else {
      Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(retVal.code).c_str());
      errCnt++;
    }
    
    
    http.end();
    return retVal;
  }

httpResponse postReq(String myURL, String post)
{
    HTTPClient http;
    httpResponse retVal;
    
    Serial.print("[HTTP] begin...\n");
    
    http.begin(myURL.c_str()); //HTTP
    long start = millis();
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");
    //  Serial.print("[HTTP] GET...\n");
    // start connection and send HTTP header
    
    retVal.code = http.POST(post.c_str());
    Serial.println(millis()-start);
    
    if (retVal.code > 0) {
      // HTTP header has been send and Server response header has been handled
      Serial.printf("[HTTP] POST... code: %d\n", retVal.code);

      // file found at server
      if (retVal.code == HTTP_CODE_OK) {
        retVal.payload = http.getString();
        Serial.println(retVal.payload);
        errCnt = 0;
      }
    } else {
      Serial.printf("[HTTP] POST... failed, error: %s\n", http.errorToString(retVal.code).c_str());
      errCnt++;
    }
    
    
    http.end();
    return retVal;
  }
  


bool fetchMode()
{
  const size_t capacity = JSON_OBJECT_SIZE(10) + 70;
  DynamicJsonBuffer jsonBuffer(capacity);

  httpResponse me = getReq(baseURL+"getMode.php?dev="+kUUID);
  
  if(me.code == HTTP_CODE_OK)
  {
  

  JsonObject& root = jsonBuffer.parseObject(me.payload);

  const char* ID = root["ID"]; // "testBox1"
  pinModes[0] = root["D0"]; // 0
  pinModes[1] = root["D1"]; // 0
  pinModes[2] = root["D2"]; // 0
  pinModes[3] = root["D3"]; // 0
  pinModes[4] = root["D4"]; // 0
  pinModes[5] = root["D5"]; // 0
  pinModes[6] = root["D6"]; // 0
  pinModes[7] = root["D7"]; // 0
  pinModes[8] = root["D8"]; // 0

  updateModes();
  return true;
  }
  else
  {
  return false;
 }
}

void updateModes()
{
  for(int i = 0; i<9; i++)
  {
    switch(pinModes[i])
    {
      case 0: 
        pinMode(pinNumbers[i], INPUT);
        break;
      case 1:
        pinMode(pinNumbers[i], OUTPUT);
        break;
      case 2:
        pinMode(pinNumbers[i], OUTPUT);
        break;
      }
    }
  
  }
bool fetchCmd()
{
  const size_t capacity = JSON_OBJECT_SIZE(10) + 70;
  DynamicJsonBuffer jsonBuffer(capacity);

  httpResponse me = postReq(baseURL+"cmdList.php", String("dev=")+kUUID+"&json="+stateJSON()+"&checksum="+sha1(stateJSON()+secret));
  Serial.print("sending checksum: ");
  Serial.println(sha1(stateJSON()+secret));
  if(me.code == HTTP_CODE_OK)
  {
    int cmds = linesInString(me.payload);
    Serial.print("received lines: ");
    Serial.println(cmds-1);
    for(int i = 1; i <= cmds-1; i++)
    {
    processCmd(getLine(me.payload, i));
    }
  return true;
  }
  else
  {
  return false;
 }
}

String stateJSON()
{
  String retVal;
const size_t capacity = JSON_OBJECT_SIZE(10);
DynamicJsonBuffer jsonBuffer(capacity);

JsonObject& root = jsonBuffer.createObject();
root["ID"] = kUUID;
root["D0"] = pinStates[0];
root["D1"] = pinStates[1];
root["D2"] = pinStates[2];
root["D3"] = pinStates[3];
root["D4"] = pinStates[4];
root["D5"] = pinStates[5];
root["D6"] = pinStates[6];
root["D7"] = pinStates[7];
root["D8"] = pinStates[8];

root.printTo(retVal);
  return retVal;
  }

void initModes()
{
  for(int i = 0; i < 9; i++)
  {
    switch(pinModes[i])
    {
      case 0:
        pinMode(pinNumbers[i], 0);
        break;
        
      case 1:
        pinMode(pinNumbers[i], INPUT);
        digitalWrite(pinNumbers[i], LOW);
        break;
      case 2:
        
      
      default:
        pinMode(pinNumbers[i], 0);
      }
    }
  }
void processCmd(String cmd)
{
  Serial.println(cmd);
  const size_t capacity = JSON_OBJECT_SIZE(10) + 70;
  DynamicJsonBuffer jsonBuffer(capacity);

  JsonObject& root = jsonBuffer.parseObject(cmd);

 
  int cmdID = root["cmdID"]; // 1
  switch(cmdID)
  {
    case 1:
    Serial.print("writing pin, val ");
    Serial.print((int)root["pin"]);
    Serial.print(",");
    Serial.println((int)root["payload"]);
    int pinNo = root["pin"];
    digitalWrite(pinNumbers[pinNo], root["payload"]);
    break;
    }
  

  }

void clientLoop()
{
  updateStates();
fetchCmd();
  delay(1000); 
}
void updateStates()
{
  for(int i = 0; i <9; i++)
  {
    switch(pinModes[i])
    {
      case 0:
        pinStates[i] = digitalRead(pinNumbers[i]);
        break;
      case 1: 
        pinStates[i] = digitalRead(pinNumbers[i]);
        break;
      case 2:
        pinStates[i] = digitalRead(pinCmds[i]);
        break;
      }
    }
  }
String getLine(String data, int index)
{
  int found = 0;
  int strIndex[] = {0, -1};
  int maxIndex = data.length()-1;
  char separator = '\n';
  for(int i=0; i<=maxIndex && found<=index; i++){
    if(data.charAt(i)==separator || i==maxIndex){
        found++;
        strIndex[0] = strIndex[1]+1;
        strIndex[1] = (i == maxIndex) ? i+1 : i;
    }
  }

  return found>index ? data.substring(strIndex[0], strIndex[1]) : "";
}
int linesInString(String data)
{
  
   int found = 0;
  int maxIndex = data.length()-1;
  char separator = '\n';
  for(int i=0; i<=maxIndex; i++){
    if(data.charAt(i)==separator){
        found++;
    }
  }
  return found;
  
  }
