#ifndef SPIFFS
    #include <FS.h>
#endif // !SPIFFS
#ifndef logger
    #include <Logger.h>
#endif // !logger
#ifndef ArduinoOTA
    #include <ArduinoOTA.h>
#endif // !ArduinoOTA


#include <ArduinoJson.h>


/**
 * Config helper.
 * This structure contains all variables related to the configuration of this firmware
 */
struct Config {
  bool using_congfig_file;
  char version[8] = __VERSION__;
  char ap_ssid[64] = "ESP WiFi";
  String device_name = "ESP thing";
  char ap_ip[20] = "10.0.1.1";
  char ap_gateway[20] = "10.0.1.1";
  char ap_mask[20] = "255.255.255.0";
  const char compilation_date[30] = __DATE__ " + " __TIME__;

  String gScript_id = "";
  String but_1_tag = "no_tag";
  String but_2_tag = "no_tag";
};

Config config;                         //Create global configuration object


/**
 * Prints the configuration file read from memory
 * @param filename Name of the configuration file including the .json extension
 **/
void printConfigFile(const char *filename) {
  // Open file for reading
  File file = SPIFFS.open(filename, "r");
  if (!file) {
    logger(F("Failed to read file"));
    return;
  }

  // Extract each characters by one by one
  while (file.available()) logger(file.readString());
  file.close();
}

/**
 * Loads given configuration file into the appropriate config object
 * @param filename Name of the configuration file, including .json extension
 * @param &config Address pointer to the config object
 **/
void loadConfiguration(const char *filename, Config &config) {
    File file = SPIFFS.open(filename, "r");
    StaticJsonDocument<512> doc;

    // Deserialize the JSON document
    DeserializationError error = deserializeJson(doc, file);
    if (error) logger(F("Failed to read config file, using default configuration"));

    // Copy values from the JsonDocument to the Config
    strlcpy(config.version,           // <- destination
            doc["version"] | "0.0",   // <- source
            sizeof(config.version));  // <- destination's capacity
    config.device_name = doc["device_name"] | "No Name (Default)";
    strlcpy(config.ap_ssid, doc["ap_ssid"] | "ESP Thing (Default)", sizeof(config.ap_ssid));  
    strlcpy(config.ap_ip, doc["ap_ip"] | "10.0.1.1", sizeof(config.ap_ip));  
    strlcpy(config.ap_gateway, doc["ap_gateway"] | "10.0.1.1", sizeof(config.ap_gateway));  
    strlcpy(config.ap_mask, doc["ap_path"] | "255.255.255.0", sizeof(config.ap_mask));  

    config.gScript_id = doc["gscript_ID"] | "";
    config.but_1_tag = doc["button_1_tag"] | "B1";
    config.but_2_tag = doc["button_2_tag"] | "B2";
}

void OTAsetup(){
    char deviceNameChar[30];
    config.device_name.toCharArray(deviceNameChar, sizeof(deviceNameChar));
    ArduinoOTA.setHostname(deviceNameChar);
    logger("  OTA Hostname: " + ArduinoOTA.getHostname());

    ArduinoOTA.onStart([]() {logger("\n*************************\n****** OTA STARTED ******");});
    ArduinoOTA.onEnd([]() {logger("\n*************************\n****** OTA FINISHED ******");});
    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {Serial.printf("*** OTA Progress: %u%%\r", (progress / (total / 100)));});
    ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("OTA Error[%u]: \n", error);
        if (error == OTA_AUTH_ERROR) logger("OTA Error: Auth Failed");
        else if (error == OTA_BEGIN_ERROR) logger("OTA Error: Begin Failed");
        else if (error == OTA_CONNECT_ERROR) logger("OTA Error: Connect Failed");
        else if (error == OTA_RECEIVE_ERROR) logger("OTA Error: Receive Failed");
        else if (error == OTA_END_ERROR) logger("OTA Error: End Failed");
    });

    ArduinoOTA.begin();
}