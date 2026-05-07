#include "fw_version.h"
#include "secrets.h"
#include <Arduino.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <Preferences.h>
#include <TFT_eSPI.h>
#include <WiFi.h>
#include <esp_now.h>
#include <time.h>

Preferences preferences;
TFT_eSPI tft = TFT_eSPI();

// NTP configuration
const char *ntpServer = "pool.ntp.org";
const long gmtOffset_sec = -25200;   // UTC-7 (Mountain Time)
const int daylightOffset_sec = 3600; // 1 hr DST

enum SyncState { SYNCING, SYNC_OK, SYNC_FAILED };
SyncState currentSyncState = SYNCING;

enum ActiveScreen { SCREEN_LESSONS, SCREEN_ACTIVITIES, SCREEN_ASSIGNMENTS };
ActiveScreen currentScreen = SCREEN_LESSONS;

String rawJsonData = "{}";

// ── Hot Potato ──────────────────────────────────────────────────────────────
// Trigger: bridge GPIO 12 and 13 together (exposed end-pins, same analog-read
// technique as the ghost32 EMF detector). When both read LOW simultaneously
// for a brief debounce period, potato mode toggles on/off.
#define POT_PIN_A      12   // T5 / ADC2_CH4 — expose & bridge to enter mode
#define POT_PIN_B      13   // T4 / ADC2_CH5 — expose & bridge to enter mode
#define POT_BRIDGE_THR 200  // analogRead < threshold = "pulled low" / bridged

struct __attribute__((packed)) PotatoPacket {
  int      timer;       // seconds remaining when sent
  bool     isExploded;  // sender blew up (shame broadcast)
  uint32_t senderID;    // low 32-bits of sender MAC
};

enum PotatoSubState { POT_IDLE, POT_ACTIVE, POT_BOOM };

static const uint8_t POT_BCAST[6] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};
static uint32_t      myChipID     = 0;
static bool          espNowReady  = false;
bool                 potatoMode   = false;

volatile PotatoSubState potState    = POT_IDLE;
volatile int            potTimer    = 0;
volatile unsigned long  potReceived = 0;

// ── ESP-NOW callbacks ────────────────────────────────────────────────────────
void onPotatoRecv(const esp_now_recv_info_t *recv_info, const uint8_t *data, int len) {
  if (len != sizeof(PotatoPacket)) return;
  PotatoPacket pkt;
  memcpy(&pkt, data, sizeof(pkt));
  if (pkt.senderID == myChipID) return;  // our own echo
  if (pkt.isExploded)           return;  // someone else blew up
  if (!potatoMode)              return;  // we're not playing
  if (potState == POT_IDLE) {
    potTimer    = pkt.timer;
    potReceived = millis();
    potState    = POT_ACTIVE;
  }
}

void onPotatoSent(const wifi_tx_info_t *tx_info, esp_now_send_status_t s) {
  Serial.printf("[Potato] send %s\n", s == ESP_NOW_SEND_SUCCESS ? "OK" : "FAIL");
}

void sendPotato(int timerSec) {
  if (!espNowReady) return;
  PotatoPacket pkt = {timerSec, false, myChipID};
  esp_now_send(POT_BCAST, (uint8_t *)&pkt, sizeof(pkt));
}

bool initEspNow() {
  if (WiFi.getMode() == WIFI_OFF) WiFi.mode(WIFI_STA);
  if (esp_now_init() != ESP_OK) { espNowReady = false; return false; }
  esp_now_register_recv_cb(onPotatoRecv);
  esp_now_register_send_cb(onPotatoSent);
  esp_now_peer_info_t peer = {};
  memcpy(peer.peer_addr, POT_BCAST, 6);
  peer.channel = 0; peer.encrypt = false;
  esp_now_add_peer(&peer);
  espNowReady = true;
  return true;
}

void deinitEspNow() {
  esp_now_deinit();
  espNowReady = false;
  // WiFi mode is left for the main sync logic to manage
}

// ── Potato display (screen is 240x135, landscape rotation 1) ─────────────────
void drawPotatoFlame(int cx, int cy) {
  tft.fillTriangle(cx-14,cy+14, cx,cy-20, cx+14,cy+14, TFT_ORANGE);
  tft.fillTriangle(cx-6, cy+14, cx-2,cy-8, cx+4, cy+14, TFT_YELLOW);
  tft.fillTriangle(cx+2, cy+14, cx+5,cy-10,cx+11,cy+14, TFT_YELLOW);
}

void displayPotato() {
  tft.setTextDatum(TC_DATUM);
  if (potState == POT_IDLE) {
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_GREEN, TFT_BLACK);
    tft.setTextSize(2);
    tft.drawString("HOT POTATO", 120, 5);
    tft.drawFastHLine(10, 30, 220, TFT_DARKGREY);
    tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
    tft.setTextSize(1);
    tft.drawString("Waiting for potato...", 120, 45);
    tft.drawString("Long-press BOOT = new game", 120, 60);
    tft.fillEllipse(120, 100, 30, 18, TFT_ORANGE);
    tft.fillEllipse(120, 95, 18, 8, 0xA240);
    tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
    tft.drawString("Bridge pins 12+13 to exit", 120, 124);
  } else if (potState == POT_ACTIVE) {
    tft.fillScreen(TFT_RED);
    tft.drawRect(1,1,238,133, TFT_YELLOW);
    tft.drawRect(3,3,234,129, TFT_YELLOW);
    tft.setTextDatum(TL_DATUM);
    tft.setTextColor(TFT_YELLOW, TFT_RED);
    tft.setTextSize(2);
    tft.drawString("POTATO!", 8, 6);
    int elapsed  = (int)((millis() - potReceived) / 1000);
    int secsLeft = max(0, potTimer - elapsed);
    tft.setTextColor(TFT_WHITE, TFT_RED);
    tft.setTextSize(5);
    tft.setTextDatum(MC_DATUM);
    tft.drawString(String(secsLeft), 95, 75);
    drawPotatoFlame(190, 72);
    tft.setTextSize(1);
    tft.setTextColor(TFT_YELLOW, TFT_RED);
    tft.setTextDatum(BC_DATUM);
    tft.drawString("BOOT = pass", 120, 132);
  } else { // POT_BOOM
    tft.fillScreen(TFT_BLACK);
    for (int a = 0; a < 360; a += 22) {
      float r = a * PI / 180.0f;
      tft.drawLine(120,65, 120+52*cosf(r), 65+42*sinf(r), TFT_ORANGE);
    }
    tft.fillCircle(120, 65, 28, TFT_YELLOW);
    tft.fillCircle(120, 65, 16, TFT_WHITE);
    tft.setTextColor(TFT_RED, TFT_BLACK);
    tft.setTextSize(3);
    tft.setTextDatum(TC_DATUM);
    tft.drawString("BOOM!", 120, 100);
    tft.setTextSize(1);
    tft.setTextColor(TFT_ORANGE, TFT_BLACK);
    tft.drawString("You held it too long!", 120, 118);
    tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
    tft.drawString("Bridge pins 12+13 to exit", 120, 128);
  }
}

// Helper function to print text with some basic wrapping
void printWrapped(int x, int y, int w, String text) {
  tft.setCursor(x, y);
  int startPos = 0;
  int breakPos = 0;

  while (startPos < text.length()) {
    int charsFit = w / (tft.textWidth("A") > 0 ? tft.textWidth("A") : 6);
    if (startPos + charsFit >= text.length()) {
      tft.println(text.substring(startPos));
      break;
    }

    breakPos = text.lastIndexOf(' ', startPos + charsFit);
    if (breakPos <= startPos) {
      breakPos = startPos + charsFit;
    }

    tft.println(text.substring(startPos, breakPos));
    tft.setCursor(x, tft.getCursorY());
    startPos = breakPos + 1; // skip the space
  }
}

void drawWifiIcon() {
  tft.fillRect(0, 110, 40, 25, TFT_BLACK);
  tft.setTextSize(1);
  if (currentSyncState == SYNCING) {
    tft.setTextColor(TFT_YELLOW, TFT_BLACK);
    tft.drawString("W...", 5, 122, 1);
  } else if (currentSyncState == SYNC_OK) {
    tft.setTextColor(TFT_GREEN, TFT_BLACK);
    tft.drawString("WiFi", 5, 122, 1);
  } else {
    tft.setTextColor(TFT_RED, TFT_BLACK);
    tft.drawString("No W", 5, 122, 1);
  }
}

void drawClock() {
  tft.fillRect(160, 110, 80, 25, TFT_BLACK);
  tft.setTextSize(1);

  struct tm timeinfo;
  bool hasTime = getLocalTime(&timeinfo, 10);

  tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);

  if (hasTime && timeinfo.tm_year > 100) { // Valid time (after 2000)
    char timeStr[10];
    strftime(timeStr, sizeof(timeStr), "%I:%M %p", &timeinfo);
    char dateStr[15];
    strftime(dateStr, sizeof(dateStr), "%b %d", &timeinfo);

    tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
    tft.drawRightString(String(dateStr), 235, 110, 1);

    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.drawRightString(String(timeStr), 235, 122, 1);
  } else {
    String storedDate = preferences.getString("lastDate", "Waiting");
    tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
    tft.drawRightString(storedDate, 235, 110, 1);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.drawRightString("--:--", 235, 122, 1);
  }
}

void displaySchedule() {
  tft.fillScreen(TFT_BLACK);

  DynamicJsonDocument doc(2048);
  DeserializationError error = deserializeJson(doc, rawJsonData);

  if (error) {
    tft.setTextColor(TFT_RED, TFT_BLACK);
    tft.setCursor(5, 5);
    tft.println("Error parsing data");
    drawWifiIcon();
    drawClock();
    return;
  }

  JsonObject cfm = doc["cfm"];
  if (cfm.isNull()) {
    tft.setTextColor(TFT_YELLOW, TFT_BLACK);
    tft.setCursor(5, 5);
    tft.println("No lesson data");
    drawWifiIcon();
    drawClock();
    return;
  }

  JsonObject thisWeek = cfm["current"];
  JsonObject nextWeek = cfm["next"];

  // Display THIS WEEK
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.setTextSize(1);
  tft.setCursor(5, 5);
  tft.println("LESSON: THIS WEEK");

  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(1);
  tft.setCursor(5, 25);
  String twScriptures =
      thisWeek["scripture"].isNull() ? "" : thisWeek["scripture"].as<String>();
  if (twScriptures == "" && !thisWeek["start"].isNull()) {
    twScriptures =
        thisWeek["start"].as<String>() + " - " + thisWeek["end"].as<String>();
  }
  printWrapped(5, 25, 230, twScriptures);

  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.setCursor(5, tft.getCursorY() + 5);
  String twTopic =
      thisWeek["topic"].isNull() ? "" : thisWeek["topic"].as<String>();
  if (twTopic.length() > 0) {
    printWrapped(5, tft.getCursorY(), 230, twTopic);
  }

  // Display NEXT WEEK
  int nextY = tft.getCursorY() + 15;
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.setTextSize(1);
  tft.setCursor(5, nextY);
  tft.println("LESSON: NEXT WEEK");

  tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
  tft.setTextSize(1);
  tft.setCursor(5, nextY + 20);
  String nwScriptures =
      nextWeek["scripture"].isNull() ? "" : nextWeek["scripture"].as<String>();
  if (nwScriptures == "" && !nextWeek["start"].isNull()) {
    nwScriptures =
        nextWeek["start"].as<String>() + " - " + nextWeek["end"].as<String>();
  }
  printWrapped(5, nextY + 20, 230, nwScriptures);

  drawWifiIcon();
  drawClock();
}

void displayActivities() {
  tft.fillScreen(TFT_BLACK);

  DynamicJsonDocument doc(2048);
  DeserializationError error = deserializeJson(doc, rawJsonData);

  if (error || doc["activities"].isNull()) {
    tft.setTextColor(TFT_YELLOW, TFT_BLACK);
    tft.setCursor(5, 5);
    tft.println("No activity data");
    drawWifiIcon();
    drawClock();
    return;
  }

  JsonObject acts = doc["activities"];
  JsonObject thisAct = acts["current"];
  JsonObject nextAct = acts["next"];

  // Display THIS ACTIVITY
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.setTextSize(1);
  tft.setCursor(5, 5);
  tft.println("ACTIVITY: THIS WK");

  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(1);
  tft.setCursor(5, 25);
  String twDate = thisAct["date"].isNull() ? "" : thisAct["date"].as<String>();
  String twTime = thisAct["time"].isNull() ? "" : thisAct["time"].as<String>();
  String twDateTime = twDate + " " + twTime;
  printWrapped(5, 25, 230, twDateTime);

  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.setCursor(5, tft.getCursorY() + 5);
  String twTopic =
      thisAct["title"].isNull() ? "" : thisAct["title"].as<String>();
  if (twTopic.length() > 0) {
    printWrapped(5, tft.getCursorY(), 230, twTopic);
  }

  // Display LATER ACTIVITY
  int nextY = tft.getCursorY() + 15;
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.setTextSize(1);
  tft.setCursor(5, nextY);
  tft.println("ACTIVITY: NEXT WK");

  tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
  tft.setTextSize(1);
  tft.setCursor(5, nextY + 20);
  String nwDate = nextAct["date"].isNull() ? "" : nextAct["date"].as<String>();
  String nwTitle =
      nextAct["title"].isNull() ? "" : nextAct["title"].as<String>();
  String nwDetails = nwDate + " " + nwTitle;
  printWrapped(5, nextY + 20, 230, nwDetails);

  drawWifiIcon();
  drawClock();
}

void displayAssignments() {
  tft.fillScreen(TFT_BLACK);

  DynamicJsonDocument doc(2048);
  DeserializationError error = deserializeJson(doc, rawJsonData);

  if (error || doc["assignments"].isNull()) {
    tft.setTextColor(TFT_YELLOW, TFT_BLACK);
    tft.setCursor(5, 5);
    tft.println("No assignment data");
    drawWifiIcon();
    drawClock();
    return;
  }

  JsonObject asg = doc["assignments"];
  JsonObject thisAssgn = asg["current"];
  JsonObject nextAssgn = asg["next"];

  // Display THIS ASSIGNMENT
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.setTextSize(1);
  tft.setCursor(5, 5);
  String thDate =
      thisAssgn["date"].isNull() ? "TBD" : thisAssgn["date"].as<String>();
  tft.println("ASSIGN: THIS WEEK");

  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(1);
  tft.setCursor(5, 25);
  String l1 =
      thisAssgn["lesson"].isNull() ? "TBD" : thisAssgn["lesson"].as<String>();
  String m1 = thisAssgn["messenger"].isNull()
                  ? "TBD"
                  : thisAssgn["messenger"].as<String>();
  tft.print("Date: ");
  tft.println(thDate);
  tft.setCursor(5, tft.getCursorY());
  tft.print("Lessons: ");
  tft.println(l1);
  tft.setCursor(5, tft.getCursorY());
  tft.print("Messenger: ");
  tft.println(m1);

  // Display LATER ASSIGNMENT
  int nextY = tft.getCursorY() + 15;
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.setTextSize(1);
  tft.setCursor(5, nextY);
  String nxDate =
      nextAssgn["date"].isNull() ? "TBD" : nextAssgn["date"].as<String>();
  tft.println("ASSIGN: NEXT WEEK");

  tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
  tft.setTextSize(1);
  tft.setCursor(5, nextY + 20);
  String l2 =
      nextAssgn["lesson"].isNull() ? "TBD" : nextAssgn["lesson"].as<String>();
  String m2 = nextAssgn["messenger"].isNull()
                  ? "TBD"
                  : nextAssgn["messenger"].as<String>();
  tft.print("Date: ");
  tft.println(nxDate);
  tft.setCursor(5, tft.getCursorY());
  tft.print("Lessons: ");
  tft.println(l2);
  tft.setCursor(5, tft.getCursorY());
  tft.print("Messenger: ");
  tft.println(m2);

  drawWifiIcon();
  drawClock();
}

void refreshDisplay() {
  if (potatoMode) {
    displayPotato();
  } else if (currentScreen == SCREEN_LESSONS) {
    displaySchedule();
  } else if (currentScreen == SCREEN_ACTIVITIES) {
    displayActivities();
  } else {
    displayAssignments();
  }
}

void syncDataFromOnlineUrl() {
  if (String(DATA_JSON_URL) == "http://YOUR_HOMELAB_IP/deacon_data.json") {
    Serial.println("Skipping sync: DATA_JSON_URL is not configured.");
    return;
  }

  Serial.println("Fetching data from JSON URL...");
  HTTPClient http;
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  http.begin(DATA_JSON_URL);

  int httpCode = http.GET();
  if (httpCode > 0) {
    if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
      String payload = http.getString();
      Serial.println("Downloaded new data.");

      // Save it and refresh
      rawJsonData = payload;
      preferences.putString("jsonData", rawJsonData);
      refreshDisplay();
    }
  } else {
    Serial.printf("HTTP GET failed, error: %s\n",
                  http.errorToString(httpCode).c_str());
  }
  http.end();
}

void setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.println(F("=== Deacon ESP32 ==="));
  Serial.printf("Firmware version: %s\n", FW_VERSION);
  Serial.printf("Git describe:     %s\n", FW_GIT_DESCRIBE);
  Serial.printf("Git commit:       %s\n", FW_GIT_COMMIT);

  tft.init();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);

  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, HIGH);

  myChipID = (uint32_t)(ESP.getEfuseMac() & 0xFFFFFFFF);
  Serial.printf("Chip ID: 0x%08X\n", myChipID);

  // Potato trigger pins — pulled up; bridge them together to activate
  pinMode(POT_PIN_A, INPUT_PULLUP);
  pinMode(POT_PIN_B, INPUT_PULLUP);

  pinMode(0, INPUT_PULLUP); // Boot button

  preferences.begin("deacon", false);
  currentScreen = (ActiveScreen)preferences.getInt("screen", SCREEN_LESSONS);
  rawJsonData = preferences.getString("jsonData", "{}");

  currentSyncState = SYNCING;
  refreshDisplay();
}

bool needsWifiAction = true;
unsigned long wifiConnectStart = 0;
int currentWifiIndex = 0;

void loop() {
  static unsigned long lastClockDraw = 0;
  static unsigned long lastHourlySync = 0;

  // ── Potato mode toggle: bridge GPIO 12 + 13 simultaneously ─────────────────
  {
    static unsigned long bridgeStart  = 0;
    static bool          bridgeActive = false;
    bool bridged = (analogRead(POT_PIN_A) < POT_BRIDGE_THR) &&
                   (analogRead(POT_PIN_B) < POT_BRIDGE_THR);
    if (bridged && !bridgeActive) {
      bridgeStart  = millis();
      bridgeActive = true;
    } else if (!bridged) {
      if (bridgeActive && millis() - bridgeStart > 300) {
        // Confirmed bridge gesture — toggle mode
        potatoMode = !potatoMode;
        if (potatoMode) {
          potState = POT_IDLE;
          initEspNow();
        } else {
          deinitEspNow();
          potState = POT_IDLE;
        }
        refreshDisplay();
      }
      bridgeActive = false;
    }
  }

  // ── Boot button ──────────────────────────────────────────────────────────────
  if (digitalRead(0) == LOW) {
    delay(50);
    if (digitalRead(0) == LOW) {
      if (potatoMode) {
        // ── Potato mode: short press = pass, long press (2s) = new game ────
        unsigned long pressStart = millis();
        while (digitalRead(0) == LOW) { delay(10); }
        bool longPress = (millis() - pressStart > 2000);

        if (longPress && potState == POT_IDLE) {
          // Start a fresh game with 30-second countdown
          sendPotato(30);
          Serial.println("[Potato] New game started, timer=30");
        } else if (!longPress && potState == POT_ACTIVE) {
          // Pass the potato; subtract 1 s as a penalty
          int elapsed  = (int)((millis() - potReceived) / 1000);
          int timeLeft = max(1, potTimer - elapsed - 1);
          sendPotato(timeLeft);
          potState = POT_IDLE;
          displayPotato();
          Serial.printf("[Potato] Passed! timer=%d\n", timeLeft);
        } else if (!longPress && potState == POT_BOOM) {
          potState = POT_IDLE;
          displayPotato();
        }
      } else {
        // ── Normal mode: cycle schedule screens ─────────────────────────────
        if (currentScreen == SCREEN_LESSONS) {
          currentScreen = SCREEN_ACTIVITIES;
        } else if (currentScreen == SCREEN_ACTIVITIES) {
          currentScreen = SCREEN_ASSIGNMENTS;
        } else {
          currentScreen = SCREEN_LESSONS;
        }
        preferences.putInt("screen", currentScreen);
        refreshDisplay();
        while (digitalRead(0) == LOW) { delay(10); }
      }
    }
  }

  // ── Potato active: update countdown, detect boom ────────────────────────────
  if (potatoMode && potState == POT_ACTIVE) {
    static unsigned long lastPotDraw = 0;
    int elapsed  = (int)((millis() - potReceived) / 1000);
    int timeLeft = potTimer - elapsed;
    if (timeLeft <= 0) {
      potState = POT_BOOM;
      displayPotato();
    } else if (millis() - lastPotDraw > 1000) {
      lastPotDraw = millis();
      displayPotato(); // redraw countdown every second
    }
  }

  // ── Clock refresh (skip in potato mode — different screen layout) ────────────
  if (!potatoMode && (millis() - lastClockDraw > 10000 || lastClockDraw == 0)) {
    drawClock();
    lastClockDraw = millis();
  }

  if (needsWifiAction) {
    if (wifiConnectStart == 0) {
      wifiConnectStart = millis();
      if (WiFi.status() == WL_CONNECTED) {
        WiFi.disconnect(true);
      }
      delay(100);
      WiFi.mode(WIFI_STA);
      Serial.printf("\nConnecting to SSID: %s ",
                    WIFI_NETWORKS[currentWifiIndex].ssid);
      WiFi.begin(WIFI_NETWORKS[currentWifiIndex].ssid,
                 WIFI_NETWORKS[currentWifiIndex].password);
      currentSyncState = SYNCING;
      drawWifiIcon();
    }

    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("Connected to WiFi successfully!");
      configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

      // Wait for time to sync
      struct tm timeinfo;
      bool timeSynced = false;
      for (int i = 0; i < 10; i++) {
        if (getLocalTime(&timeinfo, 1000)) {
          if (timeinfo.tm_year > 100) { // Make sure year is > 2000
            timeSynced = true;
            break;
          }
        }
        delay(500);
      }

      if (timeSynced) {
        currentSyncState = SYNC_OK;

        char dateStr[15];
        strftime(dateStr, sizeof(dateStr), "%b %d", &timeinfo);
        preferences.putString("lastDate", String(dateStr));

        syncDataFromOnlineUrl();
      } else {
        currentSyncState = SYNC_FAILED;
      }
      needsWifiAction = false;
      lastHourlySync = millis();
      currentWifiIndex = 0; // Reset for next time
      drawWifiIcon();
      drawClock();

      WiFi.disconnect(true);
      WiFi.mode(WIFI_OFF);
    } else if (millis() - wifiConnectStart > 20000) {
      Serial.printf("\nTimeout connecting to %s. Trying next...\n",
                    WIFI_NETWORKS[currentWifiIndex].ssid);

      WiFi.disconnect(true);
      delay(100);

      currentWifiIndex++;
      wifiConnectStart = 0;

      if (currentWifiIndex >= WIFI_NETWORKS_COUNT) {
        Serial.println("Tried all networks. Sync failed.");
        needsWifiAction = false;
        lastHourlySync = millis();
        currentWifiIndex = 0;
        currentSyncState = SYNC_FAILED;
        drawWifiIcon();
        drawClock();

        WiFi.disconnect(true);
        WiFi.mode(WIFI_OFF);
      }
    } else {
      static unsigned long lastDot = 0;
      if (millis() - lastDot > 1000) {
        Serial.print(".");
        lastDot = millis();
      }
    }
  } else {
    // Check if an hour passed
    if (millis() - lastHourlySync > 3600000) {
      needsWifiAction = true;
      wifiConnectStart = 0;
    }
  }
}
