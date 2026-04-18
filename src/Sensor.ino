Adafruit_BME280 BME;
GyverHTU21D    HTU;
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature DS(&oneWire);

// Called once from sensor_init()
static bool _ds_available = false;

// DS18B20 fail counter: skip -127 for up to 10 consecutive read errors
static byte _ds_fail_count = 0;
#define DS_FAIL_THRESHOLD 10

void sensor_init()
{
  TBLOG_LN("Scanning for sensors...");
  Wire.begin();

  // 1. Try BME280 (I2C 0x76 or 0x77)
  if (BME.begin(0x76) || BME.begin(0x77)) {
    sensorType = S_BME280;
    Sensor_fund = 1;
    TBLOG_LN("BME280 found!");
  }
  // 2. Try HTU21/SI7021 (I2C 0x40) — GyverHTU21D API
  else if (HTU.begin()) {
    sensorType = S_HTU21;
    Sensor_fund = 1;
    TBLOG_LN("HTU21/SI7021 found!");
  }
  // 3. Try DS18B20 (OneWire GPIO5) — DallasTemperature API
  else {
    DS.begin();
    DS.setResolution(12); // 12-bit resolution (precision 0.0625) ~750ms
    DS.setWaitForConversion(false); // Асинхронний запит, щоб не блокувати головний цикл
    DS.requestTemperatures();
    delay(750); // Очікуємо 750мс тільки 1 раз при ініціалізації
    float t = DS.getTempCByIndex(0);
    if (t != DEVICE_DISCONNECTED_C && t > -50) {
      sensorType  = S_DS18B20;
      Sensor_fund = 1;
      _ds_available = true;
      _ds_fail_count = 0;
      Temperature = t;                // FIX: Задаємо початкову температуру, щоб не було '0'
      DS.requestTemperatures();       // FIX: Одразу просимо нове перетворення, щоб за 5с воно було готове
      TBLOG_LN("DS18B20 found!");
    } else {
      sensorType  = S_NONE;
      Sensor_fund = 0;
      TBLOG_LN("No sensors found!");
    }
  }
}

void sensor_read()
{
  static uint32_t last_retry = 0;
  
  if (Sensor_fund == 0) {
    // Якщо датчик не знайдено, пробуємо ініціалізацію кожні 30 секунд
    if (millis() - last_retry > 30000 || last_retry == 0) {
      last_retry = millis();
      TBLOG_LN("No sensor active, retrying initialization...");
      sensor_init();
    }
    if (Sensor_fund == 0) return;
  }

  switch (sensorType) {
    case S_BME280:
      Temperature = BME.readTemperature();
      Humedity    = (byte)BME.readHumidity();
      Pressure    = BME.readPressure() / 133.3224f;
      // Перевірка на "зависання" або збій I2C (BME280 повертає NAN)
      if (isnan(Temperature)) {
        TBLOG_LN("BME280 error, re-init...");
        sensor_init();
      }
      break;

    case S_HTU21:
      HTU.requestTemperature();
      HTU.requestHumidity();
      delay(55); 
      Temperature = HTU.getTemperature();
      Humedity    = (byte)HTU.getHumidity();
      Pressure    = 0;
      if (Temperature < -40 || Humedity > 100) {
        TBLOG_LN("HTU21 error, re-init...");
        sensor_init();
      }
      break;

    case S_DS18B20:
      {
        float t = DS.getTempCByIndex(0); // Беремо вже готовий результат з минулого разу
        DS.requestTemperatures();        // Одразу відправляємо асинхронний запит на новий (без delay)
        
        if (t == DEVICE_DISCONNECTED_C || t < -50) {
          _ds_fail_count++;
          TBLOG("DS18B20 read error, fail_count="); TBLOG_LN(_ds_fail_count);
          if (_ds_fail_count >= DS_FAIL_THRESHOLD) {
            TBLOG_LN("DS18B20 confirmed absent, re-init...");
            Temperature = -127.0f;
            visual_trend = 0;
            sensor_init();
          }
        } else {
          _ds_fail_count = 0;
          if (t > Temperature + 0.01f) visual_trend = 1;
          else if (t < Temperature - 0.01f) visual_trend = -1;
          else visual_trend = 0;
          
          Temperature = t;
        }
        Humedity = 255;
        Pressure = 0;
      }
      break;

    default:
      break;
  }
}

// Compatibility stubs (called from setup)
void BME_INIT() { sensor_init(); }
void BME_READ() { sensor_read(); }
void DS_INIT()  { }   // no-op: sensor_init handles all
void DS_READ()  { }   // no-op: sensor_read handles all
