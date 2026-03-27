/*---ініціаліз бібліотек---*/

#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecureBearSSL.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPUpdateServer.h>
#include <TimeLib.h>
#include <LittleFS.h>
#include <ArduinoJson.h>  // Версія 7.x
#include <FastBot2.h>
#include <DNSServer.h>

const byte DNS_PORT = 53;
extern DNSServer dnsServer;

// Версія прошивки (порівнюється з GitHub releases tag_name)
#define FIRMWARE_VERSION "EDwIC-3.5.5"

#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <GyverHTU21D.h>

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Fonts/TomThumb.h>

// SCL GPIO5
// SDA GPIO4
#define OLED_RESET 15  // GPIO0
// Дисплей 64x48 (0.66") I2C - використовуємо конструктор локальної бібліотеки 
Adafruit_SSD1306 display(OLED_RESET);

void Telegram_Callback(fb::Update& update);
void sendWelcomeMessage(int64_t senderId);
String buildDashboard();
void sendSettingsMenu(int64_t senderId);
void sendMainMenu(int64_t senderId);
void restoreAutomationMode();
void Logger_periodicTick();

extern bool is_usb_mode;
extern volatile bool powerFailFlag;

// ASCII visualizations removed in favor of new dashboard table

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
#define ONE_WIRE_BUS    14

static const char ntpServerName[] = "pool.ntp.org";
static const char ntpServerName2[] = "time.nist.gov";
const int timeZone = 2;

#define SID_AP "CONTROL"
// #define PAS_AP "qawsedrf" // Пароль видалено для відкритої мережі
#define WEBAPP_URL "https://olegi4a.github.io/esp-telegram-settings/"
#define Token  "1613585471:AAG351riAaemnIVA7sjrTYfgtT2xKTIyGFY"
#define bos    411849588

unsigned long timer0_last = 0;
unsigned long timer1_last = 0;
unsigned long timer4_last = 0;

// Відкладений виклик registerBotCommands (після того як FastBot2 закриє SSL-сесію)
bool  pendingRegisterCommands = false;
int64_t pendingRegisterSender  = 0;


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
  STATE_WAIT_ALARM_CONFIRM
};
BotState botState = STATE_IDLE;

enum SensorType {
  S_NONE,
  S_BME280,
  S_HTU21,
  S_DS18B20
};
SensorType sensorType = S_NONE;

String SID_STA = "dmytro_and_anastasy";
String PAS_STA = "love_is...";
long   TB_pasword = 12345;
String TB_Token = Token;

// Timezone (POSIX string — handles DST automatically)
String timezone_str = "EET-2EEST,M3.5.0/3,M10.5.0/4";

byte RESTART = 0;

long Time_now;
int Time_N = 72000; // 20:00
int Time_D = 28800; // 08:00
byte profile;

long users[10];

long   alluser;

float  Sensor_set = 20.0;
float  Sensor_histeresis = 1.0;
bool   Rele_status = false;
byte   Statatus_sensor_control = 0;
byte   Start_status = 0;
byte   widget_status = 0;

int    Time_on = 1;
int    Time_off = 1;

time_t   now_Time;
byte    now_Time_off_on;

bool   display_inv = true;

byte   Sensor_fund;
float  Temperature;
float  Pressure;
byte   Humedity;

bool   Alarm_start = true;
bool   Alarm_power = true;
float  Alarm_data_u = 30.0;
float  Alarm_data_d = 15.0;
float  Alarm_Temperature;
byte   Alarm_data_set;
byte   trend;
unsigned long   Alarm_data_milis;
bool   relay_change_notify;
bool   last_rele_state;

// Telegram connection status & bot info (not saved to flash)
bool   tg_connected = false;
String botName      = "";
static unsigned long tg_last_check = 0; // щохвилинна перевірка

// Display menu page: 0=main, 1=IP, 2=QR
byte   display_page = 0;

// Keyboards removed as they are now handled dynamically via FastBot2 API.
