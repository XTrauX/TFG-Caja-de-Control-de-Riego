/**
 * @file wifiWM.cpp
 * @brief WiFi management and configuration for the ControlRiego system.
 * 
 * This file contains the implementation of WiFi management using the WiFiManager library.
 * It handles the setup of WiFi connections, configuration portal, and various callbacks
 * for handling WiFi events such as entering configuration mode, saving parameters, and
 * OTA updates.
 * 
 * The file includes functions to manage LED indicators for WiFi and AP modes, and to
 * display information on a connected display. It also provides mechanisms to save custom
 * parameters and retry connections if the initial attempt fails.
 * 
 * @note This file is part of the ControlRiego-2.5 project.
 * 
 * @author Tomas
 * @version 2.5
 * @date 2024
 * 
 * @see WiFiManager
 * @see Ticker
 */
#include "Control.h"


Ticker tic_WifiLed;
Ticker tic_APLed;

int timeout = 180;  
WiFiManager wm;

WiFiManagerParameter custom_domoticz_server("domoticz_ip", "Domoticz_ip");
WiFiManagerParameter custom_domoticz_port("domoticz_port", "puerto");
WiFiManagerParameter custom_ntpserver("ntpServer", "NTP_server");

void parpadeoLedWifi(){
  byte estado = ledStatusId(LEDG);
  led(LEDG,!estado);
}

void parpadeoLedAP(){
  byte estado = ledStatusId(LEDB);
  led(LEDB,!estado);
}

void saveWifiCallback() {
  Serial.println(F("[CALLBACK] saveWifiCallback fired"));
    tic_APLed.detach();
    led(LEDB,OFF);
    infoDisplay("----", NOBLINK, BIP, 0);
    tic_WifiLed.attach(0.2, parpadeoLedWifi);
}

void configModeCallback (WiFiManager *myWiFiManager) {
  Serial.println(F("[CALLBACK] configModeCallback fired"));
  tic_WifiLed.detach();
  led(LEDG,OFF);
  tic_APLed.attach(0.5, parpadeoLedAP);
  infoDisplay("-AP-", DEFAULTBLINK, LONGBIP, 1); 
}

void saveParamCallback()
{
  Serial.println(F("[CALLBACK] saveParamCallback fired"));
  Serial.println(F("Should save config"));
  saveConfig = true;
  wm.stopConfigPortal();
}

void preOtaUpdateCallback()
{
  Serial.println(F("[CALLBACK] setPreOtaUpdateCallback fired"));
  infoDisplay("####", DEFAULTBLINK, LONGBIP, 1);
}

void setupRedWM(Config_parm &config)
{
  connected = false;
  falloAP = false;
  saveConfig = false;
  if(initFlags.initWifi) {
    wm.resetSettings(); 
    Serial.println(F("encoderSW pulsado y multirriego en GRUPO3 --> borramos red WIFI"));
    infoDisplay("CLEA", DEFAULTBLINK, LONGBIP, 1); 
  }
  WiFi.mode(WIFI_STA);
  wm.setHostname(HOSTNAME); 
  // Descomentar para resetear configuración
  //wm.resetSettings();
  tic_WifiLed.attach(0.2, parpadeoLedWifi);
  wm.setConfigPortalTimeout(timeout);
  wm.setAPCallback(configModeCallback);
  wm.setSaveConfigCallback(saveWifiCallback);
  wm.setSaveParamsCallback(saveParamCallback);
  wm.setPreOtaUpdateCallback(preOtaUpdateCallback);
  wm.setBreakAfterConfig(true);
  wm.setTitle("Version: " + String(VERSION));
  wm.setParamsPage(true);
  wm.addParameter(&custom_domoticz_server);
  wm.addParameter(&custom_domoticz_port);
  wm.addParameter(&custom_ntpserver);
  custom_domoticz_server.setValue(config.domoticz_ip, 40);
  custom_domoticz_port.setValue(config.domoticz_port, 5);
  custom_ntpserver.setValue(config.ntpServer, 40);
  if(!wm.autoConnect("Ardomo")) {
    Serial.println(F("Fallo en la conexión (timeout)"));
    falloAP = true;
    delay(1000);
  }

  tic_APLed.detach();
  infoDisplay("----", NOBLINK, BIP, 0);
  if (falloAP && wm.getWiFiIsSaved()) {
    Serial.println(F("Hay wifi salvada -> reintentamos la conexion"));
    int j=0;
    falloAP = false;
    tic_WifiLed.attach(0.2, parpadeoLedWifi);
    while(WiFi.status() != WL_CONNECTED) {
      Serial.print(F("."));
      delay(2000);
      j++;
      if(j == MAXCONNECTRETRY) {
        falloAP = true;
        Serial.println(F("Fallo en la reconexión"));
        break;
      }
    }
  }
  NONETWORK ? led(LEDB,ON) : led(LEDB,OFF);
  tic_WifiLed.detach();
  if (checkWifi()) {
    Serial.printf("\nWifi conectado a SSID: %s\n", WiFi.SSID().c_str());
    Serial.print(F(" IP address: "));
    Serial.println(WiFi.localIP());
    Serial.printf(" RSSI: %d dBm  (%d%%)\n\n", WiFi.RSSI(), wm.getRSSIasQuality(WiFi.RSSI()));
  }
  if (saveConfig) {
    strcpy(config.domoticz_ip, custom_domoticz_server.getValue());
    strcpy(config.domoticz_port, custom_domoticz_port.getValue());
    strcpy(config.ntpServer, custom_ntpserver.getValue());
  }
}


void starConfigPortal(Config_parm &config) 
{
  wm.setConfigPortalTimeout(timeout);
  if (!wm.startConfigPortal("Ardomo")) {
    Serial.println(F(" exit or hit timeout"));
  }
  if (saveConfig) {
    strcpy(config.domoticz_ip, custom_domoticz_server.getValue());
    strcpy(config.domoticz_port, custom_domoticz_port.getValue());
    strcpy(config.ntpServer, custom_ntpserver.getValue());
  }
  tic_APLed.detach();
  NONETWORK ? led(LEDB,ON) : led(LEDB,OFF);
  infoDisplay("----", NOBLINK, BIP, 0);
  tic_WifiLed.detach();
  checkWifi();
}

bool checkWifi() {
  #ifdef TRACE
    Serial.println(F("TRACE: in checkWifi"));
  #endif
  if(WiFi.status() == WL_CONNECTED) {
    led(LEDG,ON);
    connected = true;
    return true;
  }
  else {
    Serial.println(F("[ERROR] No estamos conectados a la wifi"));
    led(LEDG,OFF);
    connected = false;
    return false;
  }
}
