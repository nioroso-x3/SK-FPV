#include "joystick_openxr.h"
#include "joystick_config.h"
#include <iostream>
#include <cmath>

extern JoystickConfig* g_joystick_config;
extern JoystickState g_joystick_state;

OpenXRJoystickInput::OpenXRJoystickInput() {
    // Initialize previous controller states
    memset(&prev_left_controller, 0, sizeof(prev_left_controller));
    memset(&prev_right_controller, 0, sizeof(prev_right_controller));
}

OpenXRJoystickInput::~OpenXRJoystickInput() {
    stop();
}

bool OpenXRJoystickInput::initialize() {
    // Load configuration-based mappings
    loadConfigurationMappings();

    // Initialize RC channels with proper values
    initializeRCChannels();

    // Set device info
    state.device_name = "OpenXR Controllers";
    state.connected = true;

    std::cout << "OpenXR joystick initialized: " << state.device_name << std::endl;
    return true;
}

void OpenXRJoystickInput::start() {
    if (running.load()) {
        return;
    }

    running.store(true);
    update_thread = std::thread([this]() {
        while (running.load()) {
            updateControllerState();
            updateRCChannels();
            processHoldButtons();

            // Copy state to global
            g_joystick_state = state;

            // Update at configured rate (default 20Hz = 50ms)
            int update_rate = g_joystick_config ? g_joystick_config->getSettings().update_rate_hz : 20;
            std::this_thread::sleep_for(std::chrono::milliseconds(1000 / update_rate));
        }
    });
}

void OpenXRJoystickInput::stop() {
    if (running.load()) {
        running.store(false);
        if (update_thread.joinable()) {
            update_thread.join();
        }
    }
    state.connected = false;
}

void OpenXRJoystickInput::updateControllerState() {
    // Get current controller states
    const controller_t* left_controller = input_controller(handed_left);
    const controller_t* right_controller = input_controller(handed_right);

    // Check if at least one controller is tracked
    bool any_tracked = (left_controller && (left_controller->tracked & button_state_active)) ||
                      (right_controller && (right_controller->tracked & button_state_active));


    state.connected = any_tracked;

    if (!any_tracked) {
        // Reset all inputs when no controllers are tracked
        state.left_stick_x = 0.0f;
        state.left_stick_y = 0.0f;
        state.right_stick_x = 0.0f;
        state.right_stick_y = 0.0f;
        state.left_trigger = 0.0f;
        state.right_trigger = 0.0f;
        for (int i = 0; i < 16; i++) {
            state.buttons[i] = false;
            state.button_pressed[i] = false;
        }
        return;
    }

    // Update axis values
    if (left_controller && (left_controller->tracked & button_state_active)) {
        // Map left controller stick to left stick
        state.left_stick_x = left_controller->stick.x;
        state.left_stick_y = left_controller->stick.y;
        state.left_trigger = left_controller->trigger;
        raw_axis_values[OPENXR_LEFT_STICK_X] = left_controller->stick.x;
        raw_axis_values[OPENXR_LEFT_STICK_Y] = left_controller->stick.y;
        raw_axis_values[OPENXR_LEFT_TRIGGER] = left_controller->trigger;
        raw_axis_values[OPENXR_LEFT_GRIP] = left_controller->grip;

        // Process left controller buttons
        processButton(OPENXR_LEFT_X1, left_controller->x1 & button_state_active);
        processButton(OPENXR_LEFT_X2, left_controller->x2 & button_state_active);
        processButton(OPENXR_LEFT_STICK_CLICK, left_controller->stick_click & button_state_active);
    }

    if (right_controller && (right_controller->tracked & button_state_active)) {
        // Map right controller stick to right stick
        state.right_stick_x = right_controller->stick.x;
        state.right_stick_y = right_controller->stick.y;
        state.right_trigger = right_controller->trigger;
        raw_axis_values[OPENXR_RIGHT_STICK_X] = right_controller->stick.x;
        raw_axis_values[OPENXR_RIGHT_STICK_Y] = right_controller->stick.y;
        raw_axis_values[OPENXR_RIGHT_TRIGGER] = right_controller->trigger;
        raw_axis_values[OPENXR_RIGHT_GRIP] = right_controller->grip;

        // Process right controller buttons
        processButton(OPENXR_RIGHT_X1, right_controller->x1 & button_state_active);
        processButton(OPENXR_RIGHT_X2, right_controller->x2 & button_state_active);
        processButton(OPENXR_RIGHT_STICK_CLICK, right_controller->stick_click & button_state_active);
    }

    // Process menu button (global across both controllers)
    button_state_ menu_state = input_controller_menu();
    processButton(OPENXR_MENU_BUTTON, menu_state & button_state_active);

    // Store previous states for next frame
    if (left_controller) prev_left_controller = *left_controller;
    if (right_controller) prev_right_controller = *right_controller;
}

void OpenXRJoystickInput::processButton(int button_code, bool pressed) {
    // Handle button mappings
    auto it = button_mappings.find(button_code);
    if (it != button_mappings.end()) {
        ButtonMapping& mapping = it->second;

        // Handle single press callback
        if (pressed && !mapping.was_pressed && mapping.callback) {
            mapping.callback();
        }

        // Handle hold state for continuous sending
        if (pressed && !mapping.was_pressed && mapping.hold_callback) {
            mapping.is_held = true;
            mapping.hold_start_time = std::chrono::steady_clock::now();
            mapping.last_hold_send = mapping.hold_start_time;
        } else if (!pressed && mapping.was_pressed) {
            mapping.is_held = false;
        }

        mapping.was_pressed = pressed;

        // Update RC channel if mapped
        if (mapping.rc_channel >= 0 && mapping.rc_channel < 16) {
            state.rc_channels[mapping.rc_channel] = pressed ? mapping.on_value : mapping.off_value;
        }
    }

    // Update general button state arrays (map OpenXR buttons to indices 0-15)
    int button_index = -1;
    if (button_code == OPENXR_LEFT_X1) button_index = 0;
    else if (button_code == OPENXR_LEFT_X2) button_index = 1;
    else if (button_code == OPENXR_LEFT_STICK_CLICK) button_index = 2;
    else if (button_code == OPENXR_RIGHT_X1) button_index = 3;
    else if (button_code == OPENXR_RIGHT_X2) button_index = 4;
    else if (button_code == OPENXR_RIGHT_STICK_CLICK) button_index = 5;
    else if (button_code == OPENXR_MENU_BUTTON) button_index = 6;

    if (button_index >= 0 && button_index < 16) {
        bool was_pressed = state.buttons[button_index];
        state.buttons[button_index] = pressed;
        // Single press detection (pressed and not was pressed)
        state.button_pressed[button_index] = pressed && !was_pressed;
    }
}

float OpenXRJoystickInput::normalizeAxis(float raw_value, const AxisMapping& mapping) {
    // OpenXR already provides normalized values (-1 to 1 for sticks, 0 to 1 for triggers)
    float normalized = raw_value;

    // Apply deadzone
    if (std::abs(normalized) < mapping.deadzone) {
        normalized = 0.0f;
    }

    // Apply expo curve (only for direct axes, not rate axes)
    if (!mapping.is_rate_axis && mapping.expo > 0.0f) {
        bool neg = (normalized < 0.0f);
        float abs_val = std::abs(normalized);

        if (abs_val > 1.0f) abs_val = 1.0f; // Clamp to valid range

        // Convert to 0-1024 range for calculation
        int x = static_cast<int>(abs_val * 1024.0f);
        int k = static_cast<int>(mapping.expo); // Already 0-100

        if (k > 0) {
            // EdgeTX expo formula
            int k256 = (k * 256 + 50) / 100; // Convert 0-100 to 0-256 with rounding

            uint32_t value = static_cast<uint32_t>(x) * x; // x^2
            value *= k256; // k * x^2
            value >>= 8; // Divide by 256
            value *= static_cast<uint32_t>(x); // k * x^3
            value >>= 12; // Divide by 4096 (1024^2 / 256)
            value += static_cast<uint32_t>(256 - k256) * x + 128;

            x = value >> 8; // Final divide by 256
        }

        // Convert back to -1.0 to 1.0 range
        float result = static_cast<float>(x) / 1024.0f;
        normalized = neg ? -result : result;
    }

    // Apply inversion
    if (mapping.invert) {
        normalized = -normalized;
    }

    // Clamp to valid range
    return std::max(-1.0f, std::min(1.0f, normalized));
}

void OpenXRJoystickInput::updateRCChannels() {
    auto now = std::chrono::steady_clock::now();

    // Update RC channels based on axis mappings
    for (auto& pair : axis_mappings) {
        int axis_code = pair.first;
        AxisMapping& mapping = pair.second;

        if (mapping.rc_channel >= 0 && mapping.rc_channel < 16) {
            auto raw_it = raw_axis_values.find(axis_code);
            if (raw_it != raw_axis_values.end()) {
                float normalized = normalizeAxis(raw_it->second, mapping);

                if (mapping.is_rate_axis) {
                    // Rate-based control (for throttle, etc.)
                    auto time_diff = std::chrono::duration_cast<std::chrono::microseconds>(
                        now - mapping.last_update_time);
                    float dt_seconds = time_diff.count() / 1000000.0f; // Convert µs to seconds

                    // Initialize time on first update
                    if (mapping.last_update_time.time_since_epoch().count() == 0) {
                        mapping.last_update_time = now;
                        dt_seconds = 0.05f; // Use default 50ms for first update
                    }

                    if (dt_seconds > 0.0f && dt_seconds < 1.0f) { // Sanity check on delta time
                        // Calculate rate change: normalized input * rate_scale * time
                        float rate_change = normalized * mapping.rate_scale * dt_seconds;

                        // Update the current value
                        float current_value = static_cast<float>(mapping.rate_current_value);
                        float new_value = current_value + rate_change;

                        // Clamp to RC range (1000-2000)
                        new_value = std::max(1000.0f, std::min(2000.0f, new_value));
                        mapping.rate_current_value = static_cast<uint16_t>(std::round(new_value));

                        mapping.last_update_time = now;
                    }

                    state.rc_channels[mapping.rc_channel] = mapping.rate_current_value;
                } else {
                    // Direct mapping (for roll, pitch, yaw)
                    uint16_t rc_value = static_cast<uint16_t>(1500 + normalized * 500);
                    state.rc_channels[mapping.rc_channel] = rc_value;
                }
            }
        }
    }
}

void OpenXRJoystickInput::mapAxisToRC(int axis_code, int rc_channel, bool invert) {
    if (rc_channel >= 0 && rc_channel < 16) {
        axis_mappings[axis_code].rc_channel = rc_channel;
        axis_mappings[axis_code].invert = invert;
    }
}

void OpenXRJoystickInput::mapButtonToRC(int button_code, int rc_channel, uint16_t on_value, uint16_t off_value) {
    if (rc_channel >= 0 && rc_channel < 16) {
        button_mappings[button_code].rc_channel = rc_channel;
        button_mappings[button_code].on_value = on_value;
        button_mappings[button_code].off_value = off_value;
    }
}

void OpenXRJoystickInput::mapButtonToFunction(int button_code, std::function<void()> callback) {
    button_mappings[button_code].callback = callback;
}

void OpenXRJoystickInput::setAxisDeadzone(int axis_code, float deadzone) {
    axis_mappings[axis_code].deadzone = deadzone;
}

void OpenXRJoystickInput::setAxisRange(int axis_code, int min_val, int max_val) {
    // OpenXR values are already normalized, so we don't need min/max ranges
    // This method is kept for interface compatibility
}

void OpenXRJoystickInput::loadConfigurationMappings() {
    if (!g_joystick_config) {
        std::cerr << "ERROR: No joystick configuration available! Using defaults." << std::endl;
        // Load defaults
        axis_mappings[OPENXR_LEFT_STICK_X] = {0, false, 0.05f, false, 250.0f, 1500, 1500};
        axis_mappings[OPENXR_LEFT_STICK_Y] = {1, true, 0.05f, false, 250.0f, 1500, 1500};
        axis_mappings[OPENXR_RIGHT_STICK_X] = {3, false, 0.05f, false, 250.0f, 1500, 1500};
        axis_mappings[OPENXR_RIGHT_STICK_Y] = {2, true, 0.05f, false, 250.0f, 1500, 1500};
        return;
    }

    // Clear existing mappings
    axis_mappings.clear();
    button_mappings.clear();

    // Map configuration axis codes to OpenXR axis codes
    std::map<int, int> evdev_to_openxr = {
        {ABS_X, OPENXR_LEFT_STICK_X},      // Left stick X
        {ABS_Y, OPENXR_LEFT_STICK_Y},      // Left stick Y
        {ABS_RX, OPENXR_RIGHT_STICK_X},    // Right stick X
        {ABS_RY, OPENXR_RIGHT_STICK_Y},    // Right stick Y
        {ABS_Z, OPENXR_LEFT_TRIGGER},      // Left trigger
        {ABS_RZ, OPENXR_RIGHT_TRIGGER}     // Right trigger
    };

    // Load axis mappings from config
    for (const auto& axis_config : g_joystick_config->getAxisConfigs()) {
        // Check if this is already an OpenXR code (direct mapping)
        if (axis_config.evdev_code >= 1000 && axis_config.evdev_code <= 1010) {

            AxisMapping mapping;
            mapping.rc_channel = axis_config.rc_channel;
            mapping.invert = axis_config.invert;
            mapping.deadzone = axis_config.deadzone;
            mapping.is_rate_axis = axis_config.is_rate_axis;
            mapping.rate_scale = axis_config.rate_scale;
            mapping.expo = axis_config.expo;
            mapping.rate_init_value = axis_config.rate_init_value;
            mapping.rate_current_value = axis_config.rate_init_value;
            mapping.last_update_time = std::chrono::steady_clock::time_point{};

            axis_mappings[axis_config.evdev_code] = mapping;
        } else {
            // Try evdev to OpenXR mapping for libevdev compatibility
            auto it = evdev_to_openxr.find(axis_config.evdev_code);
            if (it != evdev_to_openxr.end()) {
                AxisMapping mapping;
                mapping.rc_channel = axis_config.rc_channel;
                mapping.invert = axis_config.invert;
                mapping.deadzone = axis_config.deadzone;
                mapping.is_rate_axis = axis_config.is_rate_axis;
                mapping.rate_scale = axis_config.rate_scale;
                mapping.expo = axis_config.expo;
                mapping.rate_init_value = axis_config.rate_init_value;
                mapping.rate_current_value = axis_config.rate_init_value;
                mapping.last_update_time = std::chrono::steady_clock::time_point{};

                axis_mappings[it->second] = mapping;
            }
        }
    }

    // Map configuration button codes to OpenXR button codes
    std::map<int, int> evdev_to_openxr_buttons = {
        {304, OPENXR_LEFT_X1},        // BTN_SOUTH -> Left X1
        {305, OPENXR_LEFT_X2},        // BTN_EAST -> Left X2
        {307, OPENXR_RIGHT_X1},       // BTN_NORTH -> Right X1
        {308, OPENXR_RIGHT_X2},       // BTN_WEST -> Right X2
        {317, OPENXR_LEFT_STICK_CLICK},  // BTN_THUMBL -> Left stick click
        {318, OPENXR_RIGHT_STICK_CLICK}, // BTN_THUMBR -> Right stick click
        {315, OPENXR_MENU_BUTTON}     // BTN_START -> Menu button
    };

    // Load button mappings from config
    for (const auto& button_config : g_joystick_config->getButtonConfigs()) {
        // Check if this is already an OpenXR code (direct mapping)
        if (button_config.evdev_code >= 2000 && button_config.evdev_code <= 2010) {

            ButtonMapping mapping;
            mapping.rc_channel = button_config.rc_channel;
            mapping.on_value = button_config.on_value;
            mapping.off_value = button_config.off_value;
            mapping.was_pressed = false;

            // Set up function callback if specified
            if (!button_config.function.empty()) {
                auto func = JoystickConfig::getFunction(button_config.function);
                if (func) {
                    // Flight mode functions use hold callback for continuous sending
                    if (button_config.function.find("mode_") == 0) {
                        mapping.hold_callback = func;
                    } else {
                        mapping.callback = func;
                    }
                }
            }

            button_mappings[button_config.evdev_code] = mapping;
        } else {
            // Try evdev to OpenXR mapping for libevdev compatibility
            auto it = evdev_to_openxr_buttons.find(button_config.evdev_code);
            if (it != evdev_to_openxr_buttons.end()) {
                ButtonMapping mapping;
                mapping.rc_channel = button_config.rc_channel;
                mapping.on_value = button_config.on_value;
                mapping.off_value = button_config.off_value;
                mapping.was_pressed = false;

                // Set up function callback if specified
                if (!button_config.function.empty()) {
                    auto func = JoystickConfig::getFunction(button_config.function);
                    if (func) {
                        // Flight mode functions use hold callback for continuous sending
                        if (button_config.function.find("mode_") == 0) {
                            mapping.hold_callback = func;
                        } else {
                            mapping.callback = func;
                        }
                    }
                }

                button_mappings[it->second] = mapping;
            }
        }
    }
}

void OpenXRJoystickInput::initializeRCChannels() {
    // Initialize all RC channels to 0 (unmapped channels stay 0)
    for (int i = 0; i < 16; i++) {
        state.rc_channels[i] = 0;
    }

    // Set rate axes to their configured initialization values
    for (auto& [openxr_code, mapping] : axis_mappings) {
        if (mapping.is_rate_axis && mapping.rc_channel >= 0 && mapping.rc_channel < 16) {
            state.rc_channels[mapping.rc_channel] = mapping.rate_init_value;
        }
    }

    // Update global state
    g_joystick_state = state;
}

void OpenXRJoystickInput::processHoldButtons() {
    const auto now = std::chrono::steady_clock::now();
    const auto hold_interval = std::chrono::milliseconds(200); // 5Hz = 200ms interval

    for (auto& [openxr_code, mapping] : button_mappings) {
        if (mapping.is_held && mapping.hold_callback) {
            // Check if enough time has passed since last send
            if (now - mapping.last_hold_send >= hold_interval) {
                mapping.hold_callback();
                mapping.last_hold_send = now;
            }
        }
    }
}