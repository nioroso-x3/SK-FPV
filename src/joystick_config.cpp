#include "joystick_config.h"
#include <fstream>
#include <iostream>
#include <linux/input-event-codes.h>

// Global instance
JoystickConfig* g_joystick_config = nullptr;

// Static function registry
std::unordered_map<std::string, std::function<void()>> JoystickConfig::function_registry;

JoystickConfig::JoystickConfig() {
    createDefaultConfig();
}

bool JoystickConfig::loadFromFile(const std::string& filename) {
    try {
        std::ifstream file(filename);
        if (!file.is_open()) {
            std::cerr << "Could not open joystick config file: " << filename << std::endl;
            std::cerr << "Using default configuration" << std::endl;
            return false;
        }

        nlohmann::json j;
        file >> j;

        // Load settings
        if (j.contains("joystick_settings")) {
            const auto& js = j["joystick_settings"];
            if (js.contains("global_deadzone")) settings.global_deadzone = js["global_deadzone"];
            if (js.contains("update_rate_hz")) settings.update_rate_hz = js["update_rate_hz"];
            if (js.contains("enable_rc_override")) settings.enable_rc_override = js["enable_rc_override"];
            if (js.contains("rc_disable_switch_channel")) settings.rc_disable_switch_channel = js["rc_disable_switch_channel"];
            if (js.contains("rc_disable_switch_threshold")) settings.rc_disable_switch_threshold = js["rc_disable_switch_threshold"];
        }

        // Load axis configurations
        if (j.contains("axis_mappings")) {
            loadAxisConfigs(j["axis_mappings"]);
        }

        // Load button configurations
        if (j.contains("button_mappings")) {
            loadButtonConfigs(j["button_mappings"]);
        }

        std::cout << "Loaded joystick configuration from: " << filename << std::endl;
        return true;

    } catch (const std::exception& e) {
        std::cerr << "Error loading joystick config: " << e.what() << std::endl;
        std::cerr << "Using default configuration" << std::endl;
        createDefaultConfig();
        return false;
    }
}

void JoystickConfig::createDefaultConfig() {
    // Default settings
    settings.global_deadzone = 0.05f;
    settings.update_rate_hz = 20;
    settings.enable_rc_override = false;

    // Clear existing configs
    axis_configs.clear();
    button_configs.clear();

    // Default axis mappings (standard gamepad layout)
    axis_configs = {
        {"right_stick_x", ABS_RX, 0, 0, 255, false, 0.05f, false, 250.0f, 80.0f, 1500},  // Roll with expo
        {"right_stick_y", ABS_RY, 1, 0, 255, true, 0.05f, false, 250.0f, 80.0f, 1500},   // Pitch (inverted) with expo
        {"left_stick_x", ABS_X, 3, 0, 255, false, 0.05f, false, 250.0f, 80.0f, 1500},    // Yaw with expo
        {"left_stick_y", ABS_Y, 2, 0, 255, true, 0.05f, true, 800.0f, 0.0f, 1000},       // Throttle (rate, init at 1000μs)
        //{"left_trigger", ABS_Z, 4, 0, 255, false, 0.02f, false, 250.0f, 0.0f, 1500},     // Aux1
        //{"right_trigger", ABS_RZ, 5, 0, 255, false, 0.02f, false, 250.0f, 0.0f, 1500}    // Aux2
    };

    // Default button mappings (PS5 DualSense codes)
    button_configs = {
        {"button_cross", 304, -1, 2000, 1000, "mode_fbwa"},          // X/Cross (BTN_SOUTH) -> HUD toggle
        {"button_circle", 305, -1, 2000, 1000, "mode_cruise"},                    // Circle (BTN_EAST) -> RC Channel 7
        {"button_square", 308, -1, 2000, 1000, "mode_stabilize"},         // Square (BTN_WEST) -> CRUISE mode
        {"button_triangle", 307, -1, 2000, 1000, "mode_manual"},       // Triangle (BTN_NORTH) -> Camera switch
        {"button_l1", 310, -1, 2000, 1000, "mode_auto"},              // L1 (BTN_TL) -> FBWA mode
        {"button_r1", 311, -1, 2000, 1000, "mode_rtl"},               // R1 (BTN_TR) -> RTL mode
        {"button_select", 314, -1, 2000, 1000, "mode_loiter"},        // Select (BTN_SELECT) -> GUIDED mode
        {"button_start", 315, -1, 2000, 1000, "hud_toggle"},           // Start (BTN_START) -> RC Override toggle
        {"button_ps", 316, -1, 2000, 1000, "rc_toggle"},             // PS button (BTN_MODE) -> LOITER mode
        {"button_thumbr", 318, -1, 2000, 1000, "recenter_cameras"}             // PS button (BTN_MODE) -> LOITER mode
        // Note: D-pad uses ABS_HAT0X/Y (axis events), not button events
    };

    std::cout << "Created default joystick configuration" << std::endl;
}

void JoystickConfig::loadAxisConfigs(const nlohmann::json& j) {
    axis_configs.clear();

    for (const auto& axis : j) {
        AxisConfig config;
        config.name = axis.value("name", "unknown");
        config.evdev_code = parseEvdevCode(axis.value("evdev_code", ""));
        config.rc_channel = axis.value("rc_channel", -1);
        config.min_value = axis.value("min_value", -32768);
        config.max_value = axis.value("max_value", 32767);
        config.invert = axis.value("invert", false);
        config.deadzone = axis.value("deadzone", settings.global_deadzone);
        config.is_rate_axis = axis.value("is_rate_axis", false);
        config.rate_scale = axis.value("rate_scale", 250.0f);
        config.expo = axis.value("expo", 0.0f);
        config.rate_init_value = axis.value("rate_init_value", 1500);

        if (config.evdev_code != -1) {
            axis_configs.push_back(config);
        }
    }
}

void JoystickConfig::loadButtonConfigs(const nlohmann::json& j) {
    button_configs.clear();

    for (const auto& button : j) {
        ButtonConfig config;
        config.name = button.value("name", "unknown");
        config.evdev_code = parseEvdevCode(button.value("evdev_code", ""));
        config.rc_channel = button.value("rc_channel", -1);
        config.on_value = button.value("on_value", 2000);
        config.off_value = button.value("off_value", 1000);
        config.function = button.value("function", "");

        if (config.evdev_code != -1) {
            button_configs.push_back(config);
        }
    }
}

int JoystickConfig::parseEvdevCode(const std::string& code_str) const {
    if (code_str.empty()) return -1;

    // Handle basic axis codes
    if (code_str == "ABS_X") return ABS_X;
    if (code_str == "ABS_Y") return ABS_Y;
    if (code_str == "ABS_Z") return ABS_Z;
    if (code_str == "ABS_RX") return ABS_RX;
    if (code_str == "ABS_RY") return ABS_RY;
    if (code_str == "ABS_RZ") return ABS_RZ;

    // Handle button codes
    if (code_str == "BTN_GAMEPAD") return BTN_GAMEPAD;
    if (code_str == "BTN_JOYSTICK") return BTN_JOYSTICK;
    if (code_str == "BTN_START") return BTN_START;
    if (code_str == "BTN_SOUTH") return BTN_SOUTH;
    if (code_str == "BTN_EAST") return BTN_EAST;
    if (code_str == "BTN_WEST") return BTN_WEST;
    if (code_str == "BTN_NORTH") return BTN_NORTH;
    if (code_str == "BTN_TL") return BTN_TL;
    if (code_str == "BTN_TR") return BTN_TR;
    if (code_str == "BTN_TL2") return BTN_TL2;
    if (code_str == "BTN_TR2") return BTN_TR2;
    if (code_str == "BTN_SELECT") return BTN_SELECT;
    if (code_str == "BTN_MODE") return BTN_MODE;
    if (code_str == "BTN_THUMBL") return BTN_THUMBL;
    if (code_str == "BTN_THUMBR") return BTN_THUMBR;

    // Handle BTN_GAMEPAD+N format
    if (code_str.find("BTN_GAMEPAD+") == 0) {
        try {
            int offset = std::stoi(code_str.substr(12));
            return BTN_GAMEPAD + offset;
        } catch (...) {
            return -1;
        }
    }

    // Handle BTN_JOYSTICK+N format
    if (code_str.find("BTN_JOYSTICK+") == 0) {
        try {
            int offset = std::stoi(code_str.substr(13));
            return BTN_JOYSTICK + offset;
        } catch (...) {
            return -1;
        }
    }

    // Try to parse as direct integer
    try {
        return std::stoi(code_str);
    } catch (...) {
        std::cerr << "Unknown evdev code: " << code_str << std::endl;
        return -1;
    }
}

const AxisConfig* JoystickConfig::getAxisConfig(int evdev_code) const {
    for (const auto& config : axis_configs) {
        if (config.evdev_code == evdev_code) {
            return &config;
        }
    }
    return nullptr;
}

const ButtonConfig* JoystickConfig::getButtonConfig(int evdev_code) const {
    for (const auto& config : button_configs) {
        if (config.evdev_code == evdev_code) {
            return &config;
        }
    }
    return nullptr;
}

bool JoystickConfig::saveToFile(const std::string& filename) const {
    try {
        nlohmann::json j;

        // Save settings
        j["joystick_settings"] = {
            {"global_deadzone", settings.global_deadzone},
            {"update_rate_hz", settings.update_rate_hz},
            {"enable_rc_override", settings.enable_rc_override},
            {"rc_disable_switch_channel", settings.rc_disable_switch_channel},
            {"rc_disable_switch_threshold", settings.rc_disable_switch_threshold}
        };

        // Save axis mappings
        j["axis_mappings"] = nlohmann::json::array();
        for (const auto& config : axis_configs) {
            j["axis_mappings"].push_back({
                {"name", config.name},
                {"evdev_code", evdevCodeToString(config.evdev_code)},
                {"rc_channel", config.rc_channel},
                {"min_value", config.min_value},
                {"max_value", config.max_value},
                {"invert", config.invert},
                {"deadzone", config.deadzone},
                {"is_rate_axis", config.is_rate_axis},
                {"rate_scale", config.rate_scale},
                {"expo", config.expo},
                {"rate_init_value", config.rate_init_value}
            });
        }

        // Save button mappings
        j["button_mappings"] = nlohmann::json::array();
        for (const auto& config : button_configs) {
            j["button_mappings"].push_back({
                {"name", config.name},
                {"evdev_code", evdevCodeToString(config.evdev_code)},
                {"rc_channel", config.rc_channel},
                {"on_value", config.on_value},
                {"off_value", config.off_value},
                {"function", config.function}
            });
        }

        std::ofstream file(filename);
        file << j.dump(2);

        std::cout << "Saved joystick configuration to: " << filename << std::endl;
        return true;

    } catch (const std::exception& e) {
        std::cerr << "Error saving joystick config: " << e.what() << std::endl;
        return false;
    }
}

std::string JoystickConfig::evdevCodeToString(int code) const {
    switch (code) {
        case ABS_X: return "ABS_X";
        case ABS_Y: return "ABS_Y";
        case ABS_Z: return "ABS_Z";
        case ABS_RX: return "ABS_RX";
        case ABS_RY: return "ABS_RY";
        case ABS_RZ: return "ABS_RZ";
        case BTN_SOUTH: return "BTN_SOUTH";        // Same as BTN_GAMEPAD (304)
        case BTN_EAST: return "BTN_EAST";          // BTN_GAMEPAD + 1 (305)
        case BTN_WEST: return "BTN_WEST";          // BTN_GAMEPAD + 2 (306)
        case BTN_NORTH: return "BTN_NORTH";        // BTN_GAMEPAD + 3 (307)
        case BTN_TL: return "BTN_TL";              // BTN_GAMEPAD + 4 (310)
        case BTN_TR: return "BTN_TR";              // BTN_GAMEPAD + 5 (311)
        case BTN_TL2: return "BTN_TL2";            // BTN_GAMEPAD + 6 (312)
        case BTN_TR2: return "BTN_TR2";            // BTN_GAMEPAD + 7 (313)
        case BTN_SELECT: return "BTN_SELECT";      // BTN_GAMEPAD + 8 (314)
        case BTN_START: return "BTN_START";        // BTN_GAMEPAD + 9 (315)
        case BTN_MODE: return "BTN_MODE";          // BTN_GAMEPAD + 10 (316)
        case BTN_THUMBL: return "BTN_THUMBL";      // 317
        case BTN_THUMBR: return "BTN_THUMBR";      // 318
        case BTN_JOYSTICK: return "BTN_JOYSTICK";
        default:
            if (code >= BTN_GAMEPAD && code < BTN_GAMEPAD + 32) {
                return "BTN_GAMEPAD+" + std::to_string(code - BTN_GAMEPAD);
            }
            if (code >= BTN_JOYSTICK && code < BTN_JOYSTICK + 32) {
                return "BTN_JOYSTICK+" + std::to_string(code - BTN_JOYSTICK);
            }
            return std::to_string(code);
    }
}

void JoystickConfig::registerFunction(const std::string& name, std::function<void()> func) {
    function_registry[name] = func;
    std::cout << "Registered joystick function: " << name << std::endl;
}

std::function<void()> JoystickConfig::getFunction(const std::string& name) {
    auto it = function_registry.find(name);
    if (it != function_registry.end()) {
        return it->second;
    }
    return nullptr;
}

void JoystickConfig::toggleRcOverride() {
    settings.enable_rc_override = !settings.enable_rc_override;
    extern bool rc_override;
    rc_override = settings.enable_rc_override;
    std::cout << "RC Override " << (settings.enable_rc_override ? "ENABLED" : "DISABLED") << std::endl;
}

void JoystickConfig::setRcOverride(bool enabled) {
    settings.enable_rc_override = enabled;
    extern bool rc_override;
    rc_override = settings.enable_rc_override;
    std::cout << "RC Override " << (settings.enable_rc_override ? "ENABLED" : "DISABLED") << std::endl;
}
