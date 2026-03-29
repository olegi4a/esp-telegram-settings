// Ініціалізація LittleFS
void FS_INIT(void)
{
  if(LittleFS.begin())
  {
    TBLOG_LN(F("LittleFS init - ok"));
  }
  else
  {
    TBLOG_LN(F("LittleFS init - false"));
    // Якщо не вдалося ініціалізувати, спробуємо відформатувати (це допоможе при першому запуску)
    if (LittleFS.format()) {
      TBLOG_LN(F("LittleFS formatted"));
      LittleFS.begin();
    }
  }
  // WebServer сторінки для роботи з ФС
  WebServer.onNotFound(handleNotFound);
}

void handleNotFound()
{ 
  // Captive Portal Redirect
  if (WiFi.getMode() == WIFI_AP || WiFi.getMode() == WIFI_AP_STA) {
    String reqHost = WebServer.hostHeader();
    String apIP = WiFi.softAPIP().toString();
    if (reqHost != apIP) {
      WebServer.sendHeader("Location", String("http://") + apIP + String("/"), true);
      WebServer.send(302, "text/plain", "Redirecting to Captive Portal");
      return;
    }
  }

  if (!handleFileRead(WebServer.uri())) 
  {
    WebServer.send(404, "text/plain", "FileNotFound");
  }
}

String getContentType(String filename) 
{
  if (WebServer.hasArg("download")) return "application/octet-stream";
  else if (filename.endsWith(".htm")) return "text/html";
  else if (filename.endsWith(".html")) return "text/html";
  else if (filename.endsWith(".json")) return "application/json";
  else if (filename.endsWith(".css")) return "text/css";
  else if (filename.endsWith(".js")) return "application/javascript";
  else if (filename.endsWith(".png")) return "image/png";
  else if (filename.endsWith(".gif")) return "image/gif";
  else if (filename.endsWith(".jpg")) return "image/jpeg";
  else if (filename.endsWith(".ico")) return "image/x-icon";
  else if (filename.endsWith(".xml")) return "text/xml";
  else if (filename.endsWith(".pdf")) return "application/x-pdf";
  else if (filename.endsWith(".zip")) return "application/x-zip";
  else if (filename.endsWith(".gz")) return "application/x-gzip";
  return "text/plain";
}

bool handleFileRead(String path)
{
  TBLOG("Request: ");
  TBLOG_LN(path);
  
  // Обробка favicon.ico (немає в LittleFS)
  if (path.endsWith("/favicon.ico")) {
    WebServer.send(200, "image/x-icon", "");
    return true;
  }
  
  if (path.endsWith("/")) path += "index.html";
  String contentType = getContentType(path);
  String pathWithGz = path + ".gz";
  
  if (LittleFS.exists(pathWithGz) || LittleFS.exists(path))
  {
    if (LittleFS.exists(pathWithGz))
    {
      path += ".gz";
    }
    File file = LittleFS.open(path, "r");
    if(file) {
      WebServer.streamFile(file, contentType);
      file.close();
      TBLOG("Sent: ");
      TBLOG_LN(path);
      return true;
    }
  }
  TBLOG("File not found: ");
  TBLOG_LN(path);
  return false;
}
