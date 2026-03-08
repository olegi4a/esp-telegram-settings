// ============================================================
//  WEB_serwer.ino
//  Ініціалізація WebServer та REST API для локального дашборду.
//  Всі /api/ відповіді повертають JSON з CORS заголовками.
//  Файли статики (Home.html тощо) обслуговуються через FS.ino.
// ============================================================

// --- Встановлення CORS заголовків (дозволяє fetch з браузера) ---
static void _setCORS() {
  WebServer.sendHeader("Access-Control-Allow-Origin",  "*");
  WebServer.sendHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
  WebServer.sendHeader("Access-Control-Allow-Headers", "Content-Type");
  WebServer.sendHeader("Cache-Control",                "no-cache");
}

// --- Шаблон відповіді JSON ---
static void _sendJson(int code, String json) {
  _setCORS();
  WebServer.send(code, "application/json", json);
}

// ============================================================
//  GET /api/status — живі дані (температура, реле, режим тощо)
// ============================================================
void api_status() {
  JsonDocument doc;
  doc["temp"]    = serialized(String(Temperature, 1));
  doc["hum"]     = Humedity;
  doc["press"]   = serialized(String(Pressure, 1));
  doc["relay"]   = digitalRead(RELE);
  doc["mode"]    = Statatus_sensor_control;  // 0=manual,1=sensor,2=timer
  doc["profile"] = profile;                  // 0=night,1=day
  doc["power"]   = digitalRead(POWER);       // 1=220V present
  doc["uptime"]  = millis() / 1000;
  doc["heap"]    = ESP.getFreeHeap();
  char timeBuf[9];
  time_t t = time(nullptr);
  struct tm *tm_info = localtime(&t);
  sprintf(timeBuf, "%02d:%02d:%02d", tm_info->tm_hour, tm_info->tm_min, tm_info->tm_sec);
  doc["time"]    = timeBuf;
  doc["log_count"] = Logger_getCount();
  doc["log_max"]   = Logger_getMax();
  doc["wifi_rssi"] = WiFi.RSSI();
  doc["version"]   = FIRMWARE_VERSION;

  String out;
  serializeJson(doc, out);
  _sendJson(200, out);
}

// ============================================================
//  GET /api/config — повна конфігурація (глобальна + обидва профілі)
// ============================================================
void api_config() {
  JsonDocument doc;

  // Глобальні налаштування
  JsonObject g = doc["global"].to<JsonObject>();
  g["SID_STA"]      = SID_STA;
  g["Time_D_h"]     = Time_D / 3600;
  g["Time_N_h"]     = Time_N / 3600;
  g["alluser"]      = alluser;
  g["timezone_str"] = timezone_str;
  g["has_token"]    = (TB_Token.length() > 0);
  g["has_pass"]     = (TB_pasword != 12345);

  // Поточний профіль (активний)
  JsonObject cur = doc["current"].to<JsonObject>();
  cur["profile"]                    = profile;
  cur["Sensor_set"]                 = serialized(String(Sensor_set, 1));
  cur["Sensor_histeresis"]          = serialized(String(Sensor_histeresis, 1));
  cur["Statatus_sensor_control"]    = Statatus_sensor_control;
  cur["Rele_status"]                = Rele_status;
  cur["Start_status"]               = Start_status;
  cur["Time_on"]                    = Time_on;
  cur["Time_off"]                   = Time_off;
  cur["Alarm_start"]                = Alarm_start;
  cur["Alarm_data_u"]               = serialized(String(Alarm_data_u, 1));
  cur["Alarm_data_d"]               = serialized(String(Alarm_data_d, 1));
  cur["Alarm_data_set"]             = Alarm_data_set;
  cur["trend"]                      = trend;
  cur["widget_status"]              = widget_status;
  cur["Alarm_power"]                = Alarm_power;
  cur["relay_change_notify"]        = relay_change_notify;

  // Завантажуємо обидва профілі з файлів
  auto addProfile = [&doc](const char* key, const char* fname) {
    File f = LittleFS.open(fname, "r");
    if (!f) return;
    JsonDocument tmp;
    if (!deserializeJson(tmp, f)) {
      doc[key] = tmp;
    }
    f.close();
  };
  addProfile("day",   "/D_profile.json");
  addProfile("night", "/N_profile.json");

  String out;
  serializeJson(doc, out);
  _sendJson(200, out);
}


// ============================================================
//  GET /api/history_ram — повертає тільки RAM буфер
// ============================================================
void api_history_ram() {
  _setCORS();
  WebServer.setContentLength(CONTENT_LENGTH_UNKNOWN);
  WebServer.send(200, "application/x-ndjson", "");
  Logger_streamRamToWeb();
  WebServer.sendContent("");
}


// ============================================================
//  POST /api/save — зберегти налаштування (JSON-тіло запиту)
//  Формат: {"t":"D"|"N"|"B", "s":{...profile fields...}, "g":{...global...}}
// ============================================================
void api_save() {
  if (WebServer.method() == HTTP_OPTIONS) {
    _setCORS();
    WebServer.send(204);
    return;
  }

  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, WebServer.arg("plain"));
  if (err) {
    String e = "{\"ok\":false,\"error\":\"bad json\"}";
    _sendJson(400, e);
    return;
  }

  String target = doc["t"] | "D";
  JsonObject s  = doc["s"];
  JsonObject g  = doc["g"];

  // Глобальні налаштування
  if (!g.isNull()) {
    if (!g["Time_D_h"].isNull())  Time_D = g["Time_D_h"].as<int>() * 3600;
    if (!g["Time_N_h"].isNull())  Time_N = g["Time_N_h"].as<int>() * 3600;
    if (!g["SID_STA"].isNull())   SID_STA = g["SID_STA"].as<String>();
    if (!g["PAS_STA"].isNull())   PAS_STA = g["PAS_STA"].as<String>();
    if (!g["timezone_str"].isNull() && g["timezone_str"].as<String>().length() > 3) {
      timezone_str = g["timezone_str"].as<String>();
      configTime(timezone_str.c_str(), ntpServerName, ntpServerName2);
    }
    if (!g["TB_Token"].isNull() && g["TB_Token"].as<String>().length() > 10) {
      TB_Token = g["TB_Token"].as<String>();
      myBot.setToken(TB_Token);
    }
    if (!g["TB_pasword"].isNull()) TB_pasword = g["TB_pasword"].as<long>();
    Save_Config();
  }

  // Параметри профілю
  bool ok = true;
  if (target == "D" || target == "B") ok &= Patch_Profile("/D_profile.json", s);
  if (target == "N" || target == "B") ok &= Patch_Profile("/N_profile.json", s);
  
  // Перезавантажуємо активний профіль
  if (profile == 1) Load_Profile("/D_profile.json");
  else Load_Profile("/N_profile.json");

  // Логуємо подію більш точно:
  if (!s.isNull() && s.size() == 1 && !s["Statatus_sensor_control"].isNull()) {
    Logger_addEntry(14 + Statatus_sensor_control); // 14=Ручний, 15=Сенсор, 16=Таймер
  } else if (!g.isNull()) {
    Logger_addEntry(20); // 20 = Налаштування системи (G)
  } else {
    if (target == "D") Logger_addEntry(17);      // 17 = Збережено Денний
    else if (target == "N") Logger_addEntry(18); // 18 = Збережено Нічний
    else Logger_addEntry(19);                    // 19 = Збережено Обидва/Поточний
  }

  String resp = ok ? "{\"ok\":true}" : "{\"ok\":false,\"error\":\"write failed\"}";
  _sendJson(ok ? 200 : 500, resp);
}

// ============================================================
//  POST /api/mode — перемикання режимів (manual/auto) без запису у файл
//  Тіло: {"mode":"manual"} або {"mode":"auto"}
// ============================================================
void api_mode() {
  if (WebServer.method() == HTTP_OPTIONS) {
    _setCORS();
    WebServer.send(204);
    return;
  }
  JsonDocument doc;
  deserializeJson(doc, WebServer.arg("plain"));
  String req = doc["mode"] | "";

  if (req == "manual") {
    Statatus_sensor_control = 0;
    Logger_addEntry(14);
  } else if (req == "auto") {
    restoreAutomationMode();
    Logger_addEntry(14 + Statatus_sensor_control);
  } else {
    _sendJson(400, "{\"ok\":false,\"error\":\"bad mode\"}");
    return;
  }

  String resp = "{\"ok\":true,\"mode\":" + String(Statatus_sensor_control) + "}";
  _sendJson(200, resp);
}

// ============================================================
//  POST /api/relay — керування реле
//  Тіло: {"state":1} або {"state":0} або {"state":-1} для toggle
// ============================================================
void api_relay() {
  if (WebServer.method() == HTTP_OPTIONS) {
    _setCORS();
    WebServer.send(204);
    return;
  }
  if (Statatus_sensor_control != 0) {
    String e = "{\"ok\":false,\"error\":\"automation active\"}";
    _sendJson(409, e);
    return;
  }
  JsonDocument doc;
  deserializeJson(doc, WebServer.arg("plain"));
  int req = doc["state"] | -1;
  bool newState;
  if      (req == 1)  newState = HIGH;
  else if (req == 0)  newState = LOW;
  else                newState = !digitalRead(RELE); // toggle

  digitalWrite(RELE, newState);
  Logger_addEntry(1); // 1 = relay toggle event

  String resp = "{\"ok\":true,\"relay\":" + String(newState ? "1" : "0") + "}";
  _sendJson(200, resp);
}

// ============================================================
//  GET /api/reboot — перезавантаження пристрою
// ============================================================
void api_reboot() {
  _setCORS();
  WebServer.send(200, "application/json", "{\"ok\":true,\"msg\":\"rebooting\"}");
  Logger_addEntry(11); // 11 = Перезавантаження
  Logger_flushToFile();
  delay(500);
  ESP.restart();
}

// ============================================================
//  OPTIONS catch-all — для CORS preflight
// ============================================================
void api_options() {
  _setCORS();
  WebServer.send(204);
}

// ============================================================
//  GET /api/fs/list — список всіх файлів у LittleFS
//  Відповідь: [{"name":"/Config.json","size":512}, ...]
// ============================================================
static void _addDirToList(String path, String& out, bool& first) {
  Dir dir = LittleFS.openDir(path);
  while (dir.next()) {
    String fullPath = path;
    if (!fullPath.endsWith("/")) fullPath += "/";
    fullPath += dir.fileName();
    
    if (dir.isDirectory()) {
      _addDirToList(fullPath, out, first);
    } else {
      if (!first) out += ",";
      out += "{\"name\":\"" + fullPath + "\",\"size\":" + String(dir.fileSize()) + "}";
      first = false;
    }
  }
}

void api_fs_list() {
  String out = "[";
  bool first = true;
  _addDirToList("/", out, first);
  out += "]";
  _sendJson(200, out);
}

// ============================================================
//  GET /api/fs/download?file=/Config.json — стримінг файлу
// ============================================================
void api_fs_download() {
  if (!WebServer.hasArg("file")) {
    _sendJson(400, "{\"ok\":false,\"error\":\"missing file param\"}");
    return;
  }
  String path = WebServer.arg("file");
  // Захист: тільки абсолютні шляхи без ..
  if (!path.startsWith("/") || path.indexOf("..") >= 0) {
    _sendJson(400, "{\"ok\":false,\"error\":\"bad path\"}");
    return;
  }
  if (!LittleFS.exists(path)) {
    _sendJson(404, "{\"ok\":false,\"error\":\"not found\"}");
    return;
  }
  File f = LittleFS.open(path, "r");
  if (!f) {
    _sendJson(500, "{\"ok\":false,\"error\":\"open failed\"}");
    return;
  }
  _setCORS();
  // Встановлюємо Content-Disposition щоб браузер зберігав файл
  String fname = path.substring(path.lastIndexOf('/') + 1);
  WebServer.sendHeader("Content-Disposition", "attachment; filename=\"" + fname + "\"");
  WebServer.streamFile(f, "application/octet-stream");
  f.close();
}

// ============================================================
//  POST /api/fs/upload — multipart завантаження файлу у LittleFS
//  Реєструється як: WebServer.on(path, HTTP_POST, done_cb, upload_cb)
// ============================================================
static File _uploadFile;
static String _uploadFileName;

void api_fs_upload_handler() {
  HTTPUpload& upload = WebServer.upload();
  if (upload.status == UPLOAD_FILE_START) {
    _uploadFileName = upload.filename;
    if (!_uploadFileName.startsWith("/")) _uploadFileName = "/" + _uploadFileName;
    TBLOG("FS upload start: "); TBLOG_LN(_uploadFileName);
    _uploadFile = LittleFS.open(_uploadFileName, "w");
  } else if (upload.status == UPLOAD_FILE_WRITE) {
    if (_uploadFile) _uploadFile.write(upload.buf, upload.currentSize);
  } else if (upload.status == UPLOAD_FILE_END) {
    if (_uploadFile) {
      _uploadFile.close();
      TBLOG("FS upload done: "); TBLOG_LN(upload.totalSize);
    }
  }
}

void api_fs_upload_done() {
  if (_uploadFileName.isEmpty()) {
    _sendJson(500, "{\"ok\":false,\"error\":\"upload failed\"}");
    return;
  }
  String resp = "{\"ok\":true,\"file\":\"" + _uploadFileName + "\",\"size\":" + WebServer.upload().totalSize + "}";
  _sendJson(200, resp);
  _uploadFileName = "";
}

// ============================================================
//  Ініціалізація WebServer (всі маршрути)
// ============================================================
void WebServer_Init() {
  // === Нові REST API ===
  WebServer.on("/api/status",  HTTP_GET,    api_status);
  WebServer.on("/api/config",  HTTP_GET,    api_config);
  WebServer.on("/api/mode",    HTTP_POST,   api_mode);
  WebServer.on("/api/mode",    HTTP_OPTIONS, api_options);
  WebServer.on("/api/history_ram", HTTP_GET, api_history_ram);
  WebServer.on("/api/save",    HTTP_POST,   api_save);
  WebServer.on("/api/relay",   HTTP_POST,   api_relay);
  WebServer.on("/api/reboot",  HTTP_GET,    api_reboot);
  // OPTIONS для CORS preflight (браузер надсилає перед POST)
  WebServer.on("/api/save",    HTTP_OPTIONS, api_options);
  WebServer.on("/api/relay",   HTTP_OPTIONS, api_options);

  WebServer.on("/Reboot",                   handle_Reboot);

  // === Файлова система (бекап / оновлення веб-файлів) ===
  WebServer.on("/api/fs/list",     HTTP_GET,     api_fs_list);
  WebServer.on("/api/fs/download", HTTP_GET,     api_fs_download);
  WebServer.on("/api/fs/upload",   HTTP_POST,    api_fs_upload_done, api_fs_upload_handler);
  WebServer.on("/api/fs/upload",   HTTP_OPTIONS, api_options);

  WebUpdater.setup(&WebServer);
  WebServer.begin();
  TBLOG_LN(F("WebServer begin (REST API ready)"));
}

void handle_Reboot() {
  WebServer.send(200, "text/plain", "Rebooting device...");
  delay(1000);
  ESP.restart();
}
