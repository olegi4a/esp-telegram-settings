// (Шрифти FreeFonts прибрані, використовується строгий класичний шрифт)
#include <Fonts/TomThumb.h>

// WiFi icon bitmaps (16x16 pixels)
static const uint8_t PROGMEM wifi_on_bmp[] = {
  0x00, 0x00, 0x00, 0x00, 0x07, 0xE0, 0x1F, 0xF8, 
  0x38, 0x1C, 0x70, 0x0E, 0x67, 0xE6, 0x0F, 0xF0, 
  0x1C, 0x38, 0x18, 0x18, 0x03, 0xC0, 0x07, 0xE0, 
  0x07, 0xE0, 0x03, 0xC0, 0x00, 0x00, 0x00, 0x00
};
static const uint8_t PROGMEM wifi_off_bmp[] = {
  0xC0, 0x00, 0x60, 0x00, 0x37, 0xE0, 0x1F, 0xF8, 
  0x3C, 0x1C, 0x73, 0x0E, 0x67, 0xC6, 0x0F, 0xF0, 
  0x1C, 0x78, 0x18, 0x38, 0x03, 0xD8, 0x07, 0xEC, 
  0x07, 0xE6, 0x03, 0xC3, 0x00, 0x01, 0x00, 0x00
};

// Telegram icon bitmaps (16x16 pixels) - Paper Plane
static const uint8_t PROGMEM tg_on_bmp[] = {
  0x00, 0x03, 0x00, 0x07, 0x00, 0x0F, 0x00, 0x1F, 
  0x00, 0x3F, 0x00, 0x7F, 0x7F, 0xFF, 0x3F, 0xFF, 
  0x1F, 0xDF, 0x03, 0x8F, 0x03, 0x07, 0x03, 0x03, 
  0x03, 0x01, 0x03, 0x18, 0x03, 0xF0, 0x01, 0xC0
};
static const uint8_t PROGMEM tg_off_bmp[] = {
  0xC0, 0x03, 0x60, 0x07, 0x30, 0x0F, 0x18, 0x1F, 
  0x0C, 0x3F, 0x06, 0x7F, 0x7F, 0xFF, 0x3F, 0xFF, 
  0x1F, 0xFF, 0x03, 0xBF, 0x03, 0x1F, 0x03, 0x0B, 
  0x03, 0x05, 0x03, 0x1A, 0x03, 0xF1, 0x01, 0xC0
};

// --- Відображення головного вікна ---
static void display_main() {
  int16_t x1, y1;
  uint16_t w, h;

  // --- 1. Роздільна лінія (Y=21): статус реле ---
  // Використовуємо Start_status: 1 = включено, 0 = виключено
  for (int x = 0; x < 64; x += 4) {
    if (Start_status == 1) {
      display.drawFastHLine(x, 21, 3, WHITE); // суцільні штрихи — ON
    } else {
      display.drawPixel(x, 21, WHITE);        // крапки — OFF
    }
  }

  // --- 2. Верхня частина: Температура ---
  display.setFont();      // Повертаємо строгий піксельний шрифт (дефолтний)
  display.setTextSize(2); // Розмір 2 (10x14 пікселів)
  String topText;
  if (sensorType == S_NONE) {
    topText = "none";
  } else if (Temperature <= -100.0f) {
    topText = "N/A"; // DS18B20 відсутній
  } else {
    topText = String(Temperature, 1);
  }
  display.getTextBounds(topText, 0, 0, &x1, &y1, &w, &h);
  // Дефолтний шрифт малюється від верхнього лівого кута (Y - це верхня межа)
  display.setCursor((64 - w) / 2, 0); 
  display.print(topText);

  // --- 3. Нижня частина: Вологість або Час ---
  display.setFont(&TomThumb); // Строгий піксельний міні-шрифт (3x5 пікселів)
  display.setTextSize(2);     // Збільшуємо його в 2 рази = 6x10 пікселів!
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
  // Базова лінія (Y=35), щоб центрирувати між Y=22 та Y=39
  display.setCursor((64 - w) / 2, 35);
  display.print(bottomText);

  // Скидаємо шрифт назад до дефолтного для наступних екранів
  display.setFont();

  // --- 4. Нижні кути: іконки WiFi (Ліво) + TG (Право) ---
  bool wifiOk = (WiFi.status() == WL_CONNECTED);

  // WiFi: Лівий нижній кут (X=0, Y=32, розмір 16x16)
  if (wifiOk) {
    display.drawBitmap(0, 32, wifi_on_bmp, 16, 16, WHITE);
  } else {
    display.drawBitmap(0, 32, wifi_off_bmp, 16, 16, WHITE);
  }

  // Telegram: Правий нижній кут (X=48, Y=32, розмір 16x16)
  if (wifiOk && tg_connected) {
    display.drawBitmap(48, 32, tg_on_bmp, 16, 16, WHITE);
  } else {
    display.drawBitmap(48, 32, tg_off_bmp, 16, 16, WHITE);
  }
}

// --- Відображення вікна IP адреси ---
static void display_ip_page() {
  int16_t x1, y1;
  uint16_t w, h;

  String ip = WiFi.localIP().toString();

  // Розбиваємо IP на 2 рядки для великого шрифту
  int dotIdx = ip.indexOf('.', ip.indexOf('.') + 1);
  String part1 = ip.substring(0, dotIdx + 1);
  String part2 = ip.substring(dotIdx + 1);

  display.setFont(&TomThumb);
  display.setTextSize(2);

  // Рядок 1 (висота 10px, базова лінія Y=20)
  display.getTextBounds(part1, 0, 0, &x1, &y1, &w, &h);
  display.setCursor((64 - w) / 2, 20);
  display.print(part1);

  // Рядок 2 (висота 10px, базова лінія Y=40)
  display.getTextBounds(part2, 0, 0, &x1, &y1, &w, &h);
  display.setCursor((64 - w) / 2, 40);
  display.print(part2);

  display.setFont(); // Скидання шрифту
}

// --- Відображення QR-коду ---
#ifdef USE_QR_CODE
#include <qrcode.h>
static void display_qr_page() {
  // Використовуємо лише IP (без "http://"), щоб влізти у Версію 1 (макс 17 байтів).
  // Це дозволить зменшити розмір матриці з 25x25 (v2) до 21x21 (v1).
  String url = WiFi.localIP().toString(); 
  
  QRCode qrcode;
  uint8_t qrcodeBytes[qrcode_getBufferSize(1)];
  qrcode_initText(&qrcode, qrcodeBytes, 1, ECC_LOW, url.c_str());

  // qrcode.size = 21 для версії 1
  // Масштаб: 2 пікселі/модуль (42x42 пікселі → ідеально вміщується в 64x48)
  // Центруємо: offsetX = (64-42)/2=11, offsetY = (48-42)/2=3
  int scale = 2;
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
