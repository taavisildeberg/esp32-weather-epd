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

#ifndef __CONFIG_MANAGER_H__
#define __CONFIG_MANAGER_H__

#include <Arduino.h>

// Structure to hold configurable settings
struct WeatherConfig {
  // WiFi settings
  char wifi_ssid[64];
  char wifi_password[64];
  
  // Location settings
  char latitude[16];
  char longitude[16];
  char city_name[64];
  
  // API settings
  char owm_apikey[64];
  
  // Units settings
  char temp_unit[16];      // "CELSIUS", "FAHRENHEIT", "KELVIN"
  char speed_unit[32];     // "KPH", "MPH", "MPS", "KNOTS", etc.
  char pressure_unit[32];  // "HECTOPASCALS", "INCHESOFMERCURY", etc.
  char distance_unit[16];  // "KILOMETERS", "MILES"
  
  // Display settings
  int sleep_duration;      // minutes
  int bed_time;            // hour (0-23)
  int wake_time;           // hour (0-23)
  
  // Flags
  bool configured;
};

// Initialize configuration manager
void initConfigManager();

// Load configuration from LittleFS
bool loadConfig(WeatherConfig &config);

// Save configuration to LittleFS
bool saveConfig(const WeatherConfig &config);

// Get current configuration
WeatherConfig& getCurrentConfig();

// Start web configuration portal
void startConfigPortal();

// Start web server for runtime configuration
void startWebServer();

// Stop web server
void stopWebServer();

// Check if configuration button is pressed (for entering config mode)
bool isConfigButtonPressed();

#endif
