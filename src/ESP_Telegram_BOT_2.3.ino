#include "set.h"

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
  
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);  
  display.display();
  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setCursor(3,11);
  display.setTextSize(2);
  display.println("EDwIC");
  display.setTextSize(1);
  display.setCursor(23,30);
  display.println("3.4.2");
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
  // BME_INIT(); // Видалено: вже викликано всередині sensor_init()

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

  if(WiFi.getMode() != WIFI_AP)
  { 
    display.clearDisplay();
    display.setTextColor(WHITE);
    display.setCursor(2,1);
    display.println("  MY IP:  ");
    display.println(WiFi.localIP());
    display.display();
  
    TBLOG(F("WIFI mode: "));
    TBLOG_LN(WiFi.getMode());
    TBLOG(F("WiFi connected: "));
    TBLOG_LN(WiFi.localIP());

    delay(2000);

    TBLOG_LN("Starting NTP Sync");
    // NTP + POSIX timezone (handles DST automatically)
    configTime(timezone_str.c_str(), ntpServerName, ntpServerName2);

    // NTP має ПРІОРИТЕТ — чекаємо до 10 секунд
    TBLOG("Waiting for NTP time");
    time_t nowTimeRead = time(nullptr);
    byte timeout = 0;
    while (nowTimeRead < 1500000000 && timeout < 50) {
      delay(200);
      TBLOG(".");
      nowTimeRead = time(nullptr);
      timeout++;
    }
    TBLOG_LN("");

    if (nowTimeRead >= 1500000000) {
      // NTP синхронізовано — точний час
      setTime(nowTimeRead);
      setSyncProvider(getNtpTime);
      setSyncInterval(3600);
      TBLOG_LN(F("Boot: NTP OK"));
      Logger_addEntry(13); // 13 = Час синхронізовано
    } else {
      // NTP недоступний — fallback по history.json
      TBLOG_LN(F("Boot: NTP FAILED, trying history.json fallback..."));
      uint32_t lastTs = History_loadLastTimestamp();
      if (lastTs > 1500000000) {
        time_t estimated = (time_t)(lastTs + (millis() / 1000));
        setTime(estimated);
        TBLOG(F("Boot: estimated time from history: ")); TBLOG_LN((long)estimated);
      } else {
        TBLOG_LN(F("Boot: No history. Night profile default."));
      }
    }

    time_t t = time(nullptr);
    struct tm *tm_info = localtime(&t);
    Time_now = (tm_info->tm_hour * 3600) + (tm_info->tm_min * 60);

    // Завантажуємо ОБИДВА профілі, щоб у пам'яті були дані для обох (для WebApp)
    Load_Profile("/D_profile.json");
    Load_Profile("/N_profile.json");

    if(Time_D < Time_now && Time_N > Time_now)
    {
       TBLOG_LN(" day profile active ");
       Load_Profile("/D_profile.json"); // Активуємо денний
       profile = 1;
    }
    else
    {
       TBLOG_LN(" Night profile active ");
       Load_Profile("/N_profile.json"); // Активуємо нічний (вже завантажений, але для логіки)
       profile = 0;
    }
    
    // Встановлення токену Telegram та колбеку
    myBot.setToken(TB_Token);
    myBot.client.setBufferSizes(3072, 512); // Збільшуємо вхідний(recv) буфер до 3072, а вихідний(tx) залишаємо базовими 512 для економії пам'яті
    myBot.onUpdate(Telegram_Callback);
  }

  if(Start_status == 1) digitalWrite(RELE, HIGH);
  else digitalWrite(RELE, LOW);
  digitalWrite(LED_BOOTON, (Statatus_sensor_control == 0));
  if(Alarm_start == 1)
  {
    myBot.sendMessage(fb::Message(F("Бот запущено 🚀"), alluser));
  }
  Alarm_data_milis = millis() + 60000;
  Logger_addEntry(4); // 4 = Запуск пристрою
}

void loop()
{
  unsigned long currentMillis = millis();

  // Обробка Telegram (неблокуюча)
  myBot.tick();

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
        digitalWrite(RELE, !digitalRead(RELE));
        myBot.sendMessage(fb::Message("Статус реле змінено кнопкою ✅", alluser));
      } else {
        myBot.sendMessage(fb::Message("⚠️ Вимкніть автоматику для ручного керування кнопкою!", alluser));
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
      myBot.sendMessage(fb::Message(msg, alluser));
    }
    last_rele_state = current_rele;
  }
}
