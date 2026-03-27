// (Шрифти та бітмапи оновлено)


// --- Відображення головного вікна ---
static void display_main() {
  int16_t x1, y1;
  uint16_t w, h;

  // --- 1. Роздільна лінія (Y=21): статус реле ---
  // Показуємо РЕАЛЬНИЙ стан заліза:
  bool actualRelay = digitalRead(RELE);
  for (int x = 0; x < 64; x += 4) {
    if (actualRelay) {
      display.drawFastHLine(x, 21, 5, WHITE); // суцільні штрихи — ON
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
    topText = "none"; // DS18B20 відсутній
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

  // --- 4. Нижні кути: WiFi bars (Ліво) + TG text (Право) ---
  bool wifiOk = (WiFi.status() == WL_CONNECTED);
  
  // WiFi Bars: Лівий ніжній кут (Y=47)
  if (wifiOk) {
    int32_t rssi = WiFi.RSSI();
    int bars = 0;
    if (rssi > -55)      bars = 4;
    else if (rssi > -65) bars = 3;
    else if (rssi > -75) bars = 2;
    else                 bars = 1;

    for (int i = 0; i < 4; i++) {
        int barH = (i + 1) * 2 + 1; // 3, 5, 7, 9 px
        if (i < bars) {
            display.fillRect(i * 3, 48 - barH, 2, barH, WHITE);
        } else {
            display.drawRect(i * 3, 48 - barH, 2, barH, WHITE); // контур для порожніх паличок
        }
    }
  } else {
    // Не підключено — показуємо AP або X
    display.setFont(&TomThumb);
    display.setTextSize(2);
    if (WiFi.getMode() == WIFI_AP || WiFi.getMode() == WIFI_AP_STA) {
      display.setCursor(0, 47);
      display.print("AP");
    } else {
      display.setCursor(0, 47);
      display.print("X");
    }
    display.setFont();
  }

  // Telegram: Правий нижній кут
  display.setFont(&TomThumb);
  display.setTextSize(2);
  if (wifiOk && tg_connected) {
    display.setCursor(47, 47); // Підігнано під край екрану 64х48
    display.print("TG");
  } else {
    display.setCursor(56, 47);
    display.print("X");
  }
  display.setFont(); // Скидання для інших екранів
}

// --- Відображення вікна IP адреси ---
void display_ip_page() {
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
