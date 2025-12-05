// ======================================================================================
// WEB SERVER FUNCTIONS - Pulsar Rebirth v5.6
// ======================================================================================

void sendCORSResponse(int code, const String& contentType, const String& content) {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.sendHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
  server.sendHeader("Access-Control-Allow-Headers", "Content-Type");
  server.send(code, contentType, content);
}

void setupWebServer() {
  Serial.print("Setting up web server for ");
  Serial.print(Config::SYSTEM_NAME);
  Serial.println("...");
  
  server.on("/", HTTP_GET, handleCommandCenter);
  server.on("/toggleAlarm", HTTP_GET, handleToggleAlarm);
  server.on("/toggleRelay", HTTP_GET, handleToggleMotor1);
  server.on("/testAlarm", HTTP_GET, handleTestButton);
  server.on("/turnOffMotor", HTTP_GET, handleTurnOffAllMotors);
  server.on("/quickTest", HTTP_GET, handleQuickTestWeb);
  server.on("/singleFlashTest", HTTP_GET, handleSingleFlashTest);
  server.on("/toggleMotor1", HTTP_GET, handleToggleMotor1);
  server.on("/toggleMotor2", HTTP_GET, handleToggleMotor2);
  server.on("/startProgressivePattern", HTTP_GET, handleStartProgressivePattern);
  server.on("/stopProgressivePattern", HTTP_GET, handleStopProgressivePattern);
  server.on("/testPWMWarning", HTTP_GET, handleTestPWMWarning);
  server.on("/PWMtestMotor1", HTTP_GET, handlePWMTestMotor1);
  server.on("/PWMtestMotor2", HTTP_GET, handlePWMTestMotor2);
  server.on("/motorStatus", HTTP_GET, handleMotorStatus);
  server.on("/progressiveStatus", HTTP_GET, handleProgressiveStatus);
  server.on("/currentTime", HTTP_GET, handleCurrentTime);
  server.on("/nextAlarm", HTTP_GET, handleNextAlarm);
  server.on("/networkStatus", HTTP_GET, handleNetworkStatus);
  server.on("/reconnectWiFi", HTTP_GET, handleReconnectWiFi);
  server.on("/alarmsStatus", HTTP_GET, handleAlarmsStatus);
  server.on("/telemetry", HTTP_GET, handleTelemetry);
  server.on("/systemHealth", HTTP_GET, handleSystemHealth);
  server.on("/api/relay", HTTP_GET, handleRelayControl);
  server.on("/api/config/export", HTTP_GET, handleConfigExport);
  server.on("/api/config/import", HTTP_POST, handleConfigImport);

  server.on("*", HTTP_OPTIONS, []() {
    sendCORSResponse(204, "", "");
  });

  Serial.println("Web server routes setup complete");
}

void handleToggleAlarm() {
  if (server.hasArg("state")) {
    String state = server.arg("state");
    if (state == "on") {
      setAlarmsEnabled(true);
      sendCORSResponse(200, "text/plain", "Alarms Enabled");
    } else if (state == "off") {
      setAlarmsEnabled(false);
      sendCORSResponse(200, "text/plain", "Alarms Disabled");
    } else {
      sendCORSResponse(400, "text/plain", "Invalid State");
    }
  } else {
    sendCORSResponse(400, "text/plain", "No State Provided");
  }
}

void handleTestButton() {
  Serial.println("Web test - starting stepped PWM warning");
  stopAllMotors();
  sysState.alarm.triggered = true;
  sysState.alarm.stage = AlarmStage::PWM_WARNING;
  sysState.alarm.stageStartTime = millis();
  sendCORSResponse(200, "text/plain", "Stepped PWM Warning Started");
}

void handleTurnOffAllMotors() {
  stopAllMotors();
  sendCORSResponse(200, "text/plain", "All Motors Stopped");
}

void handleToggleMotor1() {
  if (server.hasArg("state")) {
    String state = server.arg("state");
    if (state == "on") {
      activateMotor1(50);
      sendCORSResponse(200, "text/plain", "Motor 1 Activated");
    } else if (state == "off") {
      ledcWrite(Config::PWM_CHANNEL, 0);
      sysState.motor.motor1Active = false;
      sendCORSResponse(200, "text/plain", "Motor 1 Deactivated");
    } else {
      sendCORSResponse(400, "text/plain", "Invalid State");
    }
  } else {
    sendCORSResponse(400, "text/plain", "No State Provided");
  }
}

void handleToggleMotor2() {
  if (server.hasArg("state")) {
    String state = server.arg("state");
    if (state == "on") {
      activateMotor2(50);
      sendCORSResponse(200, "text/plain", "Motor 2 Activated");
    } else if (state == "off") {
      ledcWrite(Config::PWM_CHANNEL2, 0);
      sysState.motor.motor2Active = false;
      sendCORSResponse(200, "text/plain", "Motor 2 Deactivated");
    } else {
      sendCORSResponse(400, "text/plain", "Invalid State");
    }
  } else {
    sendCORSResponse(400, "text/plain", "No State Provided");
  }
}

void handleStartProgressivePattern() {
  if (!sysState.motor.progressivePulse.active) {
    startProgressivePulsePattern();
    sendCORSResponse(200, "text/plain", "Progressive Pattern Started");
  } else {
    sendCORSResponse(200, "text/plain", "Pattern Already Active");
  }
}

void handleStopProgressivePattern() {
  stopProgressivePulsePattern();
  sendCORSResponse(200, "text/plain", "Progressive Pattern Stopped");
}

void handleTestPWMWarning() {
  if (!sysState.motor.pwmWarning.active) {
    stopAllMotors();
    startPWMWarning(2);
    sendCORSResponse(200, "text/plain", "Stepped PWM Warning Started");
  } else {
    sendCORSResponse(200, "text/plain", "Warning Already Active");
  }
}

void handlePWMTestMotor1() {
  activateMotor1(50);
  sendCORSResponse(200, "text/plain", "Motor 1 Test - 50%");
}

void handlePWMTestMotor2() {
  activateMotor2(50);
  sendCORSResponse(200, "text/plain", "Motor 2 Test - 50%");
}

void handleMotorStatus() {
  String json = "{ ";
  json += "\"motor1Active\": " + String(sysState.motor.motor1Active ? "true" : "false") + ", ";
  json += "\"motor2Active\": " + String(sysState.motor.motor2Active ? "true" : "false") + ", ";
  json += "\"pwmWarningActive\": " + String(sysState.motor.pwmWarning.active ? "true" : "false") + ", ";
  json += "\"progressiveActive\": " + String(sysState.motor.progressivePulse.active ? "true" : "false") + ", ";
  json += "\"currentIntensity\": " + String(sysState.motor.progressivePulse.currentIntensity) + ", ";
  json += "\"alarmStage\": " + String((int)sysState.alarm.stage);
  
  if (sysState.motor.pwmWarning.active) {
    json += ", \"pwmIntensity\": " + String(sysState.motor.pwmWarning.currentIntensity);
    json += ", \"pwmStep\": " + String(sysState.motor.pwmWarning.currentStep);
  }
  
  json += " }";
  sendCORSResponse(200, "application/json", json);
}

void handleProgressiveStatus() {
  String json = "{ ";
  json += "\"active\": " + String(sysState.motor.progressivePulse.active ? "true" : "false") + ", ";
  json += "\"currentIntensity\": " + String(sysState.motor.progressivePulse.currentIntensity) + ", ";
  json += "\"fireCount\": " + String(sysState.motor.progressivePulse.fireCount) + ", ";
  json += "\"startingIntensity\": " + String(rtConfig.mainStartingIntensity) + ", ";
  json += "\"maxIntensity\": " + String(rtConfig.mainMaxIntensity) + ", ";
  json += "\"increment\": " + String(rtConfig.mainIntensityIncrement) + ", ";
  json += "\"motor1PatternIndex\": " + String(sysState.motor.progressivePulse.motor1PatternIndex) + ", ";
  json += "\"motor2PatternIndex\": " + String(sysState.motor.progressivePulse.motor2PatternIndex) + ", ";
  json += "\"firesUntilIntensityChange\": " + String(rtConfig.mainFiresPerIntensity - sysState.motor.progressivePulse.fireCount);
  json += " }";
  sendCORSResponse(200, "application/json", json);
}

void handleQuickTestWeb() {
  if (!sysState.test.quickTestActive) {
    sysState.test.quickTestActive = true;
    sysState.test.quickTestPhase = 0;
    sysState.test.quickTestToggleCount = 0;
    sysState.test.quickTestInterval = 20;
    sysState.test.quickTestStartTime = millis();
    sendCORSResponse(200, "text/plain", "Quick Test Started");
  } else {
    sendCORSResponse(200, "text/plain", "Quick Test Already Active");
  }
}

void handleSingleFlashTest() {
  if (!sysState.test.singleFlashActive) {
    sysState.test.singleFlashActive = true;
    sysState.test.singleFlashTime = millis();
    sendCORSResponse(200, "text/plain", "Single Flash Started");
  } else {
    sendCORSResponse(200, "text/plain", "Single Flash Already Active");
  }
}

void handleCurrentTime() {
  sendCORSResponse(200, "text/plain", getFormattedTime());
}

void handleNextAlarm() {
  String nextAlarm = getNextAlarmTime();
  String timeRemaining = getTimeRemaining();
  String json = "{ \"time\": \"" + nextAlarm + "\", \"remaining\": \"" + timeRemaining + "\" }";
  sendCORSResponse(200, "application/json", json);
}

void handleNetworkStatus() {
  String status;
  if (WiFi.status() == WL_CONNECTED) {
    status = "Connected to: " + sysState.network.activeNetwork;
  } else {
    status = "Offline Mode";
  }
  sendCORSResponse(200, "text/plain", status);
}

void handleReconnectWiFi() {
  bool success = connectToWiFi();
  if (success) {
    synchronizeTime();
    sendCORSResponse(200, "text/plain", "WiFi connected");
  } else {
    sendCORSResponse(200, "text/plain", "Failed - offline mode");
  }
}

void handleAlarmsStatus() {
  bool anyMotorActive = sysState.motor.motor1Active || sysState.motor.motor2Active || 
                        sysState.motor.pwmWarning.active || sysState.motor.progressivePulse.active;
  
  String json = "{ ";
  json += "\"enabled\": " + String(sysState.alarm.enabled ? "true" : "false") + ", ";
  json += "\"anyMotorActive\": " + String(anyMotorActive ? "true" : "false") + ", ";
  json += "\"motor1Active\": " + String(sysState.motor.motor1Active ? "true" : "false") + ", ";
  json += "\"motor2Active\": " + String(sysState.motor.motor2Active ? "true" : "false") + ", ";
  json += "\"pwmActive\": " + String(sysState.motor.pwmWarning.active ? "true" : "false") + ", ";
  json += "\"progressiveActive\": " + String(sysState.motor.progressivePulse.active ? "true" : "false") + ", ";
  json += "\"currentIntensity\": " + String(sysState.motor.progressivePulse.currentIntensity);
  json += " }";
  sendCORSResponse(200, "application/json", json);
}

void handleTelemetry() {
  String json = "{ ";
  json += "\"uptime\": " + String(sysState.telemetry.uptime) + ", ";
  json += "\"alarmCount\": " + String(sysState.telemetry.alarmCount) + ", ";
  json += "\"freeHeap\": " + String(ESP.getFreeHeap()) + ", ";
  json += "\"minFreeHeap\": " + String(sysState.telemetry.minFreeHeap) + ", ";
  json += "\"lastTimeSync\": " + String(sysState.telemetry.lastTimeSync) + ", ";
  json += "\"websocketClients\": " + String(sysState.websocket.connectedClients);
  json += " }";
  sendCORSResponse(200, "application/json", json);
}

void handleSystemHealth() {
  String json = "{ ";
  json += "\"version\": \"" + String(Config::FIRMWARE_VERSION) + "\", ";
  json += "\"uptime\": " + String(sysState.telemetry.uptime / 1000) + ", ";
  json += "\"freeHeap\": " + String(ESP.getFreeHeap()) + ", ";
  json += "\"minFreeHeap\": " + String(sysState.telemetry.minFreeHeap) + ", ";
  json += "\"wifiConnected\": " + String(WiFi.status() == WL_CONNECTED ? "true" : "false") + ", ";
  json += "\"watchdogActive\": " + String(sysState.watchdog.enabled ? "true" : "false") + ", ";
  json += "\"alarmsEnabled\": " + String(sysState.alarm.enabled ? "true" : "false") + ", ";
  json += "\"alarmCount\": " + String(sysState.telemetry.alarmCount) + ", ";
  json += "\"websocketClients\": " + String(sysState.websocket.connectedClients);
  json += " }";
  sendCORSResponse(200, "application/json", json);
}

/**
 * Handle relay manual control API endpoint
 * Supports query parameters: ?action=on, ?action=off, ?action=toggle, ?action=status
 * Only works outside alarm window (safety feature)
 */
void handleRelayControl() {
  // Check if we're in alarm window - manual relay control disabled during alarm hours
  if (isInAlarmWindow()) {
    String json = "{ \"success\": false, \"error\": \"Relay manual control disabled during alarm window\", ";
    json += "\"manualMode\": false, \"relayState\": false }";
    sendCORSResponse(403, "application/json", json);
    Serial.println("🚫 API: Relay control blocked (in alarm window)");
    return;
  }

  if (server.hasArg("action")) {
    String action = server.arg("action");

    if (action == "on") {
      // Turn relay ON via manual mode
      if (!sysState.relay.manualMode) {
        toggleRelayManualMode();
      }
      String json = "{ \"success\": true, \"action\": \"on\", \"manualMode\": true, \"relayState\": true }";
      sendCORSResponse(200, "application/json", json);
      Serial.println("💡 API: Relay turned ON");

    } else if (action == "off") {
      // Turn relay OFF
      if (sysState.relay.manualMode) {
        toggleRelayManualMode();
      }
      String json = "{ \"success\": true, \"action\": \"off\", \"manualMode\": false, \"relayState\": false }";
      sendCORSResponse(200, "application/json", json);
      Serial.println("🔌 API: Relay turned OFF");

    } else if (action == "toggle") {
      // Toggle relay state
      toggleRelayManualMode();
      bool isOn = sysState.relay.manualMode;
      String json = "{ \"success\": true, \"action\": \"toggle\", \"manualMode\": " + String(isOn ? "true" : "false");
      json += ", \"relayState\": " + String(isOn ? "true" : "false") + " }";
      sendCORSResponse(200, "application/json", json);
      Serial.print("🔄 API: Relay toggled to ");
      Serial.println(isOn ? "ON" : "OFF");

    } else if (action == "status") {
      // Get current relay status
      bool isOn = sysState.relay.manualMode && sysState.relay.active;
      String json = "{ \"success\": true, \"manualMode\": " + String(sysState.relay.manualMode ? "true" : "false");
      json += ", \"relayState\": " + String(isOn ? "true" : "false");
      json += ", \"inAlarmWindow\": false }";
      sendCORSResponse(200, "application/json", json);

    } else {
      String json = "{ \"success\": false, \"error\": \"Invalid action. Use: on, off, toggle, or status\" }";
      sendCORSResponse(400, "application/json", json);
    }
  } else {
    String json = "{ \"success\": false, \"error\": \"No action provided. Use: ?action=on|off|toggle|status\" }";
    sendCORSResponse(400, "application/json", json);
  }
}

/**
 * Handle configuration export
 * Returns JSON file for download
 */
void handleConfigExport() {
  String json = generateConfigJSON();

  // Set headers for file download
  server.sendHeader("Content-Disposition", "attachment; filename=pulsar_config_backup.json");
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "application/json", json);

  Serial.println("📤 Configuration exported via HTTP");
}

/**
 * Handle configuration import
 * Accepts JSON POST and applies settings
 */
void handleConfigImport() {
  if (server.hasArg("plain")) {
    String jsonBody = server.arg("plain");

    Serial.println("📥 Received configuration import request");

    bool success = importConfigFromJSON(jsonBody);

    if (success) {
      String response = "{\"success\": true, \"message\": \"Configuration imported and saved successfully\"}";
      sendCORSResponse(200, "application/json", response);

      // Broadcast updated config to all WebSocket clients
      broadcastAlarmState();
      broadcastMotorState();
    } else {
      String response = "{\"success\": false, \"message\": \"Failed to import configuration - invalid JSON format\"}";
      sendCORSResponse(400, "application/json", response);
    }
  } else {
    String response = "{\"success\": false, \"message\": \"No JSON data received\"}";
    sendCORSResponse(400, "application/json", response);
  }
}

/**
 * PULSAR COMMAND CENTER - Professional Interface v5.6
 */
void handleCommandCenter() {
  String html = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Pulsar Command Center</title>
    <style>
        * {
            margin: 0;
            padding: 0;
            box-sizing: border-box;
        }
        
        body {
            font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
            background: #1a1a1a;
            color: #00d4ff;
            overflow-x: hidden;
            height: 100vh;
        }
        
        /* ==================== TOP BAR ==================== */
        .top-bar {
            position: relative;
            background: linear-gradient(135deg, #252525, #1f1f1f);
            border-bottom: 2px solid #00d4ff;
            padding: 12px 20px;
            display: flex;
            justify-content: space-between;
            align-items: center;
        }
        
        /* Intensity glow bar at bottom of top bar */
        .intensity-glow {
            position: absolute;
            bottom: 0;
            left: 0;
            height: 3px;
            width: 0%;
            background: linear-gradient(90deg, #ffff00, #ffcc00);
            box-shadow: 0 0 10px #ffff00, 0 0 20px #ffff00;
            transition: width 0.3s ease;
        }
        
        .logo {
            font-size: 1.3rem;
            font-weight: bold;
            color: #00d4ff;
            text-shadow: 0 0 10px #00d4ff;
            letter-spacing: 2px;
        }
        
        /* Motor pill indicators */
        .motor-pills {
            position: absolute;
            left: 50%;
            top: 50%;
            transform: translate(-50%, -50%);
            display: flex;
            gap: 15px;
        }
        
        .motor-pill {
            width: 60px;
            height: 24px;
            border: 2px solid #555;
            border-radius: 12px;
            background: transparent;
            transition: all 0.3s ease;
        }
        
        .motor-pill.active {
            background: #ffff00;
            border-color: #ffff00;
            box-shadow: 0 0 15px #ffff00, inset 0 0 10px #ffcc00;
        }
        
        .connection-badge {
            display: flex;
            align-items: center;
            gap: 8px;
            padding: 6px 15px;
            background: #252525;
            border: 1px solid #00d4ff;
            border-radius: 20px;
            font-size: 0.8rem;
        }
        
        .status-dot {
            width: 8px;
            height: 8px;
            border-radius: 50%;
            animation: pulse 1.5s infinite;
        }
        
        .connected .status-dot {
            background: #00ff00;
        }
        
        .disconnected .status-dot {
            background: #ff0066;
            animation: none;
        }
        
        @keyframes pulse {
            0%, 100% { opacity: 1; }
            50% { opacity: 0.3; }
        }
        
        /* ==================== MAIN CONTAINER ==================== */
        .container {
            display: grid;
            grid-template-columns: 280px 1fr 320px;
            height: calc(100vh - 60px);
            gap: 0;
        }
        
        .panel {
            padding: 20px;
            overflow-y: auto;
            overflow-x: hidden;
            background: #1f1f1f;
        }
        
        .panel::-webkit-scrollbar {
            width: 6px;
        }
        
        .panel::-webkit-scrollbar-track {
            background: #1a1a1a;
        }
        
        .panel::-webkit-scrollbar-thumb {
            background: #00d4ff;
            border-radius: 3px;
        }
        
        /* ==================== LEFT PANEL ==================== */
        .left-panel {
            border-right: 1px solid #00d4ff33;
            padding: 0;
            overflow: visible;
        }
        
        /* STICKY CLOCK - Never scrolls */
        .clock-container {
            position: sticky;
            top: 0;
            z-index: 100;
            background: #1f1f1f;
            padding: 20px;
        }
        
        .clock-display {
            text-align: center;
            margin-bottom: 20px;
            padding: 25px 15px;
            background: linear-gradient(135deg, #252525, #1a1a1a);
            border: 2px solid #00d4ff;
            border-radius: 12px;
            box-shadow: 0 0 20px rgba(0, 212, 255, 0.3);
            animation: clockPulse 3s ease-in-out infinite;
        }
        
        @keyframes clockPulse {
            0%, 100% {
                box-shadow: 0 0 20px rgba(0, 212, 255, 0.3);
                border-color: #00d4ff;
            }
            50% {
                box-shadow: 0 0 35px rgba(0, 212, 255, 0.6);
                border-color: #00ffff;
            }
        }
        
        .clock-time {
            font-size: 3.5rem;
            font-weight: bold;
            color: #00d4ff;
            text-shadow: 0 0 30px rgba(0, 212, 255, 0.8), 0 0 50px rgba(0, 212, 255, 0.4);
            letter-spacing: 0.05em;
            line-height: 1;
            animation: clockGlow 2s ease-in-out infinite;
        }
        
        @keyframes clockGlow {
            0%, 100% {
                text-shadow: 0 0 30px rgba(0, 212, 255, 0.8), 0 0 50px rgba(0, 212, 255, 0.4);
            }
            50% {
                text-shadow: 0 0 40px rgba(0, 212, 255, 1), 0 0 70px rgba(0, 212, 255, 0.6);
            }
        }
        
        .clock-ampm {
            font-size: 1.3rem;
            color: #ffff00;
            margin-top: 5px;
        }
        
        .clock-date {
            font-size: 0.85rem;
            color: #00ff00;
            margin-top: 8px;
            letter-spacing: 0.1em;
        }
        
        /* Scrollable content area */
        .left-content {
            padding: 0 20px 20px 20px;
        }
        
        /* Sections */
        .section {
            background: #252525;
            border-left: 3px solid #00d4ff;
            padding: 12px;
            margin-bottom: 15px;
            border-radius: 4px;
        }
        
        .section-title {
            font-size: 0.7rem;
            color: #ffff00;
            margin-bottom: 6px;
            text-transform: uppercase;
            letter-spacing: 0.1em;
        }
        
        .alarm-time {
            font-size: 1.4rem;
            color: #00d4ff;
            margin: 4px 0;
        }
        
        /* Buttons */
        .btn {
            width: 100%;
            padding: 10px;
            margin-bottom: 8px;
            border: none;
            border-radius: 6px;
            font-weight: bold;
            font-size: 0.85rem;
            cursor: pointer;
            transition: all 0.3s;
            text-transform: uppercase;
            letter-spacing: 0.05em;
        }
        
        .btn-armed {
            background: #00ff00;
            color: #000;
        }
        
        .btn-armed.disarmed {
            background: #555;
            color: #999;
        }
        
        .btn-save {
            background: linear-gradient(135deg, #00d4ff, #0099cc);
            color: #000;
        }
        
        .btn-test {
            background: #252525;
            color: #ffff00;
            border: 2px solid #ffff00;
        }
        
        .btn-emergency {
            background: #ff0066;
            color: #fff;
        }

        .btn-lightswitch {
            background: #252525;
            color: #ffff00;
            border: 2px solid #ffff00;
        }

        .btn-lightswitch.on {
            background: #ffff00;
            color: #000;
            box-shadow: 0 0 20px rgba(255, 255, 0, 0.6);
        }

        .btn:hover {
            transform: translateY(-2px);
            box-shadow: 0 4px 15px rgba(0, 212, 255, 0.5);
        }
        
        /* ==================== CENTER PANEL ==================== */
        .center-panel {
            border-right: 1px solid #00d4ff33;
            padding: 15px 20px;
        }
        
        .tuning-header {
            text-align: center;
            margin-bottom: 20px;
            padding-bottom: 12px;
            border-bottom: 2px solid #00d4ff;
        }
        
        .tuning-header h2 {
            font-size: 1.2rem;
            color: #00d4ff;
            text-transform: uppercase;
            letter-spacing: 0.15em;
        }
        
        .param-section {
            margin-bottom: 18px;
        }
        
        .param-section-title {
            font-size: 0.8rem;
            color: #ffff00;
            margin-bottom: 10px;
            text-transform: uppercase;
            letter-spacing: 0.1em;
            padding-left: 8px;
            border-left: 3px solid #ffff00;
        }
        
        .param-grid {
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(240px, 1fr));
            gap: 10px 12px;
        }
        
        .param-row {
            display: flex;
            align-items: center;
            gap: 6px;
            padding: 6px 8px;
            background: #252525;
            border-radius: 4px;
            transition: all 0.3s;
            min-width: 0;
        }
        
        .param-row.changed {
            background: #ffff00;
            color: #000;
            border: 2px solid #ffcc00;
        }
        
        .param-row.changed label,
        .param-row.changed .unit {
            color: #000;
        }
        
        .param-row.changed input {
            background: #fff;
            color: #000;
            border-color: #ffcc00;
        }
        
        .param-row label {
            flex: 0 0 auto;
            font-size: 0.75rem;
            color: #00d4ff;
            min-width: 110px;
            max-width: 140px;
            white-space: nowrap;
            overflow: hidden;
            text-overflow: ellipsis;
        }
        
        .param-row input {
            flex: 0 0 auto;
            width: 80px;
            max-width: 80px;
            padding: 5px 6px;
            background: #1a1a1a;
            border: 1px solid #00d4ff66;
            border-radius: 3px;
            color: #00d4ff;
            font-family: 'Segoe UI', sans-serif;
            font-size: 0.85rem;
            font-weight: bold;
            text-align: right;
        }
        
        .param-row input:focus {
            outline: none;
            border-color: #00d4ff;
            box-shadow: 0 0 8px rgba(0, 212, 255, 0.3);
        }
        
        .param-row .unit {
            flex: 0 0 auto;
            font-size: 0.7rem;
            color: #00ff00;
            min-width: 25px;
        }
        
        .param-row .was-value {
            flex: 0 1 auto;
            font-size: 0.65rem;
            color: #999;
            margin-left: 4px;
            overflow: hidden;
            text-overflow: ellipsis;
            white-space: nowrap;
        }
        
        .param-row.changed .was-value {
            color: #000;
            font-weight: bold;
        }
        
        .alarm-time-inputs {
            display: flex;
            align-items: center;
            gap: 6px;
        }
        
        .alarm-time-inputs input {
            width: 50px;
            max-width: 50px;
        }
        
        .alarm-time-inputs .separator {
            font-size: 1.2rem;
            color: #00d4ff;
            font-weight: bold;
        }
        
        /* ==================== RIGHT PANEL ==================== */
        .right-panel {
            padding: 15px;
        }
        
        .right-panel h2 {
            font-size: 1rem;
            color: #00d4ff;
            margin-bottom: 15px;
            text-align: center;
            text-transform: uppercase;
            letter-spacing: 0.1em;
        }
        
        .array-editor {
            background: #252525;
            border: 2px solid #ff00ff;
            border-radius: 8px;
            padding: 12px;
            margin-bottom: 18px;
        }
        
        .array-editor h3 {
            font-size: 0.8rem;
            color: #ff00ff;
            margin-bottom: 8px;
            text-transform: uppercase;
            letter-spacing: 0.1em;
        }
        
        .array-editor textarea {
            width: 100%;
            height: 120px;
            padding: 8px;
            background: #1a1a1a;
            border: 1px solid #ff00ff66;
            border-radius: 4px;
            color: #00d4ff;
            font-family: 'Courier New', monospace;
            font-size: 0.8rem;
            resize: vertical;
            line-height: 1.5;
        }
        
        .array-editor textarea:focus {
            outline: none;
            border-color: #ff00ff;
            box-shadow: 0 0 8px rgba(255, 0, 255, 0.3);
        }
        
        .array-info {
            margin: 8px 0;
            font-size: 0.75rem;
            color: #00ff00;
        }
        
        .btn-apply {
            width: 100%;
            padding: 8px;
            background: #ff00ff;
            color: #000;
            border: none;
            border-radius: 4px;
            font-weight: bold;
            font-size: 0.8rem;
            cursor: pointer;
            transition: all 0.3s;
        }
        
        .btn-apply:hover {
            transform: translateY(-2px);
            box-shadow: 0 4px 15px rgba(255, 0, 255, 0.5);
        }
    </style>
</head>
<body>
    <!-- TOP BAR -->
    <div class="top-bar">
        <div class="logo">PULSAR COMMAND CENTER</div>
        
        <!-- Motor pill indicators (unlabeled) -->
        <div class="motor-pills">
            <div class="motor-pill" id="motorPill1"></div>
            <div class="motor-pill" id="motorPill2"></div>
        </div>
        
        <div class="connection-badge" id="connectionBadge">
            <span class="status-dot"></span>
            <span id="statusText">Connecting...</span>
        </div>
        
        <!-- Intensity glow bar -->
        <div class="intensity-glow" id="intensityGlow"></div>
    </div>

    <!-- MAIN CONTAINER -->
    <div class="container">
        <!-- ==================== LEFT PANEL ==================== -->
        <div class="panel left-panel">
            <!-- STICKY CLOCK -->
            <div class="clock-container">
                <div class="clock-display">
                    <div class="clock-time" id="clockTime">--:--</div>
                    <div class="clock-ampm" id="clockAmpm">--</div>
                    <div class="clock-date" id="clockDate">Loading...</div>
                </div>
            </div>
            
            <!-- SCROLLABLE CONTENT -->
            <div class="left-content">
                <!-- ALARM STATUS -->
                <div class="section">
                    <div class="section-title">Next Alarm</div>
                    <div class="alarm-time" id="alarmTimeDisplay">--:-- --</div>
                    <button class="btn btn-armed" id="alarmArmedBtn" onclick="toggleAlarmArmed()">
                        ARMED
                    </button>
                </div>
                
                <!-- ACTION BUTTONS -->
                <button class="btn btn-save" onclick="saveToDevice()">SAVE TO DEVICE</button>
                <button class="btn btn-save" onclick="exportConfig()">📥 EXPORT CONFIG</button>
                <button class="btn btn-save" onclick="document.getElementById('importFile').click()">📤 IMPORT CONFIG</button>
                <input type="file" id="importFile" accept=".json" style="display:none" onchange="importConfig(this)" />
                <button class="btn btn-lightswitch" id="lightswitchBtn" onclick="toggleLightswitch()">
                    💡 LAMP OFF
                </button>
                <button class="btn btn-test" onclick="testPattern('pwmWarning')">TEST PWM WARNING</button>
                <button class="btn btn-test" onclick="testPattern('progressive')">TEST PROGRESSIVE</button>
                <button class="btn btn-emergency" onclick="emergencyStop()">EMERGENCY STOP</button>
            </div>
        </div>
        
        <!-- ==================== CENTER PANEL ==================== -->
        <div class="panel center-panel">
            <div class="tuning-header">
                <h2>Tuning Console</h2>
            </div>
            
            <!-- PWM Warning -->
            <div class="param-section">
                <div class="param-section-title">PWM Warning</div>
                <div class="param-grid">
                    <div class="param-row" id="row-PWM_WARNING_DURATION">
                        <label>Duration:</label>
                        <input type="number" id="PWM_WARNING_DURATION" min="0" max="999999" value="6000" 
                               onchange="markChanged('PWM_WARNING_DURATION')" />
                        <span class="unit">ms</span>
                        <span class="was-value" id="was-PWM_WARNING_DURATION"></span>
                    </div>
                    <div class="param-row" id="row-PWM_WARNING_STEPS">
                        <label>Steps:</label>
                        <input type="number" id="PWM_WARNING_STEPS" min="1" max="100" value="31" 
                               onchange="markChanged('PWM_WARNING_STEPS')" />
                        <span class="unit"></span>
                        <span class="was-value" id="was-PWM_WARNING_STEPS"></span>
                    </div>
                    <div class="param-row" id="row-WARNING_PAUSE_DURATION">
                        <label>Pause Duration:</label>
                        <input type="number" id="WARNING_PAUSE_DURATION" min="0" max="999999" value="3000" 
                               onchange="markChanged('WARNING_PAUSE_DURATION')" />
                        <span class="unit">ms</span>
                        <span class="was-value" id="was-WARNING_PAUSE_DURATION"></span>
                    </div>
                </div>
            </div>
            
            <!-- Progressive Pattern -->
            <div class="param-section">
                <div class="param-section-title">Progressive Pattern</div>
                <div class="param-grid">
                    <div class="param-row" id="row-MAIN_STARTING_INTENSITY">
                        <label>Starting Intensity:</label>
                        <input type="number" id="MAIN_STARTING_INTENSITY" min="0" max="100" value="8" 
                               onchange="markChanged('MAIN_STARTING_INTENSITY')" />
                        <span class="unit">%</span>
                        <span class="was-value" id="was-MAIN_STARTING_INTENSITY"></span>
                    </div>
                    <div class="param-row" id="row-MAIN_INTENSITY_INCREMENT">
                        <label>Increment:</label>
                        <input type="number" id="MAIN_INTENSITY_INCREMENT" min="0" max="100" value="2" 
                               onchange="markChanged('MAIN_INTENSITY_INCREMENT')" />
                        <span class="unit">%</span>
                        <span class="was-value" id="was-MAIN_INTENSITY_INCREMENT"></span>
                    </div>
                    <div class="param-row" id="row-MAIN_MAX_INTENSITY">
                        <label>Max Intensity:</label>
                        <input type="number" id="MAIN_MAX_INTENSITY" min="0" max="100" value="100" 
                               onchange="markChanged('MAIN_MAX_INTENSITY')" />
                        <span class="unit">%</span>
                        <span class="was-value" id="was-MAIN_MAX_INTENSITY"></span>
                    </div>
                    <div class="param-row" id="row-MAIN_FIRES_PER_INTENSITY">
                        <label>Fires Per Intensity:</label>
                        <input type="number" id="MAIN_FIRES_PER_INTENSITY" min="1" max="999" value="1" 
                               onchange="markChanged('MAIN_FIRES_PER_INTENSITY')" />
                        <span class="unit"></span>
                        <span class="was-value" id="was-MAIN_FIRES_PER_INTENSITY"></span>
                    </div>
                </div>
            </div>
            
            <!-- Motor Pattern -->
            <div class="param-section">
                <div class="param-section-title">Motor Pattern</div>
                <div class="param-grid">
                    <div class="param-row" id="row-MOTOR2_OFFSET_MS">
                        <label>Motor 2 Offset:</label>
                        <input type="number" id="MOTOR2_OFFSET_MS" min="0" max="999999" value="37" 
                               onchange="markChanged('MOTOR2_OFFSET_MS')" />
                        <span class="unit">ms</span>
                        <span class="was-value" id="was-MOTOR2_OFFSET_MS"></span>
                    </div>
                </div>
            </div>
            
            <!-- Relay Control -->
            <div class="param-section">
                <div class="param-section-title">Relay Control</div>
                <div class="param-grid">
                    <div class="param-row" id="row-WARNING_FLASH_INTERVAL">
                        <label>Warning Flash:</label>
                        <input type="number" id="WARNING_FLASH_INTERVAL" min="50" max="1000" value="250" 
                               onchange="markChanged('WARNING_FLASH_INTERVAL')" />
                        <span class="unit">ms</span>
                        <span class="was-value" id="was-WARNING_FLASH_INTERVAL"></span>
                    </div>
                    <div class="param-row" id="row-ALARM_FLASH_INTERVAL">
                        <label>Alarm Flash:</label>
                        <input type="number" id="ALARM_FLASH_INTERVAL" min="50" max="1000" value="250" 
                               onchange="markChanged('ALARM_FLASH_INTERVAL')" />
                        <span class="unit">ms</span>
                        <span class="was-value" id="was-ALARM_FLASH_INTERVAL"></span>
                    </div>
                </div>
            </div>
            
            <!-- Alarm Window -->
            <div class="param-section">
                <div class="param-section-title">Alarm Window</div>
                <div class="param-grid">
                    <div class="param-row" id="row-ALARM_START_TIME">
                        <label>Start Time:</label>
                        <div class="alarm-time-inputs">
                            <input type="number" id="ALARM_START_HOUR" min="0" max="23" value="21" 
                                   onchange="markChanged('ALARM_START_HOUR')" />
                            <span class="separator">:</span>
                            <input type="number" id="ALARM_START_MINUTE" min="0" max="59" value="30" 
                                   onchange="markChanged('ALARM_START_MINUTE')" />
                        </div>
                        <span class="was-value" id="was-ALARM_START_TIME"></span>
                    </div>
                    <div class="param-row" id="row-ALARM_END_TIME">
                        <label>End Time:</label>
                        <div class="alarm-time-inputs">
                            <input type="number" id="ALARM_END_HOUR" min="0" max="23" value="23" 
                                   onchange="markChanged('ALARM_END_HOUR')" />
                            <span class="separator">:</span>
                            <input type="number" id="ALARM_END_MINUTE" min="0" max="59" value="0" 
                                   onchange="markChanged('ALARM_END_MINUTE')" />
                        </div>
                        <span class="was-value" id="was-ALARM_END_TIME"></span>
                    </div>
                    <div class="param-row" id="row-ALARM_INTERVAL_MINUTES">
                        <label>Interval:</label>
                        <input type="number" id="ALARM_INTERVAL_MINUTES" min="1" max="999" value="4" 
                               onchange="markChanged('ALARM_INTERVAL_MINUTES')" />
                        <span class="unit">min</span>
                        <span class="was-value" id="was-ALARM_INTERVAL_MINUTES"></span>
                    </div>
                </div>
            </div>
            
            <!-- Snooze -->
            <div class="param-section">
                <div class="param-section-title">Snooze</div>
                <div class="param-grid">
                    <div class="param-row" id="row-SNOOZE_DURATION">
                        <label>Duration:</label>
                        <input type="number" id="SNOOZE_DURATION" min="0" max="999999" value="60000" 
                               onchange="markChanged('SNOOZE_DURATION')" />
                        <span class="unit">ms</span>
                        <span class="was-value" id="was-SNOOZE_DURATION"></span>
                    </div>
                </div>
            </div>
        </div>
        
        <!-- ==================== RIGHT PANEL ==================== -->
        <div class="panel right-panel">
            <h2>Array Editors</h2>
            
            <!-- PWM Steps Array -->
            <div class="array-editor">
                <h3>PWM Warning Steps</h3>
                <textarea id="pwmStepsArray" placeholder="0, 5, 7, 0, 5, 6, 0, 5, 7..."></textarea>
                <div class="array-info">
                    <span id="pwmStepCount">0 steps</span>
                </div>
                <button class="btn-apply" onclick="applyPwmArray()">Apply to Device</button>
            </div>
            
            <!-- Motor Pattern Array -->
            <div class="array-editor">
                <h3>Motor Pattern (ON/OFF ms)</h3>
                <textarea id="motorPattern" placeholder="100, 100, 100, 100, 200, 200..."></textarea>
                <div class="array-info">
                    <span id="motorPatternCount">0 timing values</span>
                </div>
                <button class="btn-apply" onclick="applyMotorPattern()">Apply to Device</button>
            </div>
        </div>
    </div>

    <script>
        let ws;
        let connected = false;
        const originalValues = {};
        const changedParams = new Set();
        
        function connectWebSocket() {
            ws = new WebSocket('ws://' + window.location.hostname + ':81');
            
            ws.onopen = () => {
                connected = true;
                updateConnectionStatus(true);
                ws.send(JSON.stringify({ type: 'getConfig' }));
            };
            
            ws.onmessage = (event) => {
                const data = JSON.parse(event.data);
                handleWebSocketMessage(data);
            };
            
            ws.onerror = () => {
                connected = false;
                updateConnectionStatus(false);
            };
            
            ws.onclose = () => {
                connected = false;
                updateConnectionStatus(false);
                setTimeout(connectWebSocket, 3000);
            };
        }
        
        function handleWebSocketMessage(data) {
            switch (data.type) {
                case 'configAll':
                    loadConfig(data.config);
                    break;
                case 'alarm':
                    updateAlarmStatus(data);
                    break;
                case 'motor':
                    updateMotorState(data);
                    break;
            }
        }
        
        function updateConnectionStatus(isConnected) {
            const badge = document.getElementById('connectionBadge');
            const text = document.getElementById('statusText');
            
            if (isConnected) {
                badge.classList.add('connected');
                badge.classList.remove('disconnected');
                text.textContent = 'Connected to Pulsar';
            } else {
                badge.classList.remove('connected');
                badge.classList.add('disconnected');
                text.textContent = 'Disconnected';
            }
        }
        
        function loadConfig(config) {
            for (const key in config) {
                if (key !== 'PWM_STEPS_ARRAY' && key !== 'MOTOR_PATTERN') {
                    const el = document.getElementById(key);
                    if (el) {
                        originalValues[key] = config[key];
                        el.value = config[key];
                    }
                }
            }
            
            if (config.PWM_STEPS_ARRAY) {
                document.getElementById('pwmStepsArray').value = config.PWM_STEPS_ARRAY.join(', ');
                updatePwmStepCount();
            }
            
            if (config.MOTOR_PATTERN) {
                document.getElementById('motorPattern').value = config.MOTOR_PATTERN.join(', ');
                updateMotorPatternCount();
            }
            
            updateAlarmTimeDisplay();
        }
        
        function markChanged(paramId) {
            const el = document.getElementById(paramId);
            const row = document.getElementById('row-' + paramId);
            const wasEl = document.getElementById('was-' + paramId);
            
            if (!el || !row) return;
            
            const currentValue = parseInt(el.value);
            const originalValue = originalValues[paramId];
            
            if (currentValue !== originalValue) {
                row.classList.add('changed');
                changedParams.add(paramId);
                if (wasEl) wasEl.textContent = `was: ${originalValue}`;
            } else {
                row.classList.remove('changed');
                changedParams.delete(paramId);
                if (wasEl) wasEl.textContent = '';
            }
            
            if (paramId.includes('ALARM_START') || paramId.includes('ALARM_END')) {
                updateAlarmTimeDisplay();
            }
        }
        
        function updateAlarmTimeDisplay() {
            const hour = parseInt(document.getElementById('ALARM_START_HOUR').value) || 0;
            const minute = parseInt(document.getElementById('ALARM_START_MINUTE').value) || 0;
            
            const h = hour % 12 || 12;
            const ampm = hour >= 12 ? 'PM' : 'AM';
            const timeStr = `${h}:${String(minute).padStart(2, '0')} ${ampm}`;
            
            document.getElementById('alarmTimeDisplay').textContent = timeStr;
        }
        
        function updateAlarmStatus(data) {
            const btn = document.getElementById('alarmArmedBtn');
            if (data.armed) {
                btn.textContent = 'ARMED';
                btn.classList.remove('disarmed');
            } else {
                btn.textContent = 'DISARMED';
                btn.classList.add('disarmed');
            }
            
            if (data.time) {
                document.getElementById('alarmTimeDisplay').textContent = data.time;
            }
        }
        
        function updateMotorState(data) {
            const pill1 = document.getElementById('motorPill1');
            const pill2 = document.getElementById('motorPill2');
            const glow = document.getElementById('intensityGlow');
            
            // Update motor pills
            pill1.classList.toggle('active', data.m1);
            pill2.classList.toggle('active', data.m2);
            
            // Update intensity glow bar
            const intensity = data.intensity || 0;
            glow.style.width = intensity + '%';
        }
        
        function toggleAlarmArmed() {
            const btn = document.getElementById('alarmArmedBtn');
            const currentlyArmed = !btn.classList.contains('disarmed');
            ws.send(JSON.stringify({ type: 'setAlarmArmed', armed: !currentlyArmed }));
        }
        
        function saveToDevice() {
            changedParams.forEach(paramId => {
                const el = document.getElementById(paramId);
                if (el) {
                    ws.send(JSON.stringify({
                        type: 'setConfig',
                        param: paramId,
                        value: parseInt(el.value)
                    }));
                }
            });
            
            ws.send(JSON.stringify({ type: 'saveToFlash' }));
            
            changedParams.forEach(paramId => {
                const row = document.getElementById('row-' + paramId);
                const wasEl = document.getElementById('was-' + paramId);
                if (row) row.classList.remove('changed');
                if (wasEl) wasEl.textContent = '';
                
                const el = document.getElementById(paramId);
                if (el) originalValues[paramId] = parseInt(el.value);
            });
            
            changedParams.clear();
        }
        
        function testPattern(pattern) {
            ws.send(JSON.stringify({ type: 'testPattern', pattern: pattern }));
        }
        
        function emergencyStop() {
            ws.send(JSON.stringify({ type: 'stopAll' }));
        }

        function toggleLightswitch() {
            fetch('/api/relay?action=toggle')
                .then(response => response.json())
                .then(data => {
                    updateLightswitchButton(data.relayState);
                })
                .catch(error => {
                    console.error('Error toggling relay:', error);
                });
        }

        function updateLightswitchButton(isOn) {
            const btn = document.getElementById('lightswitchBtn');
            if (isOn) {
                btn.classList.add('on');
                btn.textContent = '💡 LAMP ON';
            } else {
                btn.classList.remove('on');
                btn.textContent = '💡 LAMP OFF';
            }
        }

        function checkRelayStatus() {
            fetch('/api/relay?action=status')
                .then(response => response.json())
                .then(data => {
                    if (data.success) {
                        updateLightswitchButton(data.relayState);
                    }
                })
                .catch(error => {
                    console.error('Error checking relay status:', error);
                });
        }

        function updatePwmStepCount() {
            const text = document.getElementById('pwmStepsArray').value;
            const values = parseArrayInput(text);
            document.getElementById('pwmStepCount').textContent = values.length + ' steps';
        }
        
        function applyPwmArray() {
            const text = document.getElementById('pwmStepsArray').value;
            const values = parseArrayInput(text);
            if (values.length === 0) return alert('Please enter at least one value');
            ws.send(JSON.stringify({ type: 'setPwmArray', values: values }));
        }
        
        function updateMotorPatternCount() {
            const text = document.getElementById('motorPattern').value;
            const values = parseArrayInput(text);
            document.getElementById('motorPatternCount').textContent = values.length + ' timing values';
        }
        
        function applyMotorPattern() {
            const text = document.getElementById('motorPattern').value;
            const values = parseArrayInput(text);
            if (values.length === 0) return alert('Please enter at least one value');
            ws.send(JSON.stringify({ type: 'setMotorPattern', values: values }));
        }
        
        function parseArrayInput(text) {
            return text.split(',')
                .map(s => s.trim())
                .filter(s => s !== '')
                .map(s => parseInt(s, 10))
                .filter(n => !isNaN(n));
        }
        
        document.getElementById('pwmStepsArray').addEventListener('input', updatePwmStepCount);
        document.getElementById('motorPattern').addEventListener('input', updateMotorPatternCount);
        
        function updateClock() {
            const now = new Date();
            let hours = now.getHours();
            const minutes = now.getMinutes();
            const ampm = hours >= 12 ? 'PM' : 'AM';
            hours = hours % 12 || 12;
            
            const timeStr = `${hours}:${String(minutes).padStart(2, '0')}`;
            const options = { weekday: 'long', month: 'short', day: 'numeric' };
            const dateStr = now.toLocaleDateString('en-US', options);
            
            document.getElementById('clockTime').textContent = timeStr;
            document.getElementById('clockAmpm').textContent = ampm;
            document.getElementById('clockDate').textContent = dateStr;
        }
        
        function exportConfig() {
            fetch('/api/config/export')
                .then(response => response.blob())
                .then(blob => {
                    const url = window.URL.createObjectURL(blob);
                    const a = document.createElement('a');
                    a.style.display = 'none';
                    a.href = url;

                    // Generate filename with timestamp
                    const now = new Date();
                    const timestamp = now.toISOString().replace(/[:.]/g, '-').slice(0, -5);
                    a.download = `pulsar_config_${timestamp}.json`;

                    document.body.appendChild(a);
                    a.click();
                    window.URL.revokeObjectURL(url);
                    document.body.removeChild(a);

                    console.log('Configuration exported successfully');
                })
                .catch(error => {
                    console.error('Error exporting configuration:', error);
                    alert('Failed to export configuration');
                });
        }

        function importConfig(input) {
            const file = input.files[0];
            if (!file) {
                return;
            }

            const reader = new FileReader();
            reader.onload = function(e) {
                const jsonContent = e.target.result;

                // Confirm before importing
                if (!confirm('This will overwrite your current configuration. Continue?')) {
                    input.value = ''; // Reset file input
                    return;
                }

                fetch('/api/config/import', {
                    method: 'POST',
                    headers: {
                        'Content-Type': 'application/json',
                    },
                    body: jsonContent
                })
                .then(response => response.json())
                .then(data => {
                    if (data.success) {
                        alert('Configuration imported successfully! Refreshing page...');
                        setTimeout(() => {
                            window.location.reload();
                        }, 1000);
                    } else {
                        alert('Failed to import configuration: ' + data.message);
                    }
                })
                .catch(error => {
                    console.error('Error importing configuration:', error);
                    alert('Failed to import configuration');
                })
                .finally(() => {
                    input.value = ''; // Reset file input
                });
            };

            reader.onerror = function() {
                alert('Failed to read file');
                input.value = ''; // Reset file input
            };

            reader.readAsText(file);
        }

        connectWebSocket();
        updateClock();
        checkRelayStatus();
        setInterval(updateClock, 1000);
        setInterval(checkRelayStatus, 5000);  // Check relay status every 5 seconds
    </script>
</body>
</html>
)rawliteral";

  sendCORSResponse(200, "text/html", html);
}