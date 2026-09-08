#include "net/web_portal.h"

#include "config.h"
#include "app_state.h"
#include "core/settings.h"
#include "core/util.h"
#include "display/emote_director.h"
#include "net/weather_service.h"
#include "net/wifi_manager.h"

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
      String page = F("<!doctype html><html><head><meta charset=utf-8>"
                      "<meta name=viewport content='width=device-width,initial-scale=1'>"
                      "<title>DeskBuddy</title><style>"
                      "body{font:15px system-ui;margin:0;background:#10131c;color:#f7f8fc}"
                      "main{max-width:460px;margin:auto;padding:24px 18px}"
                      "h1{font-size:22px;margin:0 0 4px}.s{color:#a9b0c3;margin:0 0 18px}"
                      "label{display:block;color:#a9b0c3;font-size:13px;margin:14px 0 6px}"
                      "input{width:100%;box-sizing:border-box;padding:11px;border-radius:10px;"
                      "border:1px solid #424b67;background:#111522;color:#f7f8fc;font:inherit}"
                      "button{font:inherit;cursor:pointer}.save{width:100%;box-sizing:border-box;padding:11px;"
                      "border-radius:10px;margin-top:18px;background:#67e8c2;color:#08120f;font-weight:700;border:0}"
                      ".card{background:#1b2030;border:1px solid #343b53;border-radius:16px;padding:20px}"
                      ".emotions{margin-top:14px}.emotions h2{font-size:17px;margin:0 0 4px}"
                      ".mode{display:grid;grid-template-columns:1fr 1fr;gap:8px;margin:14px 0}"
                      ".mode label{margin:0;padding:10px;border:1px solid #424b67;border-radius:10px;"
                      "background:#111522;color:#f7f8fc;text-align:center;cursor:pointer}"
                      ".mode input{width:auto;margin-right:6px}.emotes{display:grid;"
                      "grid-template-columns:repeat(2,minmax(0,1fr));gap:9px}"
                      ".emote{min-height:76px;border:1px solid #424b67;border-radius:12px;"
                      "background:#111522;color:#f7f8fc;padding:9px 5px}"
                      ".emote.sel{border-color:#67e8c2;box-shadow:0 0 0 1px #67e8c2 inset}"
                      ".face{display:block;color:#67e8c2;font:700 18px monospace;margin-bottom:5px}"
                      ".hint{color:#a9b0c3;font-size:12px;margin:0 0 12px}"
                      ".st{padding:10px 12px;background:#121724;border-radius:10px;margin-bottom:14px;color:#a9b0c3}"
                      "</style></head><body><main>"
                      "<h1>DeskBuddy</h1><p class=s>Status &amp; settings</p><div class=card>");
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
        page += F(" \xC2\xB7 ");
        page += String(weather.temperature, 1);
        page += F("C ");
        page += weather.summary;
      }
      page += F(" \xC2\xB7 ");
      page += Util::htmlEscape(Settings::status());
      page += F("</div><form method=POST action=/save>"
                "<label>Wi-Fi network</label>"
                "<input name=ssid maxlength=32 required value='");
      page += Util::htmlEscape(Settings::ssid());
      page += F("'><label>Wi-Fi password</label>"
                "<input name=pass type=password maxlength=63 placeholder='Leave blank to keep current'>"
                "<label>City</label>"
                "<input name=city maxlength=32 required value='");
      page += Util::htmlEscape(Settings::city());
      page += F("'><label>Country (name or code)</label>"
                "<input name=country maxlength=32 required value='");
      page += Util::htmlEscape(Settings::country());
      page += F("'><label>Your name</label>"
                "<input name=name maxlength=20 placeholder='Shown as \"Hi name!\"' value='");
      page += Util::htmlEscape(Settings::name());
      page += F("'><button class=save type=submit>Save &amp; connect</button></form></div>"
                "<div class='card emotions'><h2>Emotes</h2>"
                "<p class=hint>Static holds the selected face. Random plays an emote for 10 seconds every 10 minutes.</p>"
                "<form method=POST action=/emote><div class=mode>"
                "<label><input type=radio name=mode value=static");
      if (!Settings::randomMode())
        page += F(" checked");
      page += F(">Static</label><label><input type=radio name=mode value=random");
      if (Settings::randomMode())
        page += F(" checked");
      page += F(">Random</label></div><div class=emotes>");
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
      page += F("</div><button class=save type=submit>Save behavior</button>"
                "</form></div></main></body></html>");
      server.send(200, F("text/html"), page);
    }

    void redirectHome()
    {
      server.sendHeader(F("Location"), F("/"), true);
      server.send(303, F("text/plain"), F("Updated"));
    }

    void handleEmote()
    {
      const String mode = server.arg("mode");
      if (mode != F("static") && mode != F("random"))
      {
        server.send(400, F("text/plain"), F("Choose Static or Random"));
        return;
      }

      const bool requestedRandom = mode == F("random");
      const bool modeChanged = requestedRandom != Settings::randomMode();
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
      else if (modeChanged && !requestedRandom)
      {
        Settings::saveMood(Settings::mood());
      }

      if (modeChanged)
      {
        Settings::saveEmoteMode(requestedRandom);
        EmoteDirector::resetSchedule();
      }

      if (requestedRandom)
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
      redirectHome();
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
      server.send(200, F("text/html"),
                  String(F("<!doctype html><meta charset=utf-8><meta name=viewport "
                           "content='width=device-width,initial-scale=1'>"
                           "<body style='font:15px system-ui;background:#10131c;color:#f7f8fc;padding:24px'>")) +
                      msg + F(". <a style='color:#67e8c2' href='/'>Back</a>"));

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
    server.onNotFound(handleRoot); // serves dashboard + captive-portal catch-all
    server.begin();
    webStarted = true;
  }

  void handle()
  {
    if (webStarted)
      server.handleClient();
  }

  bool started() { return webStarted; }
}
