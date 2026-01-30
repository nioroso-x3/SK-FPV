#ifndef JOYSTICK_INPUT_H
#define JOYSTICK_INPUT_H

#include <libevdev/libevdev.h>
#include <string>
#include <thread>
#include <atomic>
#include <map>
#include <functional>
#include <chrono>
#include "joystick_config.h"

struct JoystickState {
    // Analog axes (normalized -1.0 to 1.0)
    float left_stick_x = 0.0f;
    float left_stick_y = 0.0f;
    float right_stick_x = 0.0f;
    float right_stick_y = 0.0f;
    float left_trigger = 0.0f;
    float right_trigger = 0.0f;

    // Button states
    bool buttons[16] = {false}; // Support up to 16 buttons
    bool button_pressed[16] = {false}; // Single press detection

    // RC channel outputs (1000-2000 range for RC)
    uint16_t rc_channels[16] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

    // Device info
    std::string device_name;
    bool connected = false;
};

class JoystickInput {
public:
    JoystickInput();
    ~JoystickInput();

    bool initialize();
    void start();
    void stop();

    const JoystickState& getState() const { return state; }

    // Configuration
    void mapAxisToRC(int axis_code, int rc_channel, bool invert = false);
    void mapButtonToRC(int button_code, int rc_channel, uint16_t on_value = 2000, uint16_t off_value = 1000);
    void mapButtonToFunction(int button_code, std::function<void()> callback);

    // Axis configuration
    void setAxisDeadzone(int axis_code, float deadzone);
    void setAxisRange(int axis_code, int min_val, int max_val);

private:
    struct AxisMapping {
        int rc_channel = -1;
        bool invert = false;
        float deadzone = 0.05f;
        int min_value = -32767;
        int max_value = 32767;
        bool is_rate_axis = false;
        float rate_scale = 250.0f;  // microseconds per second
        uint16_t rate_current_value = 1500;  // Current RC value for rate axes
        uint16_t rate_init_value = 1500;  // Initial value for rate axes
        std::chrono::steady_clock::time_point last_update_time;
        float expo = 0.0f;  // 0-100 expo curve strength for direct axes
    };

    struct ButtonMapping {
        int rc_channel = -1;
        uint16_t on_value = 2000;
        uint16_t off_value = 1000;
        std::function<void()> callback = nullptr;
        std::function<void()> hold_callback = nullptr; // Function to call while button held
        bool was_pressed = false;
        bool is_held = false;
        std::chrono::steady_clock::time_point hold_start_time;
        std::chrono::steady_clock::time_point last_hold_send;
    };

    bool findCompatibleDevice();
    bool tryReconnectToDevice(const std::string& path);
    void inputLoop();
    void processEvent(const struct input_event& ev);
    float normalizeAxis(int raw_value, const AxisMapping& mapping);
    void updateRCChannels();
    void loadConfigurationMappings();
    void initializeRCChannels();
    void processHoldButtons();
    void attemptReconnection();
    void resetJoystickState();
    void cleanup();

    struct libevdev* dev = nullptr;
    int fd = -1;
    std::string device_path;
    std::string last_device_path; // Store last successful device path for fast reconnection

    JoystickState state;
    std::atomic<bool> running{false};
    std::thread input_thread;

    // Mappings
    std::map<int, AxisMapping> axis_mappings;
    std::map<int, ButtonMapping> button_mappings;

    // Raw axis values
    std::map<int, int> raw_axis_values;

    // Device requirements
    static constexpr int MIN_BUTTONS = 8;
    static constexpr int REQUIRED_AXES = 4; // Two analog sticks
};

// Global joystick state accessible from other modules
extern JoystickInput* g_joystick;
extern JoystickState g_joystick_state;

#endif // JOYSTICK_INPUT_H