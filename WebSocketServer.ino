// ======================================================================================
// WEBSOCKET SERVER - Pulsar Rebirth v5.4
// ======================================================================================
// Real-time bidirectional communication for Pulsar Command Center tuning interface
// Handles configuration updates, state broadcasting, and test commands
// ======================================================================================

#include <WebSocketsServer.h>
#include <ArduinoJson.h>

// WebSocket server on port 81
WebSocketsServer webSocket = WebSocketsServer(Config::WEBSOCKET_PORT);

// Forward declarations for functions in other files
void stopAllMotors();
void startPWMWarning(int motorNumber = 2);
void startProgressivePulsePattern();
void stopProgressivePulsePattern();
void setAlarmsEnabled(bool enabled);
void saveRuntimeConfig();
void loadRuntimeConfig();
String getFormattedTime();
void showSaveConfirmationAnimation();

// ======================================================================================
// WEBSOCKET INITIALIZATION
// ======================================================================================

void initWebSocket() {
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);
  
  Serial.print("🔌 WebSocket server started on port ");
  Serial.println(Config::WEBSOCKET_PORT);
  Serial.print("   Connect via: ws://");
  Serial.print(Config::OTA_HOSTNAME);
  Serial.print(".local:");
  Serial.println(Config::WEBSOCKET_PORT);
}

// ======================================================================================
// WEBSOCKET EVENT HANDLER
// ======================================================================================

void webSocketEvent(uint8_t clientNum, WStype_t type, uint8_t* payload, size_t length) {
  switch (type) {
    case WStype_DISCONNECTED:
      sysState.websocket.connectedClients--;
      if (sysState.websocket.connectedClients < 0) sysState.websocket.connectedClients = 0;
      sysState.websocket.clientConnected = (sysState.websocket.connectedClients > 0);
      
      if (Config::ENABLE_WEBSOCKET_DEBUG) {
        Serial.print("🔌 WebSocket client #");
        Serial.print(clientNum);
        Serial.print(" disconnected. Total: ");
        Serial.println(sysState.websocket.connectedClients);
      }
      break;
      
    case WStype_CONNECTED:
      {
        sysState.websocket.connectedClients++;
        sysState.websocket.clientConnected = true;
        
        IPAddress ip = webSocket.remoteIP(clientNum);
        if (Config::ENABLE_WEBSOCKET_DEBUG) {
          Serial.print("🔌 WebSocket client #");
          Serial.print(clientNum);
          Serial.print(" connected from ");
          Serial.print(ip);
          Serial.print(". Total: ");
          Serial.println(sysState.websocket.connectedClients);
        }
        
        // Send full config to newly connected client
        sendFullConfig(clientNum);
        sendAlarmState(clientNum);
        sendMotorState(clientNum);
      }
      break;
      
    case WStype_TEXT:
      {
        if (Config::ENABLE_WEBSOCKET_DEBUG) {
          Serial.print("📥 WebSocket message from #");
          Serial.print(clientNum);
          Serial.print(": ");
          Serial.println((char*)payload);
        }
        
        // Parse JSON message
        StaticJsonDocument<1024> doc;
        DeserializationError error = deserializeJson(doc, payload, length);
        
        if (error) {
          Serial.print("❌ JSON parse error: ");
          Serial.println(error.c_str());
          sendError(clientNum, "Invalid JSON");
          return;
        }
        
        // Handle message based on type
        const char* msgType = doc["type"];
        if (msgType) {
          handleWebSocketMessage(clientNum, msgType, doc);
        }
      }
      break;
      
    case WStype_PING:
      // Handled automatically
      break;
      
    case WStype_PONG:
      // Handled automatically
      break;
      
    default:
      break;
  }
}

// ======================================================================================
// MESSAGE HANDLER
// ======================================================================================

void handleWebSocketMessage(uint8_t clientNum, const char* msgType, JsonDocument& doc) {
  
  // === GET FULL CONFIGURATION ===
  if (strcmp(msgType, "getConfig") == 0) {
    sendFullConfig(clientNum);
  }
  
  // === SET SINGLE CONFIGURATION PARAMETER ===
  else if (strcmp(msgType, "setConfig") == 0) {
    const char* param = doc["param"];
    int value = doc["value"];
    
    if (param) {
      setConfigParam(param, value);
      
      // Acknowledge the change
      StaticJsonDocument<128> response;
      response["type"] = "configSaved";
      response["param"] = param;
      response["value"] = value;
      
      String output;
      serializeJson(response, output);
      webSocket.sendTXT(clientNum, output);
      
      if (Config::ENABLE_WEBSOCKET_DEBUG) {
        Serial.print("✅ Config updated: ");
        Serial.print(param);
        Serial.print(" = ");
        Serial.println(value);
      }
    }
  }
  
  // === SET PWM STEPS ARRAY ===
  else if (strcmp(msgType, "setPwmArray") == 0) {
    JsonArray values = doc["values"];
    if (values) {
      int count = min((int)values.size(), Config::MAX_PWM_STEPS);
      for (int i = 0; i < count; i++) {
        rtConfig.pwmStepsArray[i] = values[i];
      }
      rtConfig.pwmStepsArrayLength = count;
      
      // Also update the global pwmWarningSteps array used by existing code
      extern int pwmWarningSteps[];
      for (int i = 0; i < count; i++) {
        pwmWarningSteps[i] = rtConfig.pwmStepsArray[i];
      }
      
      sendAck(clientNum, "PWM array updated", count);
    }
  }
  
  // === SET MOTOR PATTERN ARRAY ===
  else if (strcmp(msgType, "setMotorPattern") == 0) {
    JsonArray values = doc["values"];
    if (values) {
      int count = min((int)values.size(), Config::MAX_MOTOR_PATTERN);
      for (int i = 0; i < count; i++) {
        rtConfig.motorPattern[i] = values[i];
      }
      rtConfig.motorPatternLength = count;
      
      sendAck(clientNum, "Motor pattern updated", count);
    }
  }
  
  // === SET ALARM ARMED STATE ===
  else if (strcmp(msgType, "setAlarmArmed") == 0) {
    bool armed = doc["armed"];
    setAlarmsEnabled(armed);
    broadcastAlarmState();
  }
  
  // === SET ALARM TIME (direct hour/minute) ===
  else if (strcmp(msgType, "setAlarmTime") == 0) {
    rtConfig.alarmStartHour = doc["hour"] | rtConfig.alarmStartHour;
    rtConfig.alarmStartMinute = doc["minute"] | rtConfig.alarmStartMinute;
    broadcastAlarmState();
  }
  
  // === SAVE ALL CONFIG TO FLASH ===
  else if (strcmp(msgType, "saveToFlash") == 0) {
    saveRuntimeConfig();
    sendAck(clientNum, "Configuration saved to flash", 0);
    Serial.println("💾 Runtime config saved to flash");
  }
  
  // === TEST PATTERNS ===
  else if (strcmp(msgType, "testPattern") == 0) {
    const char* pattern = doc["pattern"];
    if (pattern) {
      if (strcmp(pattern, "pwmWarning") == 0) {
        stopAllMotors();
        sysState.alarm.triggered = true;
        sysState.alarm.stage = AlarmStage::PWM_WARNING;
        sysState.alarm.stageStartTime = millis();
        sendAck(clientNum, "PWM Warning test started", 0);
      }
      else if (strcmp(pattern, "progressive") == 0) {
        stopAllMotors();
        startProgressivePulsePattern();
        sendAck(clientNum, "Progressive pattern test started", 0);
      }
    }
  }
  
  // === STOP ALL MOTORS ===
  else if (strcmp(msgType, "stopAll") == 0) {
    stopAllMotors();
    sysState.alarm.triggered = false;
    sysState.alarm.stage = AlarmStage::INACTIVE;
    broadcastMotorState();
    sendAck(clientNum, "All motors stopped", 0);
  }
  
  // === UNKNOWN MESSAGE TYPE ===
  else {
    Serial.print("⚠️ Unknown message type: ");
    Serial.println(msgType);
    sendError(clientNum, "Unknown message type");
  }
}

// ======================================================================================
// CONFIGURATION PARAMETER SETTER
// ======================================================================================

void setConfigParam(const char* param, int value) {
  // PWM Warning
  if (strcmp(param, "PWM_WARNING_DURATION") == 0) {
    rtConfig.pwmWarningDuration = value;
  }
  else if (strcmp(param, "PWM_WARNING_STEPS") == 0) {
    rtConfig.pwmWarningStepCount = value;
  }
  else if (strcmp(param, "WARNING_PAUSE_DURATION") == 0) {
    rtConfig.warningPauseDuration = value;
  }
  
  // Progressive Pattern
  else if (strcmp(param, "MAIN_STARTING_INTENSITY") == 0) {
    rtConfig.mainStartingIntensity = value;
  }
  else if (strcmp(param, "MAIN_INTENSITY_INCREMENT") == 0) {
    rtConfig.mainIntensityIncrement = value;
  }
  else if (strcmp(param, "MAIN_MAX_INTENSITY") == 0) {
    rtConfig.mainMaxIntensity = value;
  }
  else if (strcmp(param, "MAIN_FIRES_PER_INTENSITY") == 0) {
    rtConfig.mainFiresPerIntensity = value;
  }
  
  // Motor Pattern
  else if (strcmp(param, "MOTOR2_OFFSET_MS") == 0) {
    rtConfig.motor2OffsetMs = value;
  }
  
  // Alarm Window
  else if (strcmp(param, "ALARM_START_HOUR") == 0) {
    rtConfig.alarmStartHour = value;
  }
  else if (strcmp(param, "ALARM_START_MINUTE") == 0) {
    rtConfig.alarmStartMinute = value;
  }
  else if (strcmp(param, "ALARM_END_HOUR") == 0) {
    rtConfig.alarmEndHour = value;
  }
  else if (strcmp(param, "ALARM_END_MINUTE") == 0) {
    rtConfig.alarmEndMinute = value;
  }
  else if (strcmp(param, "ALARM_INTERVAL_MINUTES") == 0) {
    rtConfig.alarmIntervalMinutes = value;
  }
  
  // Snooze
  else if (strcmp(param, "SNOOZE_DURATION") == 0) {
    rtConfig.snoozeDuration = value;
  }
  
  // Relay Flash Timing
  else if (strcmp(param, "WARNING_FLASH_INTERVAL") == 0) {
    rtConfig.warningFlashInterval = constrain(value, 50, 1000);
  }
  else if (strcmp(param, "ALARM_FLASH_INTERVAL") == 0) {
    rtConfig.alarmFlashInterval = constrain(value, 50, 1000);
  }
  
  else {
    Serial.print("⚠️ Unknown config param: ");
    Serial.println(param);
  }
}

// ======================================================================================
// SEND FULL CONFIGURATION
// ======================================================================================

void sendFullConfig(uint8_t clientNum) {
  StaticJsonDocument<2048> doc;
  
  doc["type"] = "configAll";
  JsonObject config = doc.createNestedObject("config");
  
  // PWM Warning
  config["PWM_WARNING_DURATION"] = rtConfig.pwmWarningDuration;
  config["PWM_WARNING_STEPS"] = rtConfig.pwmWarningStepCount;
  config["WARNING_PAUSE_DURATION"] = rtConfig.warningPauseDuration;
  
  // Progressive Pattern
  config["MAIN_STARTING_INTENSITY"] = rtConfig.mainStartingIntensity;
  config["MAIN_INTENSITY_INCREMENT"] = rtConfig.mainIntensityIncrement;
  config["MAIN_MAX_INTENSITY"] = rtConfig.mainMaxIntensity;
  config["MAIN_FIRES_PER_INTENSITY"] = rtConfig.mainFiresPerIntensity;
  
  // Motor Pattern
  config["MOTOR2_OFFSET_MS"] = rtConfig.motor2OffsetMs;
  
  // Alarm Window
  config["ALARM_START_HOUR"] = rtConfig.alarmStartHour;
  config["ALARM_START_MINUTE"] = rtConfig.alarmStartMinute;
  config["ALARM_END_HOUR"] = rtConfig.alarmEndHour;
  config["ALARM_END_MINUTE"] = rtConfig.alarmEndMinute;
  config["ALARM_INTERVAL_MINUTES"] = rtConfig.alarmIntervalMinutes;
  
  // Snooze
  config["SNOOZE_DURATION"] = rtConfig.snoozeDuration;
  
  // Relay Flash Timing
  config["WARNING_FLASH_INTERVAL"] = rtConfig.warningFlashInterval;
  config["ALARM_FLASH_INTERVAL"] = rtConfig.alarmFlashInterval;
  
  // PWM Steps Array
  JsonArray pwmArray = config.createNestedArray("PWM_STEPS_ARRAY");
  for (int i = 0; i < rtConfig.pwmStepsArrayLength; i++) {
    pwmArray.add(rtConfig.pwmStepsArray[i]);
  }
  
  // Motor Pattern Array
  JsonArray motorArray = config.createNestedArray("MOTOR_PATTERN");
  for (int i = 0; i < rtConfig.motorPatternLength; i++) {
    motorArray.add(rtConfig.motorPattern[i]);
  }
  
  String output;
  serializeJson(doc, output);
  webSocket.sendTXT(clientNum, output);
  
  if (Config::ENABLE_WEBSOCKET_DEBUG) {
    Serial.println("📤 Sent full config to client");
  }
}

// ======================================================================================
// BROADCAST FUNCTIONS (send to all connected clients)
// ======================================================================================

void broadcastTime() {
  if (sysState.websocket.connectedClients == 0) return;
  
  StaticJsonDocument<128> doc;
  doc["type"] = "time";
  doc["value"] = getFormattedTime();
  
  // Add date
  struct tm timeInfo;
  if (getLocalTime(&timeInfo)) {
    char dateStr[16];
    sprintf(dateStr, "%d/%d", timeInfo.tm_mon + 1, timeInfo.tm_mday);
    doc["date"] = dateStr;
  }
  
  String output;
  serializeJson(doc, output);
  webSocket.broadcastTXT(output);
}

void broadcastMotorState() {
  if (sysState.websocket.connectedClients == 0) return;
  
  StaticJsonDocument<128> doc;
  doc["type"] = "motor";
  doc["m1"] = sysState.motor.motor1Active;
  doc["m2"] = sysState.motor.motor2Active;
  doc["intensity"] = sysState.motor.progressivePulse.currentIntensity;
  
  String output;
  serializeJson(doc, output);
  webSocket.broadcastTXT(output);
}

void broadcastAlarmState() {
  if (sysState.websocket.connectedClients == 0) return;
  
  StaticJsonDocument<256> doc;
  doc["type"] = "alarm";
  doc["armed"] = sysState.alarm.enabled;
  
  // Format alarm time
  int h = rtConfig.alarmStartHour % 12;
  if (h == 0) h = 12;
  const char* ampm = rtConfig.alarmStartHour >= 12 ? "PM" : "AM";
  
  char timeStr[16];
  sprintf(timeStr, "%d:%02d %s", h, rtConfig.alarmStartMinute, ampm);
  doc["time"] = timeStr;
  
  doc["triggered"] = sysState.alarm.triggered;
  doc["snoozing"] = sysState.alarm.snoozeActive;
  doc["stage"] = (int)sysState.alarm.stage;
  
  String output;
  serializeJson(doc, output);
  webSocket.broadcastTXT(output);
}

void sendAlarmState(uint8_t clientNum) {
  StaticJsonDocument<256> doc;
  doc["type"] = "alarm";
  doc["armed"] = sysState.alarm.enabled;
  
  int h = rtConfig.alarmStartHour % 12;
  if (h == 0) h = 12;
  const char* ampm = rtConfig.alarmStartHour >= 12 ? "PM" : "AM";
  
  char timeStr[16];
  sprintf(timeStr, "%d:%02d %s", h, rtConfig.alarmStartMinute, ampm);
  doc["time"] = timeStr;
  
  doc["triggered"] = sysState.alarm.triggered;
  doc["snoozing"] = sysState.alarm.snoozeActive;
  doc["stage"] = (int)sysState.alarm.stage;
  
  String output;
  serializeJson(doc, output);
  webSocket.sendTXT(clientNum, output);
}

void sendMotorState(uint8_t clientNum) {
  StaticJsonDocument<128> doc;
  doc["type"] = "motor";
  doc["m1"] = sysState.motor.motor1Active;
  doc["m2"] = sysState.motor.motor2Active;
  doc["intensity"] = sysState.motor.progressivePulse.currentIntensity;
  
  String output;
  serializeJson(doc, output);
  webSocket.sendTXT(clientNum, output);
}

// ======================================================================================
// HELPER FUNCTIONS
// ======================================================================================

void sendAck(uint8_t clientNum, const char* message, int value) {
  StaticJsonDocument<128> doc;
  doc["type"] = "ack";
  doc["message"] = message;
  if (value != 0) {
    doc["value"] = value;
  }
  
  String output;
  serializeJson(doc, output);
  webSocket.sendTXT(clientNum, output);
}

void sendError(uint8_t clientNum, const char* message) {
  StaticJsonDocument<128> doc;
  doc["type"] = "error";
  doc["message"] = message;
  
  String output;
  serializeJson(doc, output);
  webSocket.sendTXT(clientNum, output);
}

// ======================================================================================
// WEBSOCKET LOOP HANDLER
// ======================================================================================

void handleWebSocket() {
  webSocket.loop();
  
  // Periodic time broadcast (every second)
  static unsigned long lastTimeBroadcast = 0;
  if (sysState.websocket.connectedClients > 0 && millis() - lastTimeBroadcast >= 1000) {
    broadcastTime();
    lastTimeBroadcast = millis();
  }
}

// ======================================================================================
// RUNTIME CONFIG PERSISTENCE
// ======================================================================================

void saveRuntimeConfig() {
  Preferences tuningPrefs;
  tuningPrefs.begin(Config::TUNING_NAMESPACE, false);
  
  // PWM Warning
  tuningPrefs.putULong("pwmDuration", rtConfig.pwmWarningDuration);
  tuningPrefs.putInt("pwmSteps", rtConfig.pwmWarningStepCount);
  tuningPrefs.putULong("pauseDuration", rtConfig.warningPauseDuration);
  
  // Progressive Pattern
  tuningPrefs.putInt("startIntensity", rtConfig.mainStartingIntensity);
  tuningPrefs.putInt("intensityInc", rtConfig.mainIntensityIncrement);
  tuningPrefs.putInt("maxIntensity", rtConfig.mainMaxIntensity);
  tuningPrefs.putInt("firesPerInt", rtConfig.mainFiresPerIntensity);
  
  // Motor Pattern
  tuningPrefs.putInt("motor2Offset", rtConfig.motor2OffsetMs);
  tuningPrefs.putInt("motorPatLen", rtConfig.motorPatternLength);
  tuningPrefs.putBytes("motorPattern", rtConfig.motorPattern, rtConfig.motorPatternLength * sizeof(int));
  
  // Alarm Window
  tuningPrefs.putInt("alarmStartH", rtConfig.alarmStartHour);
  tuningPrefs.putInt("alarmStartM", rtConfig.alarmStartMinute);
  tuningPrefs.putInt("alarmEndH", rtConfig.alarmEndHour);
  tuningPrefs.putInt("alarmEndM", rtConfig.alarmEndMinute);
  tuningPrefs.putInt("alarmInterval", rtConfig.alarmIntervalMinutes);
  
  // Snooze
  tuningPrefs.putULong("snoozeDur", rtConfig.snoozeDuration);
  
  // Relay Flash Timing
  tuningPrefs.putULong("warnFlash", rtConfig.warningFlashInterval);
  tuningPrefs.putULong("alarmFlash", rtConfig.alarmFlashInterval);
  
  // PWM Steps Array
  tuningPrefs.putInt("pwmArrLen", rtConfig.pwmStepsArrayLength);
  tuningPrefs.putBytes("pwmStepsArr", rtConfig.pwmStepsArray, rtConfig.pwmStepsArrayLength * sizeof(int));
  
  tuningPrefs.end();

  // Show visual confirmation animation on OLED
  showSaveConfirmationAnimation();

  Serial.println("💾 Runtime configuration saved to flash");
}

void loadRuntimeConfig() {
  Preferences tuningPrefs;
  tuningPrefs.begin(Config::TUNING_NAMESPACE, true);  // Read-only
  
  // Check if config exists
  if (!tuningPrefs.isKey("pwmDuration")) {
    Serial.println("📁 No saved tuning config found, using defaults");
    tuningPrefs.end();
    return;
  }
  
  // PWM Warning
  rtConfig.pwmWarningDuration = tuningPrefs.getULong("pwmDuration", rtConfig.pwmWarningDuration);
  rtConfig.pwmWarningStepCount = tuningPrefs.getInt("pwmSteps", rtConfig.pwmWarningStepCount);
  rtConfig.warningPauseDuration = tuningPrefs.getULong("pauseDuration", rtConfig.warningPauseDuration);
  
  // Progressive Pattern
  rtConfig.mainStartingIntensity = tuningPrefs.getInt("startIntensity", rtConfig.mainStartingIntensity);
  rtConfig.mainIntensityIncrement = tuningPrefs.getInt("intensityInc", rtConfig.mainIntensityIncrement);
  rtConfig.mainMaxIntensity = tuningPrefs.getInt("maxIntensity", rtConfig.mainMaxIntensity);
  rtConfig.mainFiresPerIntensity = tuningPrefs.getInt("firesPerInt", rtConfig.mainFiresPerIntensity);
  
  // Motor Pattern
  rtConfig.motor2OffsetMs = tuningPrefs.getInt("motor2Offset", rtConfig.motor2OffsetMs);
  rtConfig.motorPatternLength = tuningPrefs.getInt("motorPatLen", rtConfig.motorPatternLength);
  if (rtConfig.motorPatternLength > 0 && rtConfig.motorPatternLength <= Config::MAX_MOTOR_PATTERN) {
    tuningPrefs.getBytes("motorPattern", rtConfig.motorPattern, rtConfig.motorPatternLength * sizeof(int));
  }
  
  // Alarm Window
  rtConfig.alarmStartHour = tuningPrefs.getInt("alarmStartH", rtConfig.alarmStartHour);
  rtConfig.alarmStartMinute = tuningPrefs.getInt("alarmStartM", rtConfig.alarmStartMinute);
  rtConfig.alarmEndHour = tuningPrefs.getInt("alarmEndH", rtConfig.alarmEndHour);
  rtConfig.alarmEndMinute = tuningPrefs.getInt("alarmEndM", rtConfig.alarmEndMinute);
  rtConfig.alarmIntervalMinutes = tuningPrefs.getInt("alarmInterval", rtConfig.alarmIntervalMinutes);
  
  // Snooze
  rtConfig.snoozeDuration = tuningPrefs.getULong("snoozeDur", rtConfig.snoozeDuration);
  
  // Relay Flash Timing
  rtConfig.warningFlashInterval = tuningPrefs.getULong("warnFlash", rtConfig.warningFlashInterval);
  rtConfig.alarmFlashInterval = tuningPrefs.getULong("alarmFlash", rtConfig.alarmFlashInterval);
  
  // PWM Steps Array
  rtConfig.pwmStepsArrayLength = tuningPrefs.getInt("pwmArrLen", rtConfig.pwmStepsArrayLength);
  if (rtConfig.pwmStepsArrayLength > 0 && rtConfig.pwmStepsArrayLength <= Config::MAX_PWM_STEPS) {
    tuningPrefs.getBytes("pwmStepsArr", rtConfig.pwmStepsArray, rtConfig.pwmStepsArrayLength * sizeof(int));
  }
  
  tuningPrefs.end();
  
  Serial.println("✅ Runtime configuration loaded from flash");
  Serial.print("   Alarm: ");
  Serial.print(rtConfig.alarmStartHour);
  Serial.print(":");
  Serial.print(rtConfig.alarmStartMinute);
  Serial.print(" - ");
  Serial.print(rtConfig.alarmEndHour);
  Serial.print(":");
  Serial.println(rtConfig.alarmEndMinute);
  Serial.print("   Motor2 Offset: ");
  Serial.print(rtConfig.motor2OffsetMs);
  Serial.println("ms");
}
