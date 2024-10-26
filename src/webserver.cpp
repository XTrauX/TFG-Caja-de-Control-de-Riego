
/**
 * @file webserver.cpp
 * @brief This file contains the implementation of a web server for the ControlRiego project.
 * 
 * The web server provides various functionalities including:
 * - Redirecting to an index or upload page.
 * - Listing files in the filesystem.
 * - Providing system information.
 * - Handling file uploads and deletions.
 * 
 * The web server is built using the ESP8266WebServer library and utilizes the LittleFS filesystem.
 * It also includes an HTTP update server for firmware updates.
 * 
 * @note This file is included only if the WEBSERVER macro is defined.
 * 
 * @version 2.5
 * @date 2024
 * 
 * 
 * 
 * @dependencies
 * - Control.h
 * - builtinfiles.h
 * - ESP8266WebServer library
 * - LittleFS library
 * - ESP8266HTTPUpdateServer library
 * - ESP8266mDNS library
 */
#ifdef WEBSERVER
   #include "Control.h"
   
   #include "builtinfiles.h"

   #define UNUSED __attribute__((unused))

   #define TRACE2(...) Serial.printf(__VA_ARGS__)


   int wsport = 8080;
   const char* update_path = "/$update";
   const char* update_username = "admin";
   const char* update_password = "admin";

   ESP8266WebServer wserver(wsport);
   ESP8266HTTPUpdateServer httpUpdater;
   String TS2Date(time_t t)
   {
   char buff[32];
   sprintf(buff, "%02d-%02d-%02d %02d:%02d:%02d", day(t), month(t), year(t), hour(t), minute(t), second(t));
   return buff;
   }
   void handleRedirect() {
   TRACE2("Redirect...");
   String url = "/index.htm";

   if (!LittleFS.exists(url)) { url = "/$upload.htm"; }

   wserver.sendHeader("Location", url, true);
   wserver.send(302);
   } 
   void handleListFiles() {
   Dir dir = LittleFS.openDir("/");
   String result;

   result += "[\n";
   while (dir.next()) {
      if (result.length() > 4) { result += ","; }
      result += "  {";
      result += " \"name\": \"" + dir.fileName() + "\", ";
      result += " \"size\": " + String(dir.fileSize()) + ", ";
      result += " \"time\": " + String(dir.fileTime());
      result += " }\n";
   }  
   result += "]";
   wserver.sendHeader("Cache-Control", "no-cache");
   wserver.send(200, "text/javascript; charset=utf-8", result);
   }  
   void handleListFiles2() {
   FSInfo fs_info;
   LittleFS.info(fs_info);
   Dir dir = LittleFS.openDir("/");
   String result;

   float fileTotalKB = (float)fs_info.totalBytes / 1024.0; 
   float fileUsedKB = (float)fs_info.usedBytes / 1024.0; 
   result += "__________________________\n";
   result += "File system (LittleFS): \n";
   result += "    Total KB: " + String(fileTotalKB) + " KB \n";
   result += "    Used  KB: " + String(fileUsedKB) + " KB \n";
   result += "    Maximum open files: "  + String(fs_info.maxOpenFiles) + "\n";
   result += "__________________________\n\n";

   result += "LittleFS directory {/} :\n\n";
   result += "\t\t\t\ttamaño \tcreado \t\t\tmodificado \n";
   while (dir.next()) {
      time_t fct = dir.fileCreationTime();
      time_t fwt = dir.fileTime();
      result += "\t";
      result += dir.fileName() ;
      result += "\t" + String(dir.fileSize());
      result += "\t" + TS2Date(fct);
      result += "\t" + TS2Date(fwt);
      result += " \n";
   } 
   wserver.sendHeader("Cache-Control", "no-cache");
   wserver.send(200, "text/plain; charset=utf-8", result);
   }  


   void handleSysInfo() {
   FSInfo fs_info;
   LittleFS.info(fs_info);
   String result;

   float fileTotalKB = (float)fs_info.totalBytes / 1024.0; 
   float fileUsedKB = (float)fs_info.usedBytes / 1024.0; 

   result += "\n\n CONTROL RIEGO V" + String(VERSION) + "    Built on " __DATE__ " at " __TIME__ " \n";
   result += "__________________________\n\n";
   result += "SysInfo :\n";
   result += "\t flashSize : \t\t" + String(ESP.getFlashChipSize()) + "\n";
   result += "\t usedSketchSpace : \t" + String(ESP.getSketchSize()) + "\n";
   result += "\t freeSketchSpace : \t" + String(ESP.getFreeSketchSpace()) + "\n";
   result += "\t freeHeap : \t\t" + String(ESP.getFreeHeap()) + "\n";
   result += "\t HeapFragmentation : \t" + String(ESP.getHeapFragmentation()) + "\n";
   result += "\t MaxFreeBlockSize : \t" + String(ESP.getMaxFreeBlockSize()) + "\n";
   result += "__________________________\n\n";
   result += "File system (LittleFS): \n";
   result += "\t    Total KB: " + String(fileTotalKB) + " KB \n";
   result += "\t    Used  KB: " + String(fileUsedKB) + " KB \n";
   result += "\t    Maximum open files: "  + String(fs_info.maxOpenFiles) + "\n";
   result += "__________________________\n\n";
   result += "\n";
   wserver.sendHeader("Cache-Control", "no-cache");
   wserver.send(200, "text/plain; charset=utf-8", result);
   }  



   class FileServerHandler : public RequestHandler {
   public:
   FileServerHandler() {
      TRACE2("FileServerHandler is registered\n");
   }


   bool canHandle(HTTPMethod requestMethod, const String UNUSED &_uri) override {
      return ((requestMethod == HTTP_POST) || (requestMethod == HTTP_DELETE));
   } 

   bool canUpload(const String &uri) override {
      return (uri == "/");
   } 

   bool handle(ESP8266WebServer &server, HTTPMethod requestMethod, const String &requestUri) override {
      String fName = requestUri;
      if (!fName.startsWith("/")) { fName = "/" + fName; }

      if (requestMethod == HTTP_POST) {

      } else if (requestMethod == HTTP_DELETE) {
         if (LittleFS.exists(fName)) { LittleFS.remove(fName); }
      } 

      wserver.send(200); 
      return (true);
   } 

   void upload(ESP8266WebServer UNUSED &server, const String UNUSED &_requestUri, HTTPUpload &upload) override {
      String fName = upload.filename;
      if (!fName.startsWith("/")) { fName = "/" + fName; }

      if (upload.status == UPLOAD_FILE_START) {
         if (LittleFS.exists(fName)) { LittleFS.remove(fName); }  
         _fsUploadFile = LittleFS.open(fName, "w");

      } else if (upload.status == UPLOAD_FILE_WRITE) {
         if (_fsUploadFile) { _fsUploadFile.write(upload.buf, upload.currentSize); }

      } else if (upload.status == UPLOAD_FILE_END) {
         if (_fsUploadFile) { _fsUploadFile.close(); }
      }
   }  

   protected:
   File _fsUploadFile;
   };
   void defWebpages() {
      TRACE2("Register service handlers...\n");

   wserver.on("/$upload.htm", []() {
      wserver.send(200, "text/html", FPSTR(uploadContent));
   });

   wserver.on("/", HTTP_GET, handleRedirect);

   wserver.on("/$list", HTTP_GET, handleListFiles);
   wserver.on("/$sysinfo", HTTP_GET, handleSysInfo);

   wserver.addHandler(new FileServerHandler());

   wserver.enableCORS(true);


   wserver.serveStatic("/", LittleFS, "/");

   wserver.onNotFound([]() {
      wserver.send(404, "text/html", FPSTR(notFoundContent));
   });

   }

   void setupWS()
   {
      if (!LittleFS.begin()) TRACE2("could not mount the filesystem...\n");
      if (!MDNS.begin(HOSTNAME)) Serial.println("Error iniciando mDNS");
      else Serial.println("mDNS iniciado");
      httpUpdater.setup(&wserver, update_path, update_username, update_password);
      defWebpages();
      MDNS.addService("http", "tcp", wsport);
      MDNS.announce();
      wserver.begin();
      Serial.println(F("[WS] HTTPUpdateServer ready!"));
      Serial.printf("[WS]    --> Open http://%s.local:%d%s in your browser and login with username '%s' and password '%s'\n\n", WiFi.getHostname(), wsport, update_path, update_username, update_password);
      TRACE2("hostname=%s\n", WiFi.getHostname());
   }
   void procesaWebServer()
   {
      wserver.handleClient();
      MDNS.update();
   }  
   void endWS()
   {
      TRACE2("cerrando filesystem...\n");
      LittleFS.end();
      TRACE2("terminando MDNS...\n");
      MDNS.end();
      TRACE2("terminando webserver...\n");
      wserver.stop();
   }


#endif