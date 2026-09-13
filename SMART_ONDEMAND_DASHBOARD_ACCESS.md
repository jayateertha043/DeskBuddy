# Smart On-Demand: Manual Dashboard Access Methods
## How to Enable WiFi & Access Web Portal

**Context:** In Smart On-Demand mode (Profile B), WiFi is off most of the time. Users need a way to manually trigger WiFi activation to access the web dashboard at http://deskbuddy.local

**Status:** Planning phase — analyzing trade-offs of different access methods

---

## 1. AVAILABLE ACCESS METHODS (Ranked by Practicality)

### Method 1: Recovery AP Portal (No Hardware Change) ⭐ RECOMMENDED
**How it works:**
- Keep current behavior: After 40 seconds offline (or on boot), device opens recovery AP
- **"DeskBuddy-Setup"** WiFi AP becomes available
- User connects to AP → visits 192.168.4.1 → full dashboard
- On save, device connects to STA and closes AP after 5 minutes (configurable)

**Advantages:**
- ✅ No new hardware required
- ✅ Works immediately (current code)
- ✅ Fallback already exists if user needs config
- ✅ Familiar to current users

**Disadvantages:**
- ❌ Requires waiting 40s for portal to activate
- ❌ User must recognize the need and wait passively
- ❌ Not intuitive for quick checks

**Effort:** Minimal (already implemented)

**User flow:**
```
Power on or boot
    ↓
WiFi off (on-demand mode)
    ↓
Wait 40 seconds (PORTAL_AFTER_MS)
    ↓
"DeskBuddy-Setup" AP appears
    ↓
Connect to AP → 192.168.4.1
    ↓
Access full dashboard
```

---

### Method 2: Hardware Button (GPIO Input) ⭐ BEST UX
**How it works:**
- Add a physical button to ESP32-C3 (e.g., GPIO 1 or 2)
- Short press (0.5s): Enable WiFi for 5 minutes; STA connects automatically
- Long press (2s): Force recovery AP portal to activate immediately
- Double press: Toggle WiFi on/off

**Advantages:**
- ✅ Instant activation (no waiting)
- ✅ Intuitive physical interaction
- ✅ Offline-capable (works without WiFi)
- ✅ Professional UX (physical controls standard on IoT devices)
- ✅ Can multitask (press → walk away → check dashboard when ready)

**Disadvantages:**
- ❌ Requires hardware modification (button + wiring)
- ❌ Increases BOM cost ($0.50–1.50 for button)
- ❌ Requires PCB respin or breakout board
- ❌ Additional GPIO pin usage (minimal impact)

**Effort:** 2–3 hours (GPIO config, debounce, state machine, web UI indicator)

**User flow:**
```
Want to check dashboard?
    ↓
Press button for 0.5s
    ↓
Device connects to WiFi (3–5s handshake)
    ↓
Browse to http://deskbuddy.local in browser
    ↓
Make changes
    ↓
WiFi auto-disables after 5 min of inactivity
```

**Code changes needed:**
```cpp
// In include/platform.h or config.h
#define BUTTON_GPIO 1
#define BUTTON_MODE_SHORT 0.5f  // seconds
#define BUTTON_MODE_LONG 2.0f   // seconds

// New module: include/input/button_input.h
namespace ButtonInput {
    void begin();
    void update();  // Call in loop()
    
    enum ButtonEvent {
        BUTTON_SHORT_PRESS,
        BUTTON_LONG_PRESS,
        BUTTON_DOUBLE_PRESS,
        BUTTON_NONE
    };
    ButtonEvent getLastEvent();
}

// In src/main.cpp
void setup() {
    ButtonInput::begin();
}

void loop() {
    ButtonInput::update();
    ButtonEvent evt = ButtonInput::getLastEvent();
    
    if (evt == BUTTON_SHORT_PRESS) {
        WifiManager::enableWiFi(true);
        WifiManager::setWiFiTimeout(5 * 60 * 1000);  // 5 min
    }
    else if (evt == BUTTON_LONG_PRESS) {
        WifiManager::startPortal();  // Force AP immediately
    }
    // ... rest of loop
}
```

---

### Method 3: NVS Persistent Flag + Dashboard Button
**How it works:**
- Once WiFi is ON (via portal or button), user can toggle a "Keep WiFi on" setting in dashboard
- NVS flag `wifi_stay_on` = true keeps WiFi running indefinitely (until changed)
- Default (false) = on-demand mode resumes

**Advantages:**
- ✅ No hardware changes
- ✅ Flexibility (user controls duration)
- ✅ Works with existing AP portal flow
- ✅ Progressive: dial in WiFi time as needed

**Disadvantages:**
- ❌ Requires initial access (need 40s wait or button)
- ❌ User must actively disable again to return to on-demand
- ❌ Risk: User forgets to toggle off; defeats power savings

**Effort:** 1–2 hours (NVS flag, dashboard toggle, loop logic)

**User flow:**
```
Power on → Wait 40s → "DeskBuddy-Setup" appears
    ↓
Connect to AP → Access 192.168.4.1
    ↓
Toggle "Keep WiFi On" checkbox in dashboard
    ↓
WiFi stays enabled for browsing/changes
    ↓
Uncheck to return to on-demand mode
```

**Code changes:**
```cpp
// In include/core/settings.h
namespace Settings {
    bool getWiFiStayOn();
    void setWiFiStayOn(bool enabled);
}

// In src/main.cpp loop
if (!WifiManager::isWiFiEnabled()) {
    if (Settings::getWiFiStayOn()) {
        WifiManager::enableWiFi(true);  // Resume STA
    }
    // else: stay in on-demand mode
}

// In web_portal.cpp (dashboard HTML)
<label>
    <input type="checkbox" name="wifi_stay_on" 
           {% if wifi_stay_on %}checked{% endif %} />
    Keep WiFi Always On (Reduces Battery Life)
</label>
```

---

### Method 4: Cloud Dashboard (Cloud API Relay) ❌ NOT RECOMMENDED
**How it works:**
- Device connects to cloud service (e.g., MQTT broker, HTTP API)
- User sends command via cloud; device wakes WiFi
- Device publishes status to cloud; user checks web UI

**Advantages:**
- ✅ Can be accessed from anywhere (remote homes)
- ✅ Intuitive (app-like experience)

**Disadvantages:**
- ❌ Requires cloud infrastructure (cost, privacy)
- ❌ Adds external dependency (no local control)
- ❌ Defeats privacy goal (open-source, no account needed)
- ❌ User data leaves home network
- ❌ Requires reliable internet gateway
- ❌ Adds complexity & security surface area
- ❌ Power draw for always-on cloud communication

**Recommendation:** Skip this. DeskBuddy philosophy is local-first, no cloud.

---

### Method 5: BLE (Bluetooth Low Energy) Mobile App ⚠️ OVERKILL
**How it works:**
- Add BLE peripheral support to ESP32-C3 (already has BLE)
- User installs mobile app; connects via Bluetooth
- App sends "enable WiFi" command; device enables WiFi for 5 min
- Dashboard accessible from phone or browser on LAN

**Advantages:**
- ✅ Works without WiFi (BLE range ~10–30m)
- ✅ Intuitive mobile interface
- ✅ No new hardware needed

**Disadvantages:**
- ❌ Requires maintaining mobile app (iOS + Android)
- ❌ BLE stack increases flash usage (~30–50 KB)
- ❌ Significant development effort (4–6 weeks)
- ❌ User must install app (friction)
- ❌ BLE battery drain nearly as bad as WiFi (defeat purpose)
- ❌ Overkill for simple "enable WiFi" command

**Recommendation:** Defer to v2.0+ if demand exists.

---

### Method 6: Scheduled WiFi Windows (Time-Based Auto-Wake)
**How it works:**
- User sets in dashboard: "WiFi on 12:00–12:30 PM every day"
- Device automatically enables WiFi during that window
- Schedule stored in NVS; check against RTC in loop

**Advantages:**
- ✅ Predictable; user knows when to check
- ✅ No button needed
- ✅ Works for recurring needs (daily standup check-in)

**Disadvantages:**
- ❌ Requires setting up schedule (friction)
- ❌ Not useful for ad-hoc dashboard access
- ❌ Relies on accurate RTC (needs periodic NTP)
- ❌ Limited use case (too prescriptive)

**Recommendation:** Nice-to-have, but not primary access method. Combine with button/portal.

---

## 2. RECOMMENDED SOLUTION: Layered Approach

### Best Practice: Combine Methods
Use **primary + fallback** to maximize UX:

```
┌─────────────────────────────────────────┐
│ User Wants Dashboard Access             │
└────────┬────────────────────────────────┘
         │
         ├─ Option 1 (PREFERRED): Press Hardware Button
         │     ↓ (Instant, professional)
         │     WiFi enables for 5 min → Access dashboard
         │
         ├─ Option 2 (Fallback): Wait 40s for AP Portal
         │     ↓ (No hardware, but slower)
         │     "DeskBuddy-Setup" appears → Connect → 192.168.4.1
         │
         ├─ Option 3 (Persistent): Toggle "Keep WiFi On"
         │     ↓ (Once accessed, user can make it permanent)
         │     Toggle in dashboard → WiFi stays enabled
         │
         └─ Option 4 (Scheduled): Auto-wake at scheduled times
              ↓ (Optional; user configures)
              WiFi auto-enables at 12:00 PM daily
```

### Implementation Phases

#### Phase 1: Immediate (0 changes to hardware)
- **Method 1 (Recovery Portal)** is already implemented
- Document in README: "If WiFi is off, wait 40 seconds for setup portal to appear"
- Add visual indicator: "Portal available in X seconds" on display

#### Phase 2: Recommended (add button)
- **Method 2 (Button)** + Method 3 (NVS toggle)
- Hardware: Add simple pushbutton to GPIO 1 (or available pin)
- Code: 2–3 hours for debounce + state machine
- UX: Instant WiFi toggle; professional feel

#### Phase 3: Enhancement (optional)
- **Method 6 (Scheduled WiFi windows)**
- Dashboard UI: Time picker for daily WiFi wake windows
- Use RTC to check windows in loop
- ~1–2 hours implementation

---

## 3. DASHBOARD ACCESS UX FLOWS

### Flow A: Button Press (Fastest)
```
User: "I want to change mood"
         ↓
Press button on device (0.5s)
         ↓
Device LED blinks (indicator WiFi enabling)
         ↓
WiFi connects to home network (3–5s)
         ↓
User opens browser to http://deskbuddy.local
         ↓
Dashboard loads instantly (mDNS resolves immediately)
         ↓
Change mood, save
         ↓
WiFi auto-disables after 5 min inactivity
         ↓
Power saved: 85% during standby
```

### Flow B: Portal (No Button)
```
User powers on device
         ↓
Waits 40 seconds (PORTAL_AFTER_MS timeout)
         ↓
Display shows "Portal Active" hint
         ↓
User's phone detects "DeskBuddy-Setup" WiFi AP
         ↓
Connect to AP → Browser auto-navigates to 192.168.4.1
         ↓
Captive portal dashboard loads
         ↓
Change settings, save
         ↓
Device auto-closes AP after save; connects to home WiFi for 5 min
         ↓
If no further interaction, WiFi disables
```

### Flow C: Persistent Toggle (Long Sessions)
```
User accesses dashboard via button or portal
         ↓
Sees toggle: "Keep WiFi On" (currently OFF)
         ↓
Clicks toggle → turns ON
         ↓
Dashboard text: "WiFi will stay on indefinitely (high power draw)"
         ↓
User browses multiple times over 30 minutes
         ↓
When done, clicks toggle again → turns OFF
         ↓
WiFi auto-disables; back to on-demand mode
```

---

## 4. DISPLAY INDICATORS FOR ON-DEMAND MODE

To keep users informed about WiFi state, add indicators to OLED display:

### Top-Right Corner WiFi Status Icon
```
┌──────────────────────────────┐
│ 72°F ⛅ 14:32  [WiFi State]  │  <- Add indicator
├──────────────────────────────┤
│                              │
│      😊  Mood: Happy         │
│                              │
├──────────────────────────────┤
│                              │
└──────────────────────────────┘
```

**WiFi State Icons:**
- **Connected:** `📶` (full signal bars)
- **Connecting:** `🔄` (spinner) + progress
- **Off:** `✕` (crossed WiFi symbol)
- **Portal Active:** `📡` (AP symbol)

**States shown:**
- "WiFi ON" (green) when connected
- "WiFi OFF" (gray) when in on-demand standby
- "Portal: 192.168.4.1" (white) when AP is active
- "Connecting... 3s" (yellow) while handshaking

### Secondary: Text Below Time
```
Last Updated: 3 min ago ⚠️
```
- Green ✅ if data < 15 min old
- Yellow ⚠️ if data 15–30 min old
- Red ❌ if data > 30 min old (no recent WiFi sync)

---

## 5. DETAILED IMPLEMENTATION: METHOD 2 (Button) + METHOD 3 (Toggle)

### Hardware Wiring (Optional)
```
GPIO 1 ──[Pushbutton]──┬─── GND
                        │
                   (Internal pull-up enabled)
```

**Parts needed:**
- 1× Momentary pushbutton (6mm, normally open)
- Optional: 10kΩ pull-up resistor (if not using internal)
- Optional: 100nF debounce capacitor (if switch noisy)

### Code Structure

**File: `include/input/button_input.h` (new)**
```cpp
#pragma once
#include <Arduino.h>

namespace ButtonInput {
    void begin();
    void update();
    
    enum ButtonEvent {
        BUTTON_NONE = 0,
        BUTTON_SHORT_PRESS = 1,
        BUTTON_LONG_PRESS = 2,
        BUTTON_DOUBLE_PRESS = 3
    };
    
    ButtonEvent getLastEvent();
    uint32_t getLastEventTime();
}
```

**File: `src/input/button_input.cpp` (new)**
```cpp
#include "input/button_input.h"
#include "config.h"

#ifndef BUTTON_GPIO
#define BUTTON_GPIO 1  // GPIO 1 on ESP32-C3
#endif

namespace ButtonInput {
    namespace {
        uint32_t lastPressTime = 0;
        uint32_t lastReleaseTime = 0;
        bool isPressed = false;
        ButtonEvent lastEvent = BUTTON_NONE;
        uint32_t lastEventTime = 0;
        int pressCount = 0;
        uint32_t lastDoubleClickReset = 0;
        
        const uint16_t DEBOUNCE_MS = 20;
        const uint16_t SHORT_PRESS_MS = 500;
        const uint16_t LONG_PRESS_MS = 2000;
        const uint16_t DOUBLE_CLICK_WINDOW_MS = 400;
    }
    
    void begin() {
        pinMode(BUTTON_GPIO, INPUT_PULLUP);
    }
    
    void update() {
        bool currentPressed = digitalRead(BUTTON_GPIO) == LOW;
        uint32_t now = millis();
        
        // Debounce
        if (currentPressed && !isPressed) {
            isPressed = true;
            lastPressTime = now;
            lastEvent = BUTTON_NONE;
        }
        else if (!currentPressed && isPressed) {
            isPressed = false;
            lastReleaseTime = now;
            uint32_t pressDuration = lastReleaseTime - lastPressTime;
            
            if (pressDuration >= LONG_PRESS_MS) {
                lastEvent = BUTTON_LONG_PRESS;
                lastEventTime = now;
                pressCount = 0;
            }
            else if (pressDuration >= DEBOUNCE_MS) {
                pressCount++;
                
                // Check for double-press within window
                if (pressCount == 2 && 
                    (now - lastDoubleClickReset) < DOUBLE_CLICK_WINDOW_MS) {
                    lastEvent = BUTTON_DOUBLE_PRESS;
                    lastEventTime = now;
                    pressCount = 0;
                    lastDoubleClickReset = 0;
                }
                else if (pressCount == 1) {
                    // Could be single press or first of double
                    lastDoubleClickReset = now;
                }
            }
        }
        
        // Single press detection (timeout if no second press)
        if (pressCount == 1 && 
            (now - lastDoubleClickReset) > DOUBLE_CLICK_WINDOW_MS) {
            lastEvent = BUTTON_SHORT_PRESS;
            lastEventTime = now;
            pressCount = 0;
        }
    }
    
    ButtonEvent getLastEvent() {
        ButtonEvent result = lastEvent;
        lastEvent = BUTTON_NONE;  // Consume event
        return result;
    }
    
    uint32_t getLastEventTime() {
        return lastEventTime;
    }
}
```

**File: `src/main.cpp` (updated)**
```cpp
#include "input/button_input.h"

void setup() {
    // ... existing setup code ...
    ButtonInput::begin();
}

void loop() {
    ButtonInput::update();
    ButtonInput::ButtonEvent btnEvent = ButtonInput::getLastEvent();
    
    if (btnEvent == ButtonInput::BUTTON_SHORT_PRESS) {
        // Enable WiFi for 5 minutes
        WifiManager::enableWiFi(true);
        WifiManager::setWiFiTimeout(5 * 60 * 1000);
        Serial.println(F("Button: WiFi ON (5 min)"));
    }
    else if (btnEvent == ButtonInput::BUTTON_LONG_PRESS) {
        // Force recovery portal
        WifiManager::startPortal();
        Serial.println(F("Button: Portal forced"));
    }
    else if (btnEvent == ButtonInput::BUTTON_DOUBLE_PRESS) {
        // Toggle WiFi on/off
        bool isOn = WifiManager::isWiFiEnabled();
        WifiManager::enableWiFi(!isOn);
        Serial.printf("Button: WiFi toggled to %s\n", isOn ? "OFF" : "ON");
    }
    
    // ... rest of loop ...
}
```

**File: `include/core/settings.h` (add)**
```cpp
namespace Settings {
    bool getWiFiStayOn();
    void setWiFiStayOn(bool enabled);
    uint32_t getWiFiTimeoutMs();
    void setWiFiTimeoutMs(uint32_t ms);
}
```

**File: `src/core/settings.cpp` (add methods)**
```cpp
namespace Settings {
    namespace {
        Preferences prefs;
        // ... existing state ...
    }
    
    bool getWiFiStayOn() {
        return prefs.getBool("wifi_stay_on", false);
    }
    
    void setWiFiStayOn(bool enabled) {
        prefs.putBool("wifi_stay_on", enabled);
    }
    
    uint32_t getWiFiTimeoutMs() {
        return prefs.getUInt("wifi_timeout_ms", 0);
    }
    
    void setWiFiTimeoutMs(uint32_t ms) {
        prefs.putUInt("wifi_timeout_ms", ms);
    }
}
```

**File: `src/net/wifi_manager.cpp` (add)**
```cpp
namespace WifiManager {
    namespace {
        // ... existing vars ...
        uint32_t wifiDisableAt = 0;
        bool wifiForceEnabled = false;
    }
    
    void setWiFiTimeout(uint32_t ms) {
        wifiDisableAt = millis() + ms;
        Settings::setWiFiTimeoutMs(ms);
    }
    
    void loop() {
        // ... existing WiFi logic ...
        
        // Check if WiFi timeout expired
        if (wifiForceEnabled && wifiDisableAt > 0 && millis() > wifiDisableAt) {
            if (!Settings::getWiFiStayOn()) {
                enableWiFi(false);
                wifiForceEnabled = false;
                wifiDisableAt = 0;
                Serial.println(F("WiFi: Timeout expired, disabling"));
            }
        }
    }
}
```

### Dashboard HTML Addition
```html
<div class="setting">
    <label for="wifi_stay_on">
        <input type="checkbox" id="wifi_stay_on" name="wifi_stay_on" 
               {% if wifi_stay_on %}checked{% endif %} />
        Keep WiFi Always On
        <span class="help">(Reduces battery life; on-demand WiFi resumes on uncheck)</span>
    </label>
</div>

<div class="info">
    <p><strong>WiFi Status:</strong> 
        {% if wifi_connected %}Connected (5 min timeout){% endif %}
        {% if wifi_stay_on %} + Persistent Mode{% endif %}
    </p>
    <p><strong>Last Updated:</strong> {{ last_update_ago }} minutes ago</p>
</div>
```

---

## 6. PROS & CONS SUMMARY

| Method | Pros | Cons | Effort | Recommended |
|--------|------|------|--------|-------------|
| **1. Recovery Portal** | No hardware changes; works now | 40s wait; not intuitive | 0h | Phase 1 ✅ |
| **2. Hardware Button** | Instant; professional | Requires hardware; button cost | 2–3h | Phase 2 ⭐ |
| **3. NVS Toggle** | Flexible; no hardware | Requires accessing dashboard first | 1–2h | Phase 2 ✅ |
| **4. Cloud API** | Remote access | Privacy loss; added complexity | 4h+ | ❌ Skip |
| **5. BLE App** | Works offline | App maintenance; overkill | 4–6w | ⚠️ Defer |
| **6. Scheduled Wake** | Predictable | Prescriptive; requires setup | 1–2h | Phase 3 🔄 |

---

## 7. RECOMMENDATION

### Phase 1 (Deploy Soon)
- **Use Method 1 (Recovery Portal)** as primary access
- **Document clearly** in README:
  ```
  ## Smart On-Demand Mode: Accessing Dashboard
  
  ### Quick Access (Recommended)
  1. Press the button on the device for 0.5 seconds
  2. Wait 3–5 seconds for WiFi to connect
  3. Open browser to http://deskbuddy.local
  4. WiFi automatically disables after 5 minutes of inactivity
  
  ### If No Button
  1. Power cycle device or wait 40 seconds after boot
  2. Look for "DeskBuddy-Setup" WiFi network on your phone
  3. Connect to it; captive portal loads automatically
  4. Access dashboard at 192.168.4.1
  ```

### Phase 2 (Add Button - Recommended)
- Add simple pushbutton to GPIO 1
- Implement Method 2 (short press = 5 min WiFi; long press = force portal)
- Implement Method 3 (toggle in dashboard to keep WiFi on)
- **UX becomes excellent:** Press button → Check dashboard → Done

### Phase 3 (Nice-to-Have)
- Add Method 6 (scheduled WiFi windows) if users request recurring checks
- User sets "WiFi on 12:00–12:10 PM daily" in dashboard

---

## 8. QUICK DECISION TABLE

| User Scenario | Immediate Solution | Ideal Solution (With Button) |
|---------------|-------------------|------------------------------|
| "I just booted, want to change mood" | Wait 40s → Use AP portal (192.168.4.1) | Press button → Open dashboard instantly |
| "WiFi is off, want to check status" | Wait 40s for portal | Press button (0.5s) → Check → Done |
| "Long session: making multiple changes" | Connect to AP; toggle "Keep WiFi On" | Press button; toggle "Keep WiFi On"; browse |
| "Forgot to enable WiFi; want OTA update" | Reboot or wait 40s → AP portal → "Check for Updates" | Press button → Dashboard → "Check for Updates" |
| "I want WiFi always on (no power savings)" | Toggle "Keep WiFi On" in dashboard | Same |

---

## NEXT STEPS

1. **Document Portal Access** (5 min): Update README with 40-second portal workflow
2. **Optional: Add Button** (2–3 hours):
   - Solder pushbutton to GPIO 1 + GND
   - Implement button debounce code
   - Add short/long press logic
   - Test WiFi enable/disable with button
3. **Add Dashboard Toggle** (1–2 hours):
   - NVS keys for `wifi_stay_on` + `wifi_timeout_ms`
   - Web UI checkbox: "Keep WiFi On"
   - Loop logic to check timeout and `wifi_stay_on` flag
4. **Test Power Draw** (1 hour):
   - Measure with USB power meter
   - Verify WiFi disables after timeout
   - Verify button press enables WiFi
   - Verify 5-min auto-disable works

**Recommended approach:** Use **Method 1 + Method 3** immediately (no hardware), then **add Method 2 (button) later** for better UX if you're building a custom board version.

