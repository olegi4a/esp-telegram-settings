// ============================================================
//  WEB_serwer.ino
//  Ініціалізація WebServer та REST API для локального дашборду.
//  Всі /api/ відповіді повертають JSON з CORS заголовками.
//  Файли статики (Home.html тощо) обслуговуються через FS.ino.
// ============================================================

// --- Шаблон відповіді JSON ---
static void _sendJson(int code, String json) {
  WebServer.sendHeader(F("Access-Control-Allow-Origin"),  F("*"));
  WebServer.sendHeader(F("Access-Control-Allow-Methods"), F("GET, POST, OPTIONS"));
  WebServer.sendHeader(F("Access-Control-Allow-Headers"), F("Content-Type"));
  WebServer.sendHeader(F("Cache-Control"),                F("no-cache"));
  WebServer.send(code, F("application/json"), json);
}

// ============================================================
//  GET /api/status — живі дані (температура, реле, режим тощо)
// ============================================================
void handle_api_status() {
  JsonDocument doc;
  // Температура: якщо -127 (датчик відсутній) — повертаємо спеціальне значення
  bool sensorOk = (Temperature > -100.0f);
  doc["sensor_ok"] = sensorOk;
  doc["temp"]    = sensorOk ? serialized(String(Temperature, 1)) : serialized(String(-127));
  doc["hum"]     = Humedity;
  doc["press"]   = (int)(Pressure + 0.5f); // округлення до цілого mmHg
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
  // Telegram статус та назва бота
  doc["tg_ok"]    = tg_connected;
  doc["bot_name"] = botName;

  String out;
  serializeJson(doc, out);
  _sendJson(200, out);
}

// ============================================================
//  GET /api/config — повна конфігурація (глобальна + обидва профілі)
// ============================================================
void handle_api_config() {
  JsonDocument doc;

  // Глобальні налаштування
  JsonObject g = doc["global"].to<JsonObject>();
  g["SID_STA"]      = SID_STA;
  g["PAS_STA"]      = PAS_STA;         // реальний пароль WiFi
  g["Time_D_h"]     = Time_D / 3600;
  g["Time_N_h"]     = Time_N / 3600;
  g["alluser"]      = alluser;
  g["timezone_str"] = timezone_str;
  g["TB_Token"]     = TB_Token;        // реальний токен бота
  g["TB_pasword"]   = TB_pasword;      // реальний пароль доступу

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
void handle_api_history_ram() {
  WebServer.sendHeader("Access-Control-Allow-Origin",  "*");
  WebServer.setContentLength(CONTENT_LENGTH_UNKNOWN);
  WebServer.send(200, "application/x-ndjson", "");
  Logger_streamRamToWeb();
  WebServer.sendContent("");
}


// ============================================================
//  POST /api/save — зберегти налаштування (JSON-тіло запиту)
//  Формат: {"t":"D"|"N"|"B", "s":{...profile fields...}, "g":{...global...}}
// ============================================================
void handle_api_save() {
  if (WebServer.method() == HTTP_OPTIONS) {
    _sendJson(204, "");
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
    Update_Global_Config(g);
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
void handle_api_mode() {
  if (WebServer.method() == HTTP_OPTIONS) {
    _sendJson(204, "");
    return;
  }
  JsonDocument doc;
  deserializeJson(doc, WebServer.arg("plain"));
  String req = doc["mode"] | "";

  if (req == "manual") {
    Statatus_sensor_control = 0;
    digitalWrite(LED_BOOTON, HIGH);
    Logger_addEntry(14);
  } else if (req == "auto") {
    restoreAutomationMode(); // restoreAutomationMode internally updates LED_BOOTON
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
void handle_api_relay() {
  if (WebServer.method() == HTTP_OPTIONS) {
    _sendJson(204, "");
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

  String resp = "{\"ok\":true,\"relay\":" + String(newState ? "1" : "0") + "}";
  _sendJson(200, resp);
}

// ============================================================
//  GET /api/reboot — перезавантаження пристрою
// ============================================================
void handle_api_reboot() {
  _sendJson(200, "{\"ok\":true,\"msg\":\"rebooting\"}");
  Logger_addEntry(11); // 11 = Перезавантаження
  Logger_flushToFile();
  delay(500);
  ESP.restart();
}

// ============================================================
//  GET /api/wifi/scan — сканування Wi-Fi мереж
// ============================================================
void handle_api_wifi_scan() {
  int n = WiFi.scanNetworks();
  String out = "[";
  for (int i = 0; i < n; ++i) {
    if (i > 0) out += ",";
    out += "{\"ssid\":\"" + WiFi.SSID(i) + "\",\"rssi\":" + String(WiFi.RSSI(i)) + "}";
  }
  out += "]";
  _sendJson(200, out);
}

// ============================================================
//  POST /api/wifi/test — Запуск тестового підключення до Wi-Fi
// ============================================================
bool   wifi_test_active = false;
uint32_t wifi_test_start = 0;
String wifi_test_result = "";
String wifi_test_ip = "";
uint32_t wifi_test_done_ms = 0; // час завершення тесту (для кешу результату)
String wifi_test_old_ssid = "";
String wifi_test_old_pass = "";

void handle_api_wifi_test() {
  if (WebServer.method() == HTTP_OPTIONS) {
    _sendJson(204, "");
    return;
  }
  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, WebServer.arg("plain"));
  if (err) {
    _sendJson(400, "{\"ok\":false,\"error\":\"bad json\"}");
    return;
  }

  String ssid = doc["ssid"] | "";
  String pass = doc["pass"] | "";

  if (ssid.isEmpty()) {
    _sendJson(400, "{\"ok\":false,\"error\":\"empty ssid\"}");
    return;
  }

  // Зберігаємо поточне з'єднання
  wifi_test_old_ssid = WiFi.SSID();
  wifi_test_old_pass = WiFi.psk();

  // Якщо ми вже в STA і підключені — залишаємось в STA (на запит користувача)
  // Якщо ми в AP або не підключені — вмикаємо AP_STA для страховки
  if (WiFi.getMode() != WIFI_STA) {
    WiFi.mode(WIFI_AP_STA);
  }
  
  WiFi.begin(ssid.c_str(), pass.c_str());

  wifi_test_active = true;
  wifi_test_start = millis();
  wifi_test_result = "testing";
  wifi_test_ip = "";

  _sendJson(200, "{\"ok\":true,\"msg\":\"test started\"}");
}

// ============================================================
//  wifi_test_periodicTick — Фоновий моніторинг тесту (викликається з loop)
// ============================================================
void wifi_test_periodicTick() {
  if (!wifi_test_active) return;

  if (WiFi.status() == WL_CONNECTED) {
    wifi_test_result = "success";
    wifi_test_ip = WiFi.localIP().toString();
    wifi_test_active = false;
    wifi_test_done_ms = millis();
    TBLOG_LN(F("WiFi Test: SUCCESS"));
    
    // Завжди повертаємось до старої мережі, щоб браузер міг "зловити" відповідь
    if (wifi_test_old_ssid.length() > 0) {
      WiFi.begin(wifi_test_old_ssid.c_str(), wifi_test_old_pass.c_str());
    }
  } 
  else if (millis() - wifi_test_start > 30000) { // Тайм-аут 30с
    wifi_test_result = "fail";
    wifi_test_active = false;
    wifi_test_done_ms = millis();
    TBLOG_LN(F("WiFi Test: FAIL (timeout)"));
    
    // Повертаємось до старої мережі
    if (wifi_test_old_ssid.length() > 0) {
      WiFi.begin(wifi_test_old_ssid.c_str(), wifi_test_old_pass.c_str());
    } else {
      WiFi.disconnect(false);
    }
  }
}

// ============================================================
//  GET /api/wifi/status — Перевірка статусу тестового підключення
// ============================================================
void handle_api_wifi_status() {
  // 1. Якщо тест активний — повертаємо "testing"
  if (wifi_test_active) {
    _sendJson(200, "{\"status\":\"testing\"}");
    return;
  }

  // 2. Повертаємо "idle", якщо тест не активний і кеш результату вже минув (30 сек)
  if (!wifi_test_active && (wifi_test_result.length() == 0 || wifi_test_result == "idle")) {
    _sendJson(200, "{\"status\":\"idle\"}");
    return;
  }
  
  
  if (!wifi_test_active && wifi_test_done_ms > 0 && (millis() - wifi_test_done_ms > 60000)) {
    wifi_test_result = "";
    wifi_test_done_ms = 0;
    _sendJson(200, "{\"status\":\"idle\"}");
    return;
  }

  // 3. Віддаємо поточний або закешований результат
  String resp = "{\"status\":\"" + wifi_test_result + "\"";
  if (wifi_test_result == "success") {
    resp += ",\"ip\":\"" + wifi_test_ip + "\"";
  }
  resp += "}";
  _sendJson(200, resp);
}

// ============================================================
//  OPTIONS catch-all — для CORS preflight
// ============================================================
void handle_api_options() {
  _sendJson(204, "");
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

void handle_api_fs_list() {
  String out = "[";
  bool first = true;
  _addDirToList("/", out, first);
  out += "]";
  _sendJson(200, out);
}

// ============================================================
//  GET /api/fs/download?file=/Config.json — стримінг файлу
// ============================================================
void handle_api_fs_download() {
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
  WebServer.sendHeader("Access-Control-Allow-Origin",  "*");
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

void handle_api_fs_upload_handler() {
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

void handle_api_fs_upload_done() {
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
  // === REST API ===
  WebServer.on("/api/status",  HTTP_GET,    handle_api_status);
  WebServer.on("/api/config",  HTTP_GET,    handle_api_config);
  WebServer.on("/api/mode",    HTTP_POST,   handle_api_mode);
  WebServer.on("/api/history_ram", HTTP_GET, handle_api_history_ram);
  WebServer.on("/api/save",    HTTP_POST,   handle_api_save);
  WebServer.on("/api/relay",   HTTP_POST,   handle_api_relay);
  WebServer.on("/api/reboot",  HTTP_GET,    handle_api_reboot);
  WebServer.on("/api/wifi/scan", HTTP_GET,  handle_api_wifi_scan);
  WebServer.on("/api/wifi/test", HTTP_POST, handle_api_wifi_test);
  WebServer.on("/api/wifi/status", HTTP_GET,  handle_api_wifi_status);

  // OPTIONS catch-all for CORS preflight
  WebServer.on("/api/mode",    HTTP_OPTIONS, handle_api_options);
  WebServer.on("/api/save",    HTTP_OPTIONS, handle_api_options);
  WebServer.on("/api/relay",   HTTP_OPTIONS, handle_api_options);
  WebServer.on("/api/wifi/test", HTTP_OPTIONS, handle_api_options);

  // === Файлова система (LittleFS) ===
  WebServer.on("/api/fs/list",     HTTP_GET,     handle_api_fs_list);
  WebServer.on("/api/fs/download", HTTP_GET,     handle_api_fs_download);
  WebServer.on("/api/fs/upload",   HTTP_POST,    handle_api_fs_upload_done, handle_api_fs_upload_handler);
  WebServer.on("/api/fs/upload",   HTTP_OPTIONS, handle_api_options);

  WebUpdater.setup(&WebServer);

  WebServer.begin();
  TBLOG_LN(F("WebServer begin (API ready)"));
}
