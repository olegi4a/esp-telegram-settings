void Telegram_Callback(fb::Update& update)
{
  TBLOG("Update: "); TBLOG_LN((int)update.type());
  fb::MessageRead msg = update.message();
  int64_t senderId = msg.from().id().toInt64();
  TBLOG("Sender: "); TBLOG_LN((int32_t)senderId);

  // Обробка Callback Query (Inline кнопки)
  if (update.isQuery()) {
    TBLOG_LN("Query detected");
    myBot.answerCallbackQuery(update.query().id(), "Готово");
    String data = update.query().data().toString();
    
    if (data == "1" || data == "on") {
      if (Statatus_sensor_control == 0) {
        digitalWrite(RELE, HIGH);
        Rele_status = 1;
        Logger_addEntry(1);
        myBot.sendMessage(fb::Message("Увімкнуто!!! ✅", senderId));
      } else {
        myBot.sendMessage(fb::Message("⚠️ Помилка: Увімкнено автоматичний режим! Вимкніть його для ручного керування.", senderId));
      }
    } else if (data == "0" || data == "off") {
      if (Statatus_sensor_control == 0) {
        digitalWrite(RELE, LOW);
        Rele_status = 0;
        Logger_addEntry(1);
        myBot.sendMessage(fb::Message("Вимкнуто!!! ❌", senderId));
      } else {
        myBot.sendMessage(fb::Message("⚠️ Помилка: Увімкнено автоматичний режим!", senderId));
      }
    } else if (data == "11") {
      Alarm_data_set = 1;
      Logger_addEntry(2);
      myBot.sendMessage(fb::Message("Повідомлення тривоги Увімкнуто! 🔔\n✅ Автозбережено.", senderId));
      Save_Current_Profile();
    } else if (data == "10") {
      Alarm_data_set = 0;
      Logger_addEntry(2);
      myBot.sendMessage(fb::Message("Повідомлення тривоги Вимкнуто! 🔕\n✅ Автозбережено.", senderId));
      Save_Current_Profile();
    } else if (data == "alarm_yes") {
      botState = STATE_WAIT_ALARM_U;
      myBot.sendMessage(fb::Message(F("Переналаштовуємо. Введіть ВЕРХНЮ межу температури:"), senderId));
    } else if (data == "alarm_no") {
      botState = STATE_IDLE;
    } else if (data == "ss0") {
      Start_status = 0;
      Logger_addEntry(2);
      myBot.sendMessage(fb::Message("✅ Початковий стан: ВИМКНЕНО (Автозбережено)", senderId));
      Save_Current_Profile();
    } else if (data == "ss1") {
      Start_status = 1;
      Logger_addEntry(2);
      myBot.sendMessage(fb::Message("✅ Початковий стан: УВІМКНЕНО (Автозбережено)", senderId));
      Save_Current_Profile();
    } else if (data == "reg_commands") {
      // Нативний myBot.setMyCommands() — безпечно викликати прямо з callback
      registerBotCommands(senderId);
    } else if (data == "new_prof:D" || data == "new_prof:N" || data == "new_prof:B") {
      String prof = data.substring(9);
      String label = (prof == "D") ? "☀️ Денний" : (prof == "N") ? "🌙 Нічний" : "💾 Обидва";
      myBot.sendMessage(fb::Message("✅ Обрано профіль: " + label + "\n"
        "Налаштування через чат в розробці.\n"
        "Поки використовуйте 📱\n"
        "- WEB app\n"
        "або\n"
        "- перейдіть на локальну веб сторінку вашого пристрою", senderId));
    }
    return;
  }

  // Перевірка авторизації
  bool isWebApp = update.isMessage() && update.message().entry.has("web_app_data");
  TBLOG("isWebApp: "); TBLOG_LN(isWebApp);

  if (user_find(senderId) == 200 && botState == STATE_IDLE && !isWebApp && (!update.isMessage() || update.message().text() != "/start")) {
    TBLOG_LN("Access denied");
    myBot.sendMessage(fb::Message(F("❌ Немає доступу. Введіть /start та пароль."), senderId));
    return;
  }

  // Обробка даних від WebApp (Mini App)
  if (isWebApp) {
    TBLOG_LN("Processing WebApp Data...");
    String json = update.message().entry["web_app_data"]["data"].toString();
    TBLOG("JSON: "); TBLOG_LN(json);
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
        TBLOG_LN("Global config saved");
      }

      bool d_ok = true, n_ok = true;
      if (target == "D" || target == "B") {
        d_ok = Patch_Profile("/D_profile.json", s);
      }
      if (target == "N" || target == "B") {
        n_ok = Patch_Profile("/N_profile.json", s);
      }

      // Перезавантажуємо активний профіль, щоб оновити глобальні змінні (в пам'яті)
      if (profile == 1) Load_Profile("/D_profile.json");
      else Load_Profile("/N_profile.json");

      String resultMsg = "📥 Отримано та застосовано!\n";
      if (target == "D") Logger_addEntry(17);
      else if (target == "N") Logger_addEntry(18);
      else Logger_addEntry(19);

      if (target == "D" || target == "B") {
        if (d_ok) {
           resultMsg += "✅ Збережено в ДЕННИЙ профіль\n";
           TBLOG_LN("Saved Day Profile via WebApp");
        } else {
           resultMsg += "❌ Помилка запису ДЕННОГО профілю\n";
        }
      }
      if (target == "N" || target == "B") {
        if (n_ok) {
           resultMsg += "✅ Збережено в НІЧНИЙ профіль\n";
           TBLOG_LN("Saved Night Profile via WebApp");
        } else {
           resultMsg += "❌ Помилка запису НІЧНОГО профілю\n";
        }
      }
      resultMsg += "✅ Клавіатуру оновлено.\n";
      myBot.sendMessage(fb::Message(resultMsg, senderId));
      TBLOG_LN("Response sent to user");
      
      sendMainMenu(senderId);
    } else {
      TBLOG("Deserialization Error: "); TBLOG_LN(error.c_str());
    }
    return;
  }

  if (!msg.text()) return;
  String text = msg.text().toString();
  TBLOG("Incoming text: ["); TBLOG(text); TBLOG_LN("]");

  // Reset state if user requests main menu or cancels
  if (text == "/start" || text == "/home" || text.equalsIgnoreCase("home") || text == "Повернутися в меню" || text == "Скасувати" ||
      text == "Налаштування" || text == "Основні дані" || text == "Ручне керування" || text == "Автоматичне керування") {
    botState = STATE_IDLE;
  }

  // --- State Machine ---
  if (botState != STATE_IDLE) {
    handleBotState(senderId, text);
    return;
  }

  // --- Основні команди ---
  if (text == "/start") {
    if (user_find(senderId) != 200) {
      // Вже авторизований — вітання + меню
      sendWelcomeMessage(senderId);
      sendMainMenu(senderId);
    } else {
      // Новий користувач — ТІЛЬКИ запит пароля
      myBot.sendMessage(fb::Message(F("🔐 Введіть пароль для доступу:"), senderId));
      botState = STATE_WAIT_AUTH_PASSWORD;
    }
  }
  else if (text == "/home" || text.equalsIgnoreCase("home")) {
    sendMainMenu(senderId);
  }
  else if (text == "/help") {
    sendMainMenu(senderId);
  }
  else if (text == "/settings") {
    sendSettingsInlineMenu(senderId);
  }
  else if (text == "/reboot" || text.equalsIgnoreCase("reboot")) {
    myBot.sendMessage(fb::Message("🔄 Перезавантаження...", senderId));
    Logger_addEntry(11);
    RESTART = 3;
  }
  else if (text == "Налаштування" || text == "налаштування" || text == "Повернутися в меню") {
    sendMainMenu(senderId);
  }
  else if (text == "Налаштування Тривоги") {
    fb::Message m("🔔 Налаштування тривоги", senderId);
    fb::Menu menu(KB(_kb6));
    menu.resize = true;
    m.setMenu(menu);
    myBot.sendMessage(m);
  }
  else if (text == "Налаштування Автоматичного керування") {
    fb::Message m("🤖 Автоматичне керування (Налаштування)", senderId);
    fb::Menu menu(KB(_kb7));
    menu.resize = true;
    m.setMenu(menu);
    myBot.sendMessage(m);
  }
  else if (text == "Інше") {
    fb::Message m("📂 Інші налаштування", senderId);
    fb::Menu menu(KB(_kb5));
    menu.resize = true;
    m.setMenu(menu);
    myBot.sendMessage(m);
  }
  else if (text == "Зберегти") {
    fb::Message m("💾 Зберегти профілі", senderId);
    fb::Menu menu(KB(_kb8));
    menu.resize = true;
    m.setMenu(menu);
    myBot.sendMessage(m);
  }
  else if (text == "Основні дані") {
    sendBasicData(senderId);
  }
  else if (text == "🌐 Web-інтерфейс") {
    String url = "http://" + WiFi.localIP().toString();
    fb::Message m("🌐 Локальна веб-сторінка вашого пристрою:\n" + url, senderId);
    fb::InlineKeyboard kb;
    gson::Str btn;
    btn('{');
    btn["text"] = "🔗 Відкрити сторінку";
    btn["url"] = url;
    btn('}');
    kb.addButton(btn);
    m.setKeyboard(&kb);
    myBot.sendMessage(m);
  }
  else if (text == "Головний користувач") {
    alluser = senderId;
    myBot.sendMessage(fb::Message(F("🌟 Ви тепер головний користувач."), senderId));
    Save_Config();
  }
  else if (text == "Тривога по даних сенсора") {
    if (Alarm_start == 1) {
      fb::Message m("🔔 Тривога Увімкнута\n Бажаєте налаштувати межі? ", senderId);
      fb::InlineMenu menu("YES;NOT", "alarm_yes;alarm_no");
      m.setInlineMenu(menu);
      myBot.sendMessage(m);
    }
    botState = STATE_WAIT_ALARM_CONFIRM;
  }
  else if (text == "Автоматика по сенсору") {
    myBot.sendMessage(fb::Message(F("Введіть бажане значення (SetPoint):"), senderId));
    botState = STATE_WAIT_SENSOR_SET;
  }
  else if (text == "Автоматика по часу") {
    myBot.sendMessage(fb::Message(F("Скільки хвилин ON?"), senderId));
    botState = STATE_WAIT_TIME_ON;
  }
  else if (text == "Змінити пароль") {
    myBot.sendMessage(fb::Message(F("Введіть новий пароль (число до 5 знаків):"), senderId));
    botState = STATE_WAIT_NEW_PASSWORD;
  }
  else if (text == "Змінити час") {
    myBot.sendMessage(fb::Message(F("Година старту ДЕННОГО профілю (0-23):"), senderId));
    botState = STATE_WAIT_TIME_D_H;
  }
  else if (text == "Керування по сенсору") {
    Statatus_sensor_control = 1;
    Logger_addEntry(6);
    myBot.sendMessage(fb::Message("✅ Керування по датчику активовано! (Автозбережено)", senderId));
    Save_Current_Profile();
  }
  else if (text == "Керування по таймеру") {
    Statatus_sensor_control = 2;
    Logger_addEntry(6);
    myBot.sendMessage(fb::Message("✅ Керування по таймеру активовано! (Автозбережено)", senderId));
    Save_Current_Profile();
  }
  else if (text == "Ручне керування") {
    Statatus_sensor_control = 0;
    Logger_addEntry(14); // 14 = Ручний режим
    
    fb::Message m("⚠️ Автоматизацію вимкнено. Перехід в ручний режим.", senderId);
    fb::InlineMenu menu("✅ Увімкнути;❌ Вимкнути", "on;off");
    m.setInlineMenu(menu);
    myBot.sendMessage(m);
  }
  else if (text == "Автоматичне керування") {
    restoreAutomationMode(); // Відновлюємо режим з файлу профілю
    Logger_addEntry(14 + Statatus_sensor_control);
    
    String modeName = "невідомо";
    if (Statatus_sensor_control == 1) modeName = "по датчику 🌡";
    else if (Statatus_sensor_control == 2) modeName = "по таймеру ⏱";
    
    myBot.sendMessage(fb::Message("🤖 Повернення в автоматичний режим: " + modeName, senderId));
  }
  else if (text == "Нічний профіль") {
     Save_Profile("/N_profile.json");
     Logger_addEntry(18); // 18 = Збережено: Нічний профіль
     myBot.sendMessage(fb::Message("🌙 Нічний профіль збережено", senderId));
  }
  else if (text == "Денний профіль") {
     Save_Profile("/D_profile.json");
     Logger_addEntry(17); // 17 = Збережено: Денний профіль
     myBot.sendMessage(fb::Message("☀️ Денний профіль збережено", senderId));
  }
  else if (text.equalsIgnoreCase("on")) {
    if (Statatus_sensor_control == 0) {
      digitalWrite(RELE, HIGH);
      Rele_status = 1;
      Logger_addEntry(1);
      myBot.sendMessage(fb::Message("Увімкнуто! ✅", senderId));
    } else {
      myBot.sendMessage(fb::Message("⚠️ Вимкніть автоматику для ручного керування.", senderId));
    }
  }
  else if (text.equalsIgnoreCase("off")) {
    if (Statatus_sensor_control == 0) {
      digitalWrite(RELE, LOW);
      Rele_status = 0;
      Logger_addEntry(1);
      myBot.sendMessage(fb::Message("Вимкнуто! ❌", senderId));
    } else {
      myBot.sendMessage(fb::Message("⚠️ Вимкніть автоматику для ручного керування.", senderId));
    }
  }
  else if (text == "Початковий стан реле") {
    String btn_text = String(Start_status == 0 ? "✅ Вимкнено" : "Вимкнено") + ";" + 
                      String(Start_status == 1 ? "✅ Увімкнено" : "Увімкнено");
    String btn_data = "ss0;ss1";
    fb::Message m("🔌 Початковий стан реле при старті:\n(поточний: " + String(Start_status ? "✅ Увімкнено" : "❌ Вимкнено") + ")\n\n⚠️ Після вибору — збережіть у профіль!", senderId);
    fb::InlineMenu menu(btn_text, btn_data);
    m.setInlineMenu(menu);
    myBot.sendMessage(m);
  }
  else if (text == "Віджет") {
    widget_status = !widget_status;
    myBot.sendMessage(fb::Message(widget_status ? "✅ Віджет увімкнено (Автозбережено)" : "❌ Віджет вимкнено (Автозбережено)", senderId));
    Save_Current_Profile();
  }
  else if (text == "Сповіщення по тенденції") {
    trend = !trend;
    myBot.sendMessage(fb::Message(trend ? "✅ Сповіщення по тенденції увімкнено (Автозбережено)" : "❌ Сповіщення по тенденції вимкнено (Автозбережено)", senderId));
    Save_Current_Profile();
  }
  else if (text == "Повідомлення про перезавантаження") {
    Alarm_start = !Alarm_start;
    myBot.sendMessage(fb::Message(Alarm_start ? "✅ Повідомлення про перезавантаження увімкнено (Автозбережено)" : "❌ Повідомлення про перезавантаження вимкнено (Автозбережено)", senderId));
    Save_Current_Profile();
  }
  else if (text == "Сповіщення відсутності живлення") {
    Alarm_power = !Alarm_power;
    myBot.sendMessage(fb::Message(Alarm_power ? "✅ Сповіщення про живлення увімкнено (Автозбережено)" : "❌ Сповіщення про живлення вимкнено (Автозбережено)", senderId));
    Save_Current_Profile();
  }
  else if (text == "Сповіщення зміни стану") {
    relay_change_notify = !relay_change_notify;
    myBot.sendMessage(fb::Message(relay_change_notify ? "✅ Сповіщення зміни стану реле увімкнено (Автозбережено)" : "❌ Сповіщення зміни стану реле вимкнено (Автозбережено)", senderId));
    Save_Current_Profile();
  }
  else if (text == "Поточні налаштування") {
    String settings = "⚙️ Поточні налаштування:\n";
    String modeName = "Ручне";
    if (Statatus_sensor_control == 1) modeName = "По сенсору";
    else if (Statatus_sensor_control == 2) modeName = "По таймеру";
    
    settings += "Режим автоматики: " + modeName + "\n";
    settings += "Тривога (Сенсор): " + String(Alarm_data_set ? "Увімк" : "Вимк") + "\n";
    settings += "Тенденція: " + String(trend ? "Увімк" : "Вимк") + "\n";
    settings += "Віджет: " + String(widget_status ? "Увімк" : "Вимк") + "\n";
    settings += "Сповіщення реле: " + String(relay_change_notify ? "Увімк" : "Вимк") + "\n";
    myBot.sendMessage(fb::Message(settings, senderId));
  }
  else {
    fb::Message m("❓ Команду не розпізнано.\nДля роботи з ботом натисніть /start", senderId);
    myBot.sendMessage(m);
  }
}
