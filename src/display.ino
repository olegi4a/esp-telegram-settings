// (Шрифти FreeFonts прибрані, використовується строгий класичний шрифт)
#include <Fonts/TomThumb.h>

// WiFi icon bitmaps (10x10 pixels)
static const uint8_t PROGMEM wifi_on_bmp[] = {
  0b00000000, 0b00000000,
  0b00111100, 0b00000000,
  0b01000010, 0b00000000,
  0b10011001, 0b00000000,
  0b00100100, 0b00000000,
  0b01011010, 0b00000000,
  0b00000000, 0b00000000,
  0b00011000, 0b00000000,
  0b00011000, 0b00000000,
  0b00000000, 0b00000000
};
static const uint8_t PROGMEM wifi_off_bmp[] = {
  0b10000000, 0b01000000,
  0b01111100, 0b00000000,
  0b00100010, 0b00000000,
  0b10011001, 0b00000000,
  0b00100100, 0b00000000,
  0b01011010, 0b00000000,
  0b00000100, 0b00000000,
  0b00011010, 0b00000000,
  0b00011001, 0b00000000,
  0b00000000, 0b01000000
};

// Telegram icon bitmaps (10x10 pixels) - Paper Plane
static const uint8_t PROGMEM tg_on_bmp[] = {
  0b00000000, 0b11000000,
  0b00000001, 0b10000000,
  0b00000011, 0b00000000,
  0b00000110, 0b00000000,
  0b10001100, 0b00000000,
  0b11011111, 0b00000000,
  0b01110100, 0b00000000,
  0b00100100, 0b00000000,
  0b00000100, 0b00000000,
  0b00000100, 0b00000000
};
static const uint8_t PROGMEM tg_off_bmp[] = {
  0b10000000, 0b11000000,
  0b01000001, 0b10000000,
  0b00100011, 0b00000000,
  0b00010110, 0b00000000,
  0b10001100, 0b00000000,
  0b11011011, 0b00000000,
  0b01110101, 0b00000000,
  0b00100100, 0b10000000,
  0b00000100, 0b01000000,
  0b00000100, 0b00100000
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

  // WiFi: Лівий нижній кут (X=0, Y=38)
  if (wifiOk) {
    display.drawBitmap(0, 38, wifi_on_bmp, 10, 10, WHITE);
  } else {
    display.drawBitmap(0, 38, wifi_off_bmp, 10, 10, WHITE);
  }

  // Telegram: Правий нижній кут (X=54, Y=38)
  if (wifiOk && tg_connected) {
    display.drawBitmap(54, 38, tg_on_bmp, 10, 10, WHITE);
  } else {
    display.drawBitmap(54, 38, tg_off_bmp, 10, 10, WHITE);
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
