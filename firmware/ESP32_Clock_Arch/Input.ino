// ================== อินพุต: IR (โบกมือ) + ปุ่ม BOOT ==================
// ปกติ:
//   โบกสั้น = EV_SHORT (ตอนเอามือออก)
//   ค้างครบ 0.7 วิ = EV_LONG (ทำงานทันทีระหว่างค้าง)
// ตอนหยุดชั่วคราว (จับเวลา/นับถอยหลัง) — เลือกได้ 2 ทาง:
//   ค้างแล้วเอามือออกก่อน 3 วิ = EV_LONG (ทำต่อ)
//   ค้างจนครบ 3 วิ             = EV_VLONG (รีเซ็ต)

// (holdDefer ประกาศไว้ในไฟล์หลัก เพราะ Display.ino ใช้ด้วย)
bool inHeld = false, longFired = false, vlongFired = false, swallow = false;
unsigned long pressStart = 0;

bool pressedRaw() {
  return digitalRead(IR_PIN) == LOW || digitalRead(BTN_PIN) == LOW;
}

// หน้าที่ "ค้างนาน = รีเซ็ต" ได้: จับเวลาที่หยุดอยู่ หรือ นับถอยหลังที่หยุดอยู่
bool vlongAllowed() {
  if (uiMode != UI_PAGE) return false;
  if (page == PG_STOPWATCH) return !swRunning && swAccum > 0;
  if (page == PG_TIMER) return tmrState == TM_PAUSE;
  return false;
}

int readInput() {
  static bool lastRaw = false;
  static unsigned long lastChange = 0;

  bool raw = pressedRaw();
  if (raw != lastRaw) { lastRaw = raw; lastChange = millis(); }
  // กันสัญญาณกระพือ: กดต้องนิ่ง 30ms, ปล่อยต้องนิ่ง 300ms
  // (เซนเซอร์ IR หลุดสั้นๆ ระหว่างค้างมือ ถ้าไม่เผื่อไว้จะกลายเป็นกดใหม่)
  if (millis() - lastChange < (raw ? 30UL : 300UL)) return EV_NONE;

  int ev = EV_NONE;
  unsigned long held = 0;
  if (raw && !inHeld) {
    inHeld = true;
    pressStart = lastChange;
    longFired = vlongFired = swallow = false;
    holdDefer = vlongAllowed();
    lastActivity = millis();
    if (!screenOn) { setScreen(true); swallow = true; }         // แค่ปลุกจอ
    if (tmrState == TM_ALARM) { stopAlarm(); swallow = true; }  // แค่หยุดการแจ้งเตือน
  } else if (raw && inHeld) {
    held = millis() - pressStart;
    if (!swallow && !holdDefer && !longFired && held >= LONG_MS) {
      longFired = true;
      ev = EV_LONG;
    } else if (!swallow && !vlongFired && held >= VLONG_MS &&
               (holdDefer || (longFired && resetPage()))) {
      vlongFired = true;
      ev = EV_VLONG;
    }
  } else if (!raw && inHeld) {
    inHeld = false;
    held = lastChange - pressStart;          // เวลาจริงที่มือบังอยู่ (ไม่รวมช่วงกันกระพือ)
    if (!swallow && !longFired && !vlongFired)
      ev = (holdDefer && held >= LONG_MS) ? EV_LONG : EV_SHORT;
  }

  if (ev != EV_NONE) lastActivity = millis();
  return ev;
}

// หน้าที่ค้างต่อถึง 3 วิ = รีเซ็ต (ทั้งตอนเดินอยู่และหยุดอยู่)
bool resetPage() {
  return uiMode == UI_PAGE && (page == PG_STOPWATCH || page == PG_TIMER);
}

// ค้างมือต่อหลังจากสั่ง "ตกลง" ไปแล้ว เพื่อไปถึงรีเซ็ต
bool holdChained() { return inHeld && longFired && !vlongFired && resetPage(); }

// ระยะเวลาที่กำลังค้างมืออยู่ (ms), -1 ถ้าไม่ได้ค้าง หรือทำงานจบแล้ว
long holdMs() {
  if (!inHeld || swallow || vlongFired) return -1;
  if (longFired && !resetPage()) return -1;
  return millis() - pressStart;
}
