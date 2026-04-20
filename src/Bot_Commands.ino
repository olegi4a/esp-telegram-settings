void Telegram_Callback(fb::Update& update)
{
  // \u0411\u0443\u0434\u044c-\u044f\u043a\u0430 \u0430\u043a\u0442\u0438\u0432\u043d\u0456\u0441\u0442\u044c \u0432\u0456\u0434 Telegram \u2014 \u0437\u043d\u0430\u0447\u0438\u0442\u044c TG online
  tg_connected = true;
  tg_last_check = millis();
  TBLOG("Update: "); TBLOG_LN((int)update.type());
  fb::MessageRead msg = update.message();
  int64_t senderId = msg.from().id().toInt64();

  // 1. Handling Callback Queries (Inline Buttons)
  if (update.isQuery()) {
    myBot.answerCallbackQuery(update.query().id(), "Готово");
    String data = update.query().data().toString();
    
    if (data == "1" || data == "on") {
      if (Statatus_sensor_control == 0) {
        digitalWrite(RELE, HIGH);
        Rele_status = 1;
        myBot.sendMessage(fb::Message(F("✅ Увімкнуто!"), senderId));
      } else {
        myBot.sendMessage(fb::Message(F("⚠️ Помилка: Увімкнено автоматичний режим!"), senderId));
      }
    } else if (data == "0" || data == "off") {
      if (Statatus_sensor_control == 0) {
        digitalWrite(RELE, LOW);
        Rele_status = 0;
        myBot.sendMessage(fb::Message(F("❌ Вимкнуто!"), senderId));
      } else {
        myBot.sendMessage(fb::Message(F("⚠️ Помилка: Увімкнено автоматичний режим!"), senderId));
      }
    } else if (data == "11") {
      Alarm_data_set = 1;
      Logger_addEntry(2);
      myBot.sendMessage(fb::Message(F("🔔 Повідомлення тривоги УВІМКНЕНО (тимчасово)"), senderId));
    } else if (data == "10") {
      Alarm_data_set = 0;
      Logger_addEntry(2);
      myBot.sendMessage(fb::Message(F("🔕 Повідомлення тривоги ВИМКНЕНО (тимчасово)"), senderId));
    } else if (data == "reg_commands") {
      registerBotCommands(senderId);
    } else if (data == "new_prof:D" || data == "new_prof:N") {
      String prof = (data == "new_prof:D") ? F("☀️ Денний") : F("🌙 Нічний");
      myBot.sendMessage(fb::Message(F("📋 Перегляд: ") + prof + F("\nДля зміни параметрів використовуйте 📱 MiniApp"), senderId));
    }
    return;
  }

  // 2. Authorization check
  bool isWebApp = update.isMessage() && update.message().entry.has("web_app_data");

  if (user_find(senderId) == 200 && botState == STATE_IDLE && !isWebApp) {
    if (update.isMessage() && update.message().text() != "/start") {
      myBot.sendMessage(fb::Message(F("❌ Немає доступу. Введіть /start та пароль."), senderId));
      return;
    }
  }

  // 3. WebApp Data processing
  if (isWebApp) {
    String json = update.message().entry["web_app_data"]["data"].toString();
    json.replace("\\\"", "\"");
    json.replace("\"{", "{");
    json.replace("}\"", "}");
    
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, json);
    if (!error) {
      String target = doc["t"] | "D";
      JsonObject s = doc["s"];
      JsonObject g = doc["g"];
      
      if (!g.isNull()) {
        if (!g["Time_D_h"].isNull()) Time_D = g["Time_D_h"].as<int>() * 3600;
        if (!g["Time_N_h"].isNull()) Time_N = g["Time_N_h"].as<int>() * 3600;
        Save_Config();
      }

      if (target == "D" || target == "B") Patch_Profile("/D_profile.json", s);
      if (target == "N" || target == "B") Patch_Profile("/N_profile.json", s);

      if (profile == 1) Load_Profile("/D_profile.json");
      else Load_Profile("/N_profile.json");

      myBot.sendMessage(fb::Message(F("📥 Налаштування збережено!"), senderId));
      myBot.sendMessage(fb::Message(F("🔄 Клавіатуру оновлено"), senderId));
      sendSettingsMenu(senderId);
    }
    return;
  }

  if (!msg.text()) return;
  String text = msg.text().toString();

  // Reset state if home or start
  if (text == "/start" || text == "/home" || text == "Повернутися в меню") {
    botState = STATE_IDLE;
  }

  // Handle states (like password entry)
  if (botState != STATE_IDLE) {
    handleBotState(senderId, text);
    return;
  }

  // 4. Main Commands
  if (text == "/start") {
    if (user_find(senderId) != 200) {
      sendWelcomeMessage(senderId);
      sendMainMenu(senderId);
    } else {
      myBot.sendMessage(fb::Message(F("🔐 Введіть пароль (тільки цифри, макс. 9 знаків):"), senderId));
      botState = STATE_WAIT_AUTH_PASSWORD;
    }
  }
  else if (text == "/home" || text == "/start") {
    sendMainMenu(senderId);
  }
  else if (text == "/settings") {
    sendSettingsMenu(senderId);
  }
  else if (text == "/reboot") {
    myBot.sendMessage(fb::Message(F("🔄 Перезавантаження пристрою (5 сек)..."), senderId));
    Logger_flushToFile();
    RESTART = 5;
  }
  else if (text == "Основні дані") {
    sendBasicData(senderId);
  }
  else if (text == "🌐 Web-інтерфейс") {
    String url = "http://" + WiFi.localIP().toString();
    myBot.sendMessage(fb::Message(F("🌐 Локальна веб-сторінка:\n") + url, senderId));
  }
  else if (text == "🔔 Стати головним (тривоги)") {
    alluser = senderId;
    Save_Config();
    myBot.sendMessage(fb::Message(F("✅ Вас зареєстровано як головного користувача для прийому тривог!"), senderId));
  }
  else if (text == "Ручне керування") {
    Statatus_sensor_control = 0;
    digitalWrite(LED_BOOTON, HIGH);
    fb::Message m(F("⚠️ Ручний режим керування реле:"), senderId);
    fb::InlineMenu menu(F("✅ УВІМК;❌ ВИМК"), "on;off");
    m.setInlineMenu(menu);
    myBot.sendMessage(m);
  }
  else if (text == "Автоматичне керування") {
    restoreAutomationMode(); 
    String modeName = (Statatus_sensor_control == 1) ? F("по датчику 🌡") : F("по таймеру ⏱");
    myBot.sendMessage(fb::Message(F("🤖 Автоматика активована: ") + modeName, senderId));
  }
  else if (text.equalsIgnoreCase("on") || text.equalsIgnoreCase("off")) {
    if (Statatus_sensor_control == 0) {
      bool st = text.equalsIgnoreCase("on");
      digitalWrite(RELE, st);
      Rele_status = st;
      myBot.sendMessage(fb::Message(st ? F("✅ Увімкнуто!") : F("❌ Вимкнуто!"), senderId));
    } else {
      myBot.sendMessage(fb::Message(F("⚠️ Вимкніть автоматику для ручного керування!"), senderId));
    }
  }
  else {
    // Default reply if command not recognized
    // sendMainMenu(senderId); 
  }
}
