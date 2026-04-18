// ============================================================
//  AA_Logger.ino — NDJSON Append-Only History Logger
//  Prefixed AA_ so Arduino compiler processes it first.
//
//  Parallel primitive arrays instead of struct — avoids any
//  forward-declaration type conflict with Arduino .ino merging.
// ============================================================

#define LOG_MAX              100        // Max entries in RAM buffer
#define LOG_PERIODIC_INTERVAL 900000UL  // Покращено до 15 хвилин
#define LOG_MINUTE_INTERVAL  60000UL
#define LOG_EMERGENCY_MAX    2

// Коли вільного місця на LittleFS стає менше 100 КБ, старі логи видаляються
#define LOG_MIN_FREE_BYTES   102400UL

// Максимальний об'єм одного файлу, хоч він і щоденний (на випадок аномалій)
#define LOG_FILE_MAX_BYTES   200000UL

#define LOG_DELTA_TEMP    0.1f
#define LOG_DELTA_HUM     1
#define LOG_DELTA_PRESS   10

#define LOG_DIR           "/log"

// --- RAM буфер (паралельні масиви — без struct) ---
static uint32_t  buf_ts   [LOG_MAX];
static float     buf_temp [LOG_MAX];
static uint8_t   buf_hum  [LOG_MAX];
static uint16_t  buf_press[LOG_MAX];
static uint8_t   buf_relay[LOG_MAX];
static uint8_t   buf_ev   [LOG_MAX];
static uint16_t  logCount = 0;

static uint8_t   emergencyFlushCount = 0;
static uint32_t  emergencyFlushWindowMs = 0;
static unsigned long lastPeriodicFlushMs = 0;
static unsigned long lastMinuteCheckMs  = 0;

static float    lastLogTemp  = -999.0f;
static uint8_t  lastLogHum   = 255;
static uint16_t lastLogPress = 0;

// ============================================================
//  Утиліта: Формування імені поточного файлу логу (по днях)
// ============================================================
String Logger_getCurrentFile() {
  time_t t = time(nullptr);
  struct tm *tm_info = localtime(&t);
  if (t < 1500000000 || (tm_info->tm_year + 1900) < 2024) {
    // Якщо час ще не синхронізовано, пишемо у тимчасовий файл
    return String(LOG_DIR) + "/system_startup.ndjson";
  }
  char buf[32];
  snprintf(buf, sizeof(buf), "%s/%04d-%02d-%02d.ndjson", LOG_DIR, tm_info->tm_year + 1900, tm_info->tm_mon + 1, tm_info->tm_mday);
  return String(buf);
}

// ============================================================
//  Очищення старих файлів, якщо мало пам'яті (Rotation)
// ============================================================
void Logger_cleanupOldLogs() {
  FSInfo fs_info;
  LittleFS.info(fs_info);
  
  // Поки вільного місця менше мінімуму (100 КБ) - видаляємо найстаріший лог
  while ((fs_info.totalBytes - fs_info.usedBytes) < LOG_MIN_FREE_BYTES) {
    Dir dir = LittleFS.openDir(LOG_DIR);
    String oldestFile = "";
    
    // Шукаємо файл із найменшим (найстарішим) іменем, бо імена YYYY-MM-DD
    while (dir.next()) {
      String fileName = dir.fileName();
      if (fileName.endsWith(".ndjson")) {
        if (oldestFile == "" || fileName < oldestFile) {
          oldestFile = fileName;
        }
      }
    }
    
    if (oldestFile != "") {
      TBLOG(F("Logger: Memory low. Deleting old log: "));
      TBLOG_LN(oldestFile);
      LittleFS.remove(String(LOG_DIR) + "/" + oldestFile);
      LittleFS.info(fs_info); // Оновлюємо інфо про місце
    } else {
      // Більше немає логів для видалення (або всі файли не є логами)
      break;
    }
  }
}

// ============================================================
//  Захист одиничного файлу від переповнення (якщо спам)
// ============================================================
void Logger_checkRotation(const String& path) {
  File f = LittleFS.open(path, "r");
  if (!f) return;
  size_t sz = f.size();
  f.close();
  // Якщо навіть щоденний файл якимось дивом став величезним, просто видаляємо його
  if (sz >= LOG_FILE_MAX_BYTES) {
    TBLOG_LN(F("Logger: Day file too big, resetting..."));
    LittleFS.remove(path);
  }
}

// ============================================================
//  Скид RAM-буфера у файл (append-only, по днях)
// ============================================================
void Logger_flushToFile() {
  if (logCount == 0) return;
  
  // 1. Очистка місця перед записом
  Logger_cleanupOldLogs();
  
  // 2. Вибір файлу
  String filePath = Logger_getCurrentFile();
  Logger_checkRotation(filePath);
  
  // 3. Відкриття і запис
  File f = LittleFS.open(filePath, "a");
  if (!f) { TBLOG_LN(F("Logger: open fail!")); return; }
  for (uint16_t i = 0; i < logCount; i++) {
    f.print(F("{\"ts\":")); f.print(buf_ts[i]);
    f.print(F(",\"t\":\"")); f.print(buf_temp[i], 1);
    f.print(F("\",\"h\":")); f.print(buf_hum[i]);
    f.print(F(",\"p\":")); f.print(buf_press[i]);
    f.print(F(",\"r\":")); f.print(buf_relay[i]);
    f.print(F(",\"ev\":")); f.print(buf_ev[i]);
    f.println('}');
  }
  f.close();
  TBLOG(F("Logger: Flushed ")); TBLOG(logCount); TBLOG_LN(F(" entries."));
  logCount = 0;
}

// ============================================================
//  Додати запис у RAM-буфер (delta-check для ev=0)
// ============================================================
void Logger_addEntry(uint8_t eventType) {
  if (eventType == 0) {
    bool tOk = fabsf(Temperature - lastLogTemp) >= LOG_DELTA_TEMP;
    bool hOk = abs((int)Humedity - (int)lastLogHum)    >= LOG_DELTA_HUM;
    bool pOk = abs((int)Pressure  - (int)lastLogPress)  >= LOG_DELTA_PRESS;
    if (!tOk && !hOk && !pOk) return;
  }
  if (logCount >= LOG_MAX) Logger_emergencyFlush(false);

  buf_ts   [logCount] = (uint32_t)time(nullptr);
  buf_temp [logCount] = Temperature;
  buf_hum  [logCount] = (uint8_t)Humedity;
  buf_press[logCount] = (uint16_t)Pressure;
  buf_relay[logCount] = (uint8_t)digitalRead(RELE);
  buf_ev   [logCount] = eventType;
  logCount++;

  lastLogTemp  = Temperature;
  lastLogHum   = (uint8_t)Humedity;
  lastLogPress = (uint16_t)Pressure;
}

// ============================================================
//  Аварійний скид
// ============================================================
void Logger_emergencyFlush(bool isPowerFail) {
  if (!isPowerFail) {
    unsigned long ms = millis();
    if (ms - emergencyFlushWindowMs >= 3600000UL) {
      emergencyFlushCount    = 0;
      emergencyFlushWindowMs = ms;
    }
    if (emergencyFlushCount >= LOG_EMERGENCY_MAX) {
      TBLOG_LN(F("Logger: limit, drop."));
      logCount = 0;
      return;
    }
    emergencyFlushCount++;
  }
  if (logCount < LOG_MAX) {
    buf_ts   [logCount] = (uint32_t)time(nullptr);
    buf_temp [logCount] = Temperature;
    buf_hum  [logCount] = (uint8_t)Humedity;
    buf_press[logCount] = (uint16_t)Pressure;
    buf_relay[logCount] = (uint8_t)digitalRead(RELE);
    buf_ev   [logCount] = isPowerFail ? 3 : 5;
    logCount++;
  }
  Logger_flushToFile();
  if (!isPowerFail && WiFi.status() == WL_CONNECTED) {
    myBot.sendMessage(fb::Message(F("⚠️ Лог: буфер переповнено."), alluser));
  }
}

// ============================================================
//  Ticks
// ============================================================
void Logger_minuteTick() {
  unsigned long ms = millis();
  if (ms - lastMinuteCheckMs < LOG_MINUTE_INTERVAL) return;
  lastMinuteCheckMs = ms;
  Logger_addEntry(0);
}

void Logger_periodicTick() {
  unsigned long ms = millis();
  if (ms - lastPeriodicFlushMs < LOG_PERIODIC_INTERVAL) return;
  lastPeriodicFlushMs = ms;
  Logger_flushToFile();
}

// ============================================================
//  Зчитати timestamp останнього запису (останній файл логу)
// ============================================================
uint32_t History_loadLastTimestamp() {
  Dir dir = LittleFS.openDir(LOG_DIR);
  String newestFile = "";
  
  // Шукаємо найновіший файл (за алфавітом YYYY-MM-DD це працює, але ігноруємо system_startup)
  while (dir.next()) {
    String fileName = dir.fileName();
    if (fileName.endsWith(".ndjson") && fileName != "system_startup.ndjson") {
      if (newestFile == "" || fileName > newestFile) {
        newestFile = fileName;
      }
    }
  }
  
  if (newestFile == "") return 0;
  
  String fullPath = String(LOG_DIR) + "/" + newestFile;
  File f = LittleFS.open(fullPath, "r");
  if (!f || f.size() == 0) { if (f) f.close(); return 0; }
  
  String lastLine = "";
  if (f.size() > 200) f.seek(f.size() - 200);
  while (f.available()) {
    String line = f.readStringUntil('\n');
    line.trim();
    if (line.length() > 5) lastLine = line;
  }
  f.close();
  
  if (lastLine.length() == 0) return 0;
  int idx = lastLine.indexOf(F("\"ts\":"));
  if (idx < 0) return 0;
  uint32_t ts = (uint32_t)lastLine.substring(idx + 5).toInt();
  if (ts > 1500000000UL) return ts;
  return 0;
}

// ============================================================
//  Ініціалізація
// ============================================================
void Logger_init() {
  // Створюємо папку логів, якщо її немає
  if (!LittleFS.exists(LOG_DIR)) {
    LittleFS.mkdir(LOG_DIR);
  }
  
  // Очистка старих логів при старті (про всяк випадок)
  Logger_cleanupOldLogs();
  
  lastPeriodicFlushMs    = millis();
  lastMinuteCheckMs      = millis();
  emergencyFlushWindowMs = millis();
  TBLOG_LN(F("Logger: NDJSON OK (Daily mode)"));
}

uint16_t Logger_getCount() { return logCount; }
uint16_t Logger_getMax() { return LOG_MAX; }

void Logger_streamRamToWeb() {
  for (uint16_t i = 0; i < logCount; i++) {
    String line = "{\"ts\":"; line += buf_ts[i];
    line += ",\"t\":\""; line += String(buf_temp[i], 1);
    line += "\",\"h\":"; line += buf_hum[i];
    line += ",\"p\":"; line += buf_press[i];
    line += ",\"r\":"; line += buf_relay[i];
    line += ",\"ev\":"; line += buf_ev[i];
    line += "}\n";
    WebServer.sendContent(line);
  }
}

