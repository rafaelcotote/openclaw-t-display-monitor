#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <TFT_eSPI.h>
#include <SPI.h>
#include <pgmspace.h>

const char* WIFI_SSID = "SEU_WIFI";
const char* WIFI_PASSWORD = "SUA_SENHA";
const char* API_URL = "http://IP_DO_HOST:8765/status";

TFT_eSPI tft = TFT_eSPI();

unsigned long lastFetchMs = 0;
unsigned long lastButtonMs = 0;

const unsigned long FETCH_INTERVAL_MS = 15000;
const unsigned long BUTTON_DEBOUNCE_MS = 250;

const int BUTTON_PIN = 0;
const int TOTAL_PAGES = 4;
int currentPage = 0;

// Cor usada como "transparente" na logo RGB565
const uint16_t LOGO_TRANSPARENT = 0xF81F;

const uint16_t openclawLogo22[22 * 22] PROGMEM = {
  0xF81F, 0xF81F, 0xF81F, 0xF81F, 0xF81F, 0xF81F, 0xF81F, 0xF81F, 0xF81F, 0xF81F, 0xF81F,
  0xF81F, 0xF81F, 0xF81F, 0xF81F, 0xF81F, 0xF81F, 0xF81F, 0xF81F, 0xF81F, 0xF81F, 0xF81F,
  0xF81F, 0xF81F, 0xF81F, 0xF81F, 0xF81F, 0xF81F, 0xF81F, 0xF81F, 0xF81F, 0xF81F, 0xF81F,
  0xF81F, 0xF81F, 0xF81F, 0xF81F, 0xF81F, 0xF81F, 0xF81F, 0xF81F, 0xF81F, 0xF81F, 0xF81F,
  0xF81F, 0xF81F, 0xF81F, 0xF81F, 0xF81F, 0xF81F, 0xF81F, 0xF81F, 0xF81F, 0xF81F, 0xF81F,
  0xF81F, 0xF81F, 0xF81F, 0xF81F, 0xF81F, 0xF81F, 0xF81F, 0xF81F, 0xF81F, 0xF81F, 0xF81F,
  0xF81F, 0xF81F, 0xF81F, 0xF81F, 0xF81F, 0xF81F, 0xF81F, 0xACF3, 0xF81F, 0xF81F, 0xF81F,
  0xF81F, 0xF81F, 0xF81F, 0xF81F, 0xF81F, 0xF81F, 0xF81F, 0xF81F, 0xF81F, 0xF81F, 0xF81F,
  0xF81F, 0xF81F, 0xF81F, 0xF81F, 0xACF3, 0xF81F, 0x936D, 0x8B4D, 0xF81F, 0xF81F, 0xF81F,
  0xF81F, 0xF81F, 0xF81F, 0xF81F, 0xF81F, 0x7B8E, 0x830C, 0x9B6E, 0x830C, 0xF81F, 0xF81F,
  0xF81F, 0xF81F, 0xF81F, 0xF81F, 0x930C, 0xF81F, 0x8229, 0x934D, 0xF81F, 0xF81F, 0xF81F,
  0xF81F, 0xF81F, 0xF81F, 0x830C, 0x6925, 0xA083, 0xF1A8, 0xE8E5, 0x6186, 0xF81F, 0xF81F,
  0xF81F, 0xF81F, 0xF81F, 0xF81F, 0x8A49, 0xF81F, 0x8249, 0xF81F, 0xF81F, 0xF81F, 0xF81F,
  0xF81F, 0xF81F, 0x8208, 0x6041, 0xA800, 0xF822, 0xC822, 0x6104, 0x7471, 0xF81F, 0xF81F,
  0xF81F, 0xF81F, 0xF81F, 0xF81F, 0x6208, 0x728A, 0x6125, 0x7B6D, 0x8491, 0xF81F, 0xF81F,
  0xF81F, 0x8187, 0x7800, 0x9000, 0xF802, 0xE105, 0x51A6, 0xF81F, 0xF81F, 0xF81F, 0xF81F,
  0xF81F, 0xF81F, 0xF81F, 0x72CB, 0x9062, 0xB800, 0xC001, 0xD8C4, 0xB1A7, 0x6AAA, 0x8D55,
  0x6186, 0xE001, 0xD801, 0xE8E5, 0xB063, 0x6925, 0x73CF, 0xF81F, 0x736D, 0x8BAE, 0xF81F,
  0xF81F, 0xF81F, 0x6B8E, 0x9062, 0x9001, 0xC822, 0xF802, 0x7021, 0x88C4, 0xC083, 0x10E3,
  0xB001, 0xF802, 0xF022, 0xD26A, 0x38E3, 0x5269, 0x8AAB, 0x8905, 0xD105, 0x71C7, 0xF81F,
  0xF81F, 0xF81F, 0x5228, 0xC801, 0x7021, 0x9801, 0xD801, 0x9801, 0xE063, 0xA001, 0x3800,
  0xD802, 0xE802, 0xB022, 0x9842, 0xC001, 0xC001, 0xD801, 0xF802, 0xB001, 0x4A69, 0xF81F,
  0xF81F, 0xF81F, 0x6410, 0x78A3, 0xF001, 0xC801, 0xC001, 0xD801, 0xA801, 0x7801, 0x8801,
  0xC002, 0xF802, 0xF002, 0xF802, 0xF822, 0xF822, 0xF802, 0xB000, 0x4166, 0xF81F, 0xF81F,
  0xF81F, 0xF81F, 0x938E, 0x6082, 0x8001, 0xF002, 0xD002, 0xA801, 0x7000, 0x9801, 0x8800,
  0x4000, 0x9801, 0xD001, 0xE001, 0xD801, 0xC001, 0x7882, 0x41E7, 0xF81F, 0xF81F, 0xF81F,
  0xF81F, 0x83CF, 0x8925, 0x8062, 0x6800, 0xF802, 0xE001, 0xE002, 0x8862, 0x4000, 0x38E3,
  0x3945, 0x31C7, 0x49A6, 0x5145, 0x5186, 0x4228, 0x5C0F, 0xF81F, 0xF81F, 0xF81F, 0xF81F,
  0xF81F, 0x8966, 0xF042, 0xB966, 0x40E3, 0x8800, 0xC842, 0xB125, 0xC105, 0x2841, 0x6208,
  0x9CB2, 0xF81F, 0xF81F, 0xF81F, 0xF81F, 0xF81F, 0xF81F, 0xF81F, 0xF81F, 0xF81F, 0xF81F,
  0xF81F, 0x8986, 0xF822, 0xB0C4, 0x6986, 0x50A3, 0x9801, 0xD022, 0xC905, 0xA0C4, 0x6800,
  0xA883, 0xB26A, 0x83EF, 0xF81F, 0xF81F, 0xF81F, 0xF81F, 0xF81F, 0xF81F, 0xF81F, 0xF81F,
  0xF81F, 0x738E, 0xB0E4, 0xD946, 0x6925, 0x7062, 0x4882, 0x8883, 0xB801, 0xD801, 0x8801,
  0x4021, 0xA0C4, 0x7B8E, 0xF81F, 0xF81F, 0xF81F, 0xF81F, 0xF81F, 0xF81F, 0xF81F, 0xF81F,
  0xF81F, 0xF81F, 0x6BEF, 0x9A08, 0x8A08, 0x7B2D, 0x9BF0, 0x5BAE, 0x6A08, 0x79C7, 0x5A28,
  0x63AE, 0x7B2C, 0xF81F, 0xF81F, 0xF81F, 0xF81F, 0xF81F, 0xF81F, 0xF81F, 0xF81F, 0xF81F,
  0xF81F, 0xF81F, 0xF81F, 0xF81F, 0xF81F, 0xF81F, 0xF81F, 0xF81F, 0xF81F, 0xF81F, 0xF81F,
  0xF81F, 0xF81F, 0xF81F, 0xF81F, 0xF81F, 0xF81F, 0xF81F, 0xF81F, 0xF81F, 0xF81F, 0xF81F,
  0xF81F, 0xF81F, 0xF81F, 0xF81F, 0xF81F, 0xF81F, 0xF81F, 0xF81F, 0xF81F, 0xF81F, 0xF81F,
  0xF81F, 0xF81F, 0xF81F, 0xF81F, 0xF81F, 0xF81F, 0xF81F, 0xF81F, 0xF81F, 0xF81F, 0xF81F,
  0xF81F, 0xF81F, 0xF81F, 0xF81F, 0xF81F, 0xF81F, 0xF81F, 0xF81F, 0xF81F, 0xF81F, 0xF81F,
  0xF81F, 0xF81F, 0xF81F, 0xF81F, 0xF81F, 0xF81F, 0xF81F, 0xF81F, 0xF81F, 0xF81F, 0xF81F,
  0xF81F, 0xF81F, 0xF81F, 0xF81F, 0xF81F, 0xF81F, 0xF81F, 0xF81F, 0xF81F, 0xF81F, 0xF81F,
  0xF81F, 0xF81F, 0xF81F, 0xF81F, 0xF81F, 0xF81F, 0xF81F, 0xF81F, 0xF81F, 0xF81F, 0xF81F,
};

struct DashboardData {
  bool ok = false;
  String hostname = "ubuntu";
  String ip = "0.0.0.0";
  String version = "-";
  String channel = "stable";
  String gatewayService = "running";
  String model = "-";
  String sessionKey = "-";
  String plan = "-";
  String localTime = "00:00";
  String localSeconds = "00:00:00";
  String weekday = "terça";
  String dateShort = "17 mar";
  String statusText = "OpenClaw online";
  int sessionsCount = 0;
  int tokensTotal = 0;
  int tokensInput = 0;
  int tokensOutput = 0;
  int tokensCache = 0;
  int remainingTokens = 0;
  int contextPercent = 0;
  int fiveHourLeftPercent = 0;
  int weekLeftPercent = 0;
  float uptimeHours = 0;
  float load1 = 0;
  float memPercent = 0;
  float diskPercent = 0;
  int dockerRunning = 0;
  String dockerNames[4];
  int secCritical = 0;
  int secWarn = 0;
  String error = "";
};

DashboardData data;

String compactInt(int value) {
  if (value >= 1000000) return String(value / 1000000.0, 1) + "M";
  if (value >= 1000) return String(value / 1000.0, 1) + "k";
  return String(value);
}

String shortText(String s, int maxLen) {
  if ((int)s.length() <= maxLen) return s;
  return s.substring(0, maxLen - 3) + "...";
}

void drawRGB565WithTransparency(int x, int y, int w, int h, const uint16_t* bitmap, uint16_t transparentColor) {
  for (int py = 0; py < h; py++) {
    for (int px = 0; px < w; px++) {
      uint16_t c = pgm_read_word(&bitmap[py * w + px]);
      if (c != transparentColor) {
        tft.drawPixel(x + px, y + py, c);
      }
    }
  }
}

void drawClawLogo(int x, int y) {
  drawRGB565WithTransparency(x, y, 22, 22, openclawLogo22, LOGO_TRANSPARENT);
}

void panel(int x, int y, int w, int h, uint16_t border, uint16_t fill, int radius = 8) {
  tft.fillRoundRect(x, y, w, h, radius, fill);
  tft.drawRoundRect(x, y, w, h, radius, border);
}

void drawHeader(const char* subtitle) {
  uint16_t headerBg = tft.color565(28, 10, 12);
  uint16_t headerBorder = tft.color565(90, 28, 35);

  panel(6, 6, 308, 42, headerBorder, headerBg, 10);

  drawClawLogo(12, 14);

  tft.setTextColor(TFT_WHITE, headerBg);
  tft.drawString("OPENCLAW", 42, 11, 4);

  tft.setTextColor(tft.color565(255, 138, 148), headerBg);
  tft.drawString(subtitle, 42, 29, 2);

  tft.setTextColor(tft.color565(255, 190, 120), headerBg);
  tft.drawRightString(data.ip, 304, 19, 2);
}

void drawGauge(int cx, int cy, int r, int percent, uint16_t color, const char* label) {
  percent = constrain(percent, 0, 100);

  for (int a = -140; a <= 140; a += 2) {
    float rad = a * 0.0174533f;
    int x1 = cx + cos(rad) * (r - 5);
    int y1 = cy + sin(rad) * (r - 5);
    int x2 = cx + cos(rad) * r;
    int y2 = cy + sin(rad) * r;

    uint16_t c = (a <= (-140.0f + (280.0f * percent / 100.0f)))
      ? color
      : tft.color565(45, 18, 22);

    tft.drawLine(x1, y1, x2, y2, c);
  }

  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.drawString(String(percent) + "%", cx, cy - 2, 4);
  tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
  tft.drawString(label, cx, cy + 18, 2);
  tft.setTextDatum(TL_DATUM);
}

void drawFooter() {
  uint16_t footerBg = tft.color565(18, 9, 11);
  tft.fillRoundRect(6, 150, 308, 10, 5, footerBg);
  tft.setTextColor(tft.color565(216, 182, 186), footerBg);
  tft.drawRightString("Created by @RafaelCotote", 311, 152, 1);
}

void renderOverview() {
  drawHeader("overview");

  uint16_t bg = tft.color565(24, 12, 14);
  uint16_t border = tft.color565(86, 30, 36);

  // Linha principal
  panel(6, 54, 150, 46, border, bg);
  panel(164, 54, 150, 46, border, bg);

  tft.setTextColor(tft.color565(255, 140, 150), bg);
  tft.drawString("Tokens ativos", 16, 62, 2);
  tft.setTextColor(TFT_WHITE, bg);
  tft.drawString(compactInt(data.tokensTotal), 16, 79, 2);

  tft.setTextColor(tft.color565(255, 140, 150), bg);
  tft.drawString("Modelo", 174, 62, 2);
  tft.setTextColor(TFT_WHITE, bg);
  tft.drawString(shortText(data.model, 10), 174, 79, 2);

  // Linha pequena
  panel(6, 104, 98, 18, border, bg, 7);
  panel(110, 104, 98, 18, border, bg, 7);
  panel(214, 104, 100, 18, border, bg, 7);

  tft.setTextColor(TFT_GREEN, bg);
  tft.drawString("RAM", 14, 108, 1);
  tft.setTextColor(TFT_WHITE, bg);
  tft.drawRightString(String((int)data.memPercent) + "%", 96, 108, 1);

  tft.setTextColor(TFT_YELLOW, bg);
  tft.drawString("DISCO", 118, 108, 1);
  tft.setTextColor(TFT_WHITE, bg);
  tft.drawRightString(String((int)data.diskPercent) + "%", 200, 108, 1);

  tft.setTextColor(tft.color565(255, 140, 150), bg);
  tft.drawString("DOCKER", 222, 108, 1);
  tft.setTextColor(TFT_WHITE, bg);
  tft.drawRightString(String(data.dockerRunning), 304, 108, 1);

  // Linha inferior com mais respiro
  panel(6, 124, 150, 24, border, bg, 8);
  panel(164, 124, 150, 24, border, bg, 8);

  tft.setTextColor(tft.color565(255, 140, 150), bg);
  tft.drawString("Contexto", 14, 128, 1);
  tft.setTextColor(TFT_WHITE, bg);
  tft.drawRightString(String(data.contextPercent) + "% usado", 148, 134, 1);

  tft.setTextColor(tft.color565(255, 140, 150), bg);
  tft.drawString("Segurança", 172, 128, 1);
  tft.setTextColor(TFT_WHITE, bg);
  tft.drawRightString("C" + String(data.secCritical) + " W" + String(data.secWarn), 306, 134, 1);

  drawFooter();
}

void renderClock() {
  drawHeader("clock + status");

  uint16_t bg = tft.color565(24, 12, 14);
  uint16_t border = tft.color565(86, 30, 36);
  uint16_t chip = tft.color565(30, 14, 16);

  panel(6, 54, 308, 78, border, bg);

  tft.setTextColor(TFT_WHITE, bg);
  tft.drawCentreString(data.localTime, 160, 66, 7);

  tft.setTextColor(tft.color565(255, 140, 150), bg);
  tft.drawCentreString((data.weekday + ", " + data.dateShort), 160, 100, 2);

  panel(12, 112, 92, 14, border, chip, 6);
  panel(114, 112, 92, 14, border, chip, 6);
  panel(216, 112, 92, 14, border, chip, 6);

  String statusShort = data.statusText;
  statusShort.replace("OpenClaw ", "Claw ");

  tft.setTextColor(TFT_LIGHTGREY, chip);
  tft.drawCentreString(shortText(statusShort, 14), 58, 116, 1);
  tft.drawCentreString("Docker " + String(data.dockerRunning), 160, 116, 1);
  tft.drawCentreString("Load " + String(data.load1, 2), 262, 116, 1);

  drawFooter();
}

void renderInfra() {
  drawHeader("infra");

  uint16_t bg = tft.color565(24, 12, 14);
  uint16_t border = tft.color565(86, 30, 36);

  panel(6, 54, 150, 36, border, bg);
  panel(164, 54, 150, 36, border, bg);

  tft.setTextColor(tft.color565(255, 140, 150), bg);
  tft.drawString("RAM", 14, 60, 2);
  tft.drawString("Disco", 172, 60, 2);

  tft.setTextColor(TFT_WHITE, bg);
  tft.drawString(String((int)data.memPercent) + "%", 14, 74, 2);
  tft.drawString(String((int)data.diskPercent) + "%", 172, 74, 2);

  panel(6, 96, 150, 36, border, bg);
  panel(164, 96, 150, 36, border, bg);

  tft.setTextColor(tft.color565(255, 140, 150), bg);
  tft.drawString("Docker", 14, 102, 2);
  tft.drawString("Gateway", 172, 102, 2);

  tft.setTextColor(TFT_WHITE, bg);
  tft.drawString(String(data.dockerRunning), 14, 116, 2);
  tft.drawString(data.gatewayService.indexOf("active") >= 0 ? "active" : "running", 172, 116, 2);

  drawFooter();
}

void renderSession() {
  drawHeader("session");

  uint16_t bg = tft.color565(24, 12, 14);
  uint16_t border = tft.color565(86, 30, 36);

  panel(6, 54, 150, 36, border, bg);
  panel(164, 54, 150, 36, border, bg);

  tft.setTextColor(tft.color565(255, 140, 150), bg);
  tft.drawString("Input", 14, 60, 2);
  tft.drawString("Output", 172, 60, 2);

  tft.setTextColor(TFT_WHITE, bg);
  tft.drawString(compactInt(data.tokensInput), 14, 74, 2);
  tft.drawString(compactInt(data.tokensOutput), 172, 74, 2);

  panel(6, 96, 150, 36, border, bg);
  panel(164, 96, 150, 36, border, bg);

  tft.setTextColor(tft.color565(255, 140, 150), bg);
  tft.drawString("Cache", 14, 102, 2);
  tft.drawString("Livre", 172, 102, 2);

  tft.setTextColor(TFT_WHITE, bg);
  tft.drawString(compactInt(data.tokensCache), 14, 116, 2);
  tft.drawString(compactInt(data.remainingTokens), 172, 116, 2);

  drawFooter();
}

void drawErrorScreen(const String& msg) {
  tft.fillScreen(TFT_BLACK);

  uint16_t errBg = tft.color565(38, 10, 10);
  uint16_t errBorder = tft.color565(239, 0, 17);

  panel(8, 8, 304, 120, errBorder, errBg, 10);
  drawClawLogo(18, 18);

  tft.setTextColor(TFT_WHITE, errBg);
  tft.drawString("OpenClaw board offline", 48, 20, 4);

  tft.setTextColor(TFT_LIGHTGREY, errBg);
  tft.drawString("Confira Wi-Fi, API_URL e o servidor Python.", 18, 60, 2);

  tft.setTextColor(TFT_YELLOW, errBg);
  tft.drawString(shortText(msg, 42), 18, 88, 2);

  drawFooter();
}

bool fetchData() {
  if (WiFi.status() != WL_CONNECTED) {
    data.ok = false;
    data.error = "Wi-Fi desconectado";
    return false;
  }

  HTTPClient http;
  http.begin(API_URL);
  http.setConnectTimeout(6000);
  http.setTimeout(8000);

  int code = http.GET();
  if (code != HTTP_CODE_OK) {
    data.ok = false;
    data.error = "HTTP " + String(code);
    http.end();
    return false;
  }

  String payload = http.getString();
  http.end();

  StaticJsonDocument<12288> doc;
  DeserializationError err = deserializeJson(doc, payload);
  if (err) {
    data.ok = false;
    data.error = String("JSON: ") + err.c_str();
    return false;
  }

  data.ok = true;
  data.error = "";

  data.hostname = doc["device"]["hostname"] | "ubuntu";
  data.ip = doc["device"]["ip"] | "0.0.0.0";
  data.version = doc["openclaw"]["version"] | "-";
  data.channel = doc["openclaw"]["channel"] | "stable";
  data.gatewayService = doc["openclaw"]["gatewayService"] | "running";
  data.model = doc["openclaw"]["currentSession"]["model"] | "-";
  data.sessionKey = doc["openclaw"]["currentSession"]["keyShort"] | "-";
  data.plan = doc["openclaw"]["providerUsage"]["plan"] | "-";

  data.localTime = doc["time"]["time"] | "00:00";
  data.localSeconds = doc["time"]["seconds"] | "00:00:00";
  data.weekday = doc["time"]["weekday"] | "terça";
  data.dateShort = doc["time"]["dateShort"] | "17 mar";

  data.statusText = doc["dashboard"]["statusText"] | "OpenClaw online";

  data.sessionsCount = doc["openclaw"]["sessionsCount"] | 0;
  data.tokensTotal = doc["openclaw"]["currentSession"]["totalTokens"] | 0;
  data.tokensInput = doc["openclaw"]["currentSession"]["inputTokens"] | 0;
  data.tokensOutput = doc["openclaw"]["currentSession"]["outputTokens"] | 0;
  data.tokensCache = doc["openclaw"]["currentSession"]["cacheRead"] | 0;
  data.remainingTokens = doc["openclaw"]["currentSession"]["remainingTokens"] | 0;
  data.contextPercent = doc["openclaw"]["currentSession"]["percentUsed"] | 0;

  data.fiveHourLeftPercent = doc["openclaw"]["providerUsage"]["fiveHourLeftPercent"] | 0;
  data.weekLeftPercent = doc["openclaw"]["providerUsage"]["weekLeftPercent"] | 0;

  data.uptimeHours = doc["system"]["uptimeHours"] | 0.0;
  data.load1 = doc["system"]["load1"] | 0.0;
  data.memPercent = doc["system"]["memory"]["percent"] | 0.0;
  data.diskPercent = doc["system"]["disk"]["percent"] | 0.0;

  data.dockerRunning = doc["docker"]["running"] | 0;

  for (int i = 0; i < 4; i++) data.dockerNames[i] = "";

  JsonArray names = doc["docker"]["names"].as<JsonArray>();
  int idx = 0;
  for (JsonVariant v : names) {
    if (idx >= 4) break;
    data.dockerNames[idx++] = String((const char*)v.as<const char*>());
  }

  data.secCritical = doc["openclaw"]["security"]["critical"] | 0;
  data.secWarn = doc["openclaw"]["security"]["warn"] | 0;

  return true;
}

void renderPage() {
  tft.fillScreen(TFT_BLACK);

  if (!data.ok) {
    drawErrorScreen(data.error);
    return;
  }

  switch (currentPage) {
    case 0: renderOverview(); break;
    case 1: renderClock(); break;
    case 2: renderInfra(); break;
    case 3: renderSession(); break;
  }
}

void connectWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  tft.fillScreen(TFT_BLACK);

  uint16_t wifiBg = tft.color565(24, 12, 14);
  panel(20, 40, 280, 90, tft.color565(239, 0, 17), wifiBg, 10);

  drawClawLogo(34, 58);

  tft.setTextColor(TFT_WHITE, wifiBg);
  tft.drawString("Conectando no Wi-Fi...", 62, 60, 4);

  tft.setTextColor(TFT_LIGHTGREY, wifiBg);
  tft.drawString(shortText(WIFI_SSID, 22), 62, 92, 2);

  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 20000) {
    delay(250);
  }
}

void setup() {
  pinMode(15, OUTPUT);
  digitalWrite(15, HIGH);

  pinMode(BUTTON_PIN, INPUT_PULLUP);

  tft.init();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);
  tft.setTextDatum(TL_DATUM);

  connectWiFi();
  fetchData();
  renderPage();

  lastFetchMs = millis();
  lastButtonMs = 0;
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    connectWiFi();
    fetchData();
    renderPage();
  }

  if (millis() - lastFetchMs >= FETCH_INTERVAL_MS) {
    fetchData();
    renderPage();
    lastFetchMs = millis();
  }

  if (digitalRead(BUTTON_PIN) == LOW && (millis() - lastButtonMs >= BUTTON_DEBOUNCE_MS)) {
    currentPage = (currentPage + 1) % TOTAL_PAGES;
    renderPage();
    lastButtonMs = millis();
    delay(180);
  }

  delay(50);
}