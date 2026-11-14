#pragma once
#include <stdint.h>
#include <stddef.h>

// Board-independent view of the ESP-class local wireless state.
// "off" means the corresponding software/controller stack is deinitialized,
// which is the state MeshInk requires while the standalone UI is running.
struct MeshInkWirelessState {
    bool wifi_off = false;
    bool bluetooth_controller_off = false;
    bool bluetooth_host_off = false;
};

inline bool meshink_wireless_local_radios_off(const MeshInkWirelessState& state) {
    return state.wifi_off &&
           state.bluetooth_controller_off &&
           state.bluetooth_host_off;
}

inline bool meshink_wireless_companion_radios_ready(const MeshInkWirelessState& state) {
    return state.wifi_off &&
           !state.bluetooth_controller_off &&
           !state.bluetooth_host_off;
}

enum class MeshInkWifiStatus : uint8_t {
    Off,
    Disconnected,
    Connecting,
    Connected,
    Failed
};

struct MeshInkWeatherData {
    bool valid = false;
    bool fetching = false;
    char error[48] = {};
    float temperature = 0.0f;
    float apparent_temperature = 0.0f;
    float temp_max = 0.0f;
    float temp_min = 0.0f;
    int humidity = 0;
    float wind_speed = 0.0f;
    int weather_code = 0;
    double latitude = 0.0;
    double longitude = 0.0;
    uint32_t last_update_ms = 0;
    uint32_t server_utc = 0; // HTTP Date header of the last fetch; 0 = unknown
};

