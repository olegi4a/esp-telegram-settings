/*---ініціаліз бібліотек---*/

#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecureBearSSL.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPUpdateServer.h>
#include <WiFiUdp.h>
#include <TimeLib.h>
#include <LittleFS.h>
#include <ArduinoJson.h>  // Версія 7.x
#include <FastBot2.h>

#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <OneWire.h>
#include <DallasTemperature.h>

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// SCL GPIO5
// SDA GPIO4
#define OLED_RESET 15  // GPIO0
// Дисплей 64x48 (0.66") I2C
Adafruit_SSD1306 display(64, 48, &Wire, OLED_RESET);

#define ESP_DEBUG_MODE         1 // enable debugmode -> print debug data on the Serial

#if ESP_DEBUG_MODE > 0
#define TBLOG_LN(x) Serial.println(x)
#define TBLOG(x)    Serial.print(x)
#else
#define TBLOG_LN(x) 0
#define TBLOG(x)    0
#endif

#define LED_STATUS      2
#define LED_BOOTON      15
#define BUTTON          0
#define POWER           12
#define RELE            13

static const char ntpServerName[] = "pool.ntp.org";
const int timeZone = 2;
WiFiUDP Udp;
unsigned int localPort = 8888;

// --- Logger: RAM record structure (defined here for global visibility) ---
struct LogEntry {
  uint32_t ts;      // Unix timestamp
  float    temp;    // Temperature °C
  uint8_t  hum;     // Humidity %
  uint16_t press;   // Pressure hPa
  uint8_t  relay;   // Relay state
  uint8_t  event;   // Event type
};


#define SID_AP "CONTROL"
#define PAS_AP "qawsedrf"
#define Token  "1736620025:AAEVvAhaHIbehL4rsjm-NQI2YbaTjYMrn9M"
#define bos    411849588

unsigned long timer0_last = 0;
unsigned long timer1_last = 0;
unsigned long timer2_last = 0;

const unsigned long TIMER0_INTERVAL = 5000;
const unsigned long TIMER1_INTERVAL = 300000;
const unsigned long TIMER2_INTERVAL = 600000;

ESP8266WebServer WebServer(80);
ESP8266HTTPUpdateServer WebUpdater;

FastBot2 myBot;

// Стан бота для неблокуючого вводу
enum BotState {
  STATE_IDLE,
  STATE_WAIT_AUTH_PASSWORD,
  STATE_WAIT_NEW_PASSWORD,
  STATE_WAIT_SENSOR_SET,
  STATE_WAIT_SENSOR_HYST,
  STATE_WAIT_TIME_ON,
  STATE_WAIT_TIME_OFF,
  STATE_WAIT_TIME_D_H,
  STATE_WAIT_TIME_D_M,
  STATE_WAIT_TIME_N_H,
  STATE_WAIT_TIME_N_M,
  STATE_WAIT_ALARM_U,
  STATE_WAIT_ALARM_D
};
BotState botState = STATE_IDLE;

String Serwer_URL = "192.168.11.7";
String DATA = "";
int    Sleep_delay = 10;
byte   share_sensor;

String SID_STA = "dmytro_and_anastasy";
String PAS_STA = "love_is...";
bool   authorization = false;
long   TB_pasword = 12345;
String TB_Token = Token;

byte RESTART = 0;

long Time_now;
int Time_N;
int Time_D;
byte profile;

long users[10];

String Sensor_DATA = "";
byte   automatic_control = 1;
long   alluser;

float  Sensor_set;
float  Sensor_histeresis;
bool   Rele_status;
byte   Statatus_sensor_control;
byte   Start_status;
byte   widget_status;

int    Time_on;
int    Time_off;

time_t   now_Time;
byte    now_Time_off_on;

bool   display_inv = true;

byte   Sensor_fund;
float  Temperature;
float  Pressure;
byte   Humedity;

String Slave_Name;
int    Slave_Sleep_delay;
int    Slave_WIFI_RSSI;
int    Slave_Voltage;

bool   Alarm_start;
bool   Alarm_power;
float  Alarm_data_u;
float  Alarm_data_d;
float  Alarm_Temperature;
byte   Alarm_data_set;
byte   trend;
unsigned long   Alarm_data_milis;
unsigned long   power_milis;

String keyboard = "{\"inline_keyboard\":[[{\"text\":\"ON\",\"callback_data\":\"1\"},{\"text\":\"OFF\",\"callback_data\":\"0\"}]]}";
String keyboard_1 = "{\"inline_keyboard\":[[{\"text\":\"ALARM_ON\",\"callback_data\":\"11\"},{\"text\":\"ALARM_OFF\",\"callback_data\":\"10\"}]]}";
String keyboard_2 = "{\"inline_keyboard\":[[{\"text\":\"YES\",\"callback_data\":\"1\"},{\"text\":\"NOT\",\"callback_data\":\"0\"}]]}";
String keyboard_3 = "{\"keyboard\":[[{\"text\":\"Основні дані\"}],[{\"text\":\"Ручне керування\"}],[{\"text\":\"Автоматичне керування\"}],[{\"text\":\"налаштування\"}],[{\"text\":\"Reboot\"}],[{\"text\":\"/help\"}]],\"resize_keyboard\":true}";
String keyboard_4 = "{\"keyboard\":[[{\"text\":\"Налаштування Тривоги\"}],[{\"text\":\"Налаштування Автоматичного керування\"}],[{\"text\":\"Налаштування безпеки\"}],[{\"text\":\"Інше\"}],[{\"text\":\"Поточні налаштування\"}],[{\"text\":\"Зберегти\"}],[{\"text\":\"Home\"}]],\"resize_keyboard\":true}";
String keyboard_5 = "{\"keyboard\":[[{\"text\":\"почятковий стан реле\"}],[{\"text\":\"Віджет\"}],[{\"text\":\"повернутися в меню\"}],[{\"text\":\"Home\"}]],\"resize_keyboard\":true}";
String keyboard_6 = "{\"keyboard\":[[{\"text\":\"Тривога по даних сенсора\"}],[{\"text\":\"сповіщення по тенденції\"}],[{\"text\":\"повідомлення про перезагрузку\"}],[{\"text\":\"сповіщення відсутності живлення\"}],[{\"text\":\"повернутися в меню\"}],[{\"text\":\"Home\"}]],\"resize_keyboard\":true}";
String keyboard_7 = "{\"keyboard\":[[{\"text\":\"Автоматика по сенсору\"}],[{\"text\":\"Автоматика по часу\"}],[{\"text\":\"повернутися в меню\"}],[{\"text\":\"Home\"}]],\"resize_keyboard\":true}";
String keyboard_8 = "{\"keyboard\":[[{\"text\":\"Нічний профіль\"}],[{\"text\":\"Денний профіль\"}],[{\"text\":\"Змінити час\"}],[{\"text\":\"повернутися в меню\"}],[{\"text\":\"Home\"}]],\"resize_keyboard\":true}";
String keyboard_9 = "{\"keyboard\":[[{\"text\":\"змінити пароль\"}],[{\"text\":\"Головний користувач\"}],[{\"text\":\"повернутися в меню\"}],[{\"text\":\"Home\"}]],\"resize_keyboard\":true}";
