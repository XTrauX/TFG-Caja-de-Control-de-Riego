
/**
 * @file parametros.cpp
 * @brief This file contains functions for handling configuration parameters for the ControlRiego system.
 * 
 * The functions in this file are responsible for loading, saving, copying, and zeroing configuration parameters.
 * Additionally, it includes utility functions for cleaning the file system, printing configuration parameters, 
 * and displaying file system information.
 * 
 * Functions:
 * - loadConfigFile: Loads configuration parameters from a file.
 * - saveConfigFile: Saves configuration parameters to a file.
 * - copyConfigFile: Copies configuration parameters from one file to another.
 * - zeroConfig: Initializes configuration parameters to default values.
 * - cleanFS: Formats the file system.
 * - printParms: Prints the current configuration parameters.
 * - filesInfo: Displays information about the file system.
 * 
 * Debug Functions (enabled with DEBUG macro):
 * - printFile: Prints the contents of a specified file.
 * - memoryInfo: Displays memory usage information.
 * - printCharArray: Prints a character array in hexadecimal format.
 * 
 * Dependencies:
 * - LittleFS: File system library for ESP8266/ESP32.
 * - ArduinoJson: Library for parsing and generating JSON.
 * 
 * @note Ensure that TRACE and DEBUG macros are defined for additional debug information.
 * 
 * @version 2.5
 * @date 2024
 * 
 * @author Tomas
 */
#include "Control.h"

bool loadConfigFile(const char *p_filename, Config_parm &cfg)
{
  #ifdef TRACE
    Serial.println(F("TRACE: in loadConfigFile"));
  #endif

  if(!LittleFS.begin()){
    Serial.println(F("An Error has occurred while mounting LittleFS"));
    return false;
  }
  File file = LittleFS.open(p_filename, "r");
  if(!file){
    Serial.println(F("Failed to open file for reading"));
    return false;
  }
  size_t size = file.size();
  if (size > 2048) {
    Serial.println(F("Config file size is too large"));
    return false;
  }
  Serial.printf("\t tamaño de %s --> %d bytes \n", p_filename, size);

  DynamicJsonDocument doc(1536);
  DeserializationError error = deserializeJson(doc, file);

  if (error) {
    Serial.print(F("\t  deserializeJson() failed: "));
    Serial.println(error.f_str());
    return false;
  }
  Serial.printf("\t memoria usada por el jsondoc: (%d) \n" , doc.memoryUsage());
  //--------------  procesa botones (IDX)  --------------
  int numzonas = doc["numzonas"] | 0; 
  if (numzonas != cfg.n_Zonas) {
    Serial.println(F("ERROR numero de zonas incorrecto"));
    return false;
  }  
  for (JsonObject botones_item : doc["botones"].as<JsonArray>()) {
    int i = botones_item["zona"] | 1; 
    cfg.botonConfig[i-1].idx = botones_item["idx"] | 0;
    strlcpy(cfg.botonConfig[i-1].desc, botones_item["nombre"] | "", sizeof(cfg.botonConfig[i-1].desc));
    i++;
  }
  //--------------  procesa parametro individuales   --------------
  cfg.minutes = doc["tiempo"]["minutos"] | 0; // 0
  cfg.seconds = doc["tiempo"]["segundos"] | 10; // 10
  strlcpy(cfg.domoticz_ip, doc["domoticz"]["ip"] | "", sizeof(cfg.domoticz_ip));
  strlcpy(cfg.domoticz_port, doc["domoticz"]["port"] | "", sizeof(cfg.domoticz_port));
  strlcpy(cfg.ntpServer, doc["ntpServer"] | "", sizeof(cfg.ntpServer));
  int numgroups = doc["numgroups"] | 1;
  if (numgroups != cfg.n_Grupos) {
    Serial.println(F("ERROR numero de grupos incorrecto"));
    return false;
  }  
  //--------------  procesa grupos  --------------
  for (JsonObject groups_item : doc["grupos"].as<JsonArray>()) {
    int i = groups_item["grupo"] | 1; 
    cfg.groupConfig[i-1].id = GRUPOS[i-1];  
    cfg.groupConfig[i-1].size = groups_item["size"] | 1;
    if (cfg.groupConfig[i-1].size == 0) {
      cfg.groupConfig[i-1].size =1;
      Serial.println(F("ERROR tamaño del grupo incorrecto, es 0 -> ponemos 1"));
    }
    strlcpy(cfg.groupConfig[i-1].desc, groups_item["desc"] | "", sizeof(cfg.groupConfig[i-1].desc)); 
    JsonArray array = groups_item["zonas"].as<JsonArray>();
    int count = array.size();
    if (count != cfg.groupConfig[i-1].size) {
      Serial.println(F("ERROR tamaño del grupo incorrecto"));
      return false;
    }  
    int j = 0;
    for(JsonVariant zonas_item_elemento : array) {
      cfg.groupConfig[i-1].serie[j] = zonas_item_elemento.as<int>();
      j++;
    }
    i++;
  cfg.initialized = 1; 
  }
  file.close();
  LittleFS.end();
  if (cfg.initialized) return true;
  else return false;
}

bool saveConfigFile(const char *p_filename, Config_parm &cfg)
{
  #ifdef TRACE
    Serial.println(F("TRACE: in saveConfigFile"));
  #endif
  if(!LittleFS.begin()){
    Serial.println(F("An Error has occurred while mounting LittleFS"));
    return false;
  }
  LittleFS.remove(p_filename);
  File file = LittleFS.open(p_filename, "w");
  if(!file){
    Serial.println(F("Failed to open file for writing"));
    return false;
  }
  DynamicJsonDocument doc(2048);
  //--------------  procesa botones (IDX)  --------------
  doc["numzonas"] = NUMZONAS;
  doc["botones"].as<JsonArray>();
  JsonArray array_botones = doc.createNestedArray("botones");
  for (int i=0; i<NUMZONAS; i++) {
    array_botones[i]["zona"]   = i+1;
    array_botones[i]["idx"]    = cfg.botonConfig[i].idx;
    array_botones[i]["nombre"] = cfg.botonConfig[i].desc;
  }
  //--------------  procesa parametro individuales   --------------
  doc["tiempo"]["minutos"]  = cfg.minutes; 
  doc["tiempo"]["segundos"] = cfg.seconds;
  doc["domoticz"]["ip"]     = cfg.domoticz_ip;
  doc["domoticz"]["port"]   = cfg.domoticz_port;
  doc["ntpServer"]          = cfg.ntpServer;
  //--------------  procesa grupos  --------------
  doc["numgroups"]          = NUMGRUPOS;
  JsonArray array_grupos = doc.createNestedArray("grupos");
  for (int i=0; i<NUMGRUPOS; i++) {
    array_grupos[i]["grupo"]   = i+1;
    array_grupos[i]["desc"]    = cfg.groupConfig[i].desc;
    array_grupos[i]["size"]    = cfg.groupConfig[i].size;
    JsonArray array_zonas = array_grupos[i].createNestedArray("zonas");
    for(int j=0; j<cfg.groupConfig[i].size; j++) {
      array_zonas[j] = cfg.groupConfig[i].serie[j];
    }  
  }
  #ifdef EXTRADEBUG 
    serializeJsonPretty(doc, Serial); 
  #endif
  int docsize = serializeJson(doc, file);
  if (docsize == 0) Serial.println(F("Failed to write to file"));
  else Serial.printf("\t tamaño del jsondoc: (%d) \n" , docsize);
  Serial.printf("\t memoria usada por el jsondoc: (%d) \n" , doc.memoryUsage());
  file.close();
  LittleFS.end();
  #ifdef DEBUG
    printFile(p_filename);
  #endif
  return true;
}


bool copyConfigFile(const char *fileFrom, const char *fileTo)
{
  #ifdef TRACE
    Serial.println(F("TRACE: in copyConfigFile"));
  #endif
  if(!LittleFS.begin()) {
  Serial.println(F("An Error has occurred while mounting LittleFS"));
  return false;
  }
  LittleFS.remove(fileTo);
  File origen = LittleFS.open(fileFrom, "r");
  if (!origen) {
    Serial.print(F("- failed to open file ")); 
    Serial.println(fileFrom);
    return false;
  }
  else{
    Serial.printf("copiando %s en %s \n", fileFrom, fileTo);
    File destino = LittleFS.open(fileTo, "w+");
    if(!destino){
      Serial.println(F("Failed to open file for writing")); Serial.println(fileTo);
      return false;
    }
    else {
      while(origen.available()){
        destino.print(origen.readStringUntil('\n')+"\n");
      }
      destino.close(); 
    }
    origen.close();  
    LittleFS.end();  
    return true;
  } 
}

void zeroConfig(Config_parm &cfg) {
  #ifdef TRACE
    Serial.println(F("TRACE: in zeroConfig"));
  #endif
  for (int j=0; j<cfg.n_Grupos; j++) {
    cfg.groupConfig[j].id = GRUPOS[j];
    cfg.groupConfig[j].size = 1;
    cfg.groupConfig[j].serie[0]=j+1;
  }  
}

void cleanFS() {
  Serial.println(F("\n\n[cleanFS]Wait. . .Borrando File System!!!"));
  LittleFS.format();
  Serial.println(F("Done!"));
}

void printParms(Config_parm &cfg) {
  Serial.println(F("contenido estructura parametros configuracion: "));
  //--------------  imprime array botones (IDX)  --------------
  Serial.printf("\tnumzonas= %d \n", cfg.n_Zonas);
  Serial.println(F("\tBotones: "));
  for(int i=0; i<7; i++) {
    Serial.printf("\t\t Zona%d: IDX=%d (%s) l=%d \n", i+1, cfg.botonConfig[i].idx, cfg.botonConfig[i].desc, sizeof(cfg.botonConfig[i].desc));
  }
  //--------------  imprime parametro individuales   --------------
  Serial.printf("\tminutes= %d seconds= %d \n", cfg.minutes, cfg.seconds);
  Serial.printf("\tdomoticz_ip= %s domoticz_port= %s \n", cfg.domoticz_ip, cfg.domoticz_port);
  Serial.printf("\tntpServer= %s \n", cfg.ntpServer);
  Serial.printf("\tnumgroups= %d \n", cfg.n_Grupos);
  //--------------  imprime array y subarray de grupos  --------------
  for(int i = 0; i < cfg.n_Grupos; i++) {
    Serial.printf("\tGrupo%d: size=%d (%s)\n", i+1, cfg.groupConfig[i].size, cfg.groupConfig[i].desc);
    for(int j = 0; j < cfg.groupConfig[i].size; j++) {
      Serial.printf("\t\t Zona%d \n", cfg.groupConfig[i].serie[j]);
    }
  }
}

void filesInfo() 
{
  LittleFS.begin();
  FSInfo fs_info;
  LittleFS.info(fs_info);

  float fileTotalKB = (float)fs_info.totalBytes / 1024.0; 
  float fileUsedKB = (float)fs_info.usedBytes / 1024.0; 
  Serial.print("__________________________\n");
  Serial.println(F("File system (LittleFS): "));
  Serial.print(F("    Total KB: ")); Serial.print(fileTotalKB); Serial.println(F(" KB"));
  Serial.print(F("    Used KB: ")); Serial.print(fileUsedKB); Serial.println(F(" KB"));
  Dir dir = LittleFS.openDir("/");
  Serial.println(F("LittleFS directory {/} :"));
  while (dir.next()) {
    Serial.print(F("  ")); Serial.println(dir.fileName());
  }
  Serial.print("__________________________\n");
  LittleFS.end();
}


// Funciones de DEVELOP
#ifdef DEBUG

void printFile(const char *p_filename) {
   #ifdef TRACE
    Serial.printf("TRACE: in printFile (%s) \n" , p_filename);
  #endif
  if(!LittleFS.begin()){
    Serial.println(F("An Error has occurred while mounting LittleFS"));
  return;
  }
  File file = LittleFS.open(p_filename, "r");
  if (!file) {
    Serial.println(F("Failed to open config file"));
    return;
  }
  Serial.println(F("File Content:"));
  while(file.available()){
    Serial.write(file.read());
  }
  Serial.println(F("\n\n"));
  file.close();
  LittleFS.end();
}

void memoryInfo() 
{
  LittleFS.begin();
  FSInfo fs_info;
  LittleFS.info(fs_info);

  float fileTotalKB = (float)fs_info.totalBytes / 1024.0; 
  float fileUsedKB = (float)fs_info.usedBytes / 1024.0; 
  
  int freeHeadSize = (int)ESP.getFreeHeap() / 1024.0;
  float freeSketchSize = (float)ESP.getFreeSketchSpace() / 1024.0;
  Serial.print("\n#####################\n");

  Serial.print("__________________________\n\n");
  
  Serial.println(F("File system (LittleFS): "));
  Serial.print(F("    Total KB: ")); Serial.print(fileTotalKB); Serial.println(F(" KB"));
  Serial.print(F("    Used KB: ")); Serial.print(fileUsedKB); Serial.println(F(" KB"));
  Serial.printf("    Maximum open files: %d\n", fs_info.maxOpenFiles);
  Serial.printf("    Maximum path length: %d\n\n", fs_info.maxPathLength);

  Dir dir = LittleFS.openDir("/");
  Serial.println(F("LittleFS directory {/} :"));
  while (dir.next()) {
    Serial.print(F("  ")); Serial.println(dir.fileName());
  }

  Serial.print("__________________________\n\n");
  
  Serial.printf("free RAM (max Head size): %d KB  <<<<<<<<<<<<<<<<<<<\n\n", freeHeadSize);
  Serial.printf("free SketchSpace: %f KB\n\n", freeSketchSize);
  Serial.println(F("#####################"));
  LittleFS.end();
}

void printCharArray(char *arr, size_t len)
{
    printf("arr: ");
    for (size_t i = 0; i < len; ++i) {
        printf("x%x, ", arr[i]);
    }
    printf("\n");
}

#endif
