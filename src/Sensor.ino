Adafruit_BME280 BME;
GyverHTU21D    HTU;
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature DS(&oneWire);

// Called once from sensor_init()
static bool _ds_available = false;

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
    DS.setResolution(9); // 9-bit resolution ~95ms conversion
    DS.requestTemperatures();
    delay(100);
    float t = DS.getTempCByIndex(0);
    if (t != DEVICE_DISCONNECTED_C && t > -50) {
      sensorType  = S_DS18B20;
      Sensor_fund = 1;
      _ds_available = true;
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
  if (Sensor_fund == 0) return;

  switch (sensorType) {
    case S_BME280:
      Temperature = BME.readTemperature();
      Humedity    = (byte)BME.readHumidity();
      Pressure    = BME.readPressure() / 133.3224f;
      break;

    case S_HTU21:
      // GyverHTU21D: request/read sequence
      HTU.requestTemperature();
      HTU.requestHumidity();
      delay(55); // wait for conversion
      Temperature = HTU.getTemperature();
      Humedity    = (byte)HTU.getHumidity();
      Pressure    = 0;
      break;

    case S_DS18B20:
      // DallasTemperature API
      DS.requestTemperatures();
      delay(100); // 9-bit takes ~95ms
      Temperature = DS.getTempCByIndex(0);
      if (Temperature == DEVICE_DISCONNECTED_C || Temperature < -50) {
        TBLOG_LN("DS18B20 lost, re-scanning...");
        sensor_init();
      }
      Humedity    = 255;
      Pressure    = 0;
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
