// WiFi icon bitmaps (8x8 pixels)
// Connected: dot + 3 arcs up
static const uint8_t PROGMEM wifi_on_bmp[] = {
  0b00000000,
  0b00111100,
  0b01000010,
  0b00111100,
  0b00011000,
  0b00011000,
  0b00001000,
  0b00000000
};
// Disconnected: same but crossed out
static const uint8_t PROGMEM wifi_off_bmp[] = {
  0b10000001,
  0b00111100,
  0b01000010,
  0b00111100,
  0b00011000,
  0b00011000,
  0b10001000,
  0b00000000
};

// --- Відображення головного вікна ---
static void display_main() {
  int16_t x1, y1;
  uint16_t w, h;

  // --- 1. Роздільна лінія (Y=22): статус реле ---
  bool releState = digitalRead(RELE);
  for (int x = 0; x < 64; x += 4) {
    if (releState) {
      display.drawFastHLine(x, 22, 3, WHITE); // суцільні штрихи — ON
    } else {
      display.drawPixel(x, 22, WHITE);        // крапки — OFF
    }
  }

  // --- 2. Верхня частина: Температура або "none" (шрифт size=2, 0..21) ---
  display.setTextSize(2);
  String topText;
  if (sensorType == S_NONE) {
    topText = "none";
  } else if (Temperature <= -100.0f) {
    topText = "N/A"; // DS18B20 відсутній
  } else {
    topText = String(Temperature, 1);
  }
  display.getTextBounds(topText, 0, 0, &x1, &y1, &w, &h);
  display.setCursor((64 - w) / 2, 11 - h / 2);
  display.print(topText);

  // --- 3. Нижня частина: Вологість або Час (шрифт size=1, рядки 24..36) ---
  display.setTextSize(1);
  String bottomText;
  if (sensorType == S_BME280 || sensorType == S_HTU21) {
    bottomText = String((int)Humedity) + "%";
  } else {
    char timeStr[6];
    time_t t = time(nullptr);
    struct tm *tm_info = localtime(&t);
    sprintf(timeStr, "%02d:%02d", tm_info->tm_hour, tm_info->tm_min);
    bottomText = String(timeStr);
  }
  display.getTextBounds(bottomText, 0, 0, &x1, &y1, &w, &h);
  display.setCursor((64 - w) / 2, 29 - h / 2);
  display.print(bottomText);

  // --- 4. Нижня смуга (Y=39): WiFi + TG статус ---
  bool wifiOk = (WiFi.status() == WL_CONNECTED);

  // WiFi іконка (8x8 bitmap) або перекреслена
  if (wifiOk) {
    display.drawBitmap(2, 39, wifi_on_bmp, 8, 8, WHITE);
  } else {
    display.drawBitmap(2, 39, wifi_off_bmp, 8, 8, WHITE);
  }

  // TG статус: текст "TG" + підкреслення або перекреслення
  display.setTextSize(1);
  display.setCursor(16, 39);
  display.print("TG");
  if (!wifiOk || !tg_connected) {
    // Перекреслення: лінія через "TG" (приблизно 12px wide, 8px high)
    display.drawLine(15, 39, 28, 46, WHITE);
  }
}

// --- Відображення вікна IP адреси ---
static void display_ip_page() {
  int16_t x1, y1;
  uint16_t w, h;

  String ip = WiFi.localIP().toString();

  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print("IP:");

  // Розбиваємо IP на 2 рядки якщо довгий
  display.setTextSize(1);
  // Показуємо IP розбитий по крапці
  int dotIdx = ip.lastIndexOf('.');
  if (dotIdx > 0 && ip.length() > 12) {
    String part1 = ip.substring(0, dotIdx + 1);
    String part2 = ip.substring(dotIdx + 1);
    display.setCursor(0, 10);
    display.print(part1);
    display.setCursor(0, 20);
    display.print(part2);
  } else {
    display.setCursor(0, 10);
    display.print(ip);
  }

  // Підказка: "SCAN QR ->"
  display.setTextSize(1);
  display.setCursor(0, 38);
  display.print("v QR next");
}

// --- Відображення QR-коду ---
#ifdef USE_QR_CODE
#include <qrcode.h>
static void display_qr_page() {
  String url = "http://" + WiFi.localIP().toString();
  
  QRCode qrcode;
  uint8_t qrcodeBytes[qrcode_getBufferSize(2)];
  qrcode_initText(&qrcode, qrcodeBytes, 2, ECC_LOW, url.c_str());

  // qrcode.size = 25 для версії 2
  // Масштаб: 1 піксель/модуль (25x25 займає 25x25 px → вміщується в 64x48)
  // Центруємо: offsetX = (64-25)/2=19, offsetY = (48-25)/2=11
  int scale = 1;
  int offX = (64 - qrcode.size * scale) / 2;
  int offY = (48 - qrcode.size * scale) / 2;
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
#else
static void display_qr_page() {
  // QR недоступний — показуємо IP ще раз
  display_ip_page();
}
#endif

void display_loop(void)
{ 
  // Збереження серцебиття (інверсія)
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
