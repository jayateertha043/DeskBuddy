#include "net/web_portal.h"

#include "config.h"
#include "app_state.h"
#include "core/settings.h"
#include "core/util.h"
#include "display/emote_director.h"
#include "net/weather_service.h"
#include "net/wifi_manager.h"
#include "pomodoro.h"

namespace WebPortal
{
    namespace
    {
        WebServerClass server(80);
        bool webStarted = false;

        void handleRoot()
        {
            const Weather &weather = WeatherService::data();
            const bool online = WiFi.status() == WL_CONNECTED;
            String page = F("<!doctype html><html><head><meta charset=utf-8><meta name=viewport content='width=device-width,initial-scale=1'><title>DeskBuddy</title><style>body{font:15px system-ui;margin:0;background:#10131c;color:#f7f8fc}main{max-width:460px;margin:auto;padding:24px 18px}h1{font-size:22px;margin:0 0 4px}.s{color:#a9b0c3;margin:0 0 18px}label{display:block;color:#a9b0c3;font-size:13px;margin:14px 0 6px}input{width:100%;box-sizing:border-box;padding:11px;border-radius:10px;border:1px solid #424b67;background:#111522;color:#f7f8fc;font:inherit}button{font:inherit;cursor:pointer}.save{width:100%;box-sizing:border-box;padding:11px;border-radius:10px;margin-top:18px;background:#67e8c2;color:#08120f;font-weight:700;border:0}.card{background:#1b2030;border:1px solid #343b53;border-radius:16px;padding:20px}.emotions{margin-top:14px}.emotions h2{font-size:17px;margin:0 0 4px}.mode{display:grid;grid-template-columns:1fr 1fr;gap:8px;margin:14px 0}.mode label{margin:0;padding:10px;border:1px solid #424b67;border-radius:10px;background:#111522;color:#f7f8fc;text-align:center;cursor:pointer}.mode input{width:auto;margin-right:6px}.emotes{display:grid;grid-template-columns:repeat(2,minmax(0,1fr));gap:9px}.emote{min-height:76px;border:1px solid #424b67;border-radius:12px;background:#111522;color:#f7f8fc;padding:9px 5px}.emote.sel{border-color:#67e8c2;box-shadow:0 0 0 1px #67e8c2 inset}.face{display:block;color:#67e8c2;font:700 18px monospace;margin-bottom:5px}.hint{color:#a9b0c3;font-size:12px;margin:0 0 12px}.st{padding:10px 12px;background:#121724;border-radius:10px;margin-bottom:14px;color:#a9b0c3}.lrow{display:flex;align-items:center;gap:8px;padding:6px 0;border-bottom:1px solid #2a3147}.lrow.off{opacity:.45}.llabel{flex:1}.lbtn{background:#111522;color:#f7f8fc;border:1px solid #424b67;border-radius:8px;padding:4px 9px;font:inherit}.lbtn:disabled{opacity:.3}</style></head><body><main><h1>DeskBuddy</h1><p class=s>Status &amp; settings</p><div class=card>");
            page += F("<div class=st>Status: ");
            if (online)
            {
                page += F("Online \xC2\xB7 ");
                page += WiFi.localIP().toString();
            }
            else
            {
                page += F("Offline, trying to connect\xE2\x80\xA6");
            }
            if (weather.valid)
            {
                char temp[8];
                snprintf(temp, sizeof(temp), " \xC2\xB7 %.1fC ", weather.temperature);
                page += temp;
                page += weather.summary;
            }
            page += F(" \xC2\xB7 ");
            page += Util::htmlEscape(Settings::status());
            page += F("</div><form method=POST action=/save><label>Wi-Fi network</label><input name=ssid maxlength=32 required value='");
            page += Util::htmlEscape(Settings::ssid());
            page += F("'><label>Wi-Fi password</label><input name=pass type=password maxlength=63 placeholder='Leave blank to keep current'><label>City</label><input name=city maxlength=32 required value='");
            page += Util::htmlEscape(Settings::city());
            page += F("'><label>Country (name or code)</label><input name=country maxlength=32 required value='");
            page += Util::htmlEscape(Settings::country());
            page += F("'><label>Your name</label><input name=name maxlength=20 placeholder='Shown as \"Hi name!\"' value='");
            page += Util::htmlEscape(Settings::name());
            page += F("'><button class=save type=submit>Save &amp; connect</button></form></div><div class='card emotions'><h2>Emotes</h2><p class=hint>Static holds face. Random plays emote 10s every 10min. Loop plays your chosen playlist.</p><form method=POST action=/emote><div class=mode><label><input type=radio name=mode value=static");
            if (Settings::emoteMode() == EMOTE_STATIC)
                page += F(" checked");
            page += F(">Static</label><label><input type=radio name=mode value=random");
            if (Settings::emoteMode() == EMOTE_RANDOM)
                page += F(" checked");
            page += F(">Random</label><label><input type=radio name=mode value=loop");
            if (Settings::emoteMode() == EMOTE_LOOP)
                page += F(" checked");
            page += F(">Loop</label></div><div class=emotes>");
            for (uint8_t i = 0; i < MOOD_COUNT; ++i)
            {
                page += F("<button class='emote");
                if (i == Settings::mood())
                    page += F(" sel");
                page += F("' type=submit name=mood value='");
                page += MOOD_SLUG[i];
                page += F("'><span class=face>");
                page += Util::htmlEscape(String(MOOD_ICON[i]));
                page += F("</span>");
                page += MOOD_LABEL[i];
                page += F("</button>");
            }
            page += F("</div><button class=save type=submit>Save behavior</button></form></div>");
            page += F("<div class='card' id=loopcard><h2>Loop playlist</h2><p class=hint>Tick emotes (incl. Auto weather) for Loop mode and use the arrows to set order.</p><div class=mode><button type=button class=lbtn id=loopAll>Select all</button><button type=button class=lbtn id=loopNone>Remove all</button></div><div id=loopEditor></div>");
            char loopSecBuf[6];
            snprintf(loopSecBuf, sizeof(loopSecBuf), "%u", Settings::loopSeconds());
            page += F("<label>Time per emote \xC2\xB7 <b id=lsv>");
            page += loopSecBuf;
            page += F("</b> s</label><input type=range id=loopSecs min=3 max=120 step=1 value=");
            page += loopSecBuf;
            page += F(" oninput=\"document.getElementById('lsv').textContent=this.value\"><button class=save id=loopSave>Save loop</button><p class=hint id=loopMsg></p></div>");
            page += F("<div class=card><h2>Focus timer</h2><p class=hint>Pomodoro with hourglass. Buddy celebrates when done.</p>");
            if (Pomodoro::running())
            {
                const uint32_t totalSec = (Pomodoro::remainingMs() + 999) / 1000;
                char timer[16];
                snprintf(timer, sizeof(timer), "<div class=st>Running \xC2\xB7 %u:%02u left</div>", totalSec / 60, totalSec % 60);
                page += timer;
            }
            char pomMin[4];
            snprintf(pomMin, sizeof(pomMin), "%u", Settings::pomodoroMinutes());
            page += F("<form method=POST action=/pomodoro><label>Session length \xC2\xB7 <b id=mv>");
            page += pomMin;
            page += F("</b> min</label><input type=range name=mins min=1 max=60 step=1 value=");
            page += pomMin;
            page += F(" oninput=\"document.getElementById('mv').textContent=this.value\"><div class=mode><button class=save style='margin:0' type=submit>Start</button><button class=save style='margin:0;background:#ff6b6b' type=submit name=action value=stop>Stop</button></div></form></div>");
            char brPct[4];
            snprintf(brPct, sizeof(brPct), "%u", Settings::brightnessPercent());
            page += F("<div class=card><h2>Display brightness</h2><p class=hint>Adjusts OLED contrast. Applies live; auto-dims further at night.</p><label>Level \xC2\xB7 <b id=bv>");
            page += brPct;
            page += F("</b>%</label><input type=range min=5 max=100 step=5 value=");
            page += brPct;
            page += F(" oninput=\"document.getElementById('bv').textContent=this.value\" onchange=\"fetch('/brightness',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:'level='+this.value})\"></div>");
            page += F("<p class=s style='text-align:center;margin-top:16px'><a style='color:#67e8c2' href='/health'>System health &amp; diagnostics \xE2\x86\x92</a></p>");
            page += F("</main><script>const moods=[");
            for (uint8_t i = 0; i < MOOD_COUNT; ++i)
            {
                if (i > 0)
                    page += ',';
                char moodObj[64];
                snprintf(moodObj, sizeof(moodObj), "{idx:%u,slug:'%s',label:'%s'}", i, MOOD_SLUG[i], MOOD_LABEL[i]);
                page += moodObj;
            }
            page += F("];var loopSeq=[");
            for (uint8_t i = 0; i < Settings::loopCount(); ++i)
            {
                if (i > 0)
                    page += ',';
                page += Settings::loopAt(i);
            }
            page += F("];");
            page += F("(function(){var cand=moods.filter(function(m){return m.idx>=0;});var items=[];loopSeq.forEach(function(ix){var m=cand.find(function(c){return c.idx==ix;});if(m)items.push({idx:m.idx,label:m.label,on:true});});cand.forEach(function(m){if(!items.some(function(it){return it.idx==m.idx;}))items.push({idx:m.idx,label:m.label,on:false});});var ed=document.getElementById('loopEditor');function render(){ed.innerHTML='';items.forEach(function(it,i){var row=document.createElement('div');row.className='lrow'+(it.on?'':' off');var cb=document.createElement('input');cb.type='checkbox';cb.checked=it.on;cb.onchange=function(){it.on=cb.checked;render();};var sp=document.createElement('span');sp.textContent=it.label;sp.className='llabel';var up=document.createElement('button');up.type='button';up.className='lbtn';up.textContent='\\u25B2';up.disabled=i==0;up.onclick=function(){var t=items[i-1];items[i-1]=items[i];items[i]=t;render();};var dn=document.createElement('button');dn.type='button';dn.className='lbtn';dn.textContent='\\u25BC';dn.disabled=i==items.length-1;dn.onclick=function(){var t=items[i+1];items[i+1]=items[i];items[i]=t;render();};row.appendChild(cb);row.appendChild(sp);row.appendChild(up);row.appendChild(dn);ed.appendChild(row);});}render();document.getElementById('loopAll').onclick=function(){items.forEach(function(it){it.on=true;});render();};document.getElementById('loopNone').onclick=function(){items.forEach(function(it){it.on=false;});render();};document.getElementById('loopSave').onclick=function(){var seq=items.filter(function(it){return it.on;}).map(function(it){return it.idx;});if(!seq.length){document.getElementById('loopMsg').textContent='Pick at least one emote.';return;}var secs=document.getElementById('loopSecs').value;fetch('/loop',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:'seq='+seq.join(',')+'&secs='+secs}).then(function(r){return r.text();}).then(function(t){document.getElementById('loopMsg').textContent=t;});};})();");
            page += F("</script></body></html>");
            server.send(200, F("text/html"), page);
        }

        // Loop-frequency tracking: a live proxy for how responsive the firmware
        // is. Sampled once per second from handle().
        uint32_t loopTicks = 0;
        uint32_t loopHz = 0;
        uint32_t loopWindowStart = 0;

        String uptimeString(uint32_t ms)
        {
            uint32_t s = ms / 1000;
            const uint32_t d = s / 86400;
            s %= 86400;
            const uint32_t h = s / 3600;
            s %= 3600;
            const uint32_t m = s / 60;
            s %= 60;
            char buf[32];
            if (d > 0)
                snprintf(buf, sizeof(buf), "%ud %uh %um %us", d, h, m, s);
            else
                snprintf(buf, sizeof(buf), "%uh %um %us", h, m, s);
            return String(buf);
        }

        // Model-based current estimate (mA @ 3.3V). The Super Mini has no shunt
        // or fuel gauge, so we sum ESP32-C3 datasheet typical figures for the
        // states we CAN read at runtime: CPU clock + radio mode/modem-sleep +
        // link quality. Returns base (CPU/core) and radio components separately.
        void estimateCurrent(float &baseMa, float &radioMa)
        {
            const uint32_t mhz = ESP.getCpuFreqMHz();
            // Core/digital draw scales with CPU clock (radio idle baseline).
            if (mhz >= 160)
                baseMa = 22.0f;
            else if (mhz >= 80)
                baseMa = 16.0f;
            else
                baseMa = 12.0f;

            radioMa = 0.0f;
#if defined(ARDUINO_ARCH_ESP8266)
            const bool sta = (WiFi.status() == WL_CONNECTED);
            const bool ap = (WiFi.getMode() & WIFI_AP) != 0;
            const bool modemSleep = (WiFi.getSleepMode() != WIFI_NONE_SLEEP);
#else
            const bool sta = (WiFi.status() == WL_CONNECTED);
            const wifi_mode_t mode = WiFi.getMode();
            const bool ap = (mode == WIFI_AP || mode == WIFI_AP_STA);
            const bool modemSleep = WiFi.getSleep();
#endif
            if (ap)
            {
                radioMa = 80.0f; // SoftAP keeps the radio fully awake (continuous RX)
            }
            else if (sta)
            {
                // Connected. With modem-sleep the radio only wakes at DTIM
                // beacons, so the average is far below continuous-RX (~80 mA).
                radioMa = modemSleep ? 15.0f : 80.0f;
                const long rssi = WiFi.RSSI(); // weak link -> higher TX / retries
                if (rssi < -75)
                    radioMa += 15.0f;
                else if (rssi < -60)
                    radioMa += 5.0f;
            }
            else
            {
                radioMa = 70.0f; // scanning / associating: radio active
            }
        }

        // Builds a JSON snapshot of every system parameter we can read on-device.
        String buildHealthJson()
        {
            const bool online = WiFi.status() == WL_CONNECTED;
            String j;
            j.reserve(768);
            j += F("{\"chip\":{");
#if defined(ARDUINO_ARCH_ESP8266)
            j += F("\"model\":\"ESP8266\",\"cores\":1,\"cpu_mhz\":");
            j += ESP.getCpuFreqMHz();
            j += F(",\"sdk\":\"");
            j += ESP.getCoreVersion();
            j += F("\"");
#else
            j += F("\"model\":\"");
            j += ESP.getChipModel();
            j += F("\",\"revision\":");
            j += ESP.getChipRevision();
            j += F(",\"cores\":");
            j += ESP.getChipCores();
            j += F(",\"cpu_mhz\":");
            j += ESP.getCpuFreqMHz();
            j += F(",\"sdk\":\"");
            j += ESP.getSdkVersion();
            j += F("\"");
#endif
            j += F("},");

            // Uptime + loop responsiveness (our best on-device "CPU" indicator).
            const uint32_t up = millis();
            j += F("\"uptime\":{\"ms\":");
            j += up;
            j += F(",\"human\":\"");
            j += uptimeString(up);
            j += F("\"},\"cpu\":{\"freq_mhz\":");
            j += ESP.getCpuFreqMHz();
            j += F(",\"loop_hz\":");
            j += loopHz;
            j += F("},");

            // Heap / RAM.
            const uint32_t heapFree = ESP.getFreeHeap();
#if defined(ARDUINO_ARCH_ESP8266)
            j += F("\"heap\":{\"free\":");
            j += heapFree;
            j += F(",\"max_alloc\":");
            j += ESP.getMaxFreeBlockSize();
            j += F(",\"frag_pct\":");
            j += ESP.getHeapFragmentation();
            j += F("},");
#else
            const uint32_t heapTotal = ESP.getHeapSize();
            j += F("\"heap\":{\"free\":");
            j += heapFree;
            j += F(",\"total\":");
            j += heapTotal;
            j += F(",\"min_free\":");
            j += ESP.getMinFreeHeap();
            j += F(",\"max_alloc\":");
            j += ESP.getMaxAllocHeap();
            j += F(",\"used_pct\":");
            j += heapTotal ? (100.0f * (heapTotal - heapFree) / heapTotal) : 0.0f;
            j += F("},");
#endif

            // Flash / sketch.
            const uint32_t sketchSize = ESP.getSketchSize();
            const uint32_t sketchFree = ESP.getFreeSketchSpace();
            j += F("\"flash\":{\"chip_size\":");
            j += ESP.getFlashChipSize();
            j += F(",\"sketch_size\":");
            j += sketchSize;
            j += F(",\"free_ota\":");
            j += sketchFree;
            j += F("},");

            // Temperature: ESP32-C3 has an internal sensor; ESP8266 does not.
#if defined(ARDUINO_ARCH_ESP8266)
            j += F("\"temperature_c\":null,");
#else
            j += F("\"temperature_c\":");
            j += temperatureRead();
            j += F(",");
#endif

            // Power: the Super Mini has no onboard current/voltage monitor, so
            // these are modeled from ESP32-C3 datasheet typicals, not measured.
            float baseMa = 0.0f, radioMa = 0.0f;
            estimateCurrent(baseMa, radioMa);
            const float estMa = baseMa + radioMa;
            j += F("\"power\":{\"supply_nominal_v\":3.3,\"estimated_ma\":");
            j += estMa;
            j += F(",\"cpu_ma\":");
            j += baseMa;
            j += F(",\"radio_ma\":");
            j += radioMa;
            j += F(",\"est_power_mw\":");
            j += estMa * 3.3f;
            j += F(",\"measured\":false,\"method\":\"datasheet model (CPU freq + radio state + RSSI)\"},");

            // Wi-Fi / network.
            j += F("\"wifi\":{\"connected\":");
            j += online ? F("true") : F("false");
            if (online)
            {
                j += F(",\"ssid\":\"");
                j += Util::htmlEscape(WiFi.SSID());
                j += F("\",\"ip\":\"");
                j += WiFi.localIP().toString();
                j += F("\",\"rssi\":");
                j += WiFi.RSSI();
            }
            j += F(",\"mac\":\"");
            j += WiFi.macAddress();
            j += F("\"}}");
            return j;
        }

        void handleHealthJson()
        {
            server.send(200, F("application/json"), buildHealthJson());
        }

        void handleHealthPage()
        {
            String page = F("<!doctype html><html><head><meta charset=utf-8><meta name=viewport content='width=device-width,initial-scale=1'><title>DeskBuddy Health</title><style>body{font:15px system-ui;margin:0;background:#10131c;color:#f7f8fc}main{max-width:460px;margin:auto;padding:24px 18px}h1{font-size:22px;margin:0 0 4px}.s{color:#a9b0c3;margin:0 0 18px}.card{background:#1b2030;border:1px solid #343b53;border-radius:16px;padding:16px 18px;margin-bottom:14px}.card h2{font-size:15px;margin:0 0 10px;color:#67e8c2}.row{display:flex;justify-content:space-between;padding:5px 0;border-bottom:1px solid #262c40;font-size:14px}.row:last-child{border-bottom:0}.k{color:#a9b0c3}.v{font-weight:600;font-family:monospace}button{font:inherit;cursor:pointer;width:100%;box-sizing:border-box;padding:11px;border-radius:10px;background:#67e8c2;color:#08120f;font-weight:700;border:0;margin-bottom:14px}a{color:#67e8c2}</style></head><body><main><h1>System Health</h1><p class=s>Live device parameters \xC2\xB7 on demand</p><button onclick=load()>Refresh</button><div id=out>Loading\xE2\x80\xA6</div><p class=s><a href='/'>\xE2\x86\x90 Dashboard</a> \xC2\xB7 <a href='/api/health'>Raw JSON</a></p></main><script>");
            page += F("function b(v){return(v/1024).toFixed(1)+' KB'}function row(k,v){return '<div class=row><span class=k>'+k+'</span><span class=v>'+v+'</span></div>'}");
            page += F("function card(t,h){return '<div class=card><h2>'+t+'</h2>'+h+'</div>'}");
            page += F("async function load(){document.getElementById('out').innerHTML='Loading\\u2026';try{const d=await(await fetch('/api/health',{cache:'no-store'})).json();let o='';");
            page += F("o+=card('Chip',row('Model',d.chip.model)+ (d.chip.revision!=null?row('Revision',d.chip.revision):'')+row('Cores',d.chip.cores)+row('SDK',d.chip.sdk));");
            page += F("o+=card('CPU / Uptime',row('CPU freq',d.cpu.freq_mhz+' MHz')+row('Loop rate',d.cpu.loop_hz+' Hz')+row('Uptime',d.uptime.human));");
            page += F("let hh=row('Free',b(d.heap.free))+(d.heap.total!=null?row('Total',b(d.heap.total))+row('Used',d.heap.used_pct.toFixed(1)+'%')+row('Min free',b(d.heap.min_free)):'')+row('Max alloc',b(d.heap.max_alloc))+(d.heap.frag_pct!=null?row('Fragmentation',d.heap.frag_pct+'%'):'');o+=card('Memory (RAM)',hh);");
            page += F("o+=card('Flash',row('Chip size',b(d.flash.chip_size))+row('Sketch',b(d.flash.sketch_size))+row('Free (OTA)',b(d.flash.free_ota)));");
            page += F("let pw=row('Supply',d.power.supply_nominal_v+' V')+row('Est. current',d.power.estimated_ma.toFixed(0)+' mA')+row('\\u2514 CPU/core',d.power.cpu_ma.toFixed(0)+' mA')+row('\\u2514 Radio',d.power.radio_ma.toFixed(0)+' mA')+row('Est. power',d.power.est_power_mw.toFixed(0)+' mW')+row('Measured',d.power.measured?'yes':'no')+row('Method',d.power.method);o+=card('Power (estimated)',pw);");
            page += F("if(d.temperature_c!=null)o+=card('Temperature',row('Core',d.temperature_c.toFixed(1)+' \\u00B0C'));");
            page += F("let w=row('Connected',d.wifi.connected?'yes':'no')+(d.wifi.connected?row('SSID',d.wifi.ssid)+row('IP',d.wifi.ip)+row('RSSI',d.wifi.rssi+' dBm'):'')+row('MAC',d.wifi.mac);o+=card('Wi-Fi',w);");
            page += F("document.getElementById('out').innerHTML=o;}catch(e){document.getElementById('out').innerHTML='Error: '+e;}}load();</script></body></html>");
            server.send(200, F("text/html"), page);
        }

        void redirectHome()
        {
            server.sendHeader(F("Location"), F("/"), true);
            server.send(303, F("text/plain"), F("Updated"));
        }

        void handlePomodoro()
        {
            if (server.hasArg("action") && server.arg("action") == F("stop"))
            {
                Pomodoro::cancel();
            }
            else if (server.hasArg("mins"))
            {
                long minutes = server.arg("mins").toInt();
                if (minutes < 1)
                    minutes = 25;
                if (minutes > 180)
                    minutes = 180;
                Pomodoro::start(static_cast<uint16_t>(minutes));
            }
            else
            {
                server.send(400, F("text/plain"), F("Missing action"));
                return;
            }
            redirectHome();
        }

        void handleEmote()
        {
            const String mode = server.arg("mode");
            if (mode != F("static") && mode != F("random") && mode != F("loop"))
            {
                server.send(400, F("text/plain"), F("Choose Static, Random, or Loop"));
                return;
            }

            uint8_t requestedMode;
            if (mode == F("static"))
                requestedMode = EMOTE_STATIC;
            else if (mode == F("random"))
                requestedMode = EMOTE_RANDOM;
            else // "loop"
                requestedMode = EMOTE_LOOP;

            const uint8_t currentMode = Settings::emoteMode();
            const bool modeChanged = requestedMode != currentMode;
            const bool moodChosen = server.hasArg("mood");
            uint8_t requestedMood = Settings::mood();
            if (moodChosen)
            {
                requestedMood = moodFromSlug(server.arg("mood"));
                if (requestedMood >= MOOD_COUNT)
                {
                    server.send(400, F("text/plain"), F("Unknown emote"));
                    return;
                }
                Settings::saveMood(requestedMood);
            }
            else if (modeChanged && requestedMode == EMOTE_STATIC)
            {
                Settings::saveMood(Settings::mood());
            }

            if (modeChanged)
            {
                Settings::saveEmoteMode(requestedMode);
                EmoteDirector::resetSchedule();
            }

            if (requestedMode == EMOTE_RANDOM)
            {
                EmoteDirector::setNextInterval();
                if (moodChosen && requestedMood != MOOD_AUTO)
                {
                    EmoteDirector::startReaction(requestedMood);
                }
                else
                {
                    Settings::setMood(MOOD_AUTO);
                    EmoteDirector::clearReaction();
                }
            }
            else if (requestedMode == EMOTE_LOOP)
            {
                Settings::setMood(MOOD_AUTO);
                EmoteDirector::clearReaction();
            }
            redirectHome();
        }

        void handleBrightness()
        {
            if (!server.hasArg("level"))
            {
                server.send(400, F("text/plain"), F("Missing level"));
                return;
            }
            long level = server.arg("level").toInt();
            if (level < 5)
                level = 5;
            if (level > 100)
                level = 100;
            Settings::saveBrightness(static_cast<uint8_t>(level));
            server.send(200, F("text/plain"), F("OK"));
        }

        void handleLoop()
        {
            if (!server.hasArg("seq"))
            {
                server.send(400, F("text/plain"), F("Missing seq"));
                return;
            }
            if (server.hasArg("secs"))
                Settings::saveLoopSeconds(static_cast<uint16_t>(server.arg("secs").toInt()));
            Settings::saveLoopSequence(server.arg("seq"));
            EmoteDirector::resetSchedule();
            String msg = F("Saved ");
            msg += Settings::loopCount();
            msg += F(" emotes, ");
            msg += Settings::loopSeconds();
            msg += F("s each.");
            server.send(200, F("text/plain"), msg);
        }

        void handleSave()
        {
            String ssid = server.arg("ssid");
            String rawPass = server.arg("pass");
            String city = server.arg("city");
            String country = server.arg("country");
            String uname = server.arg("name");
            ssid.trim();
            city.trim();
            country.trim();
            uname.trim();
            if (!ssid.length() || ssid.length() > 32 || rawPass.length() > 63 ||
                !city.length() || city.length() > 32 || !country.length() || country.length() > 32 ||
                uname.length() > 20)
            {
                server.send(400, F("text/plain"), F("Invalid input"));
                return;
            }

            // Apply only the fields that actually changed.
            const bool ssidChanged = ssid != Settings::ssid();
            const bool passChanged = rawPass.length() && rawPass != Settings::pass();
            const bool wifiChanged = ssidChanged || passChanged;
            const bool locChanged = !city.equalsIgnoreCase(Settings::city()) ||
                                    !country.equalsIgnoreCase(Settings::country());
            const bool nameChanged = uname != Settings::name();

            if (wifiChanged)
                Settings::saveCreds(ssid, passChanged ? rawPass : Settings::pass());
            if (nameChanged)
                Settings::saveName(uname);
            if (locChanged)
            {
                Settings::saveLocationInput(city, country);
                WeatherService::requestLocationRefresh();
            }

            String msg = F("Saved");
            if (wifiChanged)
                msg += F(" \xC2\xB7 reconnecting Wi-Fi");
            else if (locChanged)
                msg += F(" \xC2\xB7 updating location");
            String resp = F("<!doctype html><meta charset=utf-8><meta name=viewport content='width=device-width,initial-scale=1'><body style='font:15px system-ui;background:#10131c;color:#f7f8fc;padding:24px'>");
            resp += msg;
            resp += F(". <a style='color:#67e8c2' href='/'>Back</a>");
            server.send(200, F("text/html"), resp);

            if (wifiChanged)
            {
                delay(150);
                WifiManager::reconnect(); // only reconnect when Wi-Fi credentials changed
            }
        }

    }

    void start()
    {
        if (webStarted)
            return;
        server.on("/", handleRoot);
        server.on("/save", HTTP_POST, handleSave);
        server.on("/emote", HTTP_POST, handleEmote);
        server.on("/pomodoro", HTTP_POST, handlePomodoro);
        server.on("/brightness", HTTP_POST, handleBrightness);
        server.on("/loop", HTTP_POST, handleLoop);
        server.on("/health", handleHealthPage);
        server.on("/api/health", handleHealthJson);
        server.onNotFound(handleRoot); // serves dashboard + captive-portal catch-all
        server.begin();
        webStarted = true;
    }

    void handle()
    {
        if (!webStarted)
            return;
        server.handleClient();
        // Sample loop frequency once per second as a live responsiveness metric.
        ++loopTicks;
        const uint32_t now = millis();
        if (now - loopWindowStart >= 1000)
        {
            loopHz = loopTicks;
            loopTicks = 0;
            loopWindowStart = now;
        }
    }

    bool started() { return webStarted; }
}
