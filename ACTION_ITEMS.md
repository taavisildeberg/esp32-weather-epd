# Code Review Action Items - ESP32 Weather Display

## Immediate Action Required 🔴

### 1. Remove Hardcoded API Key
**File:** `platformio/src/config.cpp` line 61  
**Current:**
```cpp
const String OWM_APIKEY = "abcdefghijklmnopqrstuvwxyz012345";
```

**Action Steps:**
1. Create `platformio/include/secrets.h` (not tracked in git)
2. Add to `.gitignore`: `platformio/include/secrets.h`
3. Create template: `platformio/include/secrets.h.template`
4. Update README with setup instructions

**Template Content:**
```cpp
// secrets.h - DO NOT COMMIT THIS FILE
#ifndef __SECRETS_H__
#define __SECRETS_H__

// OpenWeatherMap API key from https://openweathermap.org/
const String OWM_APIKEY = "YOUR_API_KEY_HERE";

// WiFi credentials
const String WIFI_SSID = "YOUR_SSID";
const String WIFI_PASSWORD = "YOUR_PASSWORD";

#endif // __SECRETS_H__
```

---

### 2. Add Input Validation for JSON Nested Objects
**Files:** `platformio/src/api_response.cpp` lines 84-85, 122-123

**Current Code:**
```cpp
r.current.rain_1h = current["rain"]["1h"].as<float>();
r.current.snow_1h = current["snow"]["1h"].as<float>();
```

**Fixed Code:**
```cpp
// Check if nested object exists before accessing
r.current.rain_1h = 0.0f;
if (current.containsKey("rain") && current["rain"].containsKey("1h")) {
    r.current.rain_1h = current["rain"]["1h"].as<float>();
}

r.current.snow_1h = 0.0f;
if (current.containsKey("snow") && current["snow"]["1h"].as<float>();
    r.current.snow_1h = current["snow"]["1h"].as<float>();
}
```

**Apply to:** All locations where nested JSON objects are accessed (rain, snow, weather arrays).

---

### 3. Fix Array Bounds Checking Pattern
**Files:** `platformio/src/api_response.cpp` lines 106-135, 137-180

**Current Pattern:**
```cpp
i = 0;
for (JsonObject hourly : doc["hourly"].as<JsonArray>())
{
    r.hourly[i].dt = hourly["dt"].as<int64_t>();
    // ... many fields ...
    
    if (i == OWM_NUM_HOURLY - 1)  // Check AFTER accessing array
    {
        break;
    }
    ++i;
}
```

**Fixed Pattern:**
```cpp
i = 0;
for (JsonObject hourly : doc["hourly"].as<JsonArray>())
{
    if (i >= OWM_NUM_HOURLY)  // Check BEFORE accessing array
    {
        break;
    }
    
    r.hourly[i].dt = hourly["dt"].as<int64_t>();
    // ... rest of fields ...
    ++i;
}
```

**Apply to:** All array parsing loops:
- hourly[] (line 106)
- daily[] (line 137)
- alerts[] (if present)
- air pollution[] arrays

---

### 4. Replace Unsafe String Operations
**File:** `platformio/src/_strftime.cpp` lines 296-319

**Current Code:**
```cpp
strcpy(tbuf, LC_ABDAY[timeptr->tm_wday]);
```

**Fixed Code:**
```cpp
strncpy(tbuf, LC_ABDAY[timeptr->tm_wday], sizeof(tbuf) - 1);
tbuf[sizeof(tbuf) - 1] = '\0';  // Ensure null termination
```

**Apply to:** All `strcpy` calls in `_strftime.cpp` (lines 296, 298, 303, 305, 310, 312, 317, 319, 426, 428).

---

## High Priority Actions 🟡

### 5. Add Certificate Expiration Warning
**File:** `platformio/src/main.cpp` (in setup function)

**Add:**
```cpp
// Certificate expiration check
const int64_t CERT_EXPIRY_UNIX = 1767225600;  // 2026-01-01 00:00:00 UTC
const int64_t THIRTY_DAYS = 30 * 24 * 60 * 60;

void checkCertificateExpiration(time_t currentTime) {
    if (currentTime > CERT_EXPIRY_UNIX - THIRTY_DAYS) {
        Serial.println("WARNING: HTTPS certificate expires soon!");
        Serial.println("Please update cert.h with new certificate");
        // Consider displaying warning on screen
    }
}
```

---

### 6. Improve Sensor Error Handling
**File:** `platformio/src/main.cpp` lines 323-337

**Current:**
```cpp
if (!bme280_sensor.begin(BME280_ADDRESS)) {
    Serial.println(ERR_SENSOR_BEGIN);
}
```

**Improved:**
```cpp
if (!bme280_sensor.begin(BME280_ADDRESS)) {
    Serial.println(ERR_SENSOR_BEGIN);
    // Set flag to display sensor error on screen
    sensorError = true;
    // Consider reducing update frequency to save battery
}
```

---

### 7. Document Error Code Scheme
**File:** `platformio/src/client_utils.cpp` lines 178, 192

**Add Comment Block:**
```cpp
/*
 * Error Code Scheme:
 * 
 * Positive values: HTTP status codes (200, 404, etc.)
 * 0 to -11: HTTPClient library errors
 * -256 to -511: JSON deserialization errors (offset by -256)
 * -512 to -767: WiFi connection errors (offset by -512)
 * 
 * See HTTPClient.h and DeserializationError.h for error codes
 */
```

---

## Medium Priority Improvements 🟢

### 8. Eliminate Code Duplication in HTTP Functions
**File:** `platformio/src/client_utils.cpp`

Consider refactoring HTTP/HTTPS code using templates:
```cpp
template<typename ClientType>
int makeHttpRequest(ClientType& client, const String& uri, 
                   owm_resp_onecall_t& r) {
    // Common implementation
}
```

---

### 9. Define Magic Numbers as Constants
**File:** `platformio/src/config.cpp`

**Add Section:**
```cpp
// RETRY AND TIMEOUT CONFIGURATION
const int MAX_API_RETRY_ATTEMPTS = 3;
const int MIN_SLEEP_SECONDS = 120;  // Minimum 2 minutes
const float SLEEP_SKIP_THRESHOLD = 0.95f;  // 95% of sleep duration

// BATTERY THRESHOLDS (Volts)
const float BATTERY_LOW_VOLTAGE = 3.20f;
const float BATTERY_CRITICAL_VOLTAGE = 3.10f;
const float BATTERY_FULL_VOLTAGE = 4.20f;
```

---

### 10. Add Basic Input Validation Unit Tests
**New File:** `platformio/test/test_api_response.cpp`

```cpp
#include <unity.h>
#include "api_response.h"

void test_json_missing_rain_field() {
    // Test that missing rain field doesn't crash
    StaticJsonDocument<200> doc;
    doc["current"]["temp"] = 20.5;
    // Note: no "rain" field
    
    owm_resp_onecall_t response;
    deserializeOneCall(doc, response);
    
    TEST_ASSERT_EQUAL_FLOAT(0.0f, response.current.rain_1h);
}

void test_json_oversized_hourly_array() {
    // Test that API returning 100 hourly items doesn't overflow
    // (should only process first 48)
}
```

---

## Review Completion Checklist

- [x] Identified all critical security issues
- [x] Provided specific code fixes with examples
- [x] Prioritized issues by severity
- [x] Created actionable items list
- [x] Suggested testing improvements
- [x] Documented error handling recommendations

---

## Additional Resources

- **ArduinoJson Best Practices:** https://arduinojson.org/v6/api/jsonobject/containskey/
- **ESP32 Secure Storage:** Consider using ESP32 NVS for API keys
- **Static Analysis Tools:** 
  - Cppcheck: `cppcheck --enable=all platformio/src/`
  - PlatformIO Check: `pio check`

---

**Next Steps:**
1. Address issues #1-4 (Critical) immediately
2. Test thoroughly with malformed API responses
3. Add input validation tests
4. Consider implementing issues #5-7 (High priority)
5. Document security considerations in README

