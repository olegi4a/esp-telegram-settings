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
  TB_pasword = doc["TB_pasword"] | 12345;
  TB_Token = doc["TB_Token"] | Token;
  timezone_str = doc["timezone_str"] | String("EET-2EEST,M3.5.0/3,M10.5.0/4");

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
  doc["TB_pasword"] = TB_pasword;
  doc["TB_Token"] = TB_Token;
  doc["timezone_str"] = timezone_str;

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
  } else {
    Save_Profile("/N_profile.json");
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
  
  if (!s["Sensor_set"].isNull())              doc["Sensor_set"]              = s["Sensor_set"].as<String>().toFloat();
  if (!s["Sensor_histeresis"].isNull())       doc["Sensor_histeresis"]       = s["Sensor_histeresis"].as<String>().toFloat();
  if (!s["Statatus_sensor_control"].isNull()) doc["Statatus_sensor_control"] = s["Statatus_sensor_control"].as<String>().toInt();
  if (!s["Rele_status"].isNull())             doc["Rele_status"]             = (bool)s["Rele_status"].as<String>().toInt();
  if (!s["Start_status"].isNull())            doc["Start_status"]            = s["Start_status"].as<String>().toInt();
  if (!s["Time_on"].isNull())                 doc["Time_on"]                 = s["Time_on"].as<String>().toInt();
  if (!s["Time_off"].isNull())                doc["Time_off"]                = s["Time_off"].as<String>().toInt();
  if (!s["Alarm_start"].isNull())             doc["Alarm_start"]             = (bool)s["Alarm_start"].as<String>().toInt();
  if (!s["Alarm_data_u"].isNull())            doc["Alarm_data_u"]            = s["Alarm_data_u"].as<String>().toFloat();
  if (!s["Alarm_data_d"].isNull())            doc["Alarm_data_d"]            = s["Alarm_data_d"].as<String>().toFloat();
  if (!s["Alarm_data_set"].isNull())          doc["Alarm_data_set"]          = s["Alarm_data_set"].as<String>().toInt();
  if (!s["trend"].isNull())                   doc["trend"]                   = s["trend"].as<String>().toInt();
  if (!s["widget_status"].isNull())           doc["widget_status"]           = s["widget_status"].as<String>().toInt();
  if (!s["Alarm_power"].isNull())             doc["Alarm_power"]             = s["Alarm_power"].as<String>().toInt();
  if (!s["relay_change_notify"].isNull())     doc["relay_change_notify"]     = (bool)s["relay_change_notify"].as<String>().toInt();

  File fw = LittleFS.open(filename, "w");
  if (!fw) return false;
  size_t bytes = serializeJson(doc, fw);
  fw.close();
  return bytes > 0;
}
// Відновлення режиму автоматики з файлу профілю (без запису!)
void restoreAutomationMode() {
  String profil_name = (profile == 1) ? "/D_profile.json" : "/N_profile.json";
  File f = LittleFS.open(profil_name, "r");
  if (!f) return;

  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, f);
  f.close();

  if (!error) {
    Statatus_sensor_control = doc["Statatus_sensor_control"] | 0;
    TBLOG("Automation mode restored from "); TBLOG(profil_name); 
    TBLOG(": "); TBLOG_LN(Statatus_sensor_control);
  }
}
