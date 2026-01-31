# Quick Reference: Critical Security Fixes

## 🔴 CRITICAL - Must Fix Before Production

| # | Issue | File | Line | Fix Complexity |
|---|-------|------|------|----------------|
| 1 | Hardcoded API key | `config.cpp` | 61 | Easy (5 min) |
| 2 | Missing JSON validation | `api_response.cpp` | 84-85, 122-123 | Medium (30 min) |
| 3 | Array bounds checking | `api_response.cpp` | 106-180 | Easy (15 min) |
| 4 | Unsafe strcpy | `_strftime.cpp` | Multiple | Medium (20 min) |

**Total Estimated Time:** ~70 minutes

---

## Quick Fix Code Snippets

### Fix #1: API Key (5 minutes)
```bash
# 1. Create secrets template
cat > platformio/include/secrets.h.template << 'EOF'
#ifndef __SECRETS_H__
#define __SECRETS_H__
const String OWM_APIKEY = "YOUR_API_KEY_HERE";
#endif
EOF

# 2. Copy to secrets.h and add your key
cp platformio/include/secrets.h.template platformio/include/secrets.h

# 3. Update .gitignore
echo "platformio/include/secrets.h" >> .gitignore

# 4. Update config.cpp - remove line 61, add:
# #include "secrets.h"
```

### Fix #2: JSON Validation (Find & Replace)
**Before:**
```cpp
r.current.rain_1h = current["rain"]["1h"].as<float>();
```

**After:**
```cpp
r.current.rain_1h = 0.0f;
if (current.containsKey("rain") && current["rain"].containsKey("1h")) {
    r.current.rain_1h = current["rain"]["1h"].as<float>();
}
```

**Locations to fix:**
- Line 84: `current["rain"]["1h"]`
- Line 85: `current["snow"]["1h"]`
- Line 122: `hourly["rain"]["1h"]`
- Line 123: `hourly["snow"]["1h"]`

### Fix #3: Array Bounds (Pattern Replace)
**Before:**
```cpp
for (JsonObject hourly : doc["hourly"].as<JsonArray>()) {
    r.hourly[i].dt = ...;
    if (i == OWM_NUM_HOURLY - 1) break;
    ++i;
}
```

**After:**
```cpp
for (JsonObject hourly : doc["hourly"].as<JsonArray>()) {
    if (i >= OWM_NUM_HOURLY) break;
    r.hourly[i].dt = ...;
    ++i;
}
```

**Loops to fix:**
- Line 106: hourly loop
- Line 137: daily loop
- Any alert or air pollution loops

### Fix #4: String Safety (Pattern Replace)
**Before:**
```cpp
strcpy(tbuf, LC_ABDAY[timeptr->tm_wday]);
```

**After:**
```cpp
strncpy(tbuf, LC_ABDAY[timeptr->tm_wday], sizeof(tbuf) - 1);
tbuf[sizeof(tbuf) - 1] = '\0';
```

**All strcpy locations in _strftime.cpp:**
- Lines: 296, 298, 303, 305, 310, 312, 317, 319, 426, 428

---

## Testing After Fixes

```bash
# 1. Build to verify no syntax errors
cd platformio
pio run

# 2. Check for common issues
pio check

# 3. Test with invalid API responses (manual)
# - Comment out API calls temporarily
# - Inject test JSON with missing fields
# - Verify no crashes
```

---

## Git Workflow

```bash
# Create feature branch
git checkout -b fix/critical-security-issues

# Make fixes
# ... edit files ...

# Commit each fix separately
git add platformio/include/secrets.h.template .gitignore
git commit -m "feat: externalize API key to secrets file"

git add platformio/src/api_response.cpp
git commit -m "fix: add JSON validation for nested objects"

git add platformio/src/api_response.cpp
git commit -m "fix: improve array bounds checking in API parsing"

git add platformio/src/_strftime.cpp
git commit -m "fix: replace strcpy with strncpy for buffer safety"

# Push and create PR
git push origin fix/critical-security-issues
```

---

## Validation Checklist

After applying fixes, verify:

- [ ] Project builds without errors
- [ ] API key is not in any tracked file
- [ ] `.gitignore` includes `secrets.h`
- [ ] `secrets.h.template` exists with instructions
- [ ] JSON parsing handles missing rain/snow fields
- [ ] Array loops check bounds before accessing
- [ ] All strcpy replaced with strncpy
- [ ] No compiler warnings
- [ ] README updated with secrets setup instructions

---

## Impact Assessment

| Fix | Potential Failures Prevented |
|-----|------------------------------|
| #1  | API key exposure in public repos |
| #2  | Crashes when API returns unexpected format |
| #3  | Buffer overflows from oversized API responses |
| #4  | Buffer overflows from long locale strings |

**Risk Reduction:** High → Low (Security & Stability)

