#pragma once

#include <esp_wifi.h>
#include <esp_bt.h>
#include <esp_bt_main.h>
#include "../hardware/wireless_types.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#ifdef ARDUINO
#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#endif

namespace meshink_t5_wireless_detail {

inline bool wifi_driver_off() {
    wifi_mode_t mode = WIFI_MODE_NULL;
    return esp_wifi_get_mode(&mode) == ESP_ERR_WIFI_NOT_INIT;
}

inline bool bluetooth_controller_off() {
    return esp_bt_controller_get_status() == ESP_BT_CONTROLLER_STATUS_IDLE;
}

inline bool bluetooth_host_off() {
    return esp_bluedroid_get_status() == ESP_BLUEDROID_STATUS_UNINITIALIZED;
}

inline void force_wifi_off() {
    const esp_err_t stopped = esp_wifi_stop();
    if(stopped != ESP_OK && stopped != ESP_ERR_WIFI_NOT_INIT) {
        // Continue to deinit/verify. Final state, not the intermediate error,
        // is the power-policy contract exposed to the application.
    }
    const esp_err_t deinitialized = esp_wifi_deinit();
    (void)deinitialized;
}

inline void force_bluetooth_off() {
    esp_bluedroid_status_t host = esp_bluedroid_get_status();
    if(host == ESP_BLUEDROID_STATUS_ENABLED)
        esp_bluedroid_disable();
    host = esp_bluedroid_get_status();
    if(host == ESP_BLUEDROID_STATUS_INITIALIZED)
        esp_bluedroid_deinit();

    esp_bt_controller_status_t controller = esp_bt_controller_get_status();
    if(controller == ESP_BT_CONTROLLER_STATUS_ENABLED)
        esp_bt_controller_disable();
    controller = esp_bt_controller_get_status();
    if(controller == ESP_BT_CONTROLLER_STATUS_INITED)
        esp_bt_controller_deinit();
}

inline MeshInkWifiStatus& global_wifi_status() {
    static MeshInkWifiStatus s = MeshInkWifiStatus::Off;
    return s;
}

inline MeshInkWeatherData& global_weather_data() {
    static MeshInkWeatherData w{};
    return w;
}

inline char* stored_ssid() {
    static char ssid[64]{};
    return ssid;
}

inline char* stored_pass() {
    static char pass[64]{};
    return pass;
}

inline bool& weather_fetch_requested() {
    static bool req = false;
    return req;
}

inline bool& weather_updated_flag() {
    static bool flag = false;
    return flag;
}

inline double& target_lat() {
    static double lat = 0.0;
    return lat;
}

inline double& target_lon() {
    static double lon = 0.0;
    return lon;
}

inline uint32_t& wifi_connect_started() {
    static uint32_t t = 0;
    return t;
}

// Parses an RFC 1123 HTTP Date ("Thu, 12 Nov 2026 17:45:00 GMT") to UTC epoch
// seconds without touching the C library timezone. Returns 0 when malformed.
inline uint32_t parse_http_date(const char* d) {
    if(!d) return 0;
    static const char* const mons[] = {"Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"};
    int day = 0, year = 0, hh = 0, mm = 0, ss = 0;
    char mon[4] = {};
    if(sscanf(d, "%*3s, %d %3s %d %d:%d:%d", &day, mon, &year, &hh, &mm, &ss) != 6) return 0;
    int m = -1;
    for(int i = 0; i < 12; ++i) if(strcmp(mon, mons[i]) == 0) m = i + 1;
    if(m < 0 || year < 2024 || day < 1 || day > 31 || hh > 23 || mm > 59 || ss > 60) return 0;
    // days_from_civil (Howard Hinnant)
    int y = year - (m <= 2);
    const int era = y / 400;
    const int yoe = y - era * 400;
    const int doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + day - 1;
    const int doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    const long days = (long)era * 146097 + doe - 719468;
    return (uint32_t)(days * 86400L + hh * 3600L + mm * 60L + ss);
}

} // namespace meshink_t5_wireless_detail

inline MeshInkWirelessState meshink_wireless_read_state() {
    using namespace meshink_t5_wireless_detail;
    MeshInkWirelessState state{};
    state.wifi_off = wifi_driver_off();
    state.bluetooth_controller_off = bluetooth_controller_off();
    state.bluetooth_host_off = bluetooth_host_off();
    return state;
}

inline MeshInkWirelessState meshink_wireless_force_wifi_off() {
    meshink_t5_wireless_detail::force_wifi_off();
    return meshink_wireless_read_state();
}

inline MeshInkWirelessState meshink_wireless_force_local_radios_off() {
    meshink_t5_wireless_detail::force_wifi_off();
    meshink_t5_wireless_detail::force_bluetooth_off();
    return meshink_wireless_read_state();
}

inline void meshink_wireless_wifi_begin(const char* ssid, const char* password) {
#ifdef ARDUINO
    using namespace meshink_t5_wireless_detail;
    if(!ssid || !ssid[0]) return;
    snprintf(stored_ssid(), 64, "%s", ssid);
    snprintf(stored_pass(), 64, "%s", password ? password : "");
    WiFi.mode(WIFI_STA);
    WiFi.begin(stored_ssid(), stored_pass());
    global_wifi_status() = MeshInkWifiStatus::Connecting;
    wifi_connect_started() = millis();
    Serial.printf("[T5-WIFI] connecting to '%s'...\n", stored_ssid());
#else
    (void)ssid; (void)password;
#endif
}

inline MeshInkWifiStatus meshink_wireless_wifi_status() {
#ifdef ARDUINO
    using namespace meshink_t5_wireless_detail;
    if(global_wifi_status() == MeshInkWifiStatus::Off) return MeshInkWifiStatus::Off;
    if(WiFi.status() == WL_CONNECTED) {
        global_wifi_status() = MeshInkWifiStatus::Connected;
        return MeshInkWifiStatus::Connected;
    }
    if(global_wifi_status() == MeshInkWifiStatus::Connecting) {
        if(millis() - wifi_connect_started() > 15000UL) {
            global_wifi_status() = MeshInkWifiStatus::Failed;
            Serial.println("[T5-WIFI] connection timeout");
            return MeshInkWifiStatus::Failed;
        }
        return MeshInkWifiStatus::Connecting;
    }
    return global_wifi_status();
#else
    return MeshInkWifiStatus::Off;
#endif
}

inline bool meshink_wireless_wifi_get_ip(char* out, size_t max_len) {
#ifdef ARDUINO
    if(!out || max_len == 0) return false;
    out[0] = 0;
    if(WiFi.status() == WL_CONNECTED) {
        String ip = WiFi.localIP().toString();
        snprintf(out, max_len, "%s", ip.c_str());
        return true;
    }
    return false;
#else
    (void)out; (void)max_len; return false;
#endif
}

inline void meshink_wireless_wifi_disconnect() {
#ifdef ARDUINO
    using namespace meshink_t5_wireless_detail;
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
    force_wifi_off();
    global_wifi_status() = MeshInkWifiStatus::Off;
#endif
}

inline void meshink_wireless_weather_request(double latitude, double longitude) {
    using namespace meshink_t5_wireless_detail;
    target_lat() = latitude;
    target_lon() = longitude;
    weather_fetch_requested() = true;
    global_weather_data().fetching = true;
    global_weather_data().error[0] = 0;
    Serial.printf("[T5-METEO] weather requested for lat=%.4f lon=%.4f\n", latitude, longitude);
}

inline const MeshInkWeatherData& meshink_wireless_weather_data() {
    return meshink_t5_wireless_detail::global_weather_data();
}

inline bool meshink_wireless_weather_take_update() {
    using namespace meshink_t5_wireless_detail;
    const bool u = weather_updated_flag();
    weather_updated_flag() = false;
    return u;
}

inline void meshink_wireless_wifi_service_loop() {
#ifdef ARDUINO
    using namespace meshink_t5_wireless_detail;
    if(global_wifi_status() == MeshInkWifiStatus::Connecting) {
        if(WiFi.status() == WL_CONNECTED) {
            global_wifi_status() = MeshInkWifiStatus::Connected;
            Serial.printf("[T5-WIFI] connected! IP=%s\n", WiFi.localIP().toString().c_str());
            if(weather_fetch_requested() || !global_weather_data().valid) {
                weather_fetch_requested() = true;
            }
        } else if(millis() - wifi_connect_started() > 15000UL) {
            global_wifi_status() = MeshInkWifiStatus::Failed;
            global_weather_data().fetching = false;
            snprintf(global_weather_data().error, sizeof(global_weather_data().error), "WiFi connect timeout");
            weather_updated_flag() = true;
            Serial.println("[T5-WIFI] connection failed / timeout");
        }
    }

    if(weather_fetch_requested() && WiFi.status() == WL_CONNECTED) {
        weather_fetch_requested() = false;
        global_weather_data().fetching = true;
        global_weather_data().latitude = target_lat();
        global_weather_data().longitude = target_lon();

        char url[256];
        snprintf(url, sizeof(url),
            "http://api.open-meteo.com/v1/forecast?latitude=%.4f&longitude=%.4f&current=temperature_2m,relative_humidity_2m,apparent_temperature,weather_code,wind_speed_10m&daily=temperature_2m_max,temperature_2m_min&timezone=auto",
            target_lat(), target_lon());

        Serial.printf("[T5-METEO] HTTP GET %s\n", url);
        HTTPClient http;
        WiFiClient client;
        http.setTimeout(8000);
        static const char* wanted_headers[] = {"Date"};
        http.collectHeaders(wanted_headers, 1);
        if(http.begin(client, url)) {
            int code = http.GET();
            if(code == 200) {
                String payload = http.getString();
                const char* cur = strstr(payload.c_str(), "\"current\":");
                if(cur) {
                    const char* pTemp = strstr(cur, "\"temperature_2m\":");
                    if(pTemp) global_weather_data().temperature = strtof(pTemp + 17, nullptr);

                    const char* pApp = strstr(cur, "\"apparent_temperature\":");
                    if(pApp) global_weather_data().apparent_temperature = strtof(pApp + 23, nullptr);

                    const char* pHum = strstr(cur, "\"relative_humidity_2m\":");
                    if(pHum) global_weather_data().humidity = atoi(pHum + 23);

                    const char* pCode = strstr(cur, "\"weather_code\":");
                    if(pCode) global_weather_data().weather_code = atoi(pCode + 15);

                    const char* pWind = strstr(cur, "\"wind_speed_10m\":");
                    if(pWind) global_weather_data().wind_speed = strtof(pWind + 17, nullptr);
                }

                const char* daily = strstr(payload.c_str(), "\"daily\":");
                if(daily) {
                    const char* pMax = strstr(daily, "\"temperature_2m_max\":[");
                    if(pMax) global_weather_data().temp_max = strtof(pMax + 22, nullptr);

                    const char* pMin = strstr(daily, "\"temperature_2m_min\":[");
                    if(pMin) global_weather_data().temp_min = strtof(pMin + 22, nullptr);
                }

                global_weather_data().server_utc = parse_http_date(http.header("Date").c_str());
                global_weather_data().valid = true;
                global_weather_data().fetching = false;
                global_weather_data().last_update_ms = millis();
                global_weather_data().error[0] = 0;
                weather_updated_flag() = true;
                Serial.printf("[T5-METEO] parsed OK: temp=%.1f apparent=%.1f hum=%d code=%d wind=%.1f min=%.1f max=%.1f\n",
                    global_weather_data().temperature,
                    global_weather_data().apparent_temperature,
                    global_weather_data().humidity,
                    global_weather_data().weather_code,
                    global_weather_data().wind_speed,
                    global_weather_data().temp_min,
                    global_weather_data().temp_max);
            } else {
                global_weather_data().fetching = false;
                snprintf(global_weather_data().error, sizeof(global_weather_data().error), "HTTP err %d", code);
                weather_updated_flag() = true;
                Serial.printf("[T5-METEO] HTTP failed with code %d\n", code);
            }
            http.end();
        } else {
            global_weather_data().fetching = false;
            snprintf(global_weather_data().error, sizeof(global_weather_data().error), "HTTP connect failed");
            weather_updated_flag() = true;
            Serial.println("[T5-METEO] http.begin failed");
        }
    }
#endif
}

