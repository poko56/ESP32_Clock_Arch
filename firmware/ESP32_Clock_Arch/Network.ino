// ================== WiFi, NTP และหน้าเว็บตั้งค่า ==================
// ทุกอย่างไม่บล็อก: ถ้าไม่มีเน็ต นาฬิกาจะใช้เวลาจาก DS3231 ต่อไปตามปกติ
//
// ลำดับการทำงานของ WiFi:
//  - ยังไม่ได้ตั้ง WiFi บ้าน          -> เปิด WiFi ตั้งค่า (AP) ทันทีตอนเปิดเครื่อง
//  - ต่อ WiFi บ้านไม่ได้ภายใน 30 วิ  -> หยุดพยายามต่อ แล้วเปิด AP ให้มือถือเข้ามาแก้ได้
//  - เน็ตหลุดเกิน 30 วิ             -> เหมือนข้อบน
//  - AP เปิดครบ 10 นาที            -> ปิด AP แล้วลองต่อ WiFi บ้านใหม่อีกรอบ
//  ระหว่างเปิด AP จะไม่พยายามต่อ WiFi บ้านเบื้องหลัง เพราะการลองต่อจะทำให้ AP
//  กระโดดช่องสัญญาณจนมือถือเข้าไม่ได้

#define STA_TRY_MS   30000UL     // ลองต่อ WiFi บ้านนานเท่านี้ก่อนยอมแพ้
#define PORTAL_MS   600000UL     // เปิด AP นานเท่านี้แล้วปิดเอง

bool staTrying = false;          // กำลังพยายามต่อ WiFi บ้าน
unsigned long staStart = 0;
bool autoPortal = false;         // AP ที่เปิดเองอัตโนมัติ (ไม่ได้กดจากเมนู)

void timeSyncCb(struct timeval* tv) { sntpEvent = true; }

bool wifiConfigured() { return cfg.wifiOn && cfg.ssid.length(); }

void staBegin() {
  WiFi.mode(portalOn ? WIFI_AP_STA : WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.begin(cfg.ssid.c_str(), cfg.pass.c_str());
  staTrying = true;
  staStart = millis();
  Serial.printf("WiFi: connecting to %s\n", cfg.ssid.c_str());
}

void staStop() {
  WiFi.setAutoReconnect(false);
  WiFi.disconnect(false);        // หยุดพยายามต่อ แต่ไม่ปิดวิทยุ (AP ยังทำงานได้)
  staTrying = false;
}

void netBegin() {
  WiFi.persistent(false);
  sntp_set_time_sync_notification_cb(timeSyncCb);
  wifiApply();
  if (cfg.wifiOn && !cfg.ssid.length()) {   // ยังไม่เคยตั้ง WiFi -> เปิด AP ให้เลย
    startPortal();
    autoPortal = true;
  }
}

void wifiApply() {
  if (wifiConfigured()) {
    staBegin();
  } else {
    staStop();
    WiFi.mode(portalOn ? WIFI_AP : WIFI_OFF);
  }
}

void startNTP() {
  configTime(cfg.tz * 3600L, 0, "pool.ntp.org", "time.google.com", "time.cloudflare.com");
}

void syncNow() {
  if (WiFi.status() == WL_CONNECTED) { syncToast = true; startNTP(); toast("Syncing..."); }
  else toast("No internet");
}

void applyNtpToRtc() {
  time_t t = time(nullptr);
  if (t < 1700000000) return;
  struct tm tm;
  localtime_r(&t, &tm);
  rtc.adjust(DateTime(tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec));
  tnow = rtc.now();
  if (!ntpOk || syncToast) toast("Time synced");
  ntpOk = true;
  syncToast = false;
  lastNtpMs = millis();
  Serial.println("NTP -> RTC updated");
}

String wifiStatusText() {
  if (!cfg.wifiOn) return "off";
  if (!cfg.ssid.length()) return "not set";
  if (WiFi.status() == WL_CONNECTED) return cfg.ssid;
  if (staTrying) return "connecting";
  return portalOn ? "failed, AP on" : "offline";
}

void ensureServer() {
  if (serverStarted) return;
  setupRoutes();
  server.begin();
  serverStarted = true;
}

void startPortal() {
  if (portalOn) return;
  bool online = WiFi.status() == WL_CONNECTED;
  if (!online) staStop();        // หยุดลองต่อ WiFi บ้าน ไม่งั้น AP จะไม่นิ่ง
  portalOn = true;
  autoPortal = false;
  WiFi.mode(online ? WIFI_AP_STA : WIFI_AP);
  WiFi.softAP(AP_SSID, AP_PASS);
  delay(100);
  dns.start(53, "*", WiFi.softAPIP());
  WiFi.scanNetworks(true);       // สแกนหา WiFi รอบๆ เบื้องหลัง (ไว้ให้เลือกในหน้าเว็บ)
  ensureServer();
  portalStart = millis();
  Serial.printf("AP on: %s  http://192.168.4.1\n", AP_SSID);
}

void stopPortal() {
  if (!portalOn) return;
  dns.stop();
  WiFi.softAPdisconnect(true);
  portalOn = false;
  autoPortal = false;
  if (uiMode == UI_PORTAL) uiMode = UI_PAGE;
  if (wifiConfigured()) {
    if (WiFi.status() != WL_CONNECTED) staBegin();   // ลองต่อ WiFi บ้านใหม่อีกรอบ
  } else {
    WiFi.mode(WIFI_OFF);
  }
  Serial.println("AP off");
}

void netLoop() {
  static bool wasConnected = false;
  bool c = WiFi.status() == WL_CONNECTED;

  if (c && !wasConnected) {
    staTrying = false;
    Serial.print("WiFi connected, IP: ");
    Serial.println(WiFi.localIP());
    ensureServer();
    startNTP();
    toast("WiFi connected");
  }
  if (!c && wasConnected && wifiConfigured()) {      // เน็ตหลุด: ให้เวลาต่อกลับ 30 วิ
    staTrying = true;
    staStart = millis();
    Serial.println("WiFi lost, retrying");
  }
  wasConnected = c;

  // ต่อไม่ได้ภายในเวลาที่กำหนด -> หยุด แล้วเปิด AP ให้เข้ามาแก้
  if (staTrying && !c && millis() - staStart > STA_TRY_MS) {
    Serial.println("WiFi: gave up, opening setup AP");
    staStop();
    if (!portalOn) {
      startPortal();
      autoPortal = true;
      toast("WiFi fail: AP on");
    }
  }

  if (sntpEvent) { sntpEvent = false; applyNtpToRtc(); }
  if (serverStarted) server.handleClient();
  if (portalOn) {
    dns.processNextRequest();
    // AP ที่เปิดเองอัตโนมัติ: ถ้าต่อเน็ตได้แล้ว (เช่นเพิ่งแก้รหัสจากหน้าเว็บ) ปิด AP หลัง 30 วิ
    if (autoPortal && c && millis() - portalStart > 30000UL) stopPortal();
    if (millis() - portalStart > PORTAL_MS) stopPortal();
  }
}

// ---------- หน้าเว็บ ----------
const char PAGE_HEAD[] = R"HTML(<!doctype html><html lang="th"><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1"><title>ESP32 Arch Clock</title><style>
body{font-family:system-ui,-apple-system,sans-serif;background:#0f1012;color:#eee;margin:0 auto;padding:16px;max-width:480px}
h1{font-size:20px;margin:4px 0 14px}.card{background:#1b1c1f;border-radius:14px;padding:16px;margin-bottom:14px}
.big{font-size:44px;font-weight:700;text-align:center;letter-spacing:2px;font-variant-numeric:tabular-nums}
.sub{text-align:center;color:#9a9ca3;margin-top:4px}.st{color:#9a9ca3;font-size:14px;margin-top:4px}
label{display:block;margin:10px 0 4px;color:#b7b9bf;font-size:14px}
input,select{width:100%;box-sizing:border-box;padding:11px;border-radius:9px;border:1px solid #33353a;background:#0b0c0d;color:#eee;font-size:16px}
button{width:100%;padding:13px;margin-top:12px;border:0;border-radius:9px;background:#2f7cf6;color:#fff;font-size:16px;font-weight:600}
button.alt{background:#2c2e33}.msg{background:#183a22;color:#bff0cc;padding:11px;border-radius:9px;margin-bottom:14px}
.row{display:flex;gap:8px}.row>*{flex:1}b{font-size:17px}
</style></head><body><h1>⏰ ESP32 Arch Clock</h1>)HTML";

const char PAGE_SCRIPT[] = R"JS(<script>
function send(dt){const f=document.getElementById('tf');f.y.value=dt.getFullYear();f.mo.value=dt.getMonth()+1;
f.d.value=dt.getDate();f.h.value=dt.getHours();f.mi.value=dt.getMinutes();f.s.value=dt.getSeconds();f.submit()}
function phone(){send(new Date())}
function manual(){const d=document.getElementById('di').value,t=document.getElementById('ti').value;
if(!d||!t){alert('กรอกวันที่และเวลาก่อน');return}
const a=d.split('-').map(Number),b=t.split(':').map(Number);send(new Date(a[0],a[1]-1,a[2],b[0],b[1],b[2]||0))}
async function tick(){try{const j=await (await fetch('/api')).json();
document.getElementById('t').textContent=j.time;document.getElementById('d').textContent=j.date;
document.getElementById('e').textContent=j.env;document.getElementById('l').textContent=j.light}catch(e){}}
setInterval(tick,1000);tick();
</script></body></html>)JS";

String htmlEsc(const String& s) {
  String o;
  for (unsigned i = 0; i < s.length(); i++) {
    char c = s[i];
    if (c == '<') o += "&lt;";
    else if (c == '>') o += "&gt;";
    else if (c == '&') o += "&amp;";
    else if (c == '"') o += "&quot;";
    else o += c;
  }
  return o;
}

String opt(const char* v, const char* label, bool sel) {
  return String("<option value=\"") + v + "\"" + (sel ? " selected" : "") + ">" + label + "</option>";
}

void redirectMsg(int code) {
  server.sendHeader("Location", String("/?m=") + code);
  server.send(303);
}

void handleRoot() {
  const char* MSGS[] = {"", "ตั้งเวลาแล้ว ✓", "ข้อมูลเวลาไม่ถูกต้อง",
                        "บันทึก WiFi แล้ว กำลังเชื่อมต่อ... มือถืออาจหลุดจาก WiFi นาฬิกาสักครู่ ถ้าต่อไม่ได้ใน 30 วิ นาฬิกาจะเปิด WiFi ตั้งค่าให้เข้ามาแก้ใหม่ (ดู IP ได้ที่เมนู Info)",
                        "บันทึกการตั้งค่าแล้ว ✓", "กำลังซิงค์เวลาจากอินเทอร์เน็ต...",
                        "ยังไม่ได้เชื่อมต่ออินเทอร์เน็ต", "กรุณากรอกชื่อ WiFi"};
  String h;
  h.reserve(7000);
  h += PAGE_HEAD;

  int m = server.arg("m").toInt();
  if (m > 0 && m < 8) h += String("<div class=msg>") + MSGS[m] + "</div>";

  h += "<div class=card><div class=big id=t>--:--:--</div><div class=sub id=d></div><div class=sub id=e></div></div>";

  // ตั้งเวลา
  h += "<div class=card><b>ตั้งเวลา</b>"
       "<button onclick=\"phone()\">📱 ใช้เวลาจากมือถือเครื่องนี้</button>"
       "<div class=row><div><label>วันที่</label><input type=date id=di></div>"
       "<div><label>เวลา</label><input type=time id=ti step=1></div></div>"
       "<button class=alt onclick=\"manual()\">ตั้งตามที่กรอก</button>"
       "<form id=tf method=post action=/time style=display:none>"
       "<input name=y><input name=mo><input name=d><input name=h><input name=mi><input name=s></form></div>";

  // WiFi
  String nets;
  int n = WiFi.scanComplete();
  for (int i = 0; i < n; i++) nets += "<option value=\"" + htmlEsc(WiFi.SSID(i)) + "\">";
  h += "<div class=card><b>WiFi</b><div class=st>สถานะ: " + htmlEsc(wifiStatusText());
  if (WiFi.status() == WL_CONNECTED) h += " · IP " + WiFi.localIP().toString();
  h += "</div><form method=post action=/wifi><label>ชื่อ WiFi (SSID)</label>"
       "<input name=ssid list=nets autocomplete=off value=\"" + htmlEsc(cfg.ssid) + "\"><datalist id=nets>" + nets +
       "</datalist><label>รหัสผ่าน</label><input name=pass type=password placeholder=\"เว้นว่าง = ใช้รหัสเดิม\">"
       "<button>บันทึกและเชื่อมต่อ</button></form>"
       "<form method=post action=/sync><button class=alt>ซิงค์เวลาจากอินเทอร์เน็ตตอนนี้</button></form></div>";

  // ตั้งค่าอื่นๆ
  h += "<div class=card><b>ตั้งค่า</b><form method=post action=/settings>";
  h += "<label>Time zone (UTC)</label><input name=tz type=number min=-12 max=14 value=" + String(cfg.tz) + ">";
  h += "<label>รูปแบบเวลา</label><select name=h12>" + opt("0", "24 ชั่วโมง", !cfg.h12) + opt("1", "12 ชั่วโมง (AM/PM)", cfg.h12) + "</select>";
  h += "<label>ความสว่างจอ</label><select name=br>" + opt("0", "อัตโนมัติ (ตาม LDR)", cfg.bright == 0) +
       opt("1", "ต่ำ", cfg.bright == 1) + opt("2", "กลาง", cfg.bright == 2) + opt("3", "สูง", cfg.bright == 3) + "</select>";
  h += "<label>ดับจอตอนมืด</label><select name=night>" + opt("1", "เปิด (โบกมือเพื่อดูเวลา)", cfg.nightOff) +
       opt("0", "ปิด (จอติดตลอด)", !cfg.nightOff) + "</select>";
  h += "<label>ระดับความมืด (แสงตอนนี้ = <span id=l>" + String(light) + "</span>)</label>"
       "<input name=dark type=number min=0 max=4000 value=" + String(cfg.darkThr) + ">";
  h += "<label>WiFi</label><select name=wifi>" + opt("1", "เปิด", cfg.wifiOn) + opt("0", "ปิด (ใช้แบบออฟไลน์)", !cfg.wifiOn) + "</select>";
  h += "<button>บันทึก</button></form></div>";

  h += PAGE_SCRIPT;
  server.send(200, "text/html; charset=utf-8", h);
}

void handleApi() {
  char t[12], d[16], e[40];
  snprintf(t, sizeof t, "%02d:%02d:%02d", tnow.hour(), tnow.minute(), tnow.second());
  snprintf(d, sizeof d, "%02d/%02d/%04d", tnow.day(), tnow.month(), tnow.year());
  if (isnan(roomTemp)) strcpy(e, "DHT11: --");
  else snprintf(e, sizeof e, "%.1f°C · %.0f%%", roomTemp, roomHum);
  String j = String("{\"time\":\"") + t + "\",\"date\":\"" + d + "\",\"env\":\"" + e + "\",\"light\":" + light + "}";
  server.send(200, "application/json", j);
}

void handleTime() {
  int y = server.arg("y").toInt(), mo = server.arg("mo").toInt(), d = server.arg("d").toInt();
  int h = server.arg("h").toInt(), mi = server.arg("mi").toInt(), s = server.arg("s").toInt();
  if (y < 2024 || y > 2099 || mo < 1 || mo > 12 || d < 1 || d > daysInMonth(mo, y) ||
      h < 0 || h > 23 || mi < 0 || mi > 59 || s < 0 || s > 59) { redirectMsg(2); return; }
  rtc.adjust(DateTime(y, mo, d, h, mi, s));
  tnow = rtc.now();
  toast("Time set");
  redirectMsg(1);
}

void handleWifi() {
  String ss = server.arg("ssid");
  ss.trim();
  String pw = server.arg("pass");
  if (!ss.length()) { redirectMsg(7); return; }
  if (!pw.length() && ss == cfg.ssid) pw = cfg.pass;
  cfg.ssid = ss;
  cfg.pass = pw;
  cfg.wifiOn = true;
  saveSettings();
  redirectMsg(3);
  delay(200);
  wifiApply();
  toast("Connecting...");
}

void handleSettings() {
  int oldTz = cfg.tz;
  bool oldWifi = cfg.wifiOn;
  cfg.tz       = constrain(server.arg("tz").toInt(), -12, 14);
  cfg.h12      = server.arg("h12") == "1";
  cfg.bright   = constrain(server.arg("br").toInt(), 0, 3);
  cfg.nightOff = server.arg("night") == "1";
  cfg.darkThr  = constrain(server.arg("dark").toInt(), 0, 4000);
  cfg.wifiOn   = server.arg("wifi") == "1";
  saveSettings();
  redirectMsg(4);
  if (cfg.wifiOn != oldWifi) { delay(200); wifiApply(); }
  else if (cfg.tz != oldTz && WiFi.status() == WL_CONNECTED) startNTP();
  toast("Saved");
}

void handleSync() {
  if (WiFi.status() == WL_CONNECTED) { syncNow(); redirectMsg(5); }
  else redirectMsg(6);
}

void handleNotFound() {
  if (portalOn) {   // ให้มือถือเด้งเปิดหน้าตั้งค่าเอง (captive portal)
    server.sendHeader("Location", "http://192.168.4.1/");
    server.send(302);
  } else {
    server.send(404, "text/plain", "Not found");
  }
}

void setupRoutes() {
  server.on("/", HTTP_GET, handleRoot);
  server.on("/api", HTTP_GET, handleApi);
  server.on("/time", HTTP_POST, handleTime);
  server.on("/wifi", HTTP_POST, handleWifi);
  server.on("/settings", HTTP_POST, handleSettings);
  server.on("/sync", HTTP_POST, handleSync);
  server.onNotFound(handleNotFound);
}
