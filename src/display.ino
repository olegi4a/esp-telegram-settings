#include <Fonts/Org_01.h>
#include <Fonts/FreeSans24pt7b.h>
#include <Fonts/FreeSans12pt7b.h>
#include <Fonts/FreeSans9pt7b.h>

// --- Відображення головного вікна ---
static void display_main_small() {
  // --- 1. Роздільна лінія (Y=8) ---
  display.drawLine(0, 8, 63, 8, WHITE);

  // --- 2. Статус Telegram ---
  bool wifiOk = (WiFi.status() == WL_CONNECTED);
  display.setTextColor(WHITE);
  display.setTextWrap(false);
  display.setFont(&Org_01);
  display.setCursor(51, 6);
  if (wifiOk && tg_connected) {
    display.print("TG");
  } else if (wifiOk) {
    display.print("X");
  }

  // --- 3. Значок Wi-Fi (процедурний міні) ---
  if (wifiOk) {
    int32_t rssi = WiFi.RSSI();
    int bars = (rssi > -55) ? 4 : (rssi > -65) ? 3 : (rssi > -75) ? 2 : 1;
      for (int i = 0; i < 4; i++) {
        int barH = i + 2; // 2, 3, 4, 5
        int bx = 37 + i * 3; // 37, 40, 43, 46
        int by = 7 - barH;
        if (i < bars) display.fillRect(bx, by, 2, barH, WHITE);
        else          display.drawPixel(bx, 6, WHITE); // Відсутня поділка
    }
  } else {
    display.setCursor(43, 6);
    display.print("X");
  }

  // --- 4. Годинник ---
  char timeStr[6];
  time_t t = time(nullptr);
  struct tm *tm_info = localtime(&t);
  sprintf(timeStr, "%02d:%02d", tm_info->tm_hour, tm_info->tm_min);
  display.setCursor(2, 6);
  display.print(timeStr);
  if (!is_time_exact) {
    display.print("!");
  }

  // --- 5. Температура ---
  display.setFont(&FreeSans12pt7b);
  String tempStr = (sensorType == S_NONE || Temperature <= -100.0f) ? "--" : String(Temperature, 1);
  display.setCursor(1, 32);
  display.print(tempStr);

  // --- 6. Тенденція (Трикутник) ---
  if (sensorType != S_NONE && Temperature > -100.0f) {
    if (visual_trend == 1) {
      display.fillTriangle(54, 26, 50, 30, 58, 30, WHITE);
    } else if (visual_trend == -1) {
      display.fillTriangle(54, 30, 50, 26, 58, 26, WHITE);
    }
  }

  // --- 7. Профіль (Режим) ---
  display.setFont(&Org_01);
  String profStr = "GENERAL";
  if (profile_timer_en) profStr = (profile == 1) ? "DAY" : "NIGHT";
  
  int16_t x1, y1; uint16_t w, h;
  display.getTextBounds(profStr, 0, 0, &x1, &y1, &w, &h);
  display.setCursor((64 - w) / 2, 45);
  display.print(profStr);

  // Скидаємо шрифт назад до дефолтного
  display.setFont();
  display.setTextSize(1);
}

static void display_main_large() {
  // --- 1. Line (Y=14) ---
  display.drawLine(0, 14, 127, 14, WHITE);

  display.setTextColor(WHITE);
  display.setTextWrap(false);

  // --- 2. Час (Time) - Org_01 x2 ---
  display.setFont(&Org_01);
  display.setTextSize(2);
  char timeStr[6];
  time_t t = time(nullptr);
  struct tm *tm_info = localtime(&t);
  sprintf(timeStr, "%02d:%02d", tm_info->tm_hour, tm_info->tm_min);
  display.setCursor(1, 11);
  display.print(timeStr);
  if (!is_time_exact) {
    display.print("!");
  }

  // --- 3. Статус Telegram ---
  bool wifiOk = (WiFi.status() == WL_CONNECTED);
  display.setCursor(85, 11);
  if (wifiOk && tg_connected) {
    display.print("TG");
  } else if (wifiOk) {
    display.print("X");
  }

  // --- 4. Значок Wi-Fi ---
  if (wifiOk) {
    int32_t rssi = WiFi.RSSI();
    int bars = (rssi > -55) ? 4 : (rssi > -65) ? 3 : (rssi > -75) ? 2 : 1;

    for (int i = 0; i < 4; i++) {
        int barH = (i + 1) * 3; // 3, 6, 9, 12
        int bx = 112 + i * 4;   // 112, 116, 120, 124
        int by = 13 - barH;
        if (i < bars) display.fillRect(bx, by, 3, barH, WHITE);
        else          display.fillRect(bx, 12, 3, 1, WHITE); // Маленький квадратик внизу для відсутніх поділок
    }
  } else {
    display.setCursor(112, 11);
    display.print("X");
  }

  // --- 5. Температура ---
  display.setTextSize(1); 
  display.setFont(&FreeSans24pt7b);
  String tempStr = (sensorType == S_NONE || Temperature <= -100.0f) ? "--" : String(Temperature, 1);
  display.setCursor(9, 52);
  display.print(tempStr);

  // --- 6. Градуси та значок "C" ---
  display.setFont(&FreeSans9pt7b);
  display.setCursor(112, 34);
  display.print("C");
  display.drawCircle(109, 23, 2, WHITE);

  // --- 7. Трикутник (Тенденція температури) ---
  if (sensorType != S_NONE && Temperature > -100.0f) {
    if (visual_trend == 1) {
      display.fillTriangle(115, 42, 107, 50, 123, 50, WHITE);
    } else if (visual_trend == -1) {
      display.fillTriangle(115, 50, 107, 42, 123, 42, WHITE);
    }
  }

  // --- 8. Профіль (Profile) в самому низу ---
  display.setFont(&Org_01);
  String profStr = "GENERAL";
  if (profile_timer_en) profStr = (profile == 1) ? "DAY" : "NIGHT";
  
  int16_t x1, y1; uint16_t w, h;
  display.getTextBounds(profStr, 0, 0, &x1, &y1, &w, &h);
  display.setCursor(64 - (w / 2), 62); 
  display.print(profStr);

  display.setFont();
  display.setTextSize(1);
}

static void display_main() {
#ifdef SCREEN_128_64
  display_main_large();
#else
  display_main_small();
#endif
}

// --- Відображення вікна IP адреси ---
void display_ip_page() {
  int16_t x1, y1;
  uint16_t w, h;
  uint16_t screenW = display.width();
  uint16_t screenH = display.height();

  String ip = (WiFi.getMode() == WIFI_AP || WiFi.getMode() == WIFI_AP_STA) ? WiFi.softAPIP().toString() : WiFi.localIP().toString();

  int dotIdx = ip.indexOf('.', ip.indexOf('.') + 1);
  String part1 = ip.substring(0, dotIdx + 1);
  String part2 = ip.substring(dotIdx + 1);

#ifdef SCREEN_128_64
  if (true) {
#else
  if (false) {
#endif
    display.fillRect(0, 0, 128, 18, WHITE);
    display.setTextColor(BLACK);
    display.setFont(&FreeSans9pt7b);
    display.setTextSize(1);
    String title = "Wi-Fi IP";
    display.getTextBounds(title, 0, 0, &x1, &y1, &w, &h);
    display.setCursor((128 - w) / 2, 14);
    display.print(title);
    
    display.setTextColor(WHITE);
    display.getTextBounds(ip, 0, 0, &x1, &y1, &w, &h);
    display.setCursor((128 - w) / 2, 45); 
    display.print(ip);

  } else {
    display.setFont(&TomThumb);
    display.setTextSize(2);

    display.getTextBounds(part1, 0, 0, &x1, &y1, &w, &h);
    display.setCursor((screenW - w) / 2, screenH / 2 - 5);
    display.print(part1);

    display.getTextBounds(part2, 0, 0, &x1, &y1, &w, &h);
    display.setCursor((screenW - w) / 2, screenH / 2 + h + 2);
    display.print(part2);
  }

  display.setFont();
  display.display();
}

// --- Відображення QR-коду ---
#ifdef USE_QR_CODE
#include <qrcode.h>
static void display_qr_page() {
  uint16_t screenW = display.width();
  uint16_t screenH = display.height();
  String url = (WiFi.getMode() == WIFI_AP || WiFi.getMode() == WIFI_AP_STA) ? WiFi.softAPIP().toString() : WiFi.localIP().toString(); 
  
  QRCode qrcode;
  uint8_t qrcodeBytes[qrcode_getBufferSize(1)];
  qrcode_initText(&qrcode, qrcodeBytes, 1, ECC_LOW, url.c_str());

#ifdef SCREEN_128_64
  if (true) {
#else
  if (false) {
#endif
    int scale = 3; 
    int offX = (screenW - qrcode.size * scale) / 2;
    int offY = (screenH - qrcode.size * scale) / 2;
    
    for (uint8_t y = 0; y < qrcode.size; y++) {
      for (uint8_t x = 0; x < qrcode.size; x++) {
        if (qrcode_getModule(&qrcode, x, y)) {
          display.fillRect(offX + x * scale, offY + y * scale, scale, scale, WHITE);
        }
      }
    }
  } else {
    int scale = 2; 
    int offX = (screenW - qrcode.size * scale) / 2;
    int offY = (screenH - qrcode.size * scale) / 2;
    if (offX < 0) offX = 0;
    if (offY < 0) offY = 0;

    for (uint8_t y = 0; y < qrcode.size; y++) {
      for (uint8_t x = 0; x < qrcode.size; x++) {
        if (qrcode_getModule(&qrcode, x, y)) {
          display.fillRect(offX + x * scale, offY + y * scale, scale, scale, WHITE);
        }
      }
    }
  }
}
#else
static void display_qr_page() {
  display_ip_page();
}
#endif

void display_loop(void)
{ 
  if (!display_ok) return;

  display_inv = !display_inv;
  display.invertDisplay(!display_inv);

  display.clearDisplay();
  display.setTextColor(WHITE);

  switch (display_page) {
    case 0:  display_main();    break;
    case 1:  display_ip_page(); break;
    case 2:  display_qr_page(); break;
    default: display_page = 0; display_main(); break;
  }

  display.display();
}

void display_init() {
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  
  display.display();
  display_ok = true;

  // Зменшуємо яскравість на 50% (контраст 127 з 255) для продовження служби
  display.ssd1306_command(SSD1306_SETCONTRAST);
  display.ssd1306_command(127);

  TBLOG_LN("OLED Init OK");
}
