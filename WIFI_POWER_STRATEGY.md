# WiFi Power Strategy Analysis
## On-Demand vs Always-Connected Approach

**Question:** Should DeskBuddy use WiFi only on-demand to fetch data from the internet, rather than keeping it always connected?

**Status:** Planning phase — no implementation yet

---

## 1. CURRENT ARCHITECTURE (Always-Connected)

### Power Draw Breakdown (ESP32-C3)
| State | Current | Duration | Daily Energy |
|-------|---------|----------|---------------|
| WiFi connected, idle (modem sleep) | 50–80 mA | ~24h | ~1200–1920 mAh |
| WiFi disconnected, active CPU | 15–25 mA | ~0.5h | ~7.5–12.5 mAh |
| **Estimated daily total** | — | — | **~1200–1950 mAh** |

### Current WiFi Lifecycle
1. **Boot:** Immediately connect to STA (WiFi SSID/password from NVS)
2. **Connected:** Modem sleep enabled (listening for packets but minimal TX)
3. **Data fetches:** Every 15 min (weather), immediate (NTP on boot), on-demand (web dashboard)
4. **Offline fallback:** After 40s offline, switch to AP portal (SoftAP consumes **more** power: 80–120 mA)
5. **Reconnect:** Automatic; stays in STA mode

### Advantages of Current Approach
- ✅ **Responsive web dashboard:** Access http://deskbuddy.local anytime (instant load)
- ✅ **OTA updates:** Always available for firmware pushes
- ✅ **Real-time sync:** NTP synced on boot; minimal drift
- ✅ **Simpler code:** No state machine for WiFi on/off transitions
- ✅ **No reconnection delays:** No 3–5s WiFi handshake every fetch
- ✅ **mDNS always discoverable:** Device visible on network immediately
- ✅ **Emergency AP:** Graceful fallback if internet lost

### Disadvantages (Power-Focused)
- ❌ **Constant RF activity:** Modem sleep still draws 50+ mA continuously
- ❌ **High battery drain:** Unsuitable for battery-powered deployments
- ❌ **Wasted energy on idle periods:** Keeps WiFi alive even during sleep hours

---

## 2. PROPOSED: ON-DEMAND WiFi STRATEGY

### Concept
WiFi is **turned off by default**. It only activates when:
1. A network-dependent task is scheduled (weather fetch, NTP, OTA check)
2. User manually triggers a refresh via web API
3. A timer-based event requires internet (pomodoro notification, etc.)

### Power Draw Breakdown (On-Demand)
| State | Current | Duration | Frequency | Daily Energy |
|-------|---------|----------|-----------|---------------|
| WiFi off, display on | 8–12 mA | ~22h/day | 1× | ~176–264 mAh |
| WiFi connect sequence | 80–120 mA | ~3–5s | 96×/day (15 min fetches) | ~4–8 mAh |
| WiFi data transfer | 100–150 mA | ~1–2s | 96×/day | ~2.5–4 mAh |
| WiFi disconnect sequence | 50 mA | ~1s | 96×/day | ~1.3 mAh |
| **Estimated daily total** | — | — | — | **~184–276 mAh** |

### Potential Power Savings
- **Current always-on:** ~1200–1950 mAh/day
- **On-demand approach:** ~184–276 mAh/day
- **Savings:** **85–88% reduction** in daily energy consumption
- **Battery life improvement:** 5 × longer (6x with deep sleep modes)

---

## 3. DETAILED ANALYSIS: ARCHITECTURE CHANGES

### 3.1 WiFi Lifecycle (Proposed)

```
Boot
  ↓
Load NVS (WiFi credentials)
  ↓
Initialize WiFi (but don't connect yet)
  ↓
Display boots (no internet needed)
  ↓
IDLE LOOP:
  • Display face + weather (cached)
  • Check timers
  ↓
WHEN TRIGGERED (Weather fetch, NTP, OTA, manual refresh):
  • Enable WiFi
  • Connect to SSID (3–5s handshake)
  • Fetch data (1–2s)
  • Process response
  • Disconnect WiFi
  • Turn off WiFi radio
  ↓
BACK TO IDLE
```

### 3.2 Required NVS Caching

For on-demand WiFi to work, **all** network-dependent state must persist across WiFi off-periods:

| Data | Current Behavior | On-Demand Strategy | NVS Size |
|------|------------------|-------------------|----------|
| **Weather** | Fetched every 15 min | Cache for 15 min; show "Last updated: 15 min ago" | ~500 bytes |
| **Coordinates** | Once per boot (or on config change) | Cache in NVS permanently; only refetch on location change | ~50 bytes |
| **Time (NTP)** | Synced once at boot | Use RTC (with drift compensation); resync 1x/hour or less | ~10 bytes |
| **Timezone** | Cached in NVS | Keep cached; optionally resync 1x/day | ~50 bytes |
| **WiFi credentials** | Already cached | No change | ~100 bytes |

**Total new NVS overhead:** ~610 bytes (available: ~800 KB on ESP32-C3)

---

## 4. FEATURE-BY-FEATURE IMPACT

### 4.1 Weather Display
| Aspect | Always-On | On-Demand | Impact |
|--------|-----------|-----------|--------|
| **Refresh rate** | Every 15 min (live) | Every 15 min (cached) | ✅ Same |
| **Stale data indicator** | N/A | "Last updated: X min ago" | ⚠️ Requires UI tweak |
| **Offline resilience** | Falls back to cached | Same | ✅ Same |
| **Accuracy** | Always current | Up to 15 min stale max | ⚠️ Minor degradation |

**Conclusion:** Weather works fine on-demand. Show "stale" indicator if cached > 15 min without refresh.

### 4.2 Clock Display
| Aspect | Always-On | On-Demand | Impact |
|--------|-----------|-----------|--------|
| **Sync source** | NTP (boot only) | RTC + local oscillator drift | ⚠️ Drift risk |
| **Accuracy** | ±1 sec (good) | ±5–10 sec/day without resync | ⚠️ Degraded |
| **Resync frequency** | Boot only | Every 6–12h (on-demand WiFi period) | ⚠️ More complexity |
| **Visual impact** | Exact time | Off by few seconds occasionally | Minor |

**Conclusion:** Clock needs occasional NTP resync (every 6–12h). Add "last NTP sync" timestamp to NVS.

### 4.3 Web Dashboard
| Aspect | Always-On | On-Demand | Impact |
|--------|-----------|-----------|--------|
| **Discovery** | Instant (mDNS always active) | Requires WiFi powered on | ❌ **Major change** |
| **Access latency** | <100ms | ~3–5s reconnect + load | ⚠️ Delayed first load |
| **Real-time updates** | WebSocket/SSE possible | Requires WiFi on constantly | ❌ **Breaks live features** |
| **User experience** | Always accessible | Only when WiFi actively on | ❌ **Poor UX** |

**Conclusion:** Web dashboard and live sync become **impractical** with pure on-demand WiFi.

### 4.4 OTA Updates
| Aspect | Always-On | On-Demand | Impact |
|--------|-----------|-----------|--------|
| **Availability** | Always reachable | Requires explicit WiFi on | ⚠️ User must trigger |
| **Background updates** | Possible (auto-check) | Must be manual or on timer | ⚠️ User responsibility |
| **Push capability** | Real-time | Not possible | ❌ No push |

**Conclusion:** OTA becomes manual (user initiates via dashboard button or timer).

### 4.5 Pomodoro Timer
| Aspect | Always-On | On-Demand | Impact |
|--------|-----------|-----------|--------|
| **Notifications** | Can push to cloud | Local only | ✅ No change (local alerts) |
| **Time accuracy** | ±1 sec (NTP) | ±5–10 sec/12h | ⚠️ Acceptable for timer |
| **Cloud sync** | Possible | Not practical | ✅ No feature loss (local only) |

**Conclusion:** Pomodoro works fine. Timer accuracy slightly degrades but remains usable.

### 4.6 Incoming Features: Home Automation (MQTT)
| Aspect | Always-On | On-Demand | Impact |
|--------|-----------|-----------|--------|
| **Subscription** | Always listening | Periodic polling instead | ⚠️ Delay up to 15 min |
| **Command latency** | <100ms response | ~3–5s WiFi reconnect | ⚠️ Sluggish |
| **Real-time reactions** | Yes | No | ❌ **Feature incompatible** |

**Conclusion:** MQTT real-time subscriptions **incompatible** with pure on-demand WiFi.

---

## 5. HYBRID APPROACH (RECOMMENDED)

Instead of pure on-demand, consider a **configurable hybrid**:

### Three WiFi Profiles (User-Selectable in Dashboard)

#### Profile A: Always-Connected (Current)
- **WiFi:** Always on
- **Power draw:** ~50–80 mA
- **Battery life:** Not suitable for mobile use
- **Use case:** USB-powered desk; always online needed
- **Features:** Full real-time dashboard, live sync, OTA push, MQTT subscriptions

#### Profile B: Smart On-Demand (Recommended)
- **WiFi:** On-demand for tasks (weather, NTP)
- **Periodic wake:** Every 15 min for weather fetch
- **Portal access:** User enables WiFi manually via hardware button or power cycle
- **Power draw:** ~15–30 mA average (mostly display)
- **Battery life:** 10–15 days on 2000 mAh USB battery
- **Use case:** Battery-powered portable mode; occasional dashboard access
- **Limitations:** Dashboard access requires manual WiFi enable; no live sync; no push OTA
- **Features enabled:** Mood, weather (cached), clock (with occasional resync), pomodoro, local animations

#### Profile C: Aggressive Power-Save (Deep Sleep)
- **WiFi:** Off completely (no recurring fetches)
- **Display:** Only updates on user interaction or scheduled wake
- **Deep sleep:** Enabled 10pm–7am (RTC wakeup)
- **Power draw:** <1 mA during sleep; 5–10 mA when active
- **Battery life:** 30–40 days on 2000 mAh
- **Use case:** Decorative desk buddy; minimal interactivity
- **Features enabled:** Static display, manual mood/emote, no weather; periodic wake for time adjustment

### Configuration

Store in NVS as `power_mode` key:
```cpp
enum PowerMode {
    POWER_ALWAYS_ON = 0,      // Profile A
    POWER_SMART_DEMAND = 1,   // Profile B (default)
    POWER_AGGRESSIVE = 2      // Profile C
};
```

Dashboard shows:
- **Power Mode selector** (radio buttons or dropdown)
- **Estimated daily battery life** for selected mode
- **Feature availability** (which features disabled in this mode)
- **Last WiFi activity timestamp**

---

## 6. IMPLEMENTATION ROADMAP

### Phase 1: Smart On-Demand (Profile B) — Recommended First Step

**What changes:**
1. **WiFi lifecycle:** Add WiFi off/on toggle in `wifi_manager.cpp`
   - New function: `WifiManager::enableWiFi(bool enable)` 
   - Calls `WiFi.mode(WIFI_OFF)` when disabled
   - Restores STA mode when re-enabled

2. **Task scheduler:** Trigger WiFi on at scheduled intervals
   ```cpp
   // In main loop
   if (timeForWeatherFetch()) {
       WifiManager::enableWiFi(true);  // turns on, tries to connect
       // Wait for connection (blocking or timeout 10s)
       if (connected) {
           WeatherService::fetch();
           time_sync_if_needed();
       }
       WifiManager::enableWiFi(false);  // turns off
   }
   ```

3. **NVS caching:** Extend `Settings` module
   - `Settings::setWeatherCache(Weather w)` + `getWeatherCache()`
   - `Settings::setLastNtpSync(uint32_t timestamp)`
   - `Settings::setLastWeatherFetch(uint32_t timestamp)` — for "Last updated: X ago" UI

4. **Display indicators:**
   - Top right corner: WiFi icon + "🔄" spinner when fetching
   - When offline for >15 min: "Last update: 15 min ago" on weather card
   - Time shows "(~sync)" if not synced recently

5. **Portal access (on-demand):**
   - Add NVS flag `portal_enabled`: defaults true
   - User can disable portal (saves 40s boot timeout if offline)
   - Manual wake: unplug/replug or power-button long-press (if button added)

**Code changes:**
- `include/net/wifi_manager.h`: Add `enableWiFi()`, `isWiFiEnabled()`
- `src/net/wifi_manager.cpp`: Implement WiFi on/off toggle; update `loop()` to skip modem sleep setup if WiFi disabled
- `include/core/settings.h`: New NVS keys `weather_cache`, `last_ntp_sync`, `last_weather_fetch`, `power_mode`
- `src/core/settings.cpp`: Cache get/set methods
- `include/display/screen_header.h`: Add "last updated X min ago" logic
- `src/net/web_portal.cpp`: Add power mode selector + estimated battery life display
- `src/main.cpp`: Add `checkFetchSchedule()` in loop; call `enableWiFi()` conditionally

**Testing needed:**
- [ ] Power draw measurement (should drop from 50 mA to ~15 mA average)
- [ ] WiFi reconnection success rate after 15 min offline
- [ ] Weather staleness indicator accuracy
- [ ] NTP resync logic (sync every 6–12h without manual trigger)
- [ ] Battery life test on USB battery (target: 10–15 days)

### Phase 2: Always-Connected Mode Option (Profile A)
- Add UI toggle to disable on-demand mode
- Keep as default for users who prioritize real-time features
- Web dashboard + live sync only work in this mode

### Phase 3: Aggressive Power-Save (Profile C)
- Deep sleep integration (requires RTC wakeup timer)
- Display only wakes on scheduled times or user interrupt
- Complex; defer to v2.0

---

## 7. DECISION MATRIX: WHICH APPROACH?

### If DeskBuddy is USB-powered (desk use)
→ **Stick with Always-Connected (Profile A)**
- Power is unlimited
- Real-time features valuable
- Complexity not worth it
- **Action:** Optimize modem sleep settings; no major WiFi refactor needed

### If DeskBuddy is Battery-Powered (portable, occasional sync)
→ **Use Smart On-Demand (Profile B)**
- 85% power savings vs always-on
- Battery life improves from ~1 day to 10–15 days
- Trade-off: No real-time dashboard; manual portal access
- **Action:** Implement Phase 1 above

### If DeskBuddy is Low-Power / Long-Term Deployed
→ **Use Aggressive Power-Save (Profile C + B hybrid)**
- Requires deep sleep RTC
- Battery lasts 30–40 days
- Most features disabled; display only on schedule
- **Action:** Implement Phase 1 + 3 (future)

---

## 8. RECOMMENDATION

### For DeskBuddy Today:
**Start with Smart On-Demand (Profile B)** as the **default**, with option to revert to Always-Connected.

**Rationale:**
1. **Biggest impact:** 85% power reduction without losing core features
2. **Balanced trade-off:** Weather + clock + moods still work; just not real-time sync
3. **Sustainable:** Works for both battery and USB power (USB power can just disable feature)
4. **User choice:** Users who want always-on can toggle it; users who want battery life get default
5. **Phased complexity:** Phase 1 is straightforward (one task scheduler + WiFi toggle)
6. **Future-proof:** Enables Phase 3 (deep sleep) later

### Fallback Decision Tree:
```
Is user's DeskBuddy USB-powered?
    → YES: Keep Profile A (always-on); optimize modem sleep only
    → NO: Is battery-powered?
        → YES: Use Profile B (smart on-demand); ~10–15 day battery life
        → NO: Is long-term deployed?
            → YES: Plan Profile C (deep sleep); battery lasts months
            → NO: Default to Profile B; user can switch in settings
```

---

## 9. RISKS & MITIGATIONS

| Risk | Impact | Mitigation |
|------|--------|-----------|
| **WiFi reconnect fails** | No weather, clock drifts | Add exponential backoff; show error UI; fall back to cached data |
| **User confusion:** "Why no dashboard?" | Support load | Add in-app help text; "Power Mode" clearly labeled with trade-offs |
| **Clock drifts visibly** | User trust loss | Resync NTP every 6–12h; show "~" icon if time uncertain |
| **Weather data very stale** | User frustration | Limit cache age to 15 min; warn if older ("Last updated 20 min ago") |
| **Pomodoro timer accuracy affected** | Feature quality | Drift is acceptable (<5 sec per 25 min session); document in FAQ |
| **OTA updates become manual** | Users miss patches | Add dashboard "Check for Updates" button; auto-check on WiFi enable |

---

## 10. SUMMARY TABLE

| Aspect | Always-Connected | Smart On-Demand | Aggressive Sleep |
|--------|------------------|-----------------|------------------|
| **Daily power draw** | 1200–1950 mAh | 184–276 mAh | <50 mAh |
| **Typical battery life** | ~1 day (USB-only) | 10–15 days | 30–40 days |
| **Code complexity** | Simple (current) | Medium (task scheduler + WiFi toggle) | High (deep sleep RTC) |
| **Dashboard access** | Instant, real-time | Requires manual WiFi enable; ~3s load | Requires device wake; ~10s |
| **Weather freshness** | Live (15 min) | Cached, max 15 min stale | Cached, potentially hours stale |
| **Clock accuracy** | ±1 sec | ±5–10 sec/12h | ±30 sec/day without sync |
| **OTA updates** | Can auto-push | Manual or on-timer | Manual only |
| **MQTT subscriptions** | Real-time | Polling (delay up to 15 min) | Not practical |
| **Best use case** | Desk; always online | Portable; battery-powered | Long-term; minimal interaction |

---

## 11. QUICK-WIN: What to Do **Right Now** (No Major Refactor)

If you want immediate power savings **without** implementing full on-demand WiFi:

1. **Enable modem sleep aggressively:** Already done; reduce DTIM beacon interval
2. **Disable WiFi during sleep hours:** 10pm–7am, WiFi off; manual enable possible
3. **Reduce weather poll:** 15 min → 30 min (halves WiFi activity)
4. **Lazy NTP sync:** Sync 1×/day instead of boot (reduces reconnect storms)
5. **WiFi TX power already optimized:** `WIFI_POWER_8_5dBm` is set

**Power savings from above:** ~30–40% reduction (half-measure vs full 85% on-demand).

---

## NEXT STEPS

1. **Measure current power draw** with USB power meter (baseline)
2. **Choose profile** based on use case (USB vs battery)
3. **If battery-powered:** Implement Phase 1 (Smart On-Demand)
   - Effort: ~4–6 hours
   - Power savings: **85%**
   - Test with USB battery for 24h; measure discharge rate
4. **If USB-powered:** Stick with optimized always-on; focus on other efficiency wins
5. **Future:** Add user-selectable power mode in dashboard (let them choose A/B/C)

