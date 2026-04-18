// Завантаження даних, збережених у файл config.json
void Load_Config()
{
  File configFile = LittleFS.open("/Config.json", "r");
  if (!configFile)
  {
    TBLOG_LN(F("Failed to open Config file, creating default"));
    Save_Config();
    return;
  }

  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, configFile);
  configFile.close();

  if (error) {
    TBLOG_LN(F("JSON parse error!"));
    TBLOG_LN(error.c_str());
    Save_Config();
    return;
  }
  
//--------WiFi Settings------------------
  SID_STA = doc["SID_STA"] | SID_STA;
  PAS_STA = doc["PAS_STA"] | PAS_STA;

//--------Telegram Settings------------------
  alluser = doc["alluser"] | 0L;
  Time_D = doc["Time_D"] | 28800; // 08:00
  Time_N = doc["Time_N"] | 72000; // 20:00
  profile_timer_en = doc["profile_timer_en"] | false;
  TB_pasword = doc["TB_pasword"] | 12345;
  TB_Token = doc["TB_Token"] | Token;
  timezone_str = doc["timezone_str"] | String("EET-2EEST,M3.5.0/3,M10.5.0/4");
  display_type = doc["display_type"] | 1;

  JsonArray TB_users = doc["users"];
  for(byte i = 0; i < 10 && i < TB_users.size(); i++)
  {
    users[i] = TB_users[i];
  }

  TBLOG_LN(F("Configuration is loaded"));
}

// Запис даних у файл config.json
bool Save_Config()
{
  JsonDocument doc;
//--------WiFi Settings------------------
  doc["SID_STA"] = SID_STA;
  doc["PAS_STA"] = PAS_STA;

//--------Telegram Settings------------------
  doc["alluser"] = alluser;
  doc["Time_D"] = Time_D;
  doc["Time_N"] = Time_N;
  doc["profile_timer_en"] = profile_timer_en;
  doc["TB_pasword"] = TB_pasword;
  doc["TB_Token"] = TB_Token;
  doc["timezone_str"] = timezone_str;
  doc["display_type"] = display_type;

  JsonArray TB_users = doc["users"].to<JsonArray>();
  for(byte i = 0; i < 10; i++)
  {
    TB_users.add(users[i]);
  }

  File configFile = LittleFS.open("/Config.json", "w");
  if (!configFile)
  {
    TBLOG_LN(F("Failed to open config file for writing"));
    return false;
  }
  
  serializeJson(doc, configFile);
  configFile.close();
  TBLOG_LN(F("Saved config"));
  return true;
}

// Завантаження профілю
void Load_Profile(String profil_name)
{
  File configFile = LittleFS.open(profil_name, "r");
  if (!configFile)
  {
    TBLOG_LN(F("Failed to open profile file, setting defaults..."));
    Sensor_set = 20.0;
    Sensor_histeresis = 1.0;
    Time_on = 1;
    Time_off = 1;
    Statatus_sensor_control = 0;
    Save_Profile(profil_name);
    return;
  }

  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, configFile);
  configFile.close();

  if (error) {
    TBLOG_LN(F("Profile JSON parse error!"));
    Save_Profile(profil_name);
    return;
  }
  
  profile = doc["profile"] | 0;
  if (profil_name == "/G_profile.json") profile = 2; // Forced ID for general
  Sensor_set = doc["Sensor_set"] | 20.0;
  Sensor_histeresis = doc["Sensor_histeresis"] | 1.0;
  Statatus_sensor_control = doc["Statatus_sensor_control"] | 0;
  Rele_status = doc["Rele_status"] | false;
  Start_status = doc["Start_status"] | 0;   // Per-profile initial relay state
  Time_on = doc["Time_on"] | 1;
  Time_off = doc["Time_off"] | 1;
  Alarm_start = doc["Alarm_start"] | false;
  Alarm_data_u = doc["Alarm_data_u"] | 30.0;
  Alarm_data_d = doc["Alarm_data_d"] | 15.0;
  Alarm_data_set = doc["Alarm_data_set"] | 0;
  trend = doc["trend"] | 0;
  widget_status = doc["widget_status"] | 0;
  Alarm_power = doc["Alarm_power"] | 0;
  relay_change_notify = doc["relay_change_notify"] | 0;
  
  TBLOG_LN(F("Profile is loaded"));
}

// Запис профілю
void Save_Current_Profile() {
  if (profile == 1) {
    Save_Profile("/D_profile.json");
  } else if (profile == 0) {
    Save_Profile("/N_profile.json");
  } else {
    Save_Profile("/G_profile.json");
  }
}

bool Save_Profile(String profil_name)
{
  JsonDocument doc;

  doc["profile"] = profile;
  doc["Sensor_set"] = Sensor_set;
  doc["Sensor_histeresis"] = Sensor_histeresis;
  doc["Statatus_sensor_control"] = Statatus_sensor_control;
  doc["Rele_status"] = Rele_status;
  doc["Start_status"] = Start_status;
  doc["Time_on"] = Time_on;
  doc["Time_off"] = Time_off;
  doc["Alarm_start"] = (int)Alarm_start;
  doc["Alarm_data_u"] = Alarm_data_u;
  doc["Alarm_data_d"] = Alarm_data_d;
  doc["Alarm_data_set"] = Alarm_data_set;
  doc["trend"] = trend;
  doc["widget_status"] = widget_status;
  doc["Alarm_power"] = (int)Alarm_power;
  doc["relay_change_notify"] = (int)relay_change_notify;
  
  File configFile = LittleFS.open(profil_name, "w");
  if (!configFile) 
  {
    TBLOG_LN(F("Failed to open config file for writing"));
    return false;
  }
  
  serializeJson(doc, configFile);
  configFile.close();
  TBLOG_LN(F("Saved profile"));
  return true;
}

// Функція для оновлення файлу профілю без втручання в глобальні змінні
bool Patch_Profile(String filename, JsonObject s) {
  if (s.isNull() || s.size() == 0) return true;
  File f = LittleFS.open(filename, "r");
  JsonDocument doc;
  if (f) {
    deserializeJson(doc, f);
    f.close();
  } else {
    doc["profile"] = (filename == "/D_profile.json") ? 1 : 0;
  }
  
  auto patchF = [&](const char* k, const char* sk) { if (s.containsKey(k)) doc[k] = s[k].as<float>(); else if (s.containsKey(sk)) doc[k] = s[sk].as<float>(); };
  auto patchI = [&](const char* k, const char* sk) { if (s.containsKey(k)) doc[k] = s[k].as<int>(); else if (s.containsKey(sk)) doc[k] = s[sk].as<int>(); };

  patchF("Sensor_set", "ss");
  patchF("Sensor_histeresis", "sh");
  patchI("Statatus_sensor_control", "sc");
  if (s.containsKey("Rele_status")) doc["Rele_status"] = s["Rele_status"].as<int>() == 1;
  patchI("Start_status", "sts");
  patchI("Time_on", "ton");
  patchI("Time_off", "tof");
  patchI("Alarm_start", "ast");
  patchF("Alarm_data_u", "au");
  patchF("Alarm_data_d", "ad");
  patchI("Alarm_data_set", "as");
  patchI("trend", "tr");
  patchI("widget_status", "w");
  patchI("Alarm_power", "ap");
  patchI("relay_change_notify", "rn");

  File fw = LittleFS.open(filename, "w");
  if (!fw) return false;
  size_t bytes = serializeJson(doc, fw);
  fw.close();
  return bytes > 0;
}
// Відновлення режиму автоматики з файлу профілю (без запису!)
void restoreAutomationMode() {
  String profil_name;
  if (profile == 1) profil_name = "/D_profile.json";
  else if (profile == 0) profil_name = "/N_profile.json";
  else profil_name = "/G_profile.json";
  
  File f = LittleFS.open(profil_name, "r");
  if (!f) return;

  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, f);
  f.close();

  if (!error) {
    Statatus_sensor_control = doc["Statatus_sensor_control"] | 0;
    digitalWrite(LED_BOOTON, (Statatus_sensor_control == 0));
    TBLOG("Automation mode restored from "); TBLOG(profil_name); 
    TBLOG(": "); TBLOG_LN(Statatus_sensor_control);
  }
}
// Оновлення глобальних налаштувань з JSON об'єкту
void Update_Global_Config(JsonObject g) {
  if (g.isNull()) return;
  
  if (!g["profile_timer_en"].isNull()) profile_timer_en = g["profile_timer_en"].as<bool>();
  if (!g["Time_D"].isNull())  Time_D = g["Time_D"].as<int>();
  if (!g["Time_N"].isNull())  Time_N = g["Time_N"].as<int>();
  // Legacy support for hours if UI sends them
  if (!g["Time_D_h"].isNull())  Time_D = g["Time_D_h"].as<int>() * 3600;
  if (!g["Time_N_h"].isNull())  Time_N = g["Time_N_h"].as<int>() * 3600;
  
  if (!g["SID_STA"].isNull())   SID_STA = g["SID_STA"].as<String>();
  if (!g["PAS_STA"].isNull())   PAS_STA = g["PAS_STA"].as<String>();
  
  if (!g["timezone_str"].isNull() && g["timezone_str"].as<String>().length() > 3) {
    timezone_str = g["timezone_str"].as<String>();
    configTime(timezone_str.c_str(), ntpServerName, ntpServerName2);
  }
  
  if (!g["TB_Token"].isNull() && g["TB_Token"].as<String>().length() > 10) {
    TB_Token = g["TB_Token"].as<String>();
    myBot.setToken(TB_Token);
  }
  
  if (!g["TB_pasword"].isNull()) TB_pasword = g["TB_pasword"].as<long>();
  if (!g["display_type"].isNull()) display_type = g["display_type"].as<int>();
  
  Save_Config();
}
