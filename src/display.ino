void display_loop(void)
{ 
  // Збереження серцебиття (інверсія)
  display_inv = !display_inv;
  display.invertDisplay(!display_inv);

  display.clearDisplay();
  display.setTextColor(WHITE);

  // --- 1. Роздільна лінія посередині (Y = 24) ---
  // Штрихпунктирна лінія, що відображає статус реле
  // Якщо реле ON - суцільна або довгі штрихи, якщо OFF - дрібний пунктир
  bool releState = digitalRead(RELE);
  for (int x = 0; x < 64; x += 4) {
    if (releState) {
      display.drawFastHLine(x, 24, 3, WHITE); // Довші штрихи для ON
    } else {
      display.drawPixel(x, 24, WHITE); // Крапки для OFF
    }
  }

  int16_t x1, y1;
  uint16_t w, h;

  // --- 2. Верхня частина: Температура або "none" ---
  display.setTextSize(2);
  String topText = (sensorType == S_NONE) ? "none" : String(Temperature, 1);
  display.getTextBounds(topText, 0, 0, &x1, &y1, &w, &h);
  // Центрування: X = (64 - w)/2, Y = 12 (центр верхньої половини) - h/2
  display.setCursor((64 - w) / 2, 12 - h / 2);
  display.print(topText);

  // --- 3. Нижня частина: Вологість або Час ---
  display.setTextSize(2);
  String bottomText;
  if (sensorType == S_BME280 || sensorType == S_HTU21) {
    bottomText = String((int)Humedity) + "%";
  } else {
    // Форматування часу ГГ:ХВ
    char timeStr[6];
    time_t t = time(nullptr);
    struct tm *tm_info = localtime(&t);
    sprintf(timeStr, "%02d:%02d", tm_info->tm_hour, tm_info->tm_min);
    bottomText = String(timeStr);
  }
  
  display.getTextBounds(bottomText, 0, 0, &x1, &y1, &w, &h);
  // Центрування: X = (64 - w)/2, Y = 36 (центр нижньої половини) - h/2
  display.setCursor((64 - w) / 2, 36 - h / 2);
  display.print(bottomText);

  display.display();
}
