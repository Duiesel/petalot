#include <ArduinoJson.h>
#include "FS.h"


int Vo = 0;
int Tm, Rd;
uint32_t R1, R2;  // R1, R2 resistors in internal divider
double To, Kp, Ki, Kd, Max;
String msg, status, LocalIP, Gateway, Subnet;
char ssid[64], password[64];
String ifttt_event_name = "petalot_stopped";
String ifttt_api_key = "";

StaticJsonDocument<512> doc;

const char *confFile = "/config.json";


String printConf() {
  String confString;
  serializeJson(doc, confString);
  return confString;

}


void saveConfiguration() {
  SPIFFS.remove("/config.json");
  File file = SPIFFS.open("/config.json", "w");
  if (!file) {
    msg = "Failed to create file";
    return;
  }
  doc["To"] = To;
  doc["Vo"] = Vo;
  doc["Tm"] = Tm;
  doc["Kp"] = Kp;
  doc["Ki"] = Ki;
  doc["Kd"] = Kd;
  doc["Max"] = Max;
  doc["ssid"] = ssid;
  doc["password"] = password;
  doc["LocalIP"] = LocalIP;
  doc["Subnet"] = Subnet;
  doc["Gateway"] = Gateway;
  doc["Rd"] = Rd;
  doc["R1"] = R1;
  doc["R2"] = R2;
  doc["ifttt_event_name"] = ifttt_event_name;
  doc["ifttt_api_key"] = ifttt_api_key;
  if (serializeJson(doc, file) == 0) {
    msg = "Failed to write to file";
  }
  Serial.println(printConf());
  file.close();
  analogWrite(PIN_HEATER, 0);
  ESP.restart();
}


void  resetConfiguration(){
    Serial.println("reset");
    strcpy(ssid, "");
    strcpy(password, "");
    To = 220;
    Vo = 40;
    Tm = 230;
    Kp = 23.0;
    Ki = 0.043;
    Kd = 160.0;
    Max = 200;
    LocalIP = "";
    Subnet = "255.255.255.0";
    Gateway = "";
    Rd = 10000;
    R1 = 220000;
    R2 = 100000;
    ifttt_event_name = "";
    ifttt_api_key = "";
    saveConfiguration();
}


void readConfigurationSerial(){
  StaticJsonDocument<512> docInput;

  if (Serial.available() > 0)
  {

    // Deserialize the JSON document
    DeserializationError error = deserializeJson(docInput, Serial);
    if (error)
    {
      Serial.println(F("deserializeJson() failed: "));
      Serial.println(error.c_str());
      return;
    } else {
      //Serial.println("json ok");
      doc=docInput;
      //serializeJson(doc,Serial);
      File file = SPIFFS.open("/config.json", "w");
      if (!file) {
        msg = "Failed to create file";
        return;
      }
      if (serializeJson(doc, file) == 0) {
        msg = "Failed to write to file";
      }
      file.close();
      Serial.println("Configuration updated, restarting...");
      analogWrite(PIN_HEATER, 0);
      ESP.restart();
    }
  }
}


void loadConfiguration(bool reset=false) {
    File file = SPIFFS.open("/config.json", "r");
     if (!file) {
      msg = "Failed to open /config.json";
      Serial.println("Failed to open /config.json");
      resetConfiguration();
      analogWrite(PIN_HEATER, 0);
      ESP.restart();
    }
    DeserializationError error = deserializeJson(doc, file);
    if (error) {
      msg = "Failed to read file, using default configuration";
      Serial.println("Failed to read file, using default configuration");
      resetConfiguration();
      return;
    }
    file.close();

  strlcpy(ssid,
          doc["ssid"] | "",
          sizeof(ssid));

  strlcpy(password,
          doc["password"] | "",
          sizeof(password));

  To = doc["To"] | 220;
  //To = Tco;
  Vo = doc["Vo"] | 40;
  //Vo = Vco;
  Tm = doc["Tm"] | 230;
  Kp = doc["Kp"]?doc["Kp"].as<double>():23.0;
  Ki = doc["Ki"]?doc["Ki"].as<double>():0.043;
  Kd = doc["Kd"]?doc["Kd"].as<double>():160.0;
  Max = doc["Max"]?doc["Max"].as<double>():200;
  LocalIP = doc["LocalIP"] | "";
  Subnet = doc["Subnet"] | "255.255.255.0";
  Gateway = doc["Gateway"] | "";
  Rd = doc["Rd"] | 5600;
  R1 = doc["R1"] | 220000;
  R2 = doc["R2"] | 100000;
  ifttt_event_name = doc["ifttt_event_name"] | "";
  ifttt_api_key = doc["ifttt_api_key"] | "";

  Serial.println();
  Serial.println("To:Temperature");
  Serial.println("Vo:Speed");
  Serial.println("Tm:Maximum Temperature");
  Serial.println("Kp:Kp");
  Serial.println("Ki:Ki");
  Serial.println("Kd:Kd");
  Serial.println("Rd:Resistor in external thermistor measure chain.");
  Serial.println("R1:Resistor in internal Wemos board divider chain.");
  Serial.println("R2:Resistor in internal Wemos board divider chain, connected to GND.");
  Serial.println("Max:Maximum value for MOSFET (0-255)");
  Serial.println("ssid:SSID");
  Serial.println("password:SSID Password");
  Serial.println("LocalIP:IP address");
  Serial.println("Subnet:Subnet");
  Serial.println("Gateway:Gateway");
  Serial.println("ifttt_event_name:IFTTT Event Name");
  Serial.println("ifttt_api_key:IFTTT API Key");
  Serial.println(printConf());
}


void initConf() {
  if (!SPIFFS.begin()) {
    msg = "Error mounting the file system";
    return;
  }
  loadConfiguration();
}
