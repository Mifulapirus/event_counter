/*************************************************************************************************************
 * <PROJECT NAME>
 * Description: 
 * Author: Angel Hernandez
 * Contact: angel@gaubit.com
 * 
 * Description: 
 *  
 *
 **************************************************************************************************************/
#include <Arduino.h>
#include <WiFiManager.h>         //https://github.com/tzapu/WiFiManager
#include <Logger.h>
#include <ArduinoJson.h>
#include <FS.h>
#include <HTTPSRedirect.h>
#include <ESPAsyncWebServer.h>

//OTA related libraries
#include <ESP8266mDNS.h>
#include <ArduinoOTA.h>
#include <Config.h>

#define BUTTON_1_PIN  D1
#define BUTTON_2_PIN  D5
#define BLUE_LED      D4

unsigned long LED_TIMER = 5000;
unsigned long lastLedTrigger = 0;

const char *configPath = "/config.json";  
const char compile_date[] = __DATE__ " " __TIME__;

// The ID below comes from Google Sheets.
// Towards the bottom of this page, it will explain how this can be obtained
const char* host = "script.google.com";
const char* googleRedirHost = "script.googleusercontent.com";
const int httpsPort =     443;
HTTPSRedirect client(httpsPort);

String url = String("/macros/s/") + config.gScript_id + "/exec?"; //default url
const char* fingerprint = "F0 5C 74 77 3F 6B 25 D7 3B 66 4D 43 2F 7E BC 5B E9 28 86 AD";

//Web Server 
AsyncWebServer server(80);

/** Rebuilds URL with the gScript ID from the config file */ 
void setUrl(){
  logger("Rebuild G Script ID");
  if (config.gScript_id == "") {
    logger(" ERROR, config does not contain any gscript_ID");
    return;
  }
  logger(" G Script ID: " + config.gScript_id);
  url = String("/macros/s/") + config.gScript_id + "/exec?";
  logger(" URL: " + url);
}

/** Save new device name in memory */
void saveGscriptID(String newGscriptID){
  config.gScript_id = newGscriptID;
  logger("Saving G Script ID to memory " + config.gScript_id);

  //Read current config file
  File file = SPIFFS.open(configPath, "r");
  StaticJsonDocument<512> doc;
  // Deserialize the JSON document
  DeserializationError error = deserializeJson(doc, file);
  if (error) logger(F(" Failed to read config file, using default configuration"));
  file.close();

  doc["gscript_ID"] = config.gScript_id;
  //Save document to file
  if (!initFS()) {logger("  FS not mounted properly"); return;}
  file = SPIFFS.open(configPath, "w");
  if (!file) {logger("  Error, Can't create file"); return;}

  serializeJson(doc, file);
  logger("  Config file saved correctly with name " + String(file.name()));
  file.close();
}

/** Save new device name in memory */
void saveDeviceName(){
  logger("Setting new Device Name");
  if (config.device_name == "") {
    logger(" ERROR, config does not contain any device Name");
    return;
  }
  
  logger(" Saving Device Name to memory " + config.device_name);
  
  //Read current config file
  File file = SPIFFS.open(configPath, "r");
  StaticJsonDocument<512> doc;
  // Deserialize the JSON document
  DeserializationError error = deserializeJson(doc, file);
  if (error) logger(F(" Failed to read config file, using default configuration"));
  file.close();

  doc["device_name"] = config.device_name;
  //Save document to file
  if (!initFS()) {logger("  FS not mounted properly"); return;}
  file = SPIFFS.open(configPath, "w");
  if (!file) {logger("  Error, Can't create file"); return;}

  serializeJson(doc, file);
  logger("  Config file saved correctly with name " + String(file.name()));
}

bool saveButtonName(int buttonID, String buttonName) {
   logger("Saving button " + String(buttonID) + " as " + buttonName);
    if (buttonID == 1) config.but_1_tag = buttonName;
    else if (buttonID == 2) config.but_2_tag = buttonName;
    else {logger("Wrong Button ID"); return 0;}
    
    //Read current config file
    File file = SPIFFS.open(configPath, "r");
    StaticJsonDocument<512> doc;

    // Deserialize the JSON document
    DeserializationError error = deserializeJson(doc, file);
    if (error) logger(F("Failed to read config file, using default configuration"));

    if (buttonID == 1) doc["button_1_tag"] = buttonName;
    else if (buttonID == 2) doc["button_2_tag"] = buttonName;
    else {logger("Wont save config file. Wrong Button ID"); return 0;}
    file.close();

    //Save document to file
    if (!initFS()) {logger("  FS not mounted properly"); return 0;}
    file = SPIFFS.open(configPath, "w");
    if (!file) {logger("  Error, Can't create file"); return 0;}

    serializeJson(doc, file);
    logger("  Config file saved correctly with name " + String(file.name()));
    return 1;
}

/** Replacevariables on html files with whatever the appropriate html text is
 * This function gets called every time a variable shows up while reading an HTML file
 * @param var String encoded between %. i.e. %LIST_OF_STUFF%
 * @return corresponding HTML code
 **/
String webProcessor(const String& var){
  if(var == "FW_VERSION") return config.version;
  if(var == "BUT_1_NAME") return String("\"" + config.but_1_tag + "\"");
  if(var == "BUT_2_NAME") return String("\"" + config.but_2_tag + "\"");
  if(var == "IP") return WiFi.localIP().toString();
  if(var == "DEVICE_NAME") return String(config.device_name);

  if(var == "LED_LIST"){
    String led_list_html = "";
    led_list_html = "Led Number: <input list=\"ledNumbers\" name=\"ledID\" id=\"ledID\" data-lpignore=\"true\">"; 
    led_list_html += "<datalist id=\"ledNumbers\">";
    led_list_html += "</datalist>";
    return led_list_html;
  }
  
  if(var == "G_SCRIPT_STATUS"){
    if (config.gScript_id=="") return "\"Not Set Yet\"";
    else return ("\"" + config.gScript_id + "\"");
  }

  return String();
}

// This is the main method where data gets pushed to the Google sheet
void postData(String deviceID, String tag, int value, float bat){
    digitalWrite(BLUE_LED, LOW);
    if (!client.connected()){
            Serial.println("Connecting to client again…");
            client.connect(host, httpsPort);
    }
    String urlFinal = url + "deviceID=" + deviceID + "&tag=" + tag + "&value=" + String(value) + "&bat=" + String(bat);
    Serial.println(urlFinal);
    if(client.GET(urlFinal, host, googleRedirHost)) digitalWrite(BLUE_LED, HIGH);
}



/**
 * Perform all the required set up before main loop starts 
 **/
void setup() {
  Serial.begin(115200);
  pinMode(BUTTON_1_PIN, INPUT_PULLUP);
  pinMode(BUTTON_2_PIN, INPUT_PULLUP);
  pinMode(BLUE_LED, OUTPUT);

  //Mount File system
  if (initLogger(true)) logger("\nFS mounted at setup");
  else logger("Error mounting FS at setup");
  //Log the beginning of the program
  logger("\n\n-----------START----------");
  logger("| Compilation Date: " + String(compile_date));
  logger("| OTA Active");

  //Logger status
  logger("| Initialize Current Log file: " + String(clearCurrentLogFile()));

  //Load config
  logger(F("| Print config file..."));
  printConfigFile(configPath);
  logger(F("| Loading configuration..."));
  loadConfiguration(configPath, config);

  //WiFiManager
  WiFiManager wifiManager;
  
  //reset saved settings
  //wifiManager.resetSettings();
  
  //set custom ip for portal
  IPAddress ip, gw, mask;
  ip.fromString(config.ap_ip);
  gw.fromString(String(config.ap_gateway));
  mask.fromString(String(config.ap_mask));
  
  WiFi.hostname(config.device_name);

  wifiManager.setAPStaticIPConfig(ip, gw, mask);

  //fetches ssid and pass from eeprom and tries to connect
  //if it does not connect it starts an access point with the specified name
  //here  "AutoConnectAP"
  //and goes into a blocking loop awaiting configuration
  wifiManager.autoConnect(config.ap_ssid);

  logger("  " + String(config.device_name) + ": " + String(config.version));
  logger("  Host Name: " + String(WiFi.hostname()));
  logger("  Hardware MAC: " + String(WiFi.macAddress()));
  logger("  Software MAC: " + WiFi.softAPmacAddress());
  logger("  IP: " + WiFi.localIP().toString());  
  logger("  G Script ID: " + config.gScript_id);  
  OTAsetup();  

  logger("\n| Initiate TLS connection");
  setUrl();
  client.setInsecure();
  bool connected = false;
  for (int i=0; i<5; i++){
    int retval = client.connect(host, httpsPort);
    if (retval == 1) {
      connected = true;
      break;
    }
    else Serial.println("Connection failed. Retrying…");
  }

  // Connection Status, 1 = Connected, 0 is not.
  Serial.println("Connection Status: " + String(client.connected()));
  Serial.flush();
  
  if (!connected){
    Serial.print("Could not connect to server: ");
    Serial.println(host);
    Serial.println("Exiting…");
    Serial.flush();
    return;
  }

  // Data will still be pushed even certification don’t match.
  if (client.verify(fingerprint, host)) Serial.println("Certificate match."); 
  else Serial.println("Certificate mis-match");
  for (int i = 0; i < 4; i++) {
    digitalWrite(BLUE_LED, !digitalRead(D4));
    delay(100);
  }

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){request->send(SPIFFS, "/index.html", String(), false, webProcessor);});
  server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest *request){request->send(SPIFFS, "/style.css", "text/css");});
  server.on("/setButton", HTTP_GET, [](AsyncWebServerRequest *request){
    if (request->hasParam("but_1")) {
      String buttonName = request->getParam("but_1")->value();
      logger(" Saving name for button 1: " + buttonName);
      saveButtonName(1, buttonName);
    }
    if (request->hasParam("but_2")) {
      String buttonName = request->getParam("but_2")->value();
      logger(" Saving name for button 2: " + buttonName);
      saveButtonName(2, buttonName);
    }
    request->send(SPIFFS, "/index.html", String(), false, webProcessor);
  });
  server.on("/setGscriptID", HTTP_GET, [](AsyncWebServerRequest *request){
    if (request->hasParam("gscriptID")) {
      saveGscriptID(request->getParam("gscriptID")->value());
      setUrl();
    }
    request->send(SPIFFS, "/index.html", String(), false, webProcessor);
  });
  server.on("/setDeviceName", HTTP_GET, [](AsyncWebServerRequest *request){
    if (request->hasParam("device_name")) {
      config.device_name = request->getParam("device_name")->value();
      logger(" Saving new Device Name: " + config.gScript_id);
      saveDeviceName();
    }
    request->send(SPIFFS, "/index.html", String(), false, webProcessor);
  });
  

  server.begin();

  digitalWrite(BLUE_LED, HIGH);

  logger("-------Setup Finished-------");
}

void loop() {
  ArduinoOTA.handle();
  //check buttons
  if(!digitalRead(BUTTON_1_PIN)){
    logger("Button 1 Has been pushed");
    postData(config.device_name, config.but_1_tag, 1, 1);
    delay(100);
  }
  if(!digitalRead(BUTTON_2_PIN)){
    logger("Button 2 Has been pushed");
    postData(config.device_name, config.but_2_tag, 1, 1);
    delay(100);
  }
  
  if (millis()-lastLedTrigger > LED_TIMER) {
    if (client.connected()) {LED_TIMER = 5000;}
    else {LED_TIMER = 200;}
    digitalWrite(BLUE_LED, LOW);
    delay(10);
    lastLedTrigger = millis();
    digitalWrite(BLUE_LED, HIGH);
  }
}