# Code Review: ESP32 Weather E-Paper Display Project

**Date:** January 31, 2026  
**Reviewer:** GitHub Copilot  
**Repository:** taavisildeberg/esp32-weather-epd  
**Lines of Code:** ~6,700 (C++ source and headers)

## Executive Summary

This code review analyzes the ESP32 weather display project, which fetches weather data from OpenWeatherMap API and displays it on a 7.5" E-Paper screen with ultra-low power consumption for battery operation.

**Overall Code Quality: 6.5/10**

The codebase demonstrates good modular design and thoughtful attention to power management. However, several **critical security and reliability issues** require attention, particularly around input validation, buffer safety, and secret management.

---

## Critical Issues 🔴

### 1. **API Key Hardcoded in Source Code**
**Severity:** Critical  
**Location:** `platformio/src/config.cpp:61`

```cpp
const String OWM_APIKEY = "abcdefghijklmnopqrstuvwxyz012345";
```

**Issue:** API keys should never be committed to source control.

**Recommendation:**
- Move to a separate `secrets.h` file (not tracked in git)
- Add `secrets.h` to `.gitignore`
- Provide `secrets.h.template` as an example
- Document setup in README

---

### 2. **No Validation Before Accessing Nested JSON Objects**
**Severity:** Critical  
**Location:** `platformio/src/api_response.cpp:84-85, 122-123`

```cpp
r.current.rain_1h = current["rain"]["1h"].as<float>();
r.current.snow_1h = current["snow"]["1h"].as<float>();
```

**Issue:** These nested objects (`rain`, `snow`) may not exist in the API response. ArduinoJson returns null/default values without throwing errors, but this could lead to unpredictable behavior.

**Recommendation:**
```cpp
// Check if nested object exists before accessing
if (current.containsKey("rain") && current["rain"].containsKey("1h")) {
    r.current.rain_1h = current["rain"]["1h"].as<float>();
} else {
    r.current.rain_1h = 0.0f;
}
```

Apply this pattern to all nested JSON accesses (lines 84-85, 122-123).

---

### 3. **Array Overflow Risk in API Response Parsing**
**Severity:** High  
**Location:** `platformio/src/api_response.cpp:106-135`

```cpp
i = 0;
for (JsonObject hourly : doc["hourly"].as<JsonArray>())
{
    r.hourly[i].dt = hourly["dt"].as<int64_t>();
    // ... many more fields ...
    
    if (i == OWM_NUM_HOURLY - 1)
    {
        break;
    }
    ++i;
}
```

**Issue:** The check happens at the END of the loop iteration, after already writing to `r.hourly[i]`. If the API returns more items than `OWM_NUM_HOURLY` (48), the loop will correctly break, but the logic is fragile.

**Recommendation:**
```cpp
i = 0;
for (JsonObject hourly : doc["hourly"].as<JsonArray>())
{
    if (i >= OWM_NUM_HOURLY)
    {
        break;  // Check BEFORE accessing array
    }
    
    r.hourly[i].dt = hourly["dt"].as<int64_t>();
    // ... rest of fields ...
    ++i;
}
```

Apply this to all array parsing loops (hourly, daily, alerts, air pollution).

---

### 4. **Unsafe String Operations**
**Severity:** Medium  
**Location:** `platformio/src/_strftime.cpp:296-319`

```cpp
strcpy(tbuf, LC_ABDAY[timeptr->tm_wday]);
```

**Issue:** While `tbuf` is 100 bytes (safe for current locales), `strcpy` is inherently unsafe. If a locale string exceeds the buffer, it will cause a buffer overflow.

**Recommendation:**
```cpp
strncpy(tbuf, LC_ABDAY[timeptr->tm_wday], sizeof(tbuf) - 1);
tbuf[sizeof(tbuf) - 1] = '\0';  // Ensure null termination
```

Or use C++ string handling for better safety.

---

## High Priority Issues 🟡

### 5. **HTTPS Certificate Expiration Not Monitored**
**Severity:** Medium  
**Location:** `platformio/include/cert.h:1-2` (comments)

**Issue:** Comments indicate the certificate expires in 2026 and must be manually updated. No automated checking or warnings exist.

**Recommendation:**
- Add certificate expiration date as a constant
- Check on startup and warn if certificate expires within 30 days
- Document renewal process in README

---

### 6. **HTTP Fallback Without Warnings**
**Severity:** Medium  
**Location:** `platformio/src/client_utils.cpp:147-204`

**Issue:** Code supports both HTTP and HTTPS via conditional compilation, but HTTP exposes API keys in plaintext over the network.

**Recommendation:**
- Remove HTTP support or add prominent warnings
- Log a warning message if compiled with HTTP support
- Document security implications in README

---

### 7. **Silent Sensor Failures**
**Severity:** Medium  
**Location:** `platformio/src/main.cpp:323-337`

**Issue:** When BME sensor initialization fails, only a serial message is logged. The system continues with NaN values.

**Recommendation:**
```cpp
if (!bme280_sensor.begin(BME280_ADDRESS)) {
    Serial.println(ERR_SENSOR_BEGIN);
    // Display error message on screen instead of just NaN
    displaySensorError();
}
```

---

## Medium Priority Issues 🟢

### 8. **Code Duplication in HTTP Functions**
**Severity:** Low  
**Location:** `platformio/src/client_utils.cpp:147-204, 210-267`

**Issue:** Nearly identical code for HTTP vs HTTPS requests, differing only in client type.

**Recommendation:**
- Use templates or function pointers to eliminate duplication
- Single implementation with client type as parameter

---

### 9. **Magic Numbers Throughout Codebase**
**Severity:** Low  
**Location:** Multiple files

**Examples:**
```cpp
// client_utils.cpp
const int maxAttempts = 3;  // Why 3?
// config.cpp
#define LOW_BATTERY_VOLTAGE 3.20  // Undocumented threshold
```

**Recommendation:**
- Define all magic numbers as named constants
- Add comments explaining the rationale

---

### 10. **Error Code Collision Risk**
**Severity:** Low  
**Location:** `platformio/src/client_utils.cpp:178, 192`

```cpp
return -512 - static_cast<int>(connection_status);
return -256 - static_cast<int>(jsonErr.code());
```

**Issue:** Using negative offsets for custom error codes could collide with HTTP client error codes.

**Recommendation:**
- Define an error code enum with clear ranges
- Document error code scheme

---

## Positive Observations ✅

1. **Well-Organized Architecture**: Clear separation of concerns with modular design
2. **Comprehensive Configuration**: Extensive validation macros in `config.h` (lines 368-473)
3. **Good Power Management**: Thoughtful deep sleep implementation with aligned wake times
4. **Locale Support**: Excellent internationalization with multiple language support
5. **Battery Monitoring**: Tiered power-saving strategies based on battery level
6. **Retry Logic**: Proper handling of transient network failures (up to 3 attempts)
7. **Documentation**: Good inline comments and copyright headers

---

## Security Summary

### Vulnerabilities Found:
1. ✅ **Fixed**: None - this is an initial review
2. ⚠️ **Not Fixed**: API key exposure (should be externalized)
3. ⚠️ **Not Fixed**: Potential buffer overflows in string operations
4. ⚠️ **Not Fixed**: Missing input validation on API responses

### Recommendations:
- **Immediate**: Remove API key from source control, externalize to secrets file
- **High Priority**: Add input validation for all JSON deserialization
- **High Priority**: Replace unsafe string operations with safe alternatives
- **Medium Priority**: Add certificate expiration monitoring

---

## Testing Recommendations

The codebase lacks automated testing. Recommendations:

1. **Unit Tests**: Add tests for:
   - JSON deserialization with malformed inputs
   - Unit conversion functions
   - Sleep duration calculations
   - Buffer boundary conditions

2. **Integration Tests**:
   - Mock API responses with edge cases
   - Test error recovery paths
   - Validate display rendering with various data

3. **Static Analysis**:
   - Run Cppcheck or similar static analyzer
   - Enable all compiler warnings (`-Wall -Wextra`)
   - Consider using AddressSanitizer for buffer overflow detection

---

## Conclusion

This is a well-crafted embedded IoT project with thoughtful power management and modular design. However, **critical security issues** around secret management and input validation require immediate attention before production deployment.

The codebase would benefit from:
1. Externalizing secrets
2. Adding input validation
3. Replacing unsafe string operations
4. Implementing automated tests
5. Adding certificate expiration monitoring

**Recommended Priority:**
1. 🔴 Fix issues #1-4 (Critical/High)
2. 🟡 Address issues #5-7 (Medium security concerns)
3. 🟢 Improve issues #8-10 (Code quality)

---

## Review Checklist

- [x] Reviewed overall architecture and design
- [x] Identified security vulnerabilities
- [x] Analyzed error handling patterns
- [x] Checked for buffer overflows and memory safety
- [x] Evaluated resource management
- [x] Assessed code quality and maintainability
- [x] Provided specific recommendations with examples
- [x] Documented findings in detailed report
