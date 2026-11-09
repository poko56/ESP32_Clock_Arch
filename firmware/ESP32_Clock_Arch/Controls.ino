// ================== การควบคุม: หน้าหลัก, จับเวลา, นับถอยหลัง, เมนู ==================

// ---------- เมนูตั้งค่า ----------
enum { MI_TIME, MI_DATE, MI_FMT, MI_BRIGHT, MI_NIGHT, MI_DARK, MI_TZ,
       MI_WIFI, MI_SETUP, MI_SYNC, MI_INFO, MI_EXIT, MI_COUNT };
const char* MENU_LABELS[MI_COUNT] = {
  "Set Time", "Set Date", "Time Format", "Brightness", "Night Off", "Dark Level",
  "Time Zone", "WiFi", "WiFi Setup", "Sync Time", "Info", "Exit"
};
int menuSel = 0;

// ---------- ตัวแก้ค่า ----------
enum { ED_TIME, ED_DATE, ED_TZ, ED_DARK };
const int DARK_STEPS[] = {50, 100, 200, 300, 500, 800, 1200, 1600, 2000, 2500};
const int DARK_NSTEPS = sizeof(DARK_STEPS) / sizeof(DARK_STEPS[0]);
int editKind = ED_TIME, editIdx = 0, editN = 1;
int editVal[3], editMin[3], editMax[3];

String menuValue(int i) {
  char b[12];
  switch (i) {
    case MI_FMT:    return cfg.h12 ? "12H" : "24H";
    case MI_BRIGHT: { const char* v[] = {"Auto", "Low", "Mid", "High"}; return v[cfg.bright]; }
    case MI_NIGHT:  return cfg.nightOff ? "On" : "Off";
    case MI_DARK:   return String(cfg.darkThr);
    case MI_TZ:     snprintf(b, sizeof b, "UTC%+d", cfg.tz); return b;
    case MI_WIFI:   return cfg.wifiOn ? "On" : "Off";
    case MI_SETUP:  return ">";
    case MI_INFO:   return ">";
    default:        return "";
  }
}

void setField(int i, int v, int mn, int mx) { editVal[i] = v; editMin[i] = mn; editMax[i] = mx; }

void startEdit(int kind) {
  editKind = kind;
  editIdx = 0;
  uiMode = UI_EDIT;
  switch (kind) {
    case ED_TIME:
      editN = 2;
      setField(0, tnow.hour(), 0, 23);
      setField(1, tnow.minute(), 0, 59);
      break;
    case ED_DATE:
      editN = 3;
      setField(0, tnow.day(), 1, 31);
      setField(1, tnow.month(), 1, 12);
      setField(2, constrain((int)tnow.year(), 2024, 2099), 2024, 2099);
      break;
    case ED_TZ:
      editN = 1;
      setField(0, cfg.tz, -12, 14);
      break;
    case ED_DARK: {
      editN = 1;
      int best = 0;
      for (int i = 0; i < DARK_NSTEPS; i++)
        if (abs(DARK_STEPS[i] - cfg.darkThr) < abs(DARK_STEPS[best] - cfg.darkThr)) best = i;
      setField(0, best, 0, DARK_NSTEPS - 1);
      break;
    }
  }
}

int daysInMonth(int m, int y) {
  const int d[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
  if (m == 2 && ((y % 4 == 0 && y % 100 != 0) || y % 400 == 0)) return 29;
  return d[m - 1];
}

void commitEdit() {
  switch (editKind) {
    case ED_TIME:
      rtc.adjust(DateTime(tnow.year(), tnow.month(), tnow.day(), editVal[0], editVal[1], 0));
      toast("Time saved");
      break;
    case ED_DATE: {
      int d = min(editVal[0], daysInMonth(editVal[1], editVal[2]));
      rtc.adjust(DateTime(editVal[2], editVal[1], d, tnow.hour(), tnow.minute(), tnow.second()));
      toast("Date saved");
      break;
    }
    case ED_TZ:
      cfg.tz = editVal[0];
      saveSettings();
      if (WiFi.status() == WL_CONNECTED) startNTP();
      toast("Saved");
      break;
    case ED_DARK:
      cfg.darkThr = DARK_STEPS[editVal[0]];
      saveSettings();
      toast("Saved");
      break;
  }
  tnow = rtc.now();
  uiMode = UI_MENU;
}

void editEvent(int ev) {
  if (ev == EV_SHORT) {
    if (++editVal[editIdx] > editMax[editIdx]) editVal[editIdx] = editMin[editIdx];
  } else if (ev == EV_LONG) {
    if (editIdx < editN - 1) editIdx++;
    else commitEdit();
  } else if (ev == EV_VLONG) {
    uiMode = UI_MENU;
    toast("Cancelled");
  }
}

void menuEvent(int ev) {
  if (ev == EV_SHORT) { menuSel = (menuSel + 1) % MI_COUNT; return; }
  if (ev == EV_VLONG) { uiMode = UI_PAGE; return; }

  switch (menuSel) {   // EV_LONG = เลือก
    case MI_TIME:   startEdit(ED_TIME); break;
    case MI_DATE:   startEdit(ED_DATE); break;
    case MI_FMT:    cfg.h12 = !cfg.h12; saveSettings(); break;
    case MI_BRIGHT: cfg.bright = (cfg.bright + 1) % 4; saveSettings(); break;
    case MI_NIGHT:  cfg.nightOff = !cfg.nightOff; saveSettings(); break;
    case MI_DARK:   startEdit(ED_DARK); break;
    case MI_TZ:     startEdit(ED_TZ); break;
    case MI_WIFI:
      cfg.wifiOn = !cfg.wifiOn;
      saveSettings();
      wifiApply();
      if (!cfg.wifiOn) toast("WiFi off");
      else toast(cfg.ssid.length() ? "Connecting..." : "Use WiFi Setup");
      break;
    case MI_SETUP:  startPortal(); uiMode = UI_PORTAL; break;
    case MI_SYNC:   syncNow(); break;
    case MI_INFO:   uiMode = UI_INFO; break;
    case MI_EXIT:   uiMode = UI_PAGE; break;
  }
}

// ---------- จับเวลา ----------
unsigned long swElapsed() { return swAccum + (swRunning ? millis() - swStart : 0); }

// ---------- นับถอยหลัง ----------
unsigned long timerLeftMs() {
  if (tmrState == TM_RUN) { long d = (long)(tmrEnd - millis()); return d > 0 ? d : 0; }
  if (tmrState == TM_PAUSE) return tmrLeftMs;
  return tmrTotal * 1000UL;
}

void timerEvent(int ev) {
  switch (tmrState) {
    case TM_IDLE:
      if (ev == EV_SHORT) page = (page + 1) % PG_COUNT;
      else if (ev == EV_LONG) tmrState = TM_EDIT;
      break;
    case TM_EDIT:
      if (ev == EV_SHORT) { tmrPreset = (tmrPreset + 1) % TMR_NPRESET; tmrTotal = TMR_PRESETS[tmrPreset]; }
      else if (ev == EV_LONG) { tmrEnd = millis() + tmrTotal * 1000UL; tmrState = TM_RUN; }
      else if (ev == EV_VLONG) tmrState = TM_IDLE;
      break;
    case TM_RUN:
      if (ev == EV_SHORT) page = (page + 1) % PG_COUNT;
      else if (ev == EV_LONG) { tmrLeftMs = timerLeftMs(); tmrState = TM_PAUSE; }
      else if (ev == EV_VLONG) { tmrState = TM_IDLE; toast("Reset"); }
      break;
    case TM_PAUSE:
      if (ev == EV_SHORT) page = (page + 1) % PG_COUNT;
      else if (ev == EV_LONG) { tmrEnd = millis() + tmrLeftMs; tmrState = TM_RUN; }
      else if (ev == EV_VLONG) { tmrState = TM_IDLE; toast("Reset"); }
      break;
  }
}

void timerLoop() {
  if (tmrState == TM_RUN && (long)(millis() - tmrEnd) >= 0) {
    tmrState = TM_ALARM;
    alarmStart = millis();
    lastActivity = millis();
    setScreen(true);
  }
  if (tmrState == TM_ALARM && millis() - alarmStart > 60000UL) stopAlarm();
}

void stopAlarm() {
  tmrState = TM_IDLE;
  display.invertDisplay(false);
}

// ---------- หน้าหลัก ----------
void pageEvent(int ev) {
  switch (page) {
    case PG_CLOCK:
    case PG_CLIMATE:
      if (ev == EV_SHORT) page = (page + 1) % PG_COUNT;
      else if (ev == EV_LONG) { uiMode = UI_MENU; menuSel = 0; }
      else if (ev == EV_VLONG) page = PG_CLOCK;
      break;
    case PG_STOPWATCH:
      if (ev == EV_SHORT) page = (page + 1) % PG_COUNT;
      else if (ev == EV_LONG) {
        if (swRunning) { swAccum += millis() - swStart; swRunning = false; }
        else { swStart = millis(); swRunning = true; }
      } else if (ev == EV_VLONG) { swRunning = false; swAccum = 0; toast("Reset"); }
      break;
    case PG_TIMER:
      timerEvent(ev);
      break;
  }
}

void handleEvent(int ev) {
  switch (uiMode) {
    case UI_PAGE:  pageEvent(ev); break;
    case UI_MENU:  menuEvent(ev); break;
    case UI_EDIT:  editEvent(ev); break;
    case UI_INFO:  uiMode = UI_MENU; break;
    case UI_PORTAL:
      if (ev != EV_SHORT) { stopPortal(); uiMode = UI_MENU; }
      break;
  }
}
