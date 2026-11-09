/*
  ESP32 Clock v2 — ARCH edition (UI layout ใหม่ทั้งหมด สำหรับกล่องหัวโค้ง)
  ----------------------------------------------------------
  อุปกรณ์: ESP32 DevKit, DS3231, OLED SSD1306 128x64, LDR, IR (FC-51), DHT11

  การควบคุม (ใช้โบกมือที่ IR หรือกดปุ่ม BOOT บนบอร์ดก็ได้):
    โบกสั้น   (เอามือออกก่อน 0.7 วิ)  = ถัดไป / +1
    ค้างมือ   (ครบ 0.7 วิ ทำงานทันที) = ตกลง / เลือก / เริ่ม-หยุด
    ค้างต่อ   (ครบ 3 วิ)              = รีเซ็ต (เฉพาะหน้าจับเวลา/นับถอยหลัง)
  ออกจากเมนู: เลือก "Exit" หรือรอ 30 วิ

  หน้าหลัก (โบกสั้นเพื่อเปลี่ยนหน้า): นาฬิกา -> อุณหภูมิ/ความชื้น -> จับเวลา -> นับถอยหลัง
  ค้างมือที่หน้านาฬิกา/อุณหภูมิ = เปิดเมนูตั้งค่า

  ไม่มีอินเทอร์เน็ตก็ใช้ได้: DS3231 เดินเวลาเอง, ตั้งเวลาผ่านเมนู หรือเปิด "WiFi Setup"
  แล้วใช้มือถือต่อ WiFi "ESP32-Arch" (รหัส 12345678) เข้า 192.168.4.1 เพื่อกดตั้งเวลาจากมือถือ
*/

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Fonts/FreeSansBold24pt7b.h>
#include <Fonts/FreeSansBold12pt7b.h>
#include <Fonts/FreeSans9pt7b.h>
#include <Fonts/FreeSansBold9pt7b.h>
#include <RTClib.h>
#include <DHT.h>
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <Preferences.h>
#include <time.h>
#include "esp_sntp.h"

#define FW_VERSION "2.0A"

// ---------- ขา ----------
#define SDA_PIN 21
#define SCL_PIN 22
#define LDR_PIN 34
#define IR_PIN  27
#define BTN_PIN 0          // ปุ่ม BOOT บนบอร์ด (ใช้แทนการโบกมือได้)
#define DHT_PIN 4
#define DHT_TYPE DHT11
#define LDR_INVERT true

// ---------- WiFi ของตัวนาฬิกาเอง (โหมดตั้งค่า) ----------
#define AP_SSID "ESP32-Arch"
#define AP_PASS "12345678"

// ---------- เวลา ----------
#define WAKE_MS          10000UL   // ตอนมืด โบกมือแล้วจอติดนานเท่านี้
#define MENU_TIMEOUT_MS  30000UL   // ไม่ได้แตะเมนูนานเท่านี้ กลับหน้านาฬิกา
#define LONG_MS            700UL   // ค้างมือถึงนี้ = ตกลง (ทำงานทันที ไม่ต้องรอปล่อย)
#define VLONG_MS          3000UL   // ค้างต่อจนถึงนี้ = รีเซ็ต (เฉพาะหน้าจับเวลา/นับถอยหลัง)

Adafruit_SSD1306 display(128, 64, &Wire, -1);
RTC_DS3231 rtc;
DHT dht(DHT_PIN, DHT_TYPE);
Preferences prefs;
WebServer server(80);
DNSServer dns;

// ---------- การตั้งค่า (บันทึกในหน่วยความจำถาวร) ----------
struct Settings {
  String ssid = "";
  String pass = "";
  int    tz = 7;          // UTC+7 ประเทศไทย
  bool   h12 = false;     // true = แสดงแบบ 12 ชม.
  int    bright = 0;      // 0=Auto 1=Low 2=Mid 3=High
  bool   nightOff = true; // ดับจอตอนมืด
  int    darkThr = 300;   // ค่าแสงต่ำกว่านี้ = มืด
  bool   wifiOn = true;
} cfg;

enum { UI_PAGE, UI_MENU, UI_EDIT, UI_INFO, UI_PORTAL };
enum { PG_CLOCK, PG_CLIMATE, PG_STOPWATCH, PG_TIMER, PG_COUNT };
enum { EV_NONE, EV_SHORT, EV_LONG, EV_VLONG };
enum { TM_IDLE, TM_EDIT, TM_RUN, TM_PAUSE, TM_ALARM };

const char* DOW[] = {"SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"};
const char* MON[] = {"JAN", "FEB", "MAR", "APR", "MAY", "JUN",
                     "JUL", "AUG", "SEP", "OCT", "NOV", "DEC"};

// ---------- สถานะ ----------
int  uiMode = UI_PAGE;
int  page = PG_CLOCK;
bool screenOn = true;
unsigned long lastActivity = 0;
int  curContrast = -1;
bool holdDefer = false;   // กำลังค้างมือในโหมด "เอามือออก=ทำต่อ / ค้างนาน=รีเซ็ต"

DateTime tnow;
unsigned long secStartMs = 0;

float roomTemp = NAN, roomHum = NAN, tMin = NAN, tMax = NAN;
int   light = 0;

// จับเวลา
bool swRunning = false;
unsigned long swStart = 0, swAccum = 0;

// นับถอยหลัง
const uint16_t TMR_PRESETS[] = {30, 60, 120, 180, 300, 600, 900, 1200, 1500, 1800, 2700, 3600, 5400};
const int TMR_NPRESET = sizeof(TMR_PRESETS) / sizeof(TMR_PRESETS[0]);
int tmrState = TM_IDLE;
int tmrPreset = 4;                 // เริ่มที่ 5 นาที
unsigned long tmrTotal = 300;      // วินาที
unsigned long tmrEnd = 0, tmrLeftMs = 0, alarmStart = 0;

// เครือข่าย
bool portalOn = false, serverStarted = false, ntpOk = false, syncToast = false;
unsigned long portalStart = 0, lastNtpMs = 0;
volatile bool sntpEvent = false;

// ข้อความแจ้งเตือนชั่วคราว
char toastMsg[24] = "";
unsigned long toastUntil = 0;

void toast(const char* m) {
  strncpy(toastMsg, m, sizeof(toastMsg) - 1);
  toastUntil = millis() + 1500;
}

// ---------- บันทึก/โหลดการตั้งค่า ----------
void loadSettings() {
  prefs.begin("clock", false);
  cfg.ssid     = prefs.getString("ssid", "");
  cfg.pass     = prefs.getString("pass", "");
  cfg.tz       = prefs.getInt("tz", 7);
  cfg.h12      = prefs.getBool("h12", false);
  cfg.bright   = prefs.getInt("bright", 0);
  cfg.nightOff = prefs.getBool("night", true);
  cfg.darkThr  = prefs.getInt("dark", 300);
  cfg.wifiOn   = prefs.getBool("wifi", true);
  prefs.end();
}

void saveSettings() {
  prefs.begin("clock", false);
  prefs.putString("ssid", cfg.ssid);
  prefs.putString("pass", cfg.pass);
  prefs.putInt("tz", cfg.tz);
  prefs.putBool("h12", cfg.h12);
  prefs.putInt("bright", cfg.bright);
  prefs.putBool("night", cfg.nightOff);
  prefs.putInt("dark", cfg.darkThr);
  prefs.putBool("wifi", cfg.wifiOn);
  prefs.end();
}

// ชั่วโมงที่จะแสดง (รองรับ 12 ชม.)
int dispHour(bool* pm) {
  int h = tnow.hour();
  *pm = h >= 12;
  if (cfg.h12) { h %= 12; if (h == 0) h = 12; }
  return h;
}

// ---------- เซนเซอร์ ----------
int readLightRaw() {
  int r = analogRead(LDR_PIN);
  return LDR_INVERT ? 4095 - r : r;
}

void updateSensors() {
  static unsigned long lastL = 0, lastD = 0;
  if (millis() - lastL >= 50) {
    lastL = millis();
    light = (light * 7 + readLightRaw()) / 8;
  }
  if (millis() - lastD >= 2500) {
    lastD = millis();
    float t = dht.readTemperature();
    float h = dht.readHumidity();
    if (!isnan(t) && !isnan(h)) {
      roomTemp = t;
      roomHum = h;
      if (isnan(tMin) || t < tMin) tMin = t;
      if (isnan(tMax) || t > tMax) tMax = t;
    }
  }
}

// ---------- จอ: เปิด/ปิด และความสว่าง ----------
void setScreen(bool on) {
  if (on == screenOn) return;
  display.ssd1306_command(on ? SSD1306_DISPLAYON : SSD1306_DISPLAYOFF);
  screenOn = on;
}

void setContrast(int c) {
  if (c == curContrast) return;
  display.ssd1306_command(SSD1306_SETCONTRAST);
  display.ssd1306_command((uint8_t)c);
  curContrast = c;
}

void updateScreenPower() {
  bool busy = uiMode != UI_PAGE || tmrState == TM_ALARM || tmrState == TM_EDIT;
  bool dark = light < cfg.darkThr;
  if (cfg.nightOff && dark && !busy && millis() - lastActivity > WAKE_MS) setScreen(false);
  else setScreen(true);

  int c;
  switch (cfg.bright) {
    case 1: c = 8; break;
    case 2: c = 90; break;
    case 3: c = 255; break;
    default:
      if (dark) c = 1;
      else c = map(constrain(light, cfg.darkThr, 4095), cfg.darkThr, 4095, 20, 255) & 0xF0;
      if (c < 1) c = 1;
  }
  setContrast(c);
}

void checkTimeouts() {
  if ((uiMode == UI_MENU || uiMode == UI_EDIT || uiMode == UI_INFO) &&
      millis() - lastActivity > MENU_TIMEOUT_MS) {
    uiMode = UI_PAGE;
    page = PG_CLOCK;
  }
  if (tmrState == TM_EDIT && millis() - lastActivity > MENU_TIMEOUT_MS) tmrState = TM_IDLE;
}

// ---------- setup / loop ----------
void setup() {
  Serial.begin(115200);
  Wire.begin(SDA_PIN, SCL_PIN);
  Wire.setClock(400000);
  pinMode(IR_PIN, INPUT);
  pinMode(BTN_PIN, INPUT_PULLUP);
  analogReadResolution(12);
  dht.begin();
  loadSettings();

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("OLED not found!");
    while (true) delay(1000);
  }
  display.setTextColor(SSD1306_WHITE);
  display.setTextWrap(false);
  drawSplash();

  if (!rtc.begin()) {
    Serial.println("DS3231 not found!");
    display.clearDisplay();
    printCenter("RTC NOT FOUND", 28, nullptr);
    display.display();
    while (true) delay(1000);
  }
  if (rtc.lostPower()) rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  tnow = rtc.now();

  light = readLightRaw();
  netBegin();
  lastActivity = millis();
}

void loop() {
  static unsigned long lastRtc = 0, lastFrame = 0, lastLog = 0;

  if (millis() - lastRtc >= 50) {
    lastRtc = millis();
    DateTime t = rtc.now();
    if (t.second() != tnow.second()) secStartMs = millis();
    tnow = t;
  }

  updateSensors();
  netLoop();

  int ev = readInput();
  if (ev != EV_NONE) handleEvent(ev);

  timerLoop();
  checkTimeouts();
  updateScreenPower();

  if (screenOn && millis() - lastFrame >= 40) {
    lastFrame = millis();
    render();
  }

  if (millis() - lastLog >= 2000) {
    lastLog = millis();
    Serial.printf("%02d:%02d:%02d Light=%d Temp=%.1f Hum=%.0f WiFi=%d\n",
                  tnow.hour(), tnow.minute(), tnow.second(), light, roomTemp, roomHum,
                  WiFi.status() == WL_CONNECTED);
  }
}
