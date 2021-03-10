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
//https://script.google.com/macros/s/AKfycbz6JCyGbPDGMm7WRX6xyR1hZJiI9wRnIfUh5r69cYs7asQfy46fzqBjWrKHaHXGapjKiA/exec
const char *GScriptId = "AKfycbz6JCyGbPDGMm7WRX6xyR1hZJiI9wRnIfUh5r69cYs7asQfy46fzqBjWrKHaHXGapjKiA";
const char* host = "script.google.com";
const char* googleRedirHost = "script.googleusercontent.com";
const int httpsPort =     443;
HTTPSRedirect client(httpsPort);

// Prepare the url (without the varying data)
String url = String("/macros/s/") + GScriptId + "/exec?";
const char* fingerprint = "F0 5C 74 77 3F 6B 25 D7 3B 66 4D 43 2F 7E BC 5B E9 28 86 AD";

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
  OTAsetup();  

  logger("| Initiate TLS connection");
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