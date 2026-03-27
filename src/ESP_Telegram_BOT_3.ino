#include "set.h"

DNSServer dnsServer;

void setup()
{
  pinMode(LED_STATUS, OUTPUT);
  pinMode(LED_BOOTON, OUTPUT);
  pinMode(RELE, OUTPUT);
  pinMode(BUTTON, INPUT);
  pinMode(POWER, INPUT);  // Моніторинг 220В (переривання або USB-detect)

  digitalWrite(RELE, LOW);
  digitalWrite(LED_STATUS, LOW); // Було HIGH
  digitalWrite(LED_BOOTON, HIGH);
  
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);  display.clearDisplay();
  display.setTextColor(WHITE);
  
  // Logo "EDwIC" centered
  display.setTextSize(2);
  int16_t x1, y1;
  uint16_t w, h;
  display.getTextBounds("EDwIC", 0, 0, &x1, &y1, &w, &h);
  display.setCursor((display.width() - w) / 2, 5);
  display.print("EDwIC");

  // Version number centered horizontally, Y=23
  String fullVer = FIRMWARE_VERSION;
  String shortVer = (fullVer.indexOf('-') != -1) ? fullVer.substring(fullVer.indexOf('-') + 1) : fullVer;
  display.setTextSize(1);
  display.getTextBounds(shortVer, 0, 0, &x1, &y1, &w, &h);
  display.setCursor((display.width() - w) / 2, 23);
  display.print(shortVer);

  display.display();
  delay(2000);
  
  digitalWrite(LED_STATUS, HIGH); // Було LOW
  digitalWrite(LED_BOOTON, LOW);
  display.clearDisplay();
  display.display();

  Serial.begin(115200);

  TBLOG_LN(" ");
  FS_INIT();

  Logger_init();
  Load_Config();
  
  WebServer_Init();
  sensor_init();

  PowerMonitor_init();
  
  switch(BUTTON_START())
  {
    case 1:
      {
        WIFI_AP_MODE();
        break;
      }
    case 2:
      {
        digitalWrite(LED_STATUS, LOW);
        display.clearDisplay();
        display.setTextColor(WHITE);
        display.setCursor(2,1);
        display.setTextSize(1);
        display.println("   press");
        display.println(" button to");
        display.println("  default");
        display.println("  setting");
        display.display();
        while (digitalRead(BUTTON) == HIGH)
        {
          delay(500);
          digitalWrite(LED_STATUS, !digitalRead(LED_STATUS));
        }
        LittleFS.remove("/Config.json");
        LittleFS.remove("/N_profile.json");
        LittleFS.remove("/D_profile.json");
        Logger_addEntry(12); // 12 = Скидання налаштувань
        Logger_flushToFile();
        delay(1000);
        ESP.restart();
        break;
      }
    default:
      {
        WiFi_Init();
      }
  }

  // --- СИНХРОНІЗАЦІЯ ЧАСУ ---
  if(WiFi.getMode() != WIFI_AP) {
    TBLOG_LN("Starting NTP Sync");
    configTime(timezone_str.c_str(), ntpServerName, ntpServerName2);
    time_t nowTimeRead = time(nullptr);
    byte timeout = 0;
    while (nowTimeRead < 1500000000 && timeout < 50) {
      delay(200); TBLOG("."); nowTimeRead = time(nullptr); timeout++;
    }
    if (nowTimeRead >= 1500000000) {
      setTime(nowTimeRead);
      setSyncProvider(getNtpTime);
      setSyncInterval(3600);
      TBLOG_LN(F("Boot: NTP OK"));
      Logger_addEntry(13); // 13 = Час синхронізовано
    }
  }

  // 2. Fallback до останнього збереженого часу (якщо NTP не спрацював АБО ми в AP)
  if (time(nullptr) < 1500000000) {
    TBLOG_LN(F("Boot: No NTP, trying history.json fallback..."));
    uint32_t lastTs = History_loadLastTimestamp();
    if (lastTs > 1500000000) {
      time_t estimated = (time_t)(lastTs + (millis() / 1000));
      setTime(estimated);
      TBLOG(F("Boot: estimated time from history: ")); TBLOG_LN((long)estimated);
    }
  }

  // --- ЗАГАЛЬНІ НАЛАШТУВАННЯ ТА ПРОФІЛІ ---
  time_t t_now = time(nullptr);
  struct tm *tm_info = localtime(&t_now);
  Time_now = (tm_info->tm_hour * 3600) + (tm_info->tm_min * 60);

  // Завантажуємо ОБИДВА профілі, щоб у пам'яті були дані для обох (для WebApp)
  Load_Profile("/D_profile.json");
  Load_Profile("/N_profile.json");

  if(Time_D < Time_now && Time_N > Time_now) {
     TBLOG_LN(" Day profile active ");
     Load_Profile("/D_profile.json"); // Активуємо денний
     profile = 1;
  } else {
     TBLOG_LN(" Night profile active ");
     Load_Profile("/N_profile.json"); // Активуємо нічний
     profile = 0;
  }

  // --- ТЕЛЕГРАМ (Тільки для STA режиму) ---
  if (WiFi.getMode() != WIFI_AP) {
    myBot.setToken(TB_Token);
    myBot.setTimeout(8500);
    myBot.client.setBufferSizes(3072, 512);
    myBot.onUpdate(Telegram_Callback);
    
    if (TB_Token.length() > 10) {
      HTTPClient http;
      String url = String(F("https://api.telegram.org/bot")) + TB_Token + F("/getMe");
      http.begin(myBot.client, url);
      int code = http.GET();
      if (code == 200) {
        String body = http.getString();
        const char* uKey = "\"username\":\"";
        int uIdx = body.indexOf(uKey);
        if (uIdx >= 0) {
          int start = uIdx + strlen(uKey);
          int end = body.indexOf('"', start);
          if (end > start) botName = "@" + body.substring(start, end);
        }
        tg_connected = true;
      }
      http.end();
      tg_last_check = millis();
    }
  }

  if(Start_status == 1) digitalWrite(RELE, HIGH);
  else digitalWrite(RELE, LOW);
  digitalWrite(LED_BOOTON, (Statatus_sensor_control == 0));
  if(Alarm_start == 1)
  {
    fb::Message startMsg(F("Бот запущено 🚀"), alluser);
    startMsg.notification = false; // Беззвучне сповіщення
    myBot.sendMessage(startMsg);
  }
  Alarm_data_milis = millis() + 60000;
  Logger_addEntry(4); // 4 = Запуск пристрою

  // Відображення "UPDATE" на OLED під час OTA оновлення
  Update.onStart([]() {
    TBLOG_LN(F("OTA Update Started!"));
    display.clearDisplay();
    display.setFont(&TomThumb);
    display.setTextSize(2);
    // Центрування "UPDATE" (~48px) та "..." (~24px) для екрану 64х48
    display.setCursor(8, 20);
    display.print(F("UPDATE"));
    display.setCursor(20, 40);
    display.print(F("..."));
    display.display();
  });
}

void loop()
{
  unsigned long currentMillis = millis();

  // --- Auto AP Logic ---
  static unsigned long wifi_disconnected_ms = 0;
  static bool first_loop_check = true;
  
  if (WiFi.status() == WL_CONNECTED) {
    first_loop_check = false;
    if (wifi_disconnected_ms != 0) {
      wifi_disconnected_ms = 0; // Скидання таймера
      if (WiFi.getMode() == WIFI_AP_STA) {
        if (!wifi_test_active) {
          TBLOG_LN(F("WiFi restored. Disabling Auto-AP."));
          WiFi.softAPdisconnect(true);
          WiFi.mode(WIFI_STA);
        }
      }
    }
  } else {
    if (WiFi.getMode() != WIFI_AP) {
      if (wifi_disconnected_ms == 0) wifi_disconnected_ms = currentMillis;
      
      bool triggerAP = false;
      if (first_loop_check) {
        triggerAP = true; // Відсутній при завантаженні - AP відразу
        first_loop_check = false;
      } else if (currentMillis - wifi_disconnected_ms >= 120000) {
        triggerAP = true; // Зник більше ніж на 2 хв
      }

      if (WiFi.getMode() == WIFI_STA && triggerAP) {
        TBLOG_LN(F("WiFi lost. Auto-AP started."));
        WiFi.mode(WIFI_AP_STA);
        String chipId = WiFi.macAddress();
        chipId.replace(":", "");
        String ap_ssid = "ESP-" + chipId.substring(chipId.length() - 6);
        WiFi.softAP(ap_ssid.c_str(), "", 4, false, 5);
        dnsServer.start(DNS_PORT, "*", WiFi.softAPIP());
      }
    }
  }

  // LED Status Indication (Блимає якщо AP або втрачено зв'язок)
  static unsigned long lastLEDBlink = 0;
  static bool ledState = false;
  if (WiFi.getMode() == WIFI_AP || (WiFi.getMode() == WIFI_AP_STA && WiFi.status() != WL_CONNECTED)) {
    if (currentMillis - lastLEDBlink >= 500) {
      lastLEDBlink = currentMillis;
      ledState = !ledState;
      digitalWrite(LED_STATUS, ledState);
    }
  } else {
    digitalWrite(LED_STATUS, (WiFi.status() == WL_CONNECTED));
  }

  // Обробка Telegram. Використовуємо сам tick() як інструмент перевірки з'єднання!
  static unsigned long last_tick = 0;
  if (WiFi.getMode() != WIFI_AP && WiFi.status() == WL_CONNECTED) {
    // Якщо онлайн - викликаємо постійно (для миттєвого прийому повідомлень).
    // Якщо офлайн - викликаємо раз на 10 секунд (щоб перевірити, чи не з'явився інтернет).
    if (tg_connected || millis() - last_tick >= 10000) {  
      last_tick = millis();
      unsigned long start_t = millis();
      
      myBot.tick(); // Якщо інтернет є - виконується швидше. Немає - таймаут ~10с.
      
      if (millis() - start_t > 9000) {
        if (tg_connected) TBLOG_LN("TG offline (tick timeout)");
        tg_connected = false;
      } else {
        if (!tg_connected) TBLOG_LN("TG online (tick fast)");
        tg_connected = true;
      }
    }
  } else {
    tg_connected = false; // Немає WiFi - немає Telegram
  }

  // Отримуємо ім'я бота для Web-інтерфейсу (тільки якщо ми в онлайні і ім'я ще не отримане)
  if (tg_connected && botName.isEmpty() && millis() - tg_last_check >= 10000) {
    tg_last_check = millis();
    HTTPClient http;
    http.setTimeout(8000); 
    String url = String(F("https://api.telegram.org/bot")) + TB_Token + F("/getMe");
    http.begin(myBot.client, url); 
    if (http.GET() == 200) {
      String body = http.getString();
      const char* uKey = "\"username\":\"";
      int uIdx = body.indexOf(uKey);
      if (uIdx >= 0) {
        int start = uIdx + strlen(uKey);
        int end = body.indexOf('"', start);
        if (end > start) botName = "@" + body.substring(start, end);
      }
    }
    http.end();
  }

  // Відкладені дії після tick() — FastBot2 вже закрив своє SSL-з'єднання
  if (pendingRegisterCommands) {
    pendingRegisterCommands = false;
    registerBotCommands(pendingRegisterSender);
  }

  // [КРИТИЧНО] Перевірка збою живлення (прапор від ISR)
  PowerMonitor_handle();

  // Логер: щохвилинний delta-check та щогодинний запис у флеш
  Logger_minuteTick();
  Logger_periodicTick();

    // Timer 0: сенсори та дисплей (5 секунд)
    if (currentMillis - timer0_last >= TIMER0_INTERVAL) {
      timer0_last = currentMillis;
      sensor_read();
      display_loop();
      
      // Спеціальна обробка перезавантаження за таймером
      if(RESTART > 1) {
        RESTART--;
        if (RESTART == 1) {
          Logger_flushToFile();
          ESP.restart();
        }
      }
    }

  // Timer 1: widget (5 хвилин)
  if (currentMillis - timer1_last >= TIMER1_INTERVAL) {
    timer1_last = currentMillis;
    widget();
  }

    // Timer 2: Client_loop (якщо share_sensor == 1) - ВИДАЛЕНО

  WebServer.handleClient();
  yield();

  time_t t = time(nullptr);
  struct tm *tm_info = localtime(&t);
  Time_now = (tm_info->tm_hour * 3600) + (tm_info->tm_min * 60);

  if(!((Time_D < Time_now && Time_N > Time_now && profile == 1) || (Time_D > Time_now && profile == 0) || (Time_N < Time_now && profile == 0) || (WiFi.getMode() == WIFI_AP)))
  {
    RESTART = 3;
  }
  
  if(WiFi.getMode() == WIFI_AP || WiFi.getMode() == WIFI_AP_STA) {
    dnsServer.processNextRequest();
  }
  
  // [ВИДАЛЕНО] Старий polling-based digitalRead(POWER) — замінено ISR в PowerMonitor.ino
  
  if(digitalRead(BUTTON) == LOW)
  {
    unsigned long btnPressStart = millis();
    bool longPressTriggered = false;
    
    while(digitalRead(BUTTON) == LOW) {
      if(!longPressTriggered && (millis() - btnPressStart > 2500)) {
        // Довгий натиск: Перемикання між Ручним (0) та Сенсором (1)
        if (Statatus_sensor_control != 0) Statatus_sensor_control = 0;
        else Statatus_sensor_control = 1;
        
        digitalWrite(LED_BOOTON, (Statatus_sensor_control == 0));
        Logger_addEntry(14 + Statatus_sensor_control); // 14=Ручний, 15=Сенсор
        myBot.sendMessage(fb::Message("Режим автоматики змінено через кнопку 🔘\n⚠️ Не забудьте зберегти!", alluser));
        longPressTriggered = true;
      }
      yield();
    }
    
    if(!longPressTriggered && (millis() - btnPressStart > 50)) { 
      if(Statatus_sensor_control == 0)
      {
        // Ручний режим: перемикання реле
        digitalWrite(RELE, !digitalRead(RELE));
        fb::Message relMsg("Статус реле змінено кнопкою ✅", alluser);
        relMsg.notification = false; // Беззвучне
        myBot.sendMessage(relMsg);
      } else {
        // Автоматичний режим: перемикання сторінки дисплея
        display_page = (display_page + 1) % 3;
        display_loop(); // Відобразити нову сторінку одразу
      }
    }
  }
  
  // Автоматика по датчику
  if (Statatus_sensor_control == 1)
  {
    if (Temperature >= Sensor_set + Sensor_histeresis) digitalWrite(RELE, Rele_status);
    else if(Temperature <= Sensor_set - Sensor_histeresis) digitalWrite(RELE, !Rele_status);
  }

  // Автоматика по часу (циклічний таймер)
  if (Statatus_sensor_control == 2)
  {
    if (now() >= now_Time)
    {
      if(now_Time_off_on == 0)
      {
        digitalWrite(RELE, HIGH);
        now_Time_off_on = 1;
        now_Time = now() + (Time_on * 60);
      }
      else
      {
        digitalWrite(RELE, LOW);
        now_Time_off_on = 0;
        now_Time = now() + (Time_off * 60);
      }
    }
  }
  
  if(Alarm_data_set == 1  && millis() > Alarm_data_milis)
  {
    if(Temperature > Alarm_data_u || Temperature < Alarm_data_d)
    {
      Alarm_data_milis = millis() + 300000;
      
      if(!(trend == 1 && ((Alarm_data_d > Temperature && Alarm_Temperature < Temperature) ||  (Alarm_data_u < Temperature && Alarm_Temperature > Temperature))))
      {
        // Спочатку надсилаємо повну статистику
        sendBasicData(alluser);
        
        // Потім коротке повідомлення тривоги з кнопками
        fb::Message m(F("🚨 Повідомлення тривоги‼️"), alluser);
        m.setModeHTML();
        
        // Оновлені назви кнопок
        fb::InlineMenu menu(F("🔕 Вимкнути (тимчасово);🔔 Увімкнути (тимчасово)"), "10;11");
        m.setInlineMenu(menu);
        myBot.sendMessage(m);
        Logger_addEntry(10); 
      }
    }
    Alarm_Temperature = Temperature;
  }

  // Відстеження зміни статусу реле для сповіщень та ЖУРНАЛУ
  bool current_rele = digitalRead(RELE);
  if (current_rele != last_rele_state) {
    Logger_addEntry(1); // Фіксація в журналі для будь-якої зміни (автоматика, кнопка, ТГ)
    
    if (relay_change_notify) {
      String msg = "🔄 Статус реле змінено: " + String(current_rele ? "УВІМКНЕНО ✅" : "ВИМКНЕНО ❌");
      fb::Message relNotify(msg, alluser);
      relNotify.notification = false; // Беззвучне сповіщення про зміну реле
      myBot.sendMessage(relNotify);
    }
    last_rele_state = current_rele;
  }
}
