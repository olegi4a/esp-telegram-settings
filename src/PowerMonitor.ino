// ============================================================
//  PowerMonitor.ino
//  Моніторинг живлення 220В через переривання GPIO12 (POWER pin)
//  ISR мінімальна — лише виставляє volatile прапор.
//  Важка робота виконується в loop() через PowerMonitor_handle().
// ============================================================

volatile bool powerFailFlag = false;   // Прапор від ISR
bool          is_usb_mode   = false;   // true = живлення від USB, ігнорувати POWER пін

// ISR: ОБОВ'ЯЗКОВО ICACHE_RAM_ATTR, виконується з RAM — швидко і безпечно
void IRAM_ATTR onPowerFail() {
  powerFailFlag = true;
}

// ============================================================
//  Виклик ОДИН РАЗ в setup() — після затримки старту
//  Якщо пін LOW вже при старті → пристрій підключено по USB
// ============================================================
void PowerMonitor_init() {
  // Невелика пауза щоб пін стабілізувався після ввімкнення
  delay(100);

  if (digitalRead(POWER) == LOW) {
    // Пін LOW при старті → живлення USB (немає 220В)
    is_usb_mode = true;
    TBLOG_LN(F("PowerMonitor: USB mode detected. Power interrupt DISABLED."));
    return;
  }

  // Живлення від 220В — підключаємо переривання по спадаючому фронту
  attachInterrupt(digitalPinToInterrupt(POWER), onPowerFail, FALLING);
  TBLOG_LN(F("PowerMonitor: 220V mode. Interrupt on GPIO12 ENABLED."));
}

// ============================================================
//  Виклик ПЕРШИМ РЯДКОМ в loop()
//  Якщо прапор виставлений ISR — виконуємо Graceful Shutdown
// ============================================================
void PowerMonitor_handle() {
  if (!powerFailFlag || is_usb_mode) return;

  // Скидаємо прапор щоб не потрапити сюди знову (іоністор живить ще)
  powerFailFlag = false;

  TBLOG_LN(F("PowerMonitor: POWER FAIL! Starting graceful shutdown..."));

  // 1. Негайно знімаємо навантаження → збільшуємо час роботи іоністора
  digitalWrite(LED_STATUS, LOW);
  digitalWrite(LED_BOOTON, LOW);
  display.clearDisplay();
  display.display();
  // Примітка: реле НЕ вимикаємо тут автоматично (це може бути критичним
  // для обладнання, яке управляється). Реле вимкнеться тільки якщо
  // Start_status поточного профілю = 0 після наступного перезавантаження.

  // 2. Скидаємо RAM-буфер логів у флеш-пам'ять (зберігаємо точний момент збою)
  Logger_emergencyFlush(true); // true = це збій живлення

  // 3. Відправляємо сповіщення в Telegram (якщо WiFi ще живий)
  if (Alarm_power && WiFi.status() == WL_CONNECTED) {
    String statusMsg  = F("⚡ Зникнення живлення 220В!\n");
    statusMsg += F("🌡 Температура: "); statusMsg += String(Temperature, 1); statusMsg += F("°C\n");
    statusMsg += F("💧 Вологість: ");   statusMsg += String(Humedity);       statusMsg += F("%\n");
    statusMsg += F("🔌 Реле: ");        statusMsg += String(digitalRead(RELE) ? F("УВІМК") : F("ВИМК"));
    myBot.sendMessage(fb::Message(statusMsg, alluser));
    // Кілька тіків щоб повідомлення встигло надіслатись за час іоністора
    for (byte i = 0; i < 10; i++) {
      myBot.tick();
      delay(50);
      yield();
    }
  }

  TBLOG_LN(F("PowerMonitor: Shutdown complete. Waiting for power death..."));

  // 4. Чекаємо фізичного знеструмлення від розряду іоністора
  // Якщо живлення відновиться до розряду іоністора, перезавантажуємо пристрій
  while (true) {
    yield();
    // Перевіряємо, чи живлення відновилося (GPIO12 тепер HIGH)
    if (digitalRead(POWER) == HIGH) {
      TBLOG_LN(F("PowerMonitor: Power restored, restarting device..."));
      ESP.restart();
    }
  }
}
