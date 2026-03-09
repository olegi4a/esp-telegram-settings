/*-------- NTP code (Built-in SNTP version) ----------*/

// Функція-заглушка для підтримки сумісності з TimeLib
// Повертає системний час ESP8266
time_t getNtpTime()
{
  time_t nowTime = time(nullptr);
  if (nowTime < 1500000000) {
    return 0; // Час ще не синхронізовано
  }
  return nowTime;
}

// Більше не потрібно вручну відправляти пакети
void sendNTPpacket(IPAddress &address)
{
  // Функція порожня для сумісності
}
