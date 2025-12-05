// ======================================================================================
// PULSAR REBIRTH v5.6 - Main Sketch
// ======================================================================================

#include <WiFi.h>
#include <time.h>
#include <ezButton.h>
#include <ArduinoOTA.h>
#include <WebServer.h>
#include <WebSocketsServer.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Preferences.h>
#include <ESPmDNS.h>
#include <esp_task_wdt.h>
#include "Config.h"

// ======================================================================================
// GLOBAL RUNTIME CONFIG INSTANCE
// ======================================================================================
RuntimeConfig rtConfig;

// ======================================================================================
// OLED DISPLAY
// ======================================================================================
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// ======================================================================================
// PWM WARNING PATTERN ARRAY (now mirrors rtConfig for compatibility)
// ======================================================================================
int pwmWarningSteps[Config::MAX_PWM_STEPS] = {
  0, 5, 7, 0, 5, 6, 0, 5, 7, 0, 5, 6, 0, 5, 7, 0, 5, 6, 0, 5, 7, 0, 5, 7, 0, 7, 0, 7, 0, 7, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0
};

// ======================================================================================
// GLOBAL SYSTEM STATE
// ======================================================================================
SystemState sysState;

const unsigned long OLED_UPDATE_INTERVAL = 1000;

// ======================================================================================
// FUNCTION DECLARATIONS
// ======================================================================================

// Motor Control
void stopAllMotors();
void initializePWMChannels();
void startPWMWarning(int motorNumber);
void updatePWMWarning();
void activateMotor1(int intensityPercent);
void activateMotor2(int intensityPercent);
void deactivateAllMotors();
void startProgressivePulsePattern();
void stopProgressivePulsePattern();
void handleProgressivePulsePattern();

// Alarm Logic
void handleAlarmSequence();
void handleButtonStates();
void handleLongPress();
void handleLongPressAction(int buttonIndex);
void printSystemStatus();
void performFactoryReset();
void checkAndTriggerAlarms();
void handleQuickTest();
void handleSingleFlash();
void setAlarmsEnabled(bool enabled);
void handleSnooze();
void transitionAlarmStage(AlarmStage newStage);

// Display Functions
void initOLED();
void showSplashScreen();
void drawProgressBar(int x, int y, int width, int progress);
void updateOLEDDisplay();
void drawPWMIntensityBar(int x, int y, int width, float intensity);
void drawProgressiveIntensityBar(int x, int y, int width, float intensity);
void drawDualMotorStatus(int x, int y);
void drawWifiIcon(int x, int y, int strength);
void drawAlarmIcon(int x, int y, bool enabled);
void drawClockFace(int x, int y, int radius);
void showTelemetryScreen();

// Time Manager
void synchronizeTime();
String getFormattedTime();
String getFormattedDate();
String getNextAlarmTime();
String getTimeRemaining();
bool isInAlarmWindow();
bool shouldTriggerAlarm();

// Relay Control
void initializeRelay();
void startRelayFlashing();
void setRelaySolid();
void turnOffRelay();
void updateRelayFlashing();
String getRelayStatus();

// Web Server
void setupWebServer();
void sendCORSResponse(int code, const String& contentType, const String& content);
void handleRoot();
void handleToggleAlarm();
void handleTestButton();
void handleTurnOffAllMotors();
void handleToggleMotor1();
void handleToggleMotor2();
void handleStartProgressivePattern();
void handleStopProgressivePattern();
void handleTestPWMWarning();
void handlePWMTestMotor1();
void handlePWMTestMotor2();
void handleMotorStatus();
void handleProgressiveStatus();
void handleQuickTestWeb();
void handleSingleFlashTest();
void handleCurrentTime();
void handleNextAlarm();
void handleNetworkStatus();
void handleReconnectWiFi();
void handleAlarmsStatus();
void handleTelemetry();
void handleSystemHealth();
String generateEnhancedHtmlPage();

// WebSocket Server
void initWebSocket();
void handleWebSocket();
void broadcastMotorState();
void broadcastAlarmState();
void saveRuntimeConfig();
void loadRuntimeConfig();
void exportConfigToSerial();
String generateConfigJSON();
bool importConfigFromJSON(const String& jsonStr);

// WiFi Manager
bool connectToNetwork(const char* ssid, const char* password, int maxAttempts);
bool connectToWiFi();
void checkWiFiConnection();
void setupMDNS();
void setupOTA();

// System
void initWatchdog();
void feedWatchdog();
void updateTelemetry();
void syncPwmArrayFromConfig();

// ======================================================================================
// BUTTONS
// ======================================================================================
ezButton snoozeButton(Config::SNOOZE_BUTTON_PIN, INPUT_PULLUP);
ezButton snoozeButton2(Config::SNOOZE_BUTTON_PIN_2, INPUT_PULLUP);
ezButton killSwitch(Config::KILL_SWITCH_PIN, INPUT_PULLUP);
ezButton testButton(Config::TEST_BUTTON_PIN, INPUT_PULLUP);
ezButton setAlarmButton(Config::SET_ALARM_BUTTON_PIN, INPUT_PULLUP);

// ======================================================================================
// STORAGE & WEB SERVER
// ======================================================================================
Preferences preferences;
WebServer server(80);

// ======================================================================================
// WATCHDOG
// ======================================================================================
void initWatchdog() {
  Serial.print("Initializing watchdog timer (");
  Serial.print(Config::WATCHDOG_TIMEOUT_SECONDS);
  Serial.println(" second timeout)...");
  esp_task_wdt_init(Config::WATCHDOG_TIMEOUT_SECONDS, true);
  esp_task_wdt_add(NULL);
  sysState.watchdog.enabled = true;
  sysState.watchdog.lastFeed = millis();
  Serial.println("Watchdog initialized - system protected");
}

void feedWatchdog() {
  if (sysState.watchdog.enabled) {
    esp_task_wdt_reset();
    sysState.watchdog.lastFeed = millis();
  }
}

// ======================================================================================
// TELEMETRY
// ======================================================================================
void updateTelemetry() {
  sysState.telemetry.uptime = millis();
  size_t currentHeap = ESP.getFreeHeap();
  if (currentHeap < sysState.telemetry.minFreeHeap) {
    sysState.telemetry.minFreeHeap = currentHeap;
  }
}

// ======================================================================================
// SYNC PWM ARRAY FROM RUNTIME CONFIG
// ======================================================================================
void syncPwmArrayFromConfig() {
  for (int i = 0; i < rtConfig.pwmStepsArrayLength && i < Config::MAX_PWM_STEPS; i++) {
    pwmWarningSteps[i] = rtConfig.pwmStepsArray[i];
  }
  Serial.print("Synced PWM array: ");
  Serial.print(rtConfig.pwmStepsArrayLength);
  Serial.println(" steps");
}

// ======================================================================================
// SETUP
// ======================================================================================
void setup() {
  Serial.begin(9600);
  delay(300);

  Serial.println("\n\n=== PULSAR REBIRTH v5.6 ===");
  Serial.print("Version: ");
  Serial.println(Config::FIRMWARE_VERSION);
  Serial.println(Config::SYSTEM_DESCRIPTION);
  Serial.print("Motor 1: GPIO ");
  Serial.println(Config::MOTOR1_PIN);
  Serial.print("Motor 2: GPIO ");
  Serial.println(Config::MOTOR2_PIN);
  Serial.print("Relay: GPIO ");
  Serial.println(Config::RELAY_PIN);
  Serial.print("Primary Snooze: GPIO ");
  Serial.println(Config::SNOOZE_BUTTON_PIN);
  Serial.print("Secondary Snooze: GPIO ");
  Serial.println(Config::SNOOZE_BUTTON_PIN_2);
  Serial.print("HTTP: http://");
  Serial.print(Config::OTA_HOSTNAME);
  Serial.println(".local");
  Serial.print("WebSocket: ws://");
  Serial.print(Config::OTA_HOSTNAME);
  Serial.print(".local:");
  Serial.println(Config::WEBSOCKET_PORT);

  initWatchdog();
  Serial.println("✅ Watchdog protection active");

  Wire.begin();
  initOLED();

  pinMode(Config::MOTOR1_PIN, OUTPUT);
  pinMode(Config::MOTOR2_PIN, OUTPUT);
  pinMode(Config::LED_PIN, OUTPUT);

  initializePWMChannels();

  initializeRelay();

  // Load runtime config from flash BEFORE using any values
  loadRuntimeConfig();
  syncPwmArrayFromConfig();
  
  // Set initial progressive pulse intensity from runtime config
  sysState.motor.progressivePulse.currentIntensity = rtConfig.mainStartingIntensity;
  
  Serial.println("Runtime tuning system: READY");

  digitalWrite(Config::LED_PIN, sysState.alarm.enabled ? HIGH : LOW);
  Serial.print("LED: ");
  Serial.println(sysState.alarm.enabled ? "ON" : "OFF");

  snoozeButton.setDebounceTime(Config::BUTTON_DEBOUNCE_MS);
  snoozeButton2.setDebounceTime(Config::BUTTON_DEBOUNCE_MS);
  killSwitch.setDebounceTime(Config::BUTTON_DEBOUNCE_MS);
  testButton.setDebounceTime(Config::BUTTON_DEBOUNCE_MS);
  setAlarmButton.setDebounceTime(Config::BUTTON_DEBOUNCE_MS);
  Serial.println("Buttons initialized (dual snooze enabled)");

  preferences.begin(Config::PREFERENCES_NAMESPACE, false);

  snoozeButton.loop();
  snoozeButton2.loop();
  killSwitch.loop();
  testButton.loop();
  setAlarmButton.loop();

  sysState.telemetry.minFreeHeap = ESP.getFreeHeap();
  Serial.print("Initial free heap: ");
  Serial.print(sysState.telemetry.minFreeHeap / 1024);
  Serial.println(" KB");

  Serial.println("Starting WiFi...");
  if (connectToWiFi()) {
    Serial.println("WiFi connected");
    synchronizeTime();
    sysState.network.lastSyncTime = millis();
    Serial.println("Time synchronized");
    
    setupMDNS();
    setupWebServer();
    server.begin();
    Serial.println("HTTP server started on port 80");
    
    initWebSocket();
    Serial.println("WebSocket server started on port 81");
    
    Serial.print("Access: http://");
    Serial.println(WiFi.localIP());
    Serial.print("Or use: http://");
    Serial.print(Config::OTA_HOSTNAME);
    Serial.println(".local");
    setupOTA();
  } else {
    Serial.println("WiFi not connected - offline mode");
    Serial.println("Physical buttons still functional");
  }

  Serial.println("=== INITIALIZATION COMPLETE ===");
  Serial.print("Firmware: ");
  Serial.println(Config::FIRMWARE_VERSION);
  Serial.println("✅ Watchdog timer active");
  Serial.println("✅ Dual snooze buttons (GPIO 18 + GPIO 32)");
  Serial.println("✅ Single motor stepped PWM warning");
  Serial.println("✅ Synchronized dual motor progressive pattern");
  Serial.println("✅ Relay control (ULN2003AN Channel 3)");
  Serial.println("✅ WebSocket tuning server (port 81)");
  Serial.println("✅ Runtime config persistence");
  Serial.print("✅ HTTP: http://");
  Serial.print(Config::OTA_HOSTNAME);
  Serial.println(".local");
  Serial.print("✅ WebSocket: ws://");
  Serial.print(Config::OTA_HOSTNAME);
  Serial.print(".local:");
  Serial.println(Config::WEBSOCKET_PORT);
  Serial.println("=========================================");
}

// ======================================================================================
// SERIAL COMMAND HANDLER
// ======================================================================================
void handleSerialCommands() {
  if (Serial.available() > 0) {
    String command = Serial.readStringUntil('\n');
    command.trim();
    command.toLowerCase();

    if (command == "export" || command == "export config") {
      Serial.println("\n=== EXPORTING CONFIGURATION ===");
      exportConfigToSerial();
      Serial.println("=== EXPORT COMPLETE ===\n");
    }
    else if (command == "help" || command == "?") {
      Serial.println("\n=== AVAILABLE SERIAL COMMANDS ===");
      Serial.println("export       - Export all configuration as JSON");
      Serial.println("status       - Show system status");
      Serial.println("help         - Show this help message");
      Serial.println("==================================\n");
    }
    else if (command == "status") {
      printSystemStatus();
    }
  }
}

// ======================================================================================
// MAIN LOOP
// ======================================================================================
void loop() {
  static unsigned long lastWatchdogFeed = 0;
  if (millis() - lastWatchdogFeed >= Config::WATCHDOG_FEED_INTERVAL) {
    feedWatchdog();
    lastWatchdogFeed = millis();
  }

  updateTelemetry();

  handleSerialCommands();

  handleButtonStates();
  handleLongPress();

  if (WiFi.status() == WL_CONNECTED) {
    server.handleClient();
    ArduinoOTA.handle();
    handleWebSocket();
  }

  handleAlarmSequence();
  handleProgressivePulsePattern();
  updateRelayFlashing();
  handleSnooze();

  if (sysState.test.quickTestActive) {
    handleQuickTest();
  }

  if (sysState.test.singleFlashActive) {
    handleSingleFlash();
  }

  updateOLEDDisplay();
  checkWiFiConnection();

  if (WiFi.status() == WL_CONNECTED && millis() - sysState.network.lastSyncTime > Config::SYNC_INTERVAL) {
    synchronizeTime();
    sysState.network.lastSyncTime = millis();
  }

  checkAndTriggerAlarms();
}
