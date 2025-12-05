// ======================================================================================
// WIFI MANAGER - Pulsar Rebirth v5.4
// ======================================================================================

bool connectToNetwork(const char* ssid, const char* password, int maxAttempts) {
  Serial.print("Connecting to ");
  Serial.print(ssid);
  Serial.println("...");
  
  WiFi.begin(ssid, password);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < maxAttempts) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println();
    Serial.print("Connected! IP address: ");
    Serial.println(WiFi.localIP());
    sysState.network.activeNetwork = String(ssid);
    sysState.network.connected = true;
    return true;
  }
  
  Serial.println();
  Serial.println("Connection failed.");
  return false;
}

bool connectToWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);
  
  if (connectToNetwork(Config::PRIMARY_SSID, Config::PRIMARY_PASS, 20)) {
    return true;
  }
  
  Serial.println("Primary network failed. Trying secondary network...");
  
  if (connectToNetwork(Config::SECONDARY_SSID, Config::SECONDARY_PASS, 20)) {
    return true;
  }
  
  Serial.println("All WiFi networks failed. Running in offline mode.");
  sysState.network.activeNetwork = "Not Connected";
  sysState.network.connected = false;
  return false;
}

void checkWiFiConnection() {
  if (millis() - sysState.network.lastWifiCheck < Config::WIFI_CHECK_INTERVAL) {
    return;
  }
  
  sysState.network.lastWifiCheck = millis();
  
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi connection lost. Attempting to reconnect...");
    connectToWiFi();
  }
}

void setupMDNS() {
  if (MDNS.begin(Config::OTA_HOSTNAME)) {
    Serial.print("mDNS responder started: ");
    Serial.print(Config::OTA_HOSTNAME);
    Serial.println(".local");
    MDNS.addService("http", "tcp", 80);
    MDNS.addService("ws", "tcp", Config::WEBSOCKET_PORT);  // Register WebSocket service
  } else {
    Serial.println("Error setting up mDNS responder");
  }
}

void setupOTA() {
  ArduinoOTA.setHostname(Config::OTA_HOSTNAME);
  ArduinoOTA.setPassword(Config::OTA_PASSWORD);
  
  ArduinoOTA.onStart([]() {
    String type = (ArduinoOTA.getCommand() == U_FLASH) ? "sketch" : "filesystem";
    Serial.println("Start updating " + type);
  });
  
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) {
      Serial.println("Auth Failed");
    } else if (error == OTA_BEGIN_ERROR) {
      Serial.println("Begin Failed");
    } else if (error == OTA_CONNECT_ERROR) {
      Serial.println("Connect Failed");
    } else if (error == OTA_RECEIVE_ERROR) {
      Serial.println("Receive Failed");
    } else if (error == OTA_END_ERROR) {
      Serial.println("End Failed");
    }
  });
  
  ArduinoOTA.begin();
  Serial.println("OTA update functionality enabled");
}
