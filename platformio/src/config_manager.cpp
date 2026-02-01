/* Configuration Manager for esp32-weather-epd.
 * Copyright (C) 2022-2025  Luke Marzen
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "config_manager.h"
#include "config.h"
#include <LittleFS.h>
#include <ArduinoJson.h>
#include <WiFiManager.h>
#include <ESPAsyncWebServer.h>

#define CONFIG_FILE "/config.json"
#define CONFIG_BUTTON_PIN 0  // GPIO0 (BOOT button on most ESP32 boards)

static WeatherConfig currentConfig;
static AsyncWebServer* webServer = nullptr;
static bool webServerRunning = false;

// HTML for configuration page
const char CONFIG_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>Weather Display Configuration</title>
  <style>
    body { font-family: Arial, sans-serif; margin: 20px; max-width: 600px; margin: 0 auto; padding: 20px; }
    h1 { color: #333; }
    .section { margin: 20px 0; padding: 15px; border: 1px solid #ddd; border-radius: 5px; }
    .section h2 { margin-top: 0; color: #555; }
    label { display: block; margin: 10px 0 5px; }
    input, select { width: 100%; padding: 8px; margin-bottom: 10px; border: 1px solid #ddd; border-radius: 3px; box-sizing: border-box; }
    button { background-color: #4CAF50; color: white; padding: 10px 20px; border: none; border-radius: 3px; cursor: pointer; font-size: 16px; margin: 10px 5px 0 0; }
    button:hover { background-color: #45a049; }
    .btn-secondary { background-color: #008CBA; }
    .btn-secondary:hover { background-color: #007399; }
    .success { color: green; padding: 10px; display: none; }
    .error { color: red; padding: 10px; display: none; }
  </style>
</head>
<body>
  <h1>Weather Display Configuration</h1>
  <div id="message" class="success"></div>
  <div id="error" class="error"></div>
  
  <form id="configForm">
    <div class="section">
      <h2>WiFi Settings</h2>
      <label>SSID:</label>
      <input type="text" id="wifi_ssid" name="wifi_ssid" maxlength="63" required>
      <label>Password:</label>
      <input type="password" id="wifi_password" name="wifi_password" maxlength="63">
    </div>

    <div class="section">
      <h2>Location</h2>
      <label>City Name:</label>
      <input type="text" id="city_name" name="city_name" maxlength="63" required>
      <label>Latitude:</label>
      <input type="text" id="latitude" name="latitude" maxlength="15" required placeholder="e.g., 40.7128">
      <label>Longitude:</label>
      <input type="text" id="longitude" name="longitude" maxlength="15" required placeholder="e.g., -74.0060">
    </div>

    <div class="section">
      <h2>API</h2>
      <label>OpenWeatherMap API Key:</label>
      <input type="text" id="owm_apikey" name="owm_apikey" maxlength="63" required>
    </div>

    <div class="section">
      <h2>Units</h2>
      <label>Temperature:</label>
      <select id="temp_unit" name="temp_unit">
        <option value="CELSIUS">Celsius</option>
        <option value="FAHRENHEIT">Fahrenheit</option>
        <option value="KELVIN">Kelvin</option>
      </select>
      <label>Wind Speed:</label>
      <select id="speed_unit" name="speed_unit">
        <option value="KPH">Kilometers per Hour</option>
        <option value="MPH">Miles per Hour</option>
        <option value="MPS">Meters per Second</option>
        <option value="FPS">Feet per Second</option>
        <option value="KNOTS">Knots</option>
        <option value="BEAUFORT">Beaufort</option>
      </select>
      <label>Pressure:</label>
      <select id="pressure_unit" name="pressure_unit">
        <option value="HECTOPASCALS">Hectopascals</option>
        <option value="INCHESOFMERCURY">Inches of Mercury</option>
        <option value="MILLIBARS">Millibars</option>
        <option value="MILLIMETERSOFMERCURY">Millimeters of Mercury</option>
      </select>
      <label>Distance:</label>
      <select id="distance_unit" name="distance_unit">
        <option value="KILOMETERS">Kilometers</option>
        <option value="MILES">Miles</option>
      </select>
    </div>

    <div class="section">
      <h2>Display Settings</h2>
      <label>Update Interval (minutes):</label>
      <input type="number" id="sleep_duration" name="sleep_duration" min="2" max="1440" required>
      <label>Bed Time (hour, 0-23):</label>
      <input type="number" id="bed_time" name="bed_time" min="0" max="23" required>
      <label>Wake Time (hour, 0-23):</label>
      <input type="number" id="wake_time" name="wake_time" min="0" max="23" required>
    </div>

    <button type="submit">Save Configuration</button>
    <button type="button" class="btn-secondary" onclick="loadConfig()">Reload</button>
    <button type="button" class="btn-secondary" onclick="window.location.href='/restart'">Restart Device</button>
  </form>

  <script>
    async function loadConfig() {
      try {
        const response = await fetch('/config');
        const config = await response.json();
        document.getElementById('wifi_ssid').value = config.wifi_ssid || '';
        document.getElementById('wifi_password').value = '';
        document.getElementById('city_name').value = config.city_name || '';
        document.getElementById('latitude').value = config.latitude || '';
        document.getElementById('longitude').value = config.longitude || '';
        document.getElementById('owm_apikey').value = config.owm_apikey || '';
        document.getElementById('temp_unit').value = config.temp_unit || 'FAHRENHEIT';
        document.getElementById('speed_unit').value = config.speed_unit || 'MPH';
        document.getElementById('pressure_unit').value = config.pressure_unit || 'INCHESOFMERCURY';
        document.getElementById('distance_unit').value = config.distance_unit || 'MILES';
        document.getElementById('sleep_duration').value = config.sleep_duration || 30;
        document.getElementById('bed_time').value = config.bed_time || 0;
        document.getElementById('wake_time').value = config.wake_time || 6;
      } catch (error) {
        console.error('Error loading config:', error);
      }
    }

    document.getElementById('configForm').addEventListener('submit', async (e) => {
      e.preventDefault();
      const formData = new FormData(e.target);
      const config = {};
      formData.forEach((value, key) => config[key] = value);
      
      try {
        const response = await fetch('/config', {
          method: 'POST',
          headers: { 'Content-Type': 'application/json' },
          body: JSON.stringify(config)
        });
        
        if (response.ok) {
          document.getElementById('message').textContent = 'Configuration saved successfully! Device will restart.';
          document.getElementById('message').style.display = 'block';
          document.getElementById('error').style.display = 'none';
          setTimeout(() => { window.location.href = '/restart'; }, 2000);
        } else {
          throw new Error('Failed to save configuration');
        }
      } catch (error) {
        document.getElementById('error').textContent = 'Error: ' + error.message;
        document.getElementById('error').style.display = 'block';
        document.getElementById('message').style.display = 'none';
      }
    });

    // Load config on page load
    loadConfig();
  </script>
</body>
</html>
)rawliteral";

void initConfigManager() {
  // Initialize LittleFS
  if (!LittleFS.begin(true)) {
    Serial.println("Failed to mount LittleFS");
    return;
  }
  Serial.println("LittleFS mounted successfully");
  
  // Initialize config button pin
  pinMode(CONFIG_BUTTON_PIN, INPUT_PULLUP);
}

bool loadConfig(WeatherConfig &config) {
  if (!LittleFS.exists(CONFIG_FILE)) {
    Serial.println("Config file does not exist");
    config.configured = false;
    return false;
  }

  File file = LittleFS.open(CONFIG_FILE, "r");
  if (!file) {
    Serial.println("Failed to open config file");
    config.configured = false;
    return false;
  }

  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, file);
  file.close();

  if (error) {
    Serial.println("Failed to parse config file");
    config.configured = false;
    return false;
  }

  // Load settings from JSON
  strlcpy(config.wifi_ssid, doc["wifi_ssid"] | "", sizeof(config.wifi_ssid));
  strlcpy(config.wifi_password, doc["wifi_password"] | "", sizeof(config.wifi_password));
  strlcpy(config.latitude, doc["latitude"] | "", sizeof(config.latitude));
  strlcpy(config.longitude, doc["longitude"] | "", sizeof(config.longitude));
  strlcpy(config.city_name, doc["city_name"] | "", sizeof(config.city_name));
  strlcpy(config.owm_apikey, doc["owm_apikey"] | "", sizeof(config.owm_apikey));
  strlcpy(config.temp_unit, doc["temp_unit"] | "FAHRENHEIT", sizeof(config.temp_unit));
  strlcpy(config.speed_unit, doc["speed_unit"] | "MPH", sizeof(config.speed_unit));
  strlcpy(config.pressure_unit, doc["pressure_unit"] | "INCHESOFMERCURY", sizeof(config.pressure_unit));
  strlcpy(config.distance_unit, doc["distance_unit"] | "MILES", sizeof(config.distance_unit));
  config.sleep_duration = doc["sleep_duration"] | 30;
  config.bed_time = doc["bed_time"] | 0;
  config.wake_time = doc["wake_time"] | 6;
  config.configured = doc["configured"] | false;

  Serial.println("Configuration loaded successfully");
  return true;
}

bool saveConfig(const WeatherConfig &config) {
  JsonDocument doc;
  
  doc["wifi_ssid"] = config.wifi_ssid;
  doc["wifi_password"] = config.wifi_password;
  doc["latitude"] = config.latitude;
  doc["longitude"] = config.longitude;
  doc["city_name"] = config.city_name;
  doc["owm_apikey"] = config.owm_apikey;
  doc["temp_unit"] = config.temp_unit;
  doc["speed_unit"] = config.speed_unit;
  doc["pressure_unit"] = config.pressure_unit;
  doc["distance_unit"] = config.distance_unit;
  doc["sleep_duration"] = config.sleep_duration;
  doc["bed_time"] = config.bed_time;
  doc["wake_time"] = config.wake_time;
  doc["configured"] = true;

  File file = LittleFS.open(CONFIG_FILE, "w");
  if (!file) {
    Serial.println("Failed to create config file");
    return false;
  }

  if (serializeJson(doc, file) == 0) {
    Serial.println("Failed to write config file");
    file.close();
    return false;
  }

  file.close();
  Serial.println("Configuration saved successfully");
  return true;
}

WeatherConfig& getCurrentConfig() {
  return currentConfig;
}

void startConfigPortal() {
  Serial.println("Starting WiFi configuration portal...");
  
  WiFiManager wifiManager;
  wifiManager.setConfigPortalTimeout(300); // 5 minutes timeout
  
  // Start captive portal with a custom SSID
  if (!wifiManager.startConfigPortal("WeatherDisplay-Setup")) {
    Serial.println("Failed to connect or config portal timeout");
    delay(3000);
    ESP.restart();
  }
  
  Serial.println("WiFi connected via config portal");
  
  // Save WiFi credentials to config
  strlcpy(currentConfig.wifi_ssid, WiFi.SSID().c_str(), sizeof(currentConfig.wifi_ssid));
  strlcpy(currentConfig.wifi_password, WiFi.psk().c_str(), sizeof(currentConfig.wifi_password));
  
  // Start web server for additional configuration
  startWebServer();
  
  Serial.println("Configuration portal ready. Connect to:");
  Serial.print("http://");
  Serial.println(WiFi.localIP());
  
  // Keep server running until user is done
  // This would be handled by the main loop checking for completion
}

void startWebServer() {
  if (webServerRunning) {
    return;
  }
  
  webServer = new AsyncWebServer(80);
  
  // Serve the configuration page
  webServer->on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/html", CONFIG_HTML);
  });
  
  // Get current configuration
  webServer->on("/config", HTTP_GET, [](AsyncWebServerRequest *request) {
    JsonDocument doc;
    
    doc["wifi_ssid"] = currentConfig.wifi_ssid;
    // Don't send password for security
    doc["wifi_password"] = "";
    doc["latitude"] = currentConfig.latitude;
    doc["longitude"] = currentConfig.longitude;
    doc["city_name"] = currentConfig.city_name;
    doc["owm_apikey"] = currentConfig.owm_apikey;
    doc["temp_unit"] = currentConfig.temp_unit;
    doc["speed_unit"] = currentConfig.speed_unit;
    doc["pressure_unit"] = currentConfig.pressure_unit;
    doc["distance_unit"] = currentConfig.distance_unit;
    doc["sleep_duration"] = currentConfig.sleep_duration;
    doc["bed_time"] = currentConfig.bed_time;
    doc["wake_time"] = currentConfig.wake_time;
    
    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
  });
  
  // Save configuration
  webServer->on("/config", HTTP_POST, [](AsyncWebServerRequest *request) {}, NULL,
    [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
      JsonDocument doc;
      DeserializationError error = deserializeJson(doc, data);
      
      if (error) {
        request->send(400, "text/plain", "Invalid JSON");
        return;
      }
      
      // Update configuration
      if (doc.containsKey("wifi_ssid")) {
        strlcpy(currentConfig.wifi_ssid, doc["wifi_ssid"], sizeof(currentConfig.wifi_ssid));
      }
      if (doc.containsKey("wifi_password") && strlen(doc["wifi_password"]) > 0) {
        strlcpy(currentConfig.wifi_password, doc["wifi_password"], sizeof(currentConfig.wifi_password));
      }
      if (doc.containsKey("latitude")) {
        strlcpy(currentConfig.latitude, doc["latitude"], sizeof(currentConfig.latitude));
      }
      if (doc.containsKey("longitude")) {
        strlcpy(currentConfig.longitude, doc["longitude"], sizeof(currentConfig.longitude));
      }
      if (doc.containsKey("city_name")) {
        strlcpy(currentConfig.city_name, doc["city_name"], sizeof(currentConfig.city_name));
      }
      if (doc.containsKey("owm_apikey")) {
        strlcpy(currentConfig.owm_apikey, doc["owm_apikey"], sizeof(currentConfig.owm_apikey));
      }
      if (doc.containsKey("temp_unit")) {
        strlcpy(currentConfig.temp_unit, doc["temp_unit"], sizeof(currentConfig.temp_unit));
      }
      if (doc.containsKey("speed_unit")) {
        strlcpy(currentConfig.speed_unit, doc["speed_unit"], sizeof(currentConfig.speed_unit));
      }
      if (doc.containsKey("pressure_unit")) {
        strlcpy(currentConfig.pressure_unit, doc["pressure_unit"], sizeof(currentConfig.pressure_unit));
      }
      if (doc.containsKey("distance_unit")) {
        strlcpy(currentConfig.distance_unit, doc["distance_unit"], sizeof(currentConfig.distance_unit));
      }
      if (doc.containsKey("sleep_duration")) {
        currentConfig.sleep_duration = doc["sleep_duration"];
      }
      if (doc.containsKey("bed_time")) {
        currentConfig.bed_time = doc["bed_time"];
      }
      if (doc.containsKey("wake_time")) {
        currentConfig.wake_time = doc["wake_time"];
      }
      
      currentConfig.configured = true;
      
      if (saveConfig(currentConfig)) {
        request->send(200, "text/plain", "Configuration saved");
      } else {
        request->send(500, "text/plain", "Failed to save configuration");
      }
    }
  );
  
  // Restart device
  webServer->on("/restart", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/plain", "Restarting...");
    delay(1000);
    ESP.restart();
  });
  
  webServer->begin();
  webServerRunning = true;
  Serial.println("Web server started");
}

void stopWebServer() {
  if (webServer != nullptr && webServerRunning) {
    webServer->end();
    delete webServer;
    webServer = nullptr;
    webServerRunning = false;
    Serial.println("Web server stopped");
  }
}

bool isConfigButtonPressed() {
  return digitalRead(CONFIG_BUTTON_PIN) == LOW;
}
