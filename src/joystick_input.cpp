#include "joystick_input.h"
#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <cstring>
#include <cmath>

// Global instances
JoystickInput* g_joystick = nullptr;
JoystickState g_joystick_state;

JoystickInput::JoystickInput() {
    // Axis mappings will be loaded from configuration
}

JoystickInput::~JoystickInput() {
    stop();
    if (dev) {
        libevdev_free(dev);
    }
    if (fd >= 0) {
        close(fd);
    }
}

bool JoystickInput::findCompatibleDevice() {
    DIR* dir = opendir("/dev/input");
    if (!dir) {
        std::cerr << "Cannot open /dev/input directory" << std::endl;
        return false;
    }

    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        if (strncmp(entry->d_name, "event", 5) != 0) {
            continue;
        }

        std::string path = "/dev/input/" + std::string(entry->d_name);
        int test_fd = open(path.c_str(), O_RDONLY | O_NONBLOCK);
        if (test_fd < 0) {
            continue;
        }

        struct libevdev* test_dev;
        int rc = libevdev_new_from_fd(test_fd, &test_dev);
        if (rc < 0) {
            close(test_fd);
            continue;
        }

        // Check if this is a joystick/gamepad
        bool has_buttons = libevdev_has_event_type(test_dev, EV_KEY);
        bool has_axes = libevdev_has_event_type(test_dev, EV_ABS);

        if (!has_buttons || !has_axes) {
            libevdev_free(test_dev);
            close(test_fd);
            continue;
        }

        // Count buttons
        int button_count = 0;
        for (int i = BTN_JOYSTICK; i < BTN_JOYSTICK + 32; i++) {
            if (libevdev_has_event_code(test_dev, EV_KEY, i)) {
                button_count++;
            }
        }
        // Also check gamepad buttons
        for (int i = BTN_GAMEPAD; i < BTN_GAMEPAD + 32; i++) {
            if (libevdev_has_event_code(test_dev, EV_KEY, i)) {
                button_count++;
            }
        }

        // Count required axes (two analog sticks)
        int axis_count = 0;
        if (libevdev_has_event_code(test_dev, EV_ABS, ABS_X)) axis_count++;
        if (libevdev_has_event_code(test_dev, EV_ABS, ABS_Y)) axis_count++;
        if (libevdev_has_event_code(test_dev, EV_ABS, ABS_RX)) axis_count++;
        if (libevdev_has_event_code(test_dev, EV_ABS, ABS_RY)) axis_count++;

        std::cout << "Found device: " << libevdev_get_name(test_dev)
                  << " - Buttons: " << button_count
                  << ", Dual stick axes: " << axis_count << std::endl;

        if (button_count >= MIN_BUTTONS && axis_count >= REQUIRED_AXES) {
            device_path = path;
            last_device_path = path; // Store for future reconnection attempts
            fd = test_fd;
            dev = test_dev;
            state.device_name = libevdev_get_name(dev);
            state.connected = true;

            std::cout << "Selected joystick: " << state.device_name << std::endl;
            closedir(dir);
            return true;
        }

        libevdev_free(test_dev);
        close(test_fd);
    }

    closedir(dir);
    std::cerr << "No compatible joystick found (need " << MIN_BUTTONS
              << "+ buttons and dual analog sticks)" << std::endl;
    return false;
}

bool JoystickInput::initialize() {
    if (!findCompatibleDevice()) {
        return false;
    }

    // Load configuration-based mappings
    loadConfigurationMappings();

    // Initialize RC channels with proper values
    initializeRCChannels();

    std::cout << "Joystick initialized: " << state.device_name << std::endl;
    return true;
}

void JoystickInput::start() {
    if (!dev || running.load()) {
        return;
    }

    running.store(true);
    input_thread = std::thread(&JoystickInput::inputLoop, this);
}

void JoystickInput::stop() {
    if (running.load()) {
        running.store(false);
        if (input_thread.joinable()) {
            input_thread.join();
        }
    }
    state.connected = false;
}

void JoystickInput::inputLoop() {
    struct input_event ev;
    int rc;

    while (running.load()) {
        rc = libevdev_next_event(dev, LIBEVDEV_READ_FLAG_NORMAL, &ev);

        if (rc == LIBEVDEV_READ_STATUS_SUCCESS) {
            processEvent(ev);
        } else if (rc == LIBEVDEV_READ_STATUS_SYNC) {
            // Re-sync after missing events
            while (rc == LIBEVDEV_READ_STATUS_SYNC) {
                processEvent(ev);
                rc = libevdev_next_event(dev, LIBEVDEV_READ_FLAG_SYNC, &ev);
            }
        } else if (rc == -EAGAIN) {
            // No events available
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        } else {
            // Error occurred - likely disconnection
            std::cerr << "Joystick disconnected: " << strerror(-rc) << std::endl;

            // Mark as disconnected and reset state
            state.connected = false;
            resetJoystickState();
            g_joystick_state = state;

            // Disable RC override when joystick disconnects for safety
            if (g_joystick_config && g_joystick_config->isRcOverrideEnabled()) {
                g_joystick_config->toggleRcOverride();
            }

            // Clean up current connection
            cleanup();

            // Attempt reconnection
            attemptReconnection();

            // If we reach here, either reconnected successfully or shutting down
            if (!running.load()) break;
        }

        // Update RC channels after processing events
        updateRCChannels();

        // Process held buttons for continuous sending
        processHoldButtons();

        // Copy state to global
        g_joystick_state = state;
    }
}

void JoystickInput::processEvent(const struct input_event& ev) {
    switch (ev.type) {
        case EV_ABS: {
            // Store raw axis value
            raw_axis_values[ev.code] = ev.value;

            // Store axis events for RC channel mapping

            // Update normalized axis values
            switch (ev.code) {
                case ABS_X:
                    state.left_stick_x = normalizeAxis(ev.value, axis_mappings[ABS_X]);
                    break;
                case ABS_Y:
                    state.left_stick_y = normalizeAxis(ev.value, axis_mappings[ABS_Y]);
                    break;
                case ABS_RX:
                    state.right_stick_x = normalizeAxis(ev.value, axis_mappings[ABS_RX]);
                    break;
                case ABS_RY:
                    state.right_stick_y = normalizeAxis(ev.value, axis_mappings[ABS_RY]);
                    break;
                case ABS_Z:
                    state.left_trigger = normalizeAxis(ev.value, axis_mappings[ABS_Z]);
                    break;
                case ABS_RZ:
                    state.right_trigger = normalizeAxis(ev.value, axis_mappings[ABS_RZ]);
                    break;
            }
            break;
        }

        case EV_KEY: {
            bool pressed = (ev.value != 0);
            int button_index = -1;

            // Map button codes to array indices
            if (ev.code >= BTN_JOYSTICK && ev.code < BTN_JOYSTICK + 16) {
                button_index = ev.code - BTN_JOYSTICK;
            } else if (ev.code >= BTN_GAMEPAD && ev.code < BTN_GAMEPAD + 16) {
                button_index = ev.code - BTN_GAMEPAD;
            }

            // Handle button mappings (for all button types, not just indexed ones)
            auto it = button_mappings.find(ev.code);
            if (it != button_mappings.end()) {
                ButtonMapping& mapping = it->second;

                // Handle single press callback
                if (pressed && !mapping.was_pressed && mapping.callback) {
                    std::cout << "Calling button callback function" << std::endl;
                    mapping.callback();
                }

                // Handle hold state for continuous sending
                if (pressed && !mapping.was_pressed && mapping.hold_callback) {
                    mapping.is_held = true;
                    mapping.hold_start_time = std::chrono::steady_clock::now();
                    mapping.last_hold_send = mapping.hold_start_time;
                    std::cout << "Starting button hold for continuous sending" << std::endl;
                } else if (!pressed && mapping.was_pressed) {
                    mapping.is_held = false;
                    std::cout << "Button hold ended" << std::endl;
                }

                mapping.was_pressed = pressed;

                // Update RC channel if mapped
                if (mapping.rc_channel >= 0 && mapping.rc_channel < 16) {
                    state.rc_channels[mapping.rc_channel] = pressed ? mapping.on_value : mapping.off_value;
                }
            }
            // Update button state array for indexed buttons
            if (button_index >= 0 && button_index < 16) {
                bool was_pressed = state.buttons[button_index];
                state.buttons[button_index] = pressed;

                // Single press detection (pressed and not was pressed)
                state.button_pressed[button_index] = pressed && !was_pressed;
            }
            break;
        }
    }
}

float JoystickInput::normalizeAxis(int raw_value, const AxisMapping& mapping) {
    // Convert from min/max range to 0-1, then to -1 to +1
    float range = static_cast<float>(mapping.max_value - mapping.min_value);
    float normalized = static_cast<float>(raw_value - mapping.min_value) / range;
    normalized = normalized * 2.0f - 1.0f; // Convert to -1.0 to 1.0


    // Apply deadzone
    if (std::abs(normalized) < mapping.deadzone) {
        normalized = 0.0f;
    }

    // Apply expo curve (only for direct axes, not rate axes) - EdgeTX style
    if (!mapping.is_rate_axis && mapping.expo > 0.0f) {
        bool neg = (normalized < 0.0f);
        float abs_val = std::abs(normalized);

        if (abs_val > 1.0f) abs_val = 1.0f; // Clamp to valid range

        // Convert to 0-1024 range for calculation
        int x = static_cast<int>(abs_val * 1024.0f);
        int k = static_cast<int>(mapping.expo); // Already 0-100

        if (k == 0) {
            // No expo, keep original
        } else {
            // EdgeTX expo formula: f(x) = (k*x^3/1024^2 + x*(256-k) + 128) / 256
            // where k is converted from 0-100 to 0-256 range
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

void JoystickInput::updateRCChannels() {
    auto now = std::chrono::steady_clock::now();

    // Update RC channels based on axis mappings
    for (auto& pair : axis_mappings) {  // Note: non-const for rate axes
        int axis_code = pair.first;
        AxisMapping& mapping = pair.second;  // Note: non-const for rate axes

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
                        uint16_t old_value = mapping.rate_current_value;
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

    // Button RC channels are updated in processEvent(), not here
    // This prevents overriding button-specific states
}

void JoystickInput::mapAxisToRC(int axis_code, int rc_channel, bool invert) {
    if (rc_channel >= 0 && rc_channel < 16) {
        axis_mappings[axis_code].rc_channel = rc_channel;
        axis_mappings[axis_code].invert = invert;
    }
}

void JoystickInput::mapButtonToRC(int button_code, int rc_channel, uint16_t on_value, uint16_t off_value) {
    if (rc_channel >= 0 && rc_channel < 16) {
        button_mappings[button_code].rc_channel = rc_channel;
        button_mappings[button_code].on_value = on_value;
        button_mappings[button_code].off_value = off_value;
    }
}

void JoystickInput::mapButtonToFunction(int button_code, std::function<void()> callback) {
    button_mappings[button_code].callback = callback;
}

void JoystickInput::setAxisDeadzone(int axis_code, float deadzone) {
    axis_mappings[axis_code].deadzone = deadzone;
}

void JoystickInput::setAxisRange(int axis_code, int min_val, int max_val) {
    axis_mappings[axis_code].min_value = min_val;
    axis_mappings[axis_code].max_value = max_val;
}

void JoystickInput::loadConfigurationMappings() {
    if (!g_joystick_config) {
        std::cerr << "ERROR: No joystick configuration available! Using defaults." << std::endl;
        // Load defaults
        axis_mappings[ABS_X] = {0, false, 0.05f, -32767, 32767};
        axis_mappings[ABS_Y] = {1, true, 0.05f, -32767, 32767};
        axis_mappings[ABS_RX] = {3, false, 0.05f, -32767, 32767};
        axis_mappings[ABS_RY] = {2, true, 0.05f, -32767, 32767};
        return;
    }


    // Clear existing mappings
    axis_mappings.clear();
    button_mappings.clear();

    // Load axis mappings from config
    for (const auto& axis_config : g_joystick_config->getAxisConfigs()) {
        AxisMapping mapping;
        mapping.rc_channel = axis_config.rc_channel;
        mapping.invert = axis_config.invert;
        mapping.deadzone = axis_config.deadzone;
        mapping.min_value = axis_config.min_value;
        mapping.max_value = axis_config.max_value;
        mapping.is_rate_axis = axis_config.is_rate_axis;
        mapping.rate_scale = axis_config.rate_scale;
        mapping.expo = axis_config.expo;
        mapping.rate_init_value = axis_config.rate_init_value;
        mapping.rate_current_value = axis_config.rate_init_value;  // Initialize to configured value
        mapping.last_update_time = std::chrono::steady_clock::time_point{}; // Initialize to zero

        axis_mappings[axis_config.evdev_code] = mapping;

    }

    // Load button mappings from config
    for (const auto& button_config : g_joystick_config->getButtonConfigs()) {
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
                    std::cout << "Assigned " << button_config.function << " as hold callback (continuous send)" << std::endl;
                } else {
                    mapping.callback = func;
                }
            }
        }

        button_mappings[button_config.evdev_code] = mapping;
        std::cout << "Loaded button mapping: code=" << button_config.evdev_code
                  << " function=" << button_config.function
                  << " has_callback=" << (mapping.callback ? "yes" : "no") << std::endl;
    }
}

void JoystickInput::attemptReconnection() {
    const int RECONNECT_INTERVAL_MS = 2000; // Try every 2 seconds

    std::cout << "Attempting to reconnect joystick..." << std::endl;

    while (running.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(RECONNECT_INTERVAL_MS));

        // First, try to reconnect to the last known device path (faster)
        if (!last_device_path.empty() && tryReconnectToDevice(last_device_path)) {
            std::cout << "Joystick reconnected to previous device: " << state.device_name << std::endl;

            // Reload configuration mappings
            loadConfigurationMappings();

            // Initialize RC channels with proper values
            initializeRCChannels();

            // Resume normal operation - return to input loop
            return;
        }

        // If that fails, do a full device scan
        if (findCompatibleDevice()) {
            std::cout << "Joystick reconnected: " << state.device_name << std::endl;

            // Reload configuration mappings
            loadConfigurationMappings();

            // Initialize RC channels with proper values
            initializeRCChannels();

            // Resume normal operation - return to input loop
            return;
        }

        std::cout << "Joystick reconnection attempt failed, retrying in "
                  << (RECONNECT_INTERVAL_MS / 1000) << " seconds..." << std::endl;
    }
}

bool JoystickInput::tryReconnectToDevice(const std::string& path) {
    int test_fd = open(path.c_str(), O_RDONLY | O_NONBLOCK);
    if (test_fd < 0) {
        return false;
    }

    struct libevdev* test_dev;
    int rc = libevdev_new_from_fd(test_fd, &test_dev);
    if (rc < 0) {
        close(test_fd);
        return false;
    }

    // Verify this is still a compatible joystick
    bool has_buttons = libevdev_has_event_type(test_dev, EV_KEY);
    bool has_axes = libevdev_has_event_type(test_dev, EV_ABS);

    if (!has_buttons || !has_axes) {
        libevdev_free(test_dev);
        close(test_fd);
        return false;
    }

    // Count required axes (two analog sticks)
    int axis_count = 0;
    if (libevdev_has_event_code(test_dev, EV_ABS, ABS_X)) axis_count++;
    if (libevdev_has_event_code(test_dev, EV_ABS, ABS_Y)) axis_count++;
    if (libevdev_has_event_code(test_dev, EV_ABS, ABS_RX)) axis_count++;
    if (libevdev_has_event_code(test_dev, EV_ABS, ABS_RY)) axis_count++;

    if (axis_count < REQUIRED_AXES) {
        libevdev_free(test_dev);
        close(test_fd);
        return false;
    }

    // Success! Set up the connection
    device_path = path;
    fd = test_fd;
    dev = test_dev;
    state.device_name = libevdev_get_name(dev);
    state.connected = true;

    return true;
}

void JoystickInput::resetJoystickState() {
    // Reset RC channels to 0 (unmapped channels stay 0)
    for (int i = 0; i < 16; i++) {
        state.rc_channels[i] = 0;
    }

    // Reset rate axes to their configured initialization values
    for (auto& [evdev_code, mapping] : axis_mappings) {
        if (mapping.is_rate_axis && mapping.rc_channel >= 0 && mapping.rc_channel < 16) {
            mapping.rate_current_value = mapping.rate_init_value;
            state.rc_channels[mapping.rc_channel] = mapping.rate_init_value;
        }
    }

    // Reset analog inputs to center
    state.left_stick_x = 0.0f;
    state.left_stick_y = 0.0f;
    state.right_stick_x = 0.0f;
    state.right_stick_y = 0.0f;
    state.left_trigger = 0.0f;
    state.right_trigger = 0.0f;

    // Reset all button states
    for (int i = 0; i < 16; i++) {
        state.buttons[i] = false;
        state.button_pressed[i] = false;
    }

    // Clear device info but keep connected flag for caller to set
    state.device_name = "Disconnected";
}

void JoystickInput::cleanup() {
    if (dev) {
        libevdev_free(dev);
        dev = nullptr;
    }
    if (fd >= 0) {
        close(fd);
        fd = -1;
    }
    device_path.clear();
}

void JoystickInput::initializeRCChannels() {
    // Initialize all RC channels to 0 (unmapped channels stay 0)
    for (int i = 0; i < 16; i++) {
        state.rc_channels[i] = 0;
    }

    // Set rate axes to their configured initialization values
    for (auto& [evdev_code, mapping] : axis_mappings) {
        if (mapping.is_rate_axis && mapping.rc_channel >= 0 && mapping.rc_channel < 16) {
            state.rc_channels[mapping.rc_channel] = mapping.rate_init_value;
            std::cout << "Initialized rate axis RC channel " << mapping.rc_channel
                      << " to " << mapping.rate_init_value << "μs" << std::endl;
        }
    }

    // Update global state
    g_joystick_state = state;
}

void JoystickInput::processHoldButtons() {
    const auto now = std::chrono::steady_clock::now();
    const auto hold_interval = std::chrono::milliseconds(200); // 5Hz = 200ms interval

    for (auto& [evdev_code, mapping] : button_mappings) {
        if (mapping.is_held && mapping.hold_callback) {
            // Check if enough time has passed since last send
            if (now - mapping.last_hold_send >= hold_interval) {
                mapping.hold_callback();
                mapping.last_hold_send = now;
            }
        }
    }
}
