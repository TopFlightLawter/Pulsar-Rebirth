// ======================================================================================
// OLED DISPLAY FUNCTIONS - Pulsar Rebirth v5.6
// ======================================================================================

/**
 * Helper: format percent as right-justified 3-character string
 */
String formatPct(int pct) {
  if (pct < 0) pct = 0;
  if (pct > 999) pct = 999;
  if (pct < 10) {
    return "  " + String(pct);
  } else if (pct < 100) {
    return " " + String(pct);
  } else {
    return String(pct);
  }
}

/**
 * Initialize the OLED display
 */
void initOLED() {
  Serial.print("Initializing OLED display for ");
  Serial.print(Config::SYSTEM_NAME);
  Serial.println("...");

  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    sysState.display.available = false;
    return;
  }

  sysState.display.available = true;
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  sysState.display.splashStartTime = millis();
  sysState.display.showSplash = true;
  sysState.display.animationFrame = 0;

  showSplashScreen();

  Serial.print("OLED Display initialized successfully (");
  Serial.print(Config::FIRMWARE_VERSION);
  Serial.println(")");
}

/**
 * Show splash screen
 */
void showSplashScreen() {
  if (!sysState.display.available) return;

  display.clearDisplay();

  display.drawRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, SSD1306_WHITE);
  display.drawRect(1, 1, SCREEN_WIDTH - 2, SCREEN_HEIGHT - 2, SSD1306_WHITE);

  display.setTextSize(2);
  String line1 = "PULSAR";
  int16_t x1, y1;
  uint16_t w1, h1;
  display.getTextBounds(line1, 0, 0, &x1, &y1, &w1, &h1);
  int pulsar_x = (SCREEN_WIDTH - w1) / 2;
  display.setCursor(pulsar_x, 2);
  display.println(F("PULSAR"));

  display.drawLine(0, 20, SCREEN_WIDTH, 20, SSD1306_WHITE);

  display.setTextSize(1);
  String line2 = "REBIRTH";
  display.getTextBounds(line2, 0, 0, &x1, &y1, &w1, &h1);
  int rebirth_x = (SCREEN_WIDTH - w1) / 2;
  display.setCursor(rebirth_x, 24);
  display.println(F("REBIRTH"));

  String versionStr = String(Config::FIRMWARE_VERSION);
  if (!versionStr.startsWith("v") && !versionStr.startsWith("V")) {
    versionStr = "v" + versionStr;
  }
  display.getTextBounds(versionStr, 0, 0, &x1, &y1, &w1, &h1);
  int version_x = (SCREEN_WIDTH - w1) / 2;
  display.setCursor(version_x, 34);
  display.println(versionStr);

  String line4 = "WEBSOCKET TUNING";
  display.getTextBounds(line4, 0, 0, &x1, &y1, &w1, &h1);
  int breath_x = (SCREEN_WIDTH - w1) / 2;
  display.setCursor(breath_x, 46);
  display.println(F("WEBSOCKET TUNING"));

  drawProgressBar(14, 56, 100, sysState.display.animationFrame * 25);

  display.display();
}

/**
 * Draw a progress bar on the display
 */
void drawProgressBar(int x, int y, int width, int progress) {
  progress = constrain(progress, 0, 100);
  int barWidth = map(progress, 0, 100, 0, width);

  display.drawRect(x, y, width, 6, SSD1306_WHITE);

  for (int i = 0; i < barWidth - 2; i++) {
    if (i % 3 != 0) {
      display.drawLine(x + 1 + i, y + 1, x + 1 + i, y + 4, SSD1306_WHITE);
    }
  }
}

/**
 * Enhanced OLED display update
 */
void updateOLEDDisplay() {
  if (!sysState.display.available) return;

  if (sysState.display.showSplash) {
    if (millis() - sysState.display.animationTime > 500) {
      sysState.display.animationFrame = (sysState.display.animationFrame + 1) % Config::LOADING_FRAMES;
      sysState.display.animationTime = millis();
      showSplashScreen();
    }

    if (millis() - sysState.display.splashStartTime > Config::SPLASH_DURATION) {
      Serial.println("Splash screen complete, transitioning to main display");
      sysState.display.showSplash = false;
      sysState.display.lastUpdate = 0;
    }

    if (sysState.display.showSplash) return;
  }

  bool forceUpdate = sysState.motor.motor1Active || sysState.motor.motor2Active || 
                     sysState.motor.pwmWarning.active;
  
  if (!forceUpdate && millis() - sysState.display.lastUpdate < OLED_UPDATE_INTERVAL) {
    return;
  }

  sysState.display.lastUpdate = millis();

  display.clearDisplay();

  display.drawLine(0, 10, SCREEN_WIDTH, 10, SSD1306_WHITE);

  if (WiFi.status() == WL_CONNECTED) {
    drawWifiIcon(2, 2, 3);
  } else {
    display.drawLine(2, 2, 8, 8, SSD1306_WHITE);
    display.drawLine(2, 8, 8, 2, SSD1306_WHITE);
  }

  drawAlarmIcon(SCREEN_WIDTH - 18, 2, sysState.alarm.enabled);

  drawDualMotorStatus(12, 2);

  if (sysState.motor.pwmWarning.active) {
    display.setTextSize(2);
    display.setCursor(10, 16);
    display.print("WARNING");
    
    drawPWMIntensityBar(10, 32, 108, sysState.motor.pwmWarning.currentIntensity);
    
    display.setTextSize(1);
    display.setCursor(10, 52);
    display.print(F("Intensity ("));
    display.print(formatPct((int)sysState.motor.pwmWarning.currentIntensity));
    display.print(F(")%"));
    
  } else if (sysState.motor.progressivePulse.active) {
    display.setTextSize(2);
    display.setCursor(25, 16);
    display.print("GET UP!");
    
    drawProgressiveIntensityBar(10, 32, 108, sysState.motor.progressivePulse.currentIntensity);
    
    display.setTextSize(1);
    display.setCursor(10, 52);
    display.print(F("INTENSITY ("));
    display.print(formatPct((int)sysState.motor.progressivePulse.currentIntensity));
    display.print(F(")%"));
    
  } else {
    String timeStr = getFormattedTime();
    display.setTextSize(2);

    int16_t x1, y1;
    uint16_t w, h;
    display.getTextBounds(timeStr, 0, 0, &x1, &y1, &w, &h);
    display.setCursor((SCREEN_WIDTH - w) / 2, 16);
    display.print(timeStr);

    drawClockFace(SCREEN_WIDTH - 10, 25, 6);

    display.drawLine(0, 34, SCREEN_WIDTH, 34, SSD1306_WHITE);

    display.setTextSize(1);
    String nextAlarm = getNextAlarmTime();

    if (nextAlarm.startsWith("Today at ")) {
      nextAlarm = nextAlarm.substring(9);
    }

    display.setCursor(2, 37);
    display.print(F("Next Alarm: "));
    display.print(nextAlarm);

    display.setCursor(2, 47);
    String netStatus;
    if (WiFi.status() == WL_CONNECTED) {
      netStatus = sysState.network.activeNetwork;
      if (netStatus.length() > 16) {
        netStatus = netStatus.substring(0, 13) + "...";
      }
    } else {
      netStatus = "Offline Mode";
    }
    display.print(netStatus);

    display.setCursor(2, 56);
    String remaining = getTimeRemaining();
    if (remaining != "Leave me alone." && remaining != "No alarms for today." && remaining != "Unavailable") {
      display.print(F("In: "));
      display.print(remaining);
    }
  }

  if (sysState.alarm.snoozeActive) {
    unsigned long remainingSnooze = rtConfig.snoozeDuration - (millis() - sysState.alarm.snoozeStartTime);
    int snoozeSecs = remainingSnooze / 1000;

    display.fillRect(0, SCREEN_HEIGHT - 10, SCREEN_WIDTH, 10, SSD1306_BLACK);
    display.drawRect(0, SCREEN_HEIGHT - 10, SCREEN_WIDTH, 10, SSD1306_WHITE);

    display.setCursor(2, SCREEN_HEIGHT - 8);
    display.print(F("Snooze: "));
    display.print(snoozeSecs);
    display.print(" s");
  }

  if (sysState.motor.motor1Active || sysState.motor.motor2Active || 
      sysState.motor.pwmWarning.active || sysState.motor.progressivePulse.active) {
    
    if ((millis() / 500) % 2 == 0) {
      display.drawRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, SSD1306_WHITE);
      display.drawRect(2, 2, SCREEN_WIDTH - 4, SCREEN_HEIGHT - 4, SSD1306_WHITE);
    }

    if (!sysState.motor.pwmWarning.active && !sysState.motor.progressivePulse.active) {
      display.setTextSize(2);
      display.setCursor(32, 42);
      display.print(F("ALARM!"));
    }
  }

  if (sysState.watchdog.enabled) {
    display.fillCircle(SCREEN_WIDTH - 3, SCREEN_HEIGHT - 3, 1, SSD1306_WHITE);
  }

  display.display();
}

/**
 * Draw PWM intensity bar during warning sequence
 */
void drawPWMIntensityBar(int x, int y, int width, float intensity) {
  display.drawRect(x, y, width, 8, SSD1306_WHITE);
  
  int fillWidth = (int)((intensity / 100.0) * (width - 2));
  display.fillRect(x + 1, y + 1, fillWidth, 6, SSD1306_WHITE);
  
  for (int i = 20; i <= 80; i += 20) {
    int markerX = x + (int)((i / 100.0) * width);
    display.drawLine(markerX, y - 2, markerX, y + 9, SSD1306_WHITE);
  }
}

/**
 * Draw progressive intensity bar
 */
void drawProgressiveIntensityBar(int x, int y, int width, float intensity) {
  display.drawRect(x, y, width, 8, SSD1306_WHITE);
  
  float normalizedIntensity = (intensity - rtConfig.mainStartingIntensity) / 
                              (rtConfig.mainMaxIntensity - rtConfig.mainStartingIntensity);
  int fillWidth = (int)(normalizedIntensity * (width - 2));
  display.fillRect(x + 1, y + 1, fillWidth, 6, SSD1306_WHITE);
  
  for (int i = 20; i <= 100; i += 20) {
    float markerPos = (i - rtConfig.mainStartingIntensity) / 
                      (float)(rtConfig.mainMaxIntensity - rtConfig.mainStartingIntensity);
    int markerX = x + (int)(markerPos * width);
    display.drawLine(markerX, y - 2, markerX, y + 9, SSD1306_WHITE);
  }
  
  float currentPos = normalizedIntensity;
  int currentX = x + (int)(currentPos * width);
  display.drawLine(currentX, y - 3, currentX, y + 10, SSD1306_WHITE);
}

/**
 * Draw dual motor status in header
 */
void drawDualMotorStatus(int x, int y) {
  const int boxW = 6;
  const int boxH = 6;
  int motor1X = x;
  int motor2X = SCREEN_WIDTH - x - boxW;

  if (sysState.motor.motor1Active || sysState.motor.pwmWarning.active) {
    display.fillRect(motor1X, y, boxW, boxH, SSD1306_WHITE);
  } else {
    display.drawRect(motor1X, y, boxW, boxH, SSD1306_WHITE);
  }
  display.setTextSize(1);
  display.setCursor(motor1X + boxW + 2, y + 1);
  display.print("1");

  if (sysState.motor.motor2Active || sysState.motor.pwmWarning.active) {
    display.fillRect(motor2X, y, boxW, boxH, SSD1306_WHITE);
  } else {
    display.drawRect(motor2X, y, boxW, boxH, SSD1306_WHITE);
  }
  int label2X = motor2X - 10;
  display.setCursor(label2X, y + 1);
  display.print("2");
}

/**
 * Draw WiFi signal strength icon
 */
void drawWifiIcon(int x, int y, int strength) {
  for (int i = 0; i < strength; i++) {
    int radius = 2 + (i * 2);
    display.drawCircle(x, y + 7, radius, SSD1306_WHITE);
  }
}

/**
 * Draw alarm enabled/disabled icon
 */
void drawAlarmIcon(int x, int y, bool enabled) {
  const int smallLen = 4;
  const int midLen = 7;
  const int longLen = 10;

  int baseRight = x + longLen;

  display.drawLine(baseRight - smallLen + 1, y, baseRight, y, SSD1306_WHITE);

  if (enabled) {
    display.drawLine(baseRight - midLen + 1, y + 2, baseRight, y + 2, SSD1306_WHITE);
  }

  display.drawLine(baseRight - longLen + 1, y + 4, baseRight, y + 4, SSD1306_WHITE);
}

/**
 * Draw a small clock face
 */
void drawClockFace(int x, int y, int radius) {
  struct tm timeInfo = { 0 };
  if (!getLocalTime(&timeInfo)) {
    display.drawCircle(x, y, radius, SSD1306_WHITE);
    display.drawLine(x, y, x, y - radius + 2, SSD1306_WHITE);
    display.drawLine(x, y, x + 2, y, SSD1306_WHITE);
    return;
  }

  display.drawCircle(x, y, radius, SSD1306_WHITE);

  float hourAngle = ((timeInfo.tm_hour % 12) + timeInfo.tm_min / 60.0) * 30.0 - 90.0;
  float minuteAngle = timeInfo.tm_min * 6.0 - 90.0;

  hourAngle = hourAngle * PI / 180.0;
  minuteAngle = minuteAngle * PI / 180.0;

  int hourX = x + (radius - 2) * cos(hourAngle);
  int hourY = y + (radius - 2) * sin(hourAngle);
  display.drawLine(x, y, hourX, hourY, SSD1306_WHITE);

  int minuteX = x + (radius - 1) * cos(minuteAngle);
  int minuteY = y + (radius - 1) * sin(minuteAngle);
  display.drawLine(x, y, minuteX, minuteY, SSD1306_WHITE);

  display.drawPixel(x, y, SSD1306_WHITE);
}

/**
 * Show telemetry screen
 */
void showTelemetryScreen() {
  if (!sysState.display.available) return;

  display.clearDisplay();
  display.setTextSize(1);

  display.setCursor(0, 0);
  display.print(F("=== TELEMETRY ==="));
  display.drawLine(0, 9, SCREEN_WIDTH, 9, SSD1306_WHITE);

  unsigned long uptimeSeconds = sysState.telemetry.uptime / 1000;
  unsigned long hours = uptimeSeconds / 3600;
  unsigned long minutes = (uptimeSeconds % 3600) / 60;

  display.setCursor(0, 12);
  display.print(F("Uptime: "));
  display.print(hours);
  display.print("h ");
  display.print(minutes);
  display.print("m");

  display.setCursor(0, 22);
  display.print(F("Alarms: "));
  display.print(sysState.telemetry.alarmCount);

  display.setCursor(0, 32);
  display.print(F("Version: "));
  display.print(Config::FIRMWARE_VERSION);

  display.setCursor(0, 42);
  display.print(F("Heap: "));
  display.print(ESP.getFreeHeap() / 1024);
  display.print("KB");

  display.setCursor(0, 52);
  display.print(F("WS Clients: "));
  display.print(sysState.websocket.connectedClients);

  display.display();
}

/**
 * Show save confirmation animation - "energy ball" racing around screen border
 */
void showSaveConfirmationAnimation() {
  if (!sysState.display.available) return;

  // Perimeter calculation:
  // Top: 128 pixels (0,0) to (127,0)
  // Right: 64 pixels (127,0) to (127,63)
  // Bottom: 128 pixels (127,63) to (0,63)
  // Left: 64 pixels (0,63) to (0,0)
  // Total: 384 pixels

  const int ANIMATION_DELAY = 5;  // 5ms per pixel step for smooth motion
  const int TOTAL_PERIMETER = 384;

  // Loop through entire perimeter once
  for (int i = 0; i < TOTAL_PERIMETER; i++) {
    int x, y;
    int x2, y2;  // Second pixel position for "energy ball" effect

    // Calculate current pixel position based on position around perimeter
    if (i < 128) {
      // Top edge: left to right (0,0) -> (127,0)
      x = i;
      y = 0;
      x2 = (i < 127) ? i + 1 : i;  // Adjacent pixel to the right
      y2 = 0;
    }
    else if (i < 128 + 64) {
      // Right edge: top to bottom (127,0) -> (127,63)
      x = 127;
      y = i - 128;
      x2 = 127;
      y2 = (y < 63) ? y + 1 : y;  // Adjacent pixel below
    }
    else if (i < 128 + 64 + 128) {
      // Bottom edge: right to left (127,63) -> (0,63)
      x = 127 - (i - 128 - 64);
      y = 63;
      x2 = (x > 0) ? x - 1 : x;  // Adjacent pixel to the left
      y2 = 63;
    }
    else {
      // Left edge: bottom to top (0,63) -> (0,0)
      x = 0;
      y = 63 - (i - 128 - 64 - 128);
      x2 = 0;
      y2 = (y > 0) ? y - 1 : y;  // Adjacent pixel above
    }

    // Draw the "energy ball" (2 adjacent pixels)
    display.drawPixel(x, y, SSD1306_WHITE);
    display.drawPixel(x2, y2, SSD1306_WHITE);
    display.display();

    // Small delay for smooth animation
    delay(ANIMATION_DELAY);

    // Clear the pixels to create moving effect (unless at start to avoid flicker)
    if (i > 0) {
      display.drawPixel(x, y, SSD1306_BLACK);
      display.drawPixel(x2, y2, SSD1306_BLACK);
    }
  }

  // Clear display and prepare for normal operation
  display.clearDisplay();
  display.display();

  // Reset the display update timer so normal display refreshes immediately
  sysState.display.lastUpdate = 0;
}
