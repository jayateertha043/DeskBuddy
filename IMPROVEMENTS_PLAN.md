# DeskBuddy Improvements Plan
**Scope:** UI/UX, Efficiency, New Features, Power Saving  
**Status:** Planning Phase (No Implementation Yet)

---

## 1. UI/UX IMPROVEMENTS

### 1.1 Display Rendering & Animation
**Problem:** Small 128×64 OLED limits visual information and animation detail  
**Current State:** 17 mood animations, weather display, clock in header strip

#### A. High-Refresh Display Rendering
- **Increase frame rate:** Current implicit framerate likely 15-20fps
  - Target: 30fps+ for smoother animations (reduces motion blur perception)
  - Action: Profile animationFrame update interval; reduce screen_*.cpp render overhead
  - Benefit: Eyes blink smoother, scrolling/transitions feel less choppy
  
#### B. Dynamic Layout Modes
- **Current:** Fixed layout (header strip + face + footer)
- **Proposed modes:**
  - **Compact Mode:** Weather only, larger face
  - **Info Mode:** Weather + time + temperature stacked vertically
  - **Fullscreen Mode:** Face only (for decorative use)
  - **Data Mode:** Minimal face, max data (temp, humidity, UV, pressure)
  - **Toggle:** Add "Layout" to dashboard or cycle via long-press (TBD - no hardware button currently)

#### C. Enhanced Weather Visual Feedback
- **Current:** Animated icons (rain, snow, lightning, fog, clouds)
- **Additions:**
  - Real-time AQI (Air Quality Index) indicator if available
  - Humidity visualization (eye wetness variation)
  - Wind direction arrow (subtle rotation)
  - Pressure trend (rising/falling indicator)
  - UV intensity (eye color shift: blue→yellow→red)

#### D. Smoother Transitions
- **Current:** Instant screen switches (boot → weather → manual mood, etc.)
- **Proposed:** Fade/slide transitions between screens
  - Implementation: Add opacity envelope in Canvas (8-10ms cross-fade)
  - Benefit: More polished, less jarring

#### E. Interactive Feedback
- **On-screen touches/inputs (future hardware):** Subtle reactions (blink, startled face)
- **Web dashboard hover:** Improve mood preview responsiveness (already has animated preview)

---

### 1.2 Web Portal Enhancements

#### A. Dashboard Theme & Accessibility
- **Current:** Functional but minimal CSS
- **Additions:**
  - **Dark mode toggle:** Save preference in NVS
  - **High contrast mode:** For accessibility
  - **Font sizing options:** Especially for mobile/accessibility
  - **Color palette customization:** Let user pick accent colors (stored in NVS)

#### B. Real-time Data Sync
- **Current:** Dashboard is static snapshots on page load/save
- **Proposed:**
  - WebSocket or Server-Sent Events (SSE) for live updates (temp, time, WiFi signal, mood)
  - Live mood preview synced from OLED (not just animated preview)
  - Status indicators (WiFi signal strength, battery estimate, uptime)

#### C. Advanced Controls
- **Current:** Basic mood select, emote mode (Static/Random/Loop)
- **Proposed additions:**
  - **Emote scheduling:** Set mood changes by time-of-day (e.g., "excited 9am, sleepy 11pm")
  - **Weather-based reactions:** "Show excited when temp > 25°C"
  - **Multi-step animations:** Record custom animation sequences
  - **Ambient light sensor mock:** Simulate brightness for design (if hardware added)

#### D. Data Logging & Analytics
- **Light analytics dashboard:** Show trends (most common weather, daily mood patterns)
- **Activity timeline:** Last 24h WiFi/connection events
- **Export:** CSV of weather history, NVS backup/restore

#### E. System Information & Diagnostics
- **Dashboard page:** Display heap usage, flash usage, WiFi RSSI, CPU load
- **Log viewer:** Recent Serial output (if UDP syslog relay added) or circular buffer
- **Firmware version & OTA status display**

---

## 2. EFFICIENCY IMPROVEMENTS

### 2.1 Memory Optimization (RAM: currently 13.2% C3)

#### A. String Storage
- **Current:** Likely some hardcoded strings in RAM
- **Audit & convert:** All strings to `PROGMEM` (Flash) where not performance-critical
- **Benefit:** Save ~500–1500 bytes RAM for mood labels, API URLs, greeting messages

#### B. Display Frame Buffer
- **Current:** Adafruit_SSD1306 uses full frame buffer (~1 KB)
- **Consider:** Partial frame buffer for less frequent redraws
- **Trade-off:** Complexity vs 1 KB savings (not critical, but nice-to-have)

#### C. JSON Parsing Optimization
- **Current:** Custom `json_lite` module (lightweight)
- **Review:** Replace recursive calls with iterative parsing if memory-bound
- **Benefit:** Predictable stack usage; harder to overflow

#### D. Task Scheduling Refactor
- **Current:** Multiple timers/intervals in different modules (weather, emote, clock)
- **Proposed:** Unified task scheduler / task queue
- **Benefit:** Single fixed-size array vs scattered static vars; deterministic memory

---

### 2.2 CPU/Processing Efficiency

#### A. Render Optimization
- **Current:** Full screen redrawn every frame (animationFrame++)
- **Proposed:**
  - Dirty rect tracking: only update changed regions
  - Double-buffer with difference detection
  - Conditional renders: skip if frame unchanged
- **Benefit:** Reduce I2C/SPI traffic to OLED; lower CPU wake time

#### B. Weather API Polling
- **Current:** 15 min interval (WEATHER_INTERVAL_MS)
- **Proposed:**
  - Add adaptive polling: increase interval if WiFi weak (reduce retry storms)
  - Cache weather across reboots (survives power loss) for instant display
  - Conditional fetch: only if location changed or > 15 min elapsed
- **Benefit:** Fewer API calls, better resilience

#### C. WiFi Scanning & Connection
- **Current:** Automatic reconnect on disconnect
- **Proposed:**
  - Reduce WiFi scan frequency if signal strong (rssi > -60 dBm)
  - Implement exponential backoff for reconnect attempts (capped at 15 min retry)
  - Skip auto-reconnect if user manually set offline mode (future NVS flag)
- **Benefit:** Avoid constant scanning; less RF interference

#### D. I2C Clock Stretching
- **Current:** Standard Arduino Wire library defaults
- **Proposed:** Audit Wire configuration for timeout/stretch limits
- **Benefit:** Catch OLED hangs early; avoid blocking entire loop

---

### 2.3 Flash Optimization (currently 78.7% C3)

#### A. Code Bloat Audit
- **Current:** 78.7% flash on C3; plenty of room, but identifies hotspots
- **Identify:** Largest .o files (platform.h pulls in a lot of headers)
- **Trim:**
  - Remove unused includes (e.g., AsyncWebServer headers if possible)
  - Consolidate repeated string literals (PROGMEM)
  - Consider linker script optimization (LTO already on?)

#### B. Feature Compilation Flags
- **Proposed:** `#ifdef` guards for optional features
  - Disable OTA for minimal builds
  - Disable web portal for headless/read-only devices
  - Disable mood animations (keep weather-only mode)
- **Benefit:** Future flexibility; different board profiles (mini vs. standard)

#### C. Library Updates
- **Audit:** U8g2 (display), ArduinoJson, HTTPClient versions
- **Action:** Check for newer lightweight alternatives or updates with smaller footprint
- **Benefit:** Potential 1-5% flash savings + security patches

---

## 3. NEW FEATURES

### 3.1 Immediate/High-Value Features

#### A. Pomodoro Timer Enhancement
- **Current:** Basic pomodoro.cpp module exists but limited UI exposure
- **Additions:**
  - Full display visual: countdown timer on OLED (large numbers)
  - Animated faces for work/break phases (focused vs relaxed)
  - Auto-repeat mode (run back-to-back pomodoros)
  - Preset templates (25/5, 50/10, custom durations)
  - Dashboard control: start/pause/skip
  - Sound alert (if buzzer added to hardware) or mood reaction

#### B. Greeting Customization
- **Current:** "Hi {name}!" every N minutes
- **Proposed:**
  - Time-aware greetings: "Good morning {name}!" (6am-12pm), "Good afternoon..." (12-6pm), etc.
  - Mood-aware: "Cheer up {name}!" (when sad mood selected)
  - Weather-aware: "Bundle up, {name}! It's snowing!" 
  - Random greeting pool per time block
  - Frequency control: every 15/30/60 min

#### C. Time Zone Auto-Detection
- **Current:** Geocoding provides coordinates; NTP provides UTC
- **Issue:** Timezone offset hard to infer from lat/lon alone (need API call)
- **Proposed:** Add lightweight timezone database lookup or call Open-Meteo timezone API
- **Benefit:** Automatic DST handling; user never sets timezone manually

#### D. Hardware Button Support (Future-Ready)
- **Add GPIO placeholder:** Define GPIO for hypothetical button
- **Actions:**
  - Short press: Toggle mood/emote mode
  - Long press: Enter WiFi setup portal (without waiting 40s)
  - Double-press: Trigger emote reaction
- **Documentation:** Wiring diagram for optional button module

#### E. Dark/Sleep Mode
- **Proposed:** Scheduled display off-time (e.g., 10pm–7am)
  - Dashboard setting: sleep start/end times
  - Visual: Blank screen or minimal indicator (low power consumption)
  - Wake behavior: Display still processes but dims (0% brightness → 100% over 5s)
  - Manual override: Any mood change wakes display early

#### F. Local Weather Data Sharing
- **Proposed:** mDNS txt records or simple HTTP endpoint for raw weather JSON
- **Use case:** Other devices on network can query deskbuddy.local/weather.json
- **Benefit:** Ecosystem integration; home automation rules

### 3.2 Medium-Term Features

#### A. Multi-Location Support
- **Current:** Single location (city/country)
- **Proposed:**
  - Store 3–5 favorite locations in NVS
  - Dashboard carousel to switch between locations
  - OLED cycles through locations every N min (configurable)
  - Useful for: Travel planning, multi-home setups
- **Trade-off:** Uses more NVS space; updates 5x more weather

#### B. Customizable Animations
- **Proposed:** Simple animation "builder" in web UI
  - Drag-drop mood elements: eyes, eyebrows, mouth, effects
  - Save to NVS (binary layout or minimal JSON)
  - OLED interprets and renders custom animations
- **Challenge:** Limited screen real estate; needs compact encoding
- **Benefit:** Deep personalization; fun engagement factor

#### C. Integration with Home Automation
- **Proposed:** Home Assistant MQTT support
  - Publish: current mood, temperature, weather condition
  - Subscribe: commands to change mood, trigger emote
  - Enable automations: "If [home_occupancy] == true, DeskBuddy mood = excited"
- **Alternative:** REST API (simpler; already have web server)
- **Benefit:** Smart home ecosystem inclusion

#### D. Voice/Audio Alerts (Hardware-Dependent)
- **Proposed:** Buzzer or speaker module (GPIO + passive buzzer)
  - Audio tone patterns for: pomodoro end, WiFi reconnect, alert
  - Optional: Text-to-speech (likely too heavy for ESP32-C3; dismiss as out-of-scope)
- **Placeholder:** Add GPIO/audio module to platform.h with no-op stubs

#### E. Log-Based Analytics Dashboard
- **Proposed:** JavaScript app that queries /api/logs endpoint
  - Line chart: temperature over 24h
  - Bar chart: mood frequency
  - Pie: weather condition distribution
- **Data source:** Circular buffer in NVS (store last 288 readings @ 5min intervals)

### 3.3 Stretch Features (Low Priority)

#### A. Machine Learning Mood Prediction
- **Concept:** TinyML model to predict mood based on weather/time/user history
- **Challenge:** Requires model training & embedded inference; >10% flash
- **Decision:** Defer unless TinyML library licensing/size proven feasible

#### B. Collaborative Deskbuddies
- **Proposed:** Mesh network or cloud sync between multiple DeskBuddies
  - Share mood/greeting swaps: "When A is excited, B shows excited too"
  - Leaderboard: "Who's been happiest this week?"
- **Challenge:** Adds complexity, privacy concerns, WiFi overhead
- **Decision:** Out-of-scope for MVP but documentable roadmap

#### C. Camera Module Integration
- **Proposed:** Add OV2640 camera; detect user presence/emotion
  - Mood reacts to user: happy if you're nearby, sad if you leave
  - Time-lapse photo upload to cloud
- **Challenge:** Power draw, complexity, privacy implications
- **Decision:** Future hardware variant

---

## 4. POWER SAVING IMPROVEMENTS

### 4.1 Sleep Modes (Software)

#### A. Modem Sleep Optimization
- **Current:** Enabled on WiFi connect via `platformWifiSleep(true)` call
- **Issue:** Modem keeps listening; 30–50 mA baseline
- **Proposed:**
  - Add WiFi Sleep policy dropdown (Aggressive/Balanced/Disabled)
  - **Aggressive:** Sleep until next weather fetch (15 min)
  - **Balanced:** Light sleep on inactivity, wake on DTIM (15 min + real-time WiFi)
  - **Disabled:** Always listening (current)
  - Store choice in NVS; apply on boot
- **Benefit:** Extended battery life if powered by USB (UPS scenario)

#### B. Display Brightness Control
- **Current:** Fixed brightness (display.setContrast(200) or similar)
- **Proposed:**
  - Soft PWM on CS pin (if possible) or full display off during sleep times
  - Auto-brightness based on time-of-day (e.g., dim 10pm–7am)
  - Manual brightness slider in dashboard (0–100%)
  - Setting stored in NVS
- **Benefit:** OLED power draw reduced by 30–60% when dimmed; eye strain reduced

#### C. CPU Frequency Scaling
- **Current:** CPU runs at default clock (160 MHz on ESP32-C3)
- **Proposed:** Arduino/IDF provides `setCpuFrequencyMhz()` calls
  - Reduce to 80 MHz during idle periods (no active WiFi, no active rendering)
  - Only when next task > 1 second away
  - Keep high freq for animation frames
- **Trade-off:** Code complexity; marginal power savings (~5 mA)
- **Benefit:** Thermal management; battery life extension

#### D. NTP Sync Frequency
- **Current:** Synced on boot; relies on drifting RTC
- **Proposed:**
  - Reduce sync to once per 24h (rather than every boot)
  - Or use system uptime to estimate drift; only resync if > 5 sec off
- **Benefit:** Fewer WiFi wakeups; fewer NTP packets

---

### 4.2 Hardware-Level Power Optimization

#### A. Enable Hardware Deep Sleep (Future)
- **Proposed (requires new feature):** Scheduled wake timer
  - User sets: "Sleep from 11pm–7am; wake at 7:15 am for weather"
  - Calls `esp_deep_sleep_enable_timer_wakeup()` with RTC timer
  - Power draw: < 1 mA during deep sleep (vs 30–80 mA active)
  - Trade-off: No real-time interactions during sleep; display is off
- **Impact:** 10x+ power savings if user sleeps/works away 10h/day
- **Complexity:** Requires testing; may conflict with OTA/portal modes

#### B. Dynamic Current Limiting
- **Monitor:** Heap/stack exhaustion, which can cause brown-outs
- **Action:** Proactively reduce WiFi TX power or disable WiFi if supply weak
- **Tool:** Add ADC reading of internal voltage; set threshold

#### C. Power Budget Per Feature
- **Proposed:** NVS-stored "power profile" that disables expensive features
  - **Battery Mode:** Disable OTA, reduce weather poll to 30 min, dim display
  - **Standard Mode:** All features, 15 min weather poll, normal brightness
  - **Performance Mode:** 5 min poll, full brightness, all animations
- **Benefit:** User-controlled trade-off; settings persist across reboots

---

### 4.3 Network Efficiency

#### A. Reduce Payload Size
- **Current:** Weather JSON from Open-Meteo (~500–1000 bytes)
- **Proposed:**
  - Cache full response in NVS; only fetch diffs if needed
  - Compress JSON with gzip before storing (if library available)
  - Request only essential fields from API (temp, condition, wind)
- **Benefit:** Fewer bytes transmitted; shorter WiFi TX bursts

#### B. Batch API Calls
- **Current:** Separate calls for weather + geocoding + NTP
- **Proposed:**
  - Combine NTP offset lookup into weather response (if API supports)
  - Cache geocoding results for hours (don't re-query same city)
  - Batch requests on boot; skip on routine loops
- **Benefit:** Fewer connection overhead; fewer TCP handshakes

#### C. WiFi Retry Backoff
- **Current:** 15 sec retry interval (WIFI_RECONNECT_MS)
- **Proposed:**
  - Exponential backoff: 15s → 30s → 60s → 120s (cap at 15 min)
  - Reset to 15s on successful reconnect
  - User-configurable backoff curve in dashboard
- **Benefit:** Reduced RF interference; lower power if internet down long-term

#### D. Multicast DNS Optimization
- **Current:** mDNS update every loop on ESP8266 (platformMdnsUpdate)
- **Proposed:**
  - Reduce update rate: every 100ms instead of every loop
  - Only update if WiFi status changed
- **Benefit:** Marginal CPU savings

---

## 5. QUICK-WIN PRIORITY MATRIX

### High Impact + Low Effort (Do First)
1. **Display brightness slider** (1-2h) → 30% power savings, user-requested UX
2. **Pomodoro timer visual display** (1-2h) → Completes existing feature
3. **Time zone auto-detection** (1-2h) → Better UX; user never sets timezone
4. **WiFi retry exponential backoff** (1h) → Network resilience + power savings
5. **Greeter message variations** (30m) → Engagement; minimal code

### Medium Impact + Medium Effort (Next Batch)
6. **Display layout modes** (2-3h) → Customization; needs LCD real estate planning
7. **Dark/Sleep mode** (2h) → Power savings for overnight scenarios
8. **PROGMEM string audit** (1-2h) → ~500 bytes RAM recovered; good practice
9. **Web dashboard theme + SSE live sync** (3-4h) → Modern UX; WebSocket complexity
10. **Dynamic display refresh rate** (2h) → Smoother animations; render profiling

### Lower Priority (Nice-to-Have)
11. **Emote scheduling by time-of-day** (2-3h) → Advanced but niche
12. **Home Assistant MQTT integration** (3-4h) → Ecosystem; adds external dependency
13. **Weather-aware reaction system** (2h) → Fun but non-essential
14. **Customizable animation builder** (4-5h) → High complexity; UX challenge

### Deferred/Stretch (v2.0+)
15. **Deep sleep timer mode** → Requires rearchitecture
16. **Multi-location support** → Increases flash/NVS usage
17. **Custom animation encoding** → Complex; needs compact format
18. **TinyML mood prediction** → Large footprint; license TBD

---

## 6. IMPLEMENTATION NOTES

### Per-Module Changes Summary

| Module | Improvements | Effort | Benefit |
|--------|--------------|--------|---------|
| **display/canvas.cpp** | Brightness PWM, dirty rect tracking, 30fps target | Med | Power + visual quality |
| **display/screen_*.cpp** | Layout modes, transition fades | Med | Customization |
| **net/wifi_manager.cpp** | Exponential backoff, WiFi sleep policy | Low | Resilience + power |
| **net/weather_service.cpp** | Caching, reduced poll, payload minify | Med | Bandwidth + power |
| **net/web_portal.cpp** | SSE live sync, theme toggle, analytics | High | UX + debugging |
| **core/settings.cpp** | New NVS keys for layout, brightness, sleep times | Low | Persistent state |
| **core/clock.cpp** | Timezone API integration | Low | Auto timezone |
| **display/emote_director.cpp** | Pomodoro visual, scheduled mood changes | Med | Feature completion |
| **include/platform.h** | Button GPIO stubs, power profile enum | Low | Future-ready |

### Testing Checklist (Pre-Implementation)
- [ ] Power profile measurements (active vs sleep vs deep sleep)
- [ ] Display refresh rate profiling (current FPS)
- [ ] RAM profiling (baseline vs with new features)
- [ ] WiFi connection stability across all new retry strategies
- [ ] OTA upload success with reduced TX power
- [ ] Timezone detection accuracy across 5+ cities
- [ ] Battery drain test over 48h (if powered by backup battery)

---

## 7. ROADMAP SUMMARY

### Phase 1 (Weeks 1–2): UX Wins & Power Basics
- [ ] Brightness slider + PWM implementation
- [ ] Time zone auto-detection
- [ ] Pomodoro timer OLED display
- [ ] WiFi retry backoff
- [ ] Greeting variations

### Phase 2 (Weeks 3–4): Efficiency & Display
- [ ] Display layout modes (Compact/Info/Fullscreen/Data)
- [ ] Render optimization (dirty rect or conditional refresh)
- [ ] Dark/Sleep mode scheduling
- [ ] Web portal live sync (SSE)
- [ ] PROGMEM audit

### Phase 3 (Weeks 5–6): Features & Integration
- [ ] Pomodoro presets & auto-repeat
- [ ] Weather-aware reactions
- [ ] Deep sleep timer mode (optional)
- [ ] Home Assistant MQTT (optional)
- [ ] Emote scheduling by time-of-day

### Phase 4+ (Future): Stretch Goals
- [ ] Custom animation builder
- [ ] Multi-location carousel
- [ ] TinyML mood prediction
- [ ] Hardware button support (with PCB design)

---

**Next Step:** Prioritize Phase 1 items and create GitHub issues/PRs for each. Test power savings with a USB power meter.
