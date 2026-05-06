#include "fw_version.h"
#include "secrets.h"
#include <Arduino.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <Preferences.h>
#include <TFT_eSPI.h>
#include <WiFi.h>
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
  if (currentScreen == SCREEN_LESSONS) {
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

  // Poll button for screen toggling
  if (digitalRead(0) == LOW) {
    delay(50); // basic debounce
    if (digitalRead(0) == LOW) {
      if (currentScreen == SCREEN_LESSONS) {
        currentScreen = SCREEN_ACTIVITIES;
      } else if (currentScreen == SCREEN_ACTIVITIES) {
        currentScreen = SCREEN_ASSIGNMENTS;
      } else {
        currentScreen = SCREEN_LESSONS;
      }
      preferences.putInt("screen", currentScreen);
      refreshDisplay();

      while (digitalRead(0) == LOW) {
        delay(10);
      }
    }
  }

  if (millis() - lastClockDraw > 10000 || lastClockDraw == 0) {
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
