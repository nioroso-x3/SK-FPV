#ifndef JOYSTICK_CONFIG_H
#define JOYSTICK_CONFIG_H

#include <string>
#include <unordered_map>
#include <functional>
#include <nlohmann/json.hpp>

struct AxisConfig {
    std::string name;
    int evdev_code;
    int rc_channel = -1;  // -1 = not mapped to RC
    int min_value = -32767;
    int max_value = 32767;
    bool invert = false;
    float deadzone = 0.05f;
    bool is_rate_axis = false;  // true = rate control, false = direct mapping
    float rate_scale = 250.0f;  // microseconds per second for rate axes
    float expo = 0.0f;  // 0-100 expo curve strength for direct axes
    uint16_t rate_init_value = 1500;  // Initial value for rate axes (1000 for throttle, 1500 for others)
};

struct ButtonConfig {
    std::string name;
    int evdev_code;
    int rc_channel = -1;  // -1 = not mapped to RC
    uint16_t on_value = 2000;
    uint16_t off_value = 1000;
    std::string function = "";  // Function name for hashmap lookup
};

struct JoystickSettings {
    float global_deadzone = 0.05f;
    int update_rate_hz = 20;
    bool enable_rc_override = true;
    int rc_disable_switch_channel = -1;  // -1 = disabled, 0-15 = RC channel to monitor
    uint16_t rc_disable_switch_threshold = 1700;  // Above this value = force RC override OFF
};

class JoystickConfig {
public:
    JoystickConfig();

    bool loadFromFile(const std::string& filename);
    bool saveToFile(const std::string& filename) const;
    void createDefaultConfig();

    // Getters
    const JoystickSettings& getSettings() const { return settings; }
    const std::vector<AxisConfig>& getAxisConfigs() const { return axis_configs; }
    const std::vector<ButtonConfig>& getButtonConfigs() const { return button_configs; }

    // RC Override control
    void toggleRcOverride();
    void setRcOverride(bool enabled);
    bool isRcOverrideEnabled() const { return settings.enable_rc_override; }

    // Lookup by evdev code
    const AxisConfig* getAxisConfig(int evdev_code) const;
    const ButtonConfig* getButtonConfig(int evdev_code) const;

    // Function registry for button mappings
    static void registerFunction(const std::string& name, std::function<void()> func);
    static std::function<void()> getFunction(const std::string& name);

private:
    JoystickSettings settings;
    std::vector<AxisConfig> axis_configs;
    std::vector<ButtonConfig> button_configs;

    // Static function registry
    static std::unordered_map<std::string, std::function<void()>> function_registry;

    // Helper methods
    int parseEvdevCode(const std::string& code_str) const;
    std::string evdevCodeToString(int code) const;
    void loadAxisConfigs(const nlohmann::json& j);
    void loadButtonConfigs(const nlohmann::json& j);
};

// Global config instance
extern JoystickConfig* g_joystick_config;

#endif // JOYSTICK_CONFIG_H