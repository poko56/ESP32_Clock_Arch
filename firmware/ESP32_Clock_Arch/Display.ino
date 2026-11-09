// ================== หน้าจอแบบ "ARCH" (เลย์เอาต์ใหม่ทั้งหมด) ==================
//  นาฬิกา:   ตัวเลขใหญ่ 24pt, เส้นวินาทีด้านบน, แถบวันที่/อุณหภูมิแบบกลับสีด้านล่าง
//  อากาศ:    เกจครึ่งวงกลม 2 อัน (อุณหภูมิ / ความชื้น)
//  จับเวลา:  หน้าปัดเข็มวินาทีด้านซ้าย + ตัวเลขด้านขวา
//  ถอยหลัง:  วงแหวนเวลาที่เหลือด้านซ้าย + ตัวเลขด้านขวา
//  เมนู:     แบบหมุนทีละรายการ (carousel)
//  แก้ค่า:   ช่องตัวเลขเป็นกล่อง ช่องที่เลือกเป็นกล่องทึบ

// ---------- ตัวช่วยข้อความ ----------
int advWidth(const char* s, const GFXfont* f) {
  if (!f) return strlen(s) * 6;
  int w = 0;
  for (const char* p = s; *p; p++) {
    uint8_t c = (uint8_t)*p;
    if (c < f->first || c > f->last) continue;
    w += f->glyph[c - f->first].xAdvance;
  }
  return w;
}
// ฟอนต์ปกติ (nullptr) y = ขอบบน, ฟอนต์ Free* y = เส้นฐาน
void printAt(const char* s, int x, int y, const GFXfont* f) {
  display.setFont(f);
  display.setCursor(x, y);
  display.print(s);
}
void printCenter(const char* s, int y, const GFXfont* f) { printAt(s, 64 - advWidth(s, f) / 2, y, f); }
void printCenterAt(const char* s, int cx, int y, const GFXfont* f) { printAt(s, cx - advWidth(s, f) / 2, y, f); }
void printRight(const char* s, int xr, int y, const GFXfont* f) { printAt(s, xr - advWidth(s, f), y, f); }

void dottedLine(int y) { for (int x = 0; x < 128; x += 3) display.drawPixel(x, y, WHITE); }

// ---------- ไอคอน ----------
void iconWifi(int x, int y) {
  display.fillRect(x, y + 5, 2, 2, WHITE);
  display.fillRect(x + 3, y + 3, 2, 4, WHITE);
  display.fillRect(x + 6, y + 1, 2, 6, WHITE);
}
void iconHourglass(int x, int y) {
  display.drawFastHLine(x, y, 7, WHITE);
  display.drawFastHLine(x, y + 7, 7, WHITE);
  display.drawLine(x + 1, y + 1, x + 5, y + 6, WHITE);
  display.drawLine(x + 5, y + 1, x + 1, y + 6, WHITE);
}
void iconStopwatch(int x, int y) {
  display.drawCircle(x + 3, y + 4, 3, WHITE);
  display.drawFastVLine(x + 3, y + 2, 3, WHITE);
  display.drawPixel(x + 3, y, WHITE);
}

// ไอคอนสถานะเรียงจากขวาไปซ้าย, คืนค่าตำแหน่ง x ซ้ายสุดที่ใช้
int drawIndicators(int xr, int y) {
  int x = xr;
  if (portalOn) { x -= 12; printAt("AP", x, y, nullptr); x -= 3; }
  if (cfg.wifiOn && cfg.ssid.length()) {
    x -= 8;
    if (WiFi.status() == WL_CONNECTED || (millis() / 500) % 2) iconWifi(x, y);
    x -= 4;
  }
  if (tmrState == TM_RUN || tmrState == TM_PAUSE) { x -= 7; iconHourglass(x, y); x -= 4; }
  if (swRunning) { x -= 7; iconStopwatch(x, y); x -= 4; }
  return x;
}

// แท็บหัวข้อแบบกลับสีที่มุมซ้ายบน
void drawTab(const char* t) {
  int w = advWidth(t, nullptr) + 8;
  display.fillRoundRect(0, 0, w, 11, 3, WHITE);
  display.setTextColor(BLACK);
  printAt(t, 4, 2, nullptr);
  display.setTextColor(WHITE);
  drawIndicators(128, 2);
}

// ---------- รูปทรงวงกลม ----------
// เส้นโค้งหนา: มุม 0 = ขวา, 90 = บน (องศา)
void arcThick(int cx, int cy, int r, int t, float a0, float a1) {
  if (a1 < a0) { float tmp = a0; a0 = a1; a1 = tmp; }
  for (float a = a0; a <= a1; a += 1.5) {
    float c = cos(a * DEG_TO_RAD), s = sin(a * DEG_TO_RAD);
    for (int k = 0; k < t; k++)
      display.drawPixel(cx + (int)lround((r - k) * c), cy - (int)lround((r - k) * s), WHITE);
  }
}

// เกจครึ่งวงกลม (ซ้าย -> ขวา)
void drawGauge(int cx, int cy, int r, float frac) {
  for (int a = 0; a <= 180; a += 10)
    display.drawPixel(cx + (int)lround(r * cos(a * DEG_TO_RAD)), cy - (int)lround(r * sin(a * DEG_TO_RAD)), WHITE);
  display.drawFastHLine(cx - r, cy + 1, 4, WHITE);
  display.drawFastHLine(cx + r - 3, cy + 1, 4, WHITE);
  if (frac > 1) frac = 1;
  if (frac > 0.005) arcThick(cx, cy, r, 3, 180 - 180 * frac, 180);
}

// วงแหวนเต็มวง เริ่มที่ด้านบน หมุนตามเข็ม
void drawRing(int cx, int cy, int r, float frac) {
  display.drawCircle(cx, cy, r - 2, WHITE);
  if (frac > 1) frac = 1;
  if (frac > 0.003) arcThick(cx, cy, r, 4, 90 - 360 * frac, 90);
}

// หน้าปัดจับเวลา: เข็มวินาทีหมุนรอบละ 60 วิ
void drawDial(int cx, int cy, int r, float sec) {
  display.drawCircle(cx, cy, r, WHITE);
  for (int i = 0; i < 12; i++) {
    float a = i * 30 * DEG_TO_RAD;
    int l = (i % 3 == 0) ? 4 : 2;
    display.drawLine(cx + (int)lround((r - 1) * sin(a)), cy - (int)lround((r - 1) * cos(a)),
                     cx + (int)lround((r - l) * sin(a)), cy - (int)lround((r - l) * cos(a)), WHITE);
  }
  float a = sec * 6 * DEG_TO_RAD;
  display.drawLine(cx, cy, cx + (int)lround((r - 4) * sin(a)), cy - (int)lround((r - 4) * cos(a)), WHITE);
  display.fillCircle(cx, cy, 2, WHITE);
}

// ---------- หน้า: นาฬิกา ----------
const char* DOW_FULL[] = {"SUNDAY", "MONDAY", "TUESDAY", "WEDNESDAY", "THURSDAY", "FRIDAY", "SATURDAY"};

void drawClockPage() {
  char buf[24];

  // เส้นวินาทีวิ่งด้านบน
  float sp = (tnow.second() + min(999UL, millis() - secStartMs) / 1000.0) / 60.0;
  display.fillRect(0, 0, (int)(128 * sp), 2, WHITE);

  // เวลาตัวใหญ่ 24pt
  bool pm;
  int h = dispHour(&pm);
  char hh[4], mm[4];
  snprintf(hh, sizeof hh, "%02d", h);
  snprintf(mm, sizeof mm, "%02d", tnow.minute());
  const GFXfont* F = &FreeSansBold24pt7b;
  int wH = advWidth(hh, F), wM = advWidth(mm, F), gap = 12;
  int x0 = (128 - (wH + gap + wM)) / 2;
  int base = 38;
  printAt(hh, x0, base, F);
  printAt(mm, x0 + wH + gap, base, F);
  if (millis() - secStartMs < 500) {
    int cx = x0 + wH + gap / 2 - 2;
    display.fillRect(cx, base - 24, 4, 5, WHITE);
    display.fillRect(cx, base - 9, 4, 5, WHITE);
  }

  // แถวกลาง: ชื่อวันเต็ม (+AM/PM) และไอคอนสถานะ
  if (cfg.h12) snprintf(buf, sizeof buf, "%s  %s", DOW_FULL[tnow.dayOfTheWeek()], pm ? "PM" : "AM");
  else snprintf(buf, sizeof buf, "%s", DOW_FULL[tnow.dayOfTheWeek()]);
  printAt(buf, 1, 42, nullptr);
  drawIndicators(128, 42);

  // แถบล่างกลับสี: วันที่ | อุณหภูมิ ความชื้น
  display.fillRoundRect(0, 53, 128, 11, 3, WHITE);
  display.setTextColor(BLACK);
  snprintf(buf, sizeof buf, "%02d %s %04d", tnow.day(), MON[tnow.month() - 1], tnow.year());
  printAt(buf, 3, 55, nullptr);
  if (isnan(roomTemp)) strcpy(buf, "--C --%");
  else snprintf(buf, sizeof buf, "%.0fC %.0f%%", roomTemp, roomHum);
  printRight(buf, 125, 55, nullptr);
  display.setTextColor(WHITE);
}

// ---------- หน้า: อุณหภูมิ / ความชื้น ----------
const char* comfortLabel() {
  if (isnan(roomTemp)) return "";
  if (roomTemp >= 30) return "HOT";
  if (roomTemp <= 18) return "COLD";
  if (roomHum >= 70) return "HUMID";
  if (roomHum <= 30) return "DRY";
  return "COMFY";
}

void drawGaugeValue(int cx, float v, const char* unit) {
  char ib[8], db[4];
  const GFXfont* F = &FreeSansBold12pt7b;
  if (isnan(v)) { strcpy(ib, "--"); db[0] = 0; }
  else {
    snprintf(ib, sizeof ib, "%d", (int)v);
    if (unit[0] == 'C') snprintf(db, sizeof db, ".%d", (int)(fabs(v) * 10) % 10); else db[0] = 0;
  }
  int x = cx - advWidth(ib, F) / 2;                        // จัดกลางเฉพาะจำนวนเต็ม ทศนิยมห้อยขวา
  printAt(ib, x, 39, F);                                    // ตัวเลขอยู่ใต้จุดศูนย์กลางเกจ
  if (db[0]) printAt(db, x + advWidth(ib, F), 32, nullptr);
}

void drawClimatePage() {
  char buf[20];
  bool pm;
  int h = dispHour(&pm);
  if (cfg.h12) snprintf(buf, sizeof buf, "%d:%02d %s", h, tnow.minute(), pm ? "PM" : "AM");
  else snprintf(buf, sizeof buf, "%02d:%02d", h, tnow.minute());
  printCenter(buf, 0, nullptr);
  drawIndicators(128, 0);

  drawGauge(32, 30, 20, isnan(roomTemp) ? 0 : roomTemp / 50.0);
  drawGauge(96, 30, 20, isnan(roomHum) ? 0 : roomHum / 100.0);
  drawGaugeValue(32, roomTemp, "C");
  drawGaugeValue(96, roomHum, "%");
  printCenterAt("TEMP C", 32, 42, nullptr);
  printCenterAt("HUMID %", 96, 42, nullptr);

  dottedLine(51);
  if (!isnan(tMax)) {
    snprintf(buf, sizeof buf, "H%.1f L%.1f", tMax, tMin);
    printAt(buf, 0, 55, nullptr);
  }
  const char* c = comfortLabel();
  if (*c) {
    int w = advWidth(c, nullptr) + 8;
    display.fillRoundRect(128 - w, 53, w, 11, 5, WHITE);
    display.setTextColor(BLACK);
    printAt(c, 128 - w + 4, 55, nullptr);
    display.setTextColor(WHITE);
  }
}

// ---------- ตัวเลขเวลาด้านขวา (ใช้กับจับเวลา/ถอยหลัง) ----------
void drawRightDuration(unsigned long sec, const char* suffix, bool boxed) {
  char buf[16];
  const GFXfont* F = &FreeSansBold12pt7b;
  if (sec >= 3600) {
    snprintf(buf, sizeof buf, "%d:%02d:%02d", (int)(sec / 3600), (int)(sec / 60 % 60), (int)(sec % 60));
    suffix = nullptr;
  } else {
    snprintf(buf, sizeof buf, "%02d:%02d", (int)(sec / 60), (int)(sec % 60));
  }
  int x = 45, base = 34;
  printAt(buf, x, base, F);
  int w = advWidth(buf, F);
  if (suffix) printAt(suffix, x + w + 1, base - 7, nullptr);   // .cc ตัวเล็กด้านขวา
  if (boxed) display.drawRoundRect(x - 4, base - 21, w + 8, 27, 4, WHITE);
}

void drawRightHint(const char* l1, const char* l2) {
  if (l2) { printAt(l1, 45, 44, nullptr); printAt(l2, 45, 54, nullptr); }
  else printAt(l1, 45, 49, nullptr);
}

// ---------- หน้า: จับเวลา ----------
void drawStopwatchPage() {
  drawTab("STOPWATCH");
  unsigned long ms = swElapsed();
  drawDial(21, 38, 18, (ms % 60000) / 1000.0);
  char cs[5];
  snprintf(cs, sizeof cs, ".%02d", (int)(ms % 1000 / 10));
  drawRightDuration(ms / 1000, cs, false);
  if (swRunning) drawRightHint("hold: pause", nullptr);
  else if (ms > 0) drawRightHint("hold: go", "3s:   reset");
  else drawRightHint("hold: start", nullptr);
}

// ---------- หน้า: นับถอยหลัง ----------
void drawTimerPage() {
  drawTab("TIMER");
  unsigned long left = timerLeftMs();
  float frac = (tmrState == TM_RUN || tmrState == TM_PAUSE) ? left / (tmrTotal * 1000.0) : 1.0;
  drawRing(21, 38, 18, frac);
  // สัญลักษณ์กลางวง
  if (tmrState == TM_RUN) display.fillTriangle(18, 33, 18, 43, 26, 38, WHITE);
  else if (tmrState == TM_PAUSE) { display.fillRect(16, 33, 3, 10, WHITE); display.fillRect(22, 33, 3, 10, WHITE); }
  else printCenterAt("SET", 21, 35, nullptr);

  bool blinkBox = tmrState == TM_EDIT && (millis() / 400) % 2;
  drawRightDuration((left + 999) / 1000, nullptr, blinkBox);

  if (tmrState == TM_EDIT) drawRightHint("wave: +time", "hold: start");
  else if (tmrState == TM_RUN) drawRightHint("hold: pause", nullptr);
  else if (tmrState == TM_PAUSE) drawRightHint("hold: go", "3s:   reset");
  else drawRightHint("hold: set", nullptr);
}

// ---------- หมดเวลา ----------
void drawAlarm() {
  int t = (millis() / 30) % 36;
  for (int k = 0; k < 3; k++) display.drawCircle(64, 32, (t + k * 12) % 36 + 30, WHITE);
  display.fillRoundRect(22, 18, 84, 28, 6, WHITE);
  display.setTextColor(BLACK);
  printCenter("TIME UP", 37, &FreeSansBold9pt7b);
  display.setTextColor(WHITE);
  display.fillRect(28, 52, 72, 10, BLACK);
  printCenter("wave to stop", 53, nullptr);
}

// ---------- เมนูแบบหมุนทีละรายการ ----------
void drawMenu() {
  char idx[8];
  snprintf(idx, sizeof idx, "%d/%d", menuSel + 1, MI_COUNT);
  printAt("SETTINGS", 0, 0, nullptr);
  printRight(idx, 128, 0, nullptr);
  dottedLine(9);

  int prev = (menuSel + MI_COUNT - 1) % MI_COUNT;
  int next = (menuSel + 1) % MI_COUNT;
  printCenter(MENU_LABELS[prev], 11, nullptr);

  display.drawRoundRect(2, 20, 124, 33, 6, WHITE);
  printCenter(MENU_LABELS[menuSel], 35, &FreeSansBold9pt7b);
  String v = menuValue(menuSel);
  if (v.length()) {
    int w = advWidth(v.c_str(), nullptr) + 12;
    display.fillRoundRect((128 - w) / 2, 39, w, 11, 5, WHITE);
    display.setTextColor(BLACK);
    printCenter(v.c_str(), 41, nullptr);
    display.setTextColor(WHITE);
  }

  display.fillTriangle(4, 56, 10, 56, 7, 60, WHITE);         // ลูกศร = โบกไปรายการถัดไป
  display.fillTriangle(118, 56, 124, 56, 121, 60, WHITE);
  printCenter(MENU_LABELS[next], 55, nullptr);
}

// ---------- ตัวแก้ค่า: ช่องเป็นกล่อง ----------
void drawEditor() {
  const char* titles[] = {"SET TIME", "SET DATE", "TIME ZONE", "DARK LEVEL"};
  printCenter(titles[editKind], 0, nullptr);
  dottedLine(9);

  const GFXfont* F = (editKind == ED_DATE) ? &FreeSansBold9pt7b : &FreeSansBold12pt7b;
  int fh = (editKind == ED_DATE) ? 13 : 17;
  char parts[3][10];
  const char* sep = "";
  switch (editKind) {
    case ED_TIME:
      snprintf(parts[0], 10, "%02d", editVal[0]);
      snprintf(parts[1], 10, "%02d", editVal[1]);
      sep = ":";
      break;
    case ED_DATE:
      snprintf(parts[0], 10, "%02d", editVal[0]);
      snprintf(parts[1], 10, "%02d", editVal[1]);
      snprintf(parts[2], 10, "%04d", editVal[2]);
      sep = "/";
      break;
    case ED_TZ:   snprintf(parts[0], 10, "UTC%+d", editVal[0]); break;
    case ED_DARK: snprintf(parts[0], 10, "%d", DARK_STEPS[editVal[0]]); break;
  }

  const int pad = 4, sepGap = 3;
  int total = 0;
  for (int i = 0; i < editN; i++) total += advWidth(parts[i], F) + 2 * pad;
  total += (editN - 1) * (advWidth(sep, F) + 2 * sepGap);
  int x = (128 - total) / 2, base = 36;
  for (int i = 0; i < editN; i++) {
    int w = advWidth(parts[i], F) + 2 * pad;
    if (i == editIdx) {
      display.fillRoundRect(x, base - fh - 3, w, fh + 7, 4, WHITE);
      display.setTextColor(BLACK);
    } else {
      display.drawRoundRect(x, base - fh - 3, w, fh + 7, 4, WHITE);
    }
    printAt(parts[i], x + pad, base, F);
    display.setTextColor(WHITE);
    x += w;
    if (i < editN - 1) { printAt(sep, x + sepGap, base, F); x += advWidth(sep, F) + 2 * sepGap; }
  }

  if (editKind == ED_DARK) {
    char b[24];
    snprintf(b, sizeof b, "light now %d", light);
    printCenter(b, 44, nullptr);
  }
  printAt("wave +1", 0, 56, nullptr);
  printRight(editIdx < editN - 1 ? "hold next" : "hold save", 128, 56, nullptr);
}

// ---------- ข้อมูลระบบ: ป้าย | ค่า ----------
void infoRow(int y, const char* label, const char* value) {
  printAt(label, 0, y, nullptr);
  char v[16];
  strncpy(v, value, 15);
  v[15] = 0;
  printRight(v, 128, y, nullptr);
}

void drawInfo() {
  printAt("INFO", 0, 0, nullptr);
  printRight("v" FW_VERSION, 128, 0, nullptr);
  dottedLine(9);
  char b[24];
  unsigned long up = millis() / 60000;
  snprintf(b, sizeof b, "%lud %luh %lum", up / 1440, up / 60 % 24, up % 60);
  infoRow(12, "Uptime", b);
  infoRow(22, "WiFi", wifiStatusText().c_str());
  String ip = "-";
  if (WiFi.status() == WL_CONNECTED) ip = WiFi.localIP().toString();
  else if (portalOn) ip = WiFi.softAPIP().toString();
  infoRow(32, "IP", ip.c_str());
  if (ntpOk) snprintf(b, sizeof b, "%lu min ago", (millis() - lastNtpMs) / 60000);
  else strcpy(b, "never");
  infoRow(42, "NTP", b);
  snprintf(b, sizeof b, "%.1fC", rtc.getTemperature());
  infoRow(52, "RTC temp", b);
}

// ---------- ตั้งค่า WiFi ผ่านมือถือ: เป็นขั้นตอน ----------
void stepNum(int x, int y, const char* n) {
  display.fillCircle(x + 4, y + 3, 5, WHITE);
  display.setTextColor(BLACK);
  printAt(n, x + 2, y, nullptr);
  display.setTextColor(WHITE);
}

void drawPortal() {
  display.fillRect(0, 0, 128, 11, WHITE);
  display.setTextColor(BLACK);
  printAt("WIFI SETUP", 3, 2, nullptr);
  char b[24];
  snprintf(b, sizeof b, "%d phone", WiFi.softAPgetStationNum());
  printRight(b, 125, 2, nullptr);
  display.setTextColor(WHITE);

  stepNum(0, 15, "1");
  printAt(AP_SSID, 14, 15, nullptr);
  printAt("pass " AP_PASS, 14, 25, nullptr);
  stepNum(0, 38, "2");
  printAt("192.168.4.1", 14, 38, nullptr);
  printRight("hold: close", 128, 55, nullptr);
}

// ---------- ส่วนซ้อนทับ ----------
void drawHoldBar() {
  long d = holdMs();
  if (d < 200) return;
  bool chained = holdChained();
  if (!holdDefer && !chained) {            // แถบที่ "ด้านบน" จอ
    display.fillRect(0, 0, 128, 3, BLACK);
    display.fillRect(0, 0, (int)(128 * min(1.0f, d / (float)LONG_MS)), 3, WHITE);
    return;
  }
  // ป๊อปอัปรีเซ็ตแบบกลับสี
  display.fillRoundRect(8, 14, 112, 36, 6, WHITE);
  display.setTextColor(BLACK);
  if (chained) {
    printCenter("keep holding", 18, nullptr);
    printCenter("RESET", 36, &FreeSansBold9pt7b);
  } else if (d >= (long)LONG_MS) {
    printCenter("let go = RESUME", 18, nullptr);
    printCenter("hold = RESET", 28, nullptr);
  } else {
    printCenter("keep holding...", 24, nullptr);
  }
  display.setTextColor(WHITE);
  display.fillRect(14, 42, 100, 4, BLACK);
  display.fillRect(14, 42, (int)(100 * min(1.0f, d / (float)VLONG_MS)), 4, WHITE);
  display.drawRect(14, 42, 100, 4, BLACK);
}

void drawToast() {                          // แบนเนอร์ด้านบน
  if ((long)(toastUntil - millis()) <= 0) return;
  display.fillRect(0, 0, 128, 12, WHITE);
  display.setTextColor(BLACK);
  printCenter(toastMsg, 2, nullptr);
  display.setTextColor(WHITE);
}

void drawSplash() {
  display.clearDisplay();
  display.drawCircle(64, 24, 20, WHITE);
  display.drawCircle(64, 24, 19, WHITE);
  for (int i = 0; i < 12; i++) {
    float a = i * 30 * DEG_TO_RAD;
    display.drawPixel(64 + (int)lround(16 * sin(a)), 24 - (int)lround(16 * cos(a)), WHITE);
  }
  display.drawLine(64, 24, 64, 12, WHITE);
  display.drawLine(64, 24, 73, 28, WHITE);
  display.fillCircle(64, 24, 2, WHITE);
  printCenter("ARCH CLOCK", 50, nullptr);
  display.display();
  delay(900);
}

// ---------- วาดทั้งหมด ----------
void render() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);

  if (tmrState == TM_ALARM) {
    drawAlarm();
  } else {
    switch (uiMode) {
      case UI_PAGE:
        switch (page) {
          case PG_CLOCK:     drawClockPage(); break;
          case PG_CLIMATE:   drawClimatePage(); break;
          case PG_STOPWATCH: drawStopwatchPage(); break;
          case PG_TIMER:     drawTimerPage(); break;
        }
        break;
      case UI_MENU:   drawMenu(); break;
      case UI_EDIT:   drawEditor(); break;
      case UI_INFO:   drawInfo(); break;
      case UI_PORTAL: drawPortal(); break;
    }
  }
  drawHoldBar();
  drawToast();
  display.display();
}
