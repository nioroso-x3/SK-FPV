#ifndef JOYSTICK_OPENXR_H
#define JOYSTICK_OPENXR_H

#include "joystick_input.h"
#include <stereokit.h>

using namespace sk;

// OpenXR constants for configuration (public so config parser can access them)
constexpr int OPENXR_LEFT_STICK_X = 1000;
constexpr int OPENXR_LEFT_STICK_Y = 1001;
constexpr int OPENXR_RIGHT_STICK_X = 1002;
constexpr int OPENXR_RIGHT_STICK_Y = 1003;
constexpr int OPENXR_LEFT_TRIGGER = 1004;
constexpr int OPENXR_RIGHT_TRIGGER = 1005;
constexpr int OPENXR_LEFT_GRIP = 1006;
constexpr int OPENXR_RIGHT_GRIP = 1007;

constexpr int OPENXR_LEFT_X1 = 2000;
constexpr int OPENXR_LEFT_X2 = 2001;
constexpr int OPENXR_LEFT_STICK_CLICK = 2002;
constexpr int OPENXR_RIGHT_X1 = 2003;
constexpr int OPENXR_RIGHT_X2 = 2004;
constexpr int OPENXR_RIGHT_STICK_CLICK = 2005;
constexpr int OPENXR_MENU_BUTTON = 2006;

class OpenXRJoystickInput {
public:
    OpenXRJoystickInput();
    ~OpenXRJoystickInput();

    bool initialize();
    void start();
    void stop();

    const JoystickState& getState() const { return state; }

    // Configuration methods (same interface as JoystickInput)
    void mapAxisToRC(int axis_code, int rc_channel, bool invert = false);
    void mapButtonToRC(int button_code, int rc_channel, uint16_t on_value = 2000, uint16_t off_value = 1000);
    void mapButtonToFunction(int button_code, std::function<void()> callback);
    void setAxisDeadzone(int axis_code, float deadzone);
    void setAxisRange(int axis_code, int min_val, int max_val);

private:
    enum OpenXRAxis {
        OPENXR_LEFT_STICK_X = 1000,
        OPENXR_LEFT_STICK_Y = 1001,
        OPENXR_RIGHT_STICK_X = 1002,
        OPENXR_RIGHT_STICK_Y = 1003,
        OPENXR_LEFT_TRIGGER = 1004,
        OPENXR_RIGHT_TRIGGER = 1005,
        OPENXR_LEFT_GRIP = 1006,
        OPENXR_RIGHT_GRIP = 1007
    };

    enum OpenXRButton {
        OPENXR_LEFT_X1 = 2000,
        OPENXR_LEFT_X2 = 2001,
        OPENXR_LEFT_STICK_CLICK = 2002,
        OPENXR_RIGHT_X1 = 2003,
        OPENXR_RIGHT_X2 = 2004,
        OPENXR_RIGHT_STICK_CLICK = 2005,
        OPENXR_MENU_BUTTON = 2006
    };

    struct AxisMapping {
        int rc_channel = -1;
        bool invert = false;
        float deadzone = 0.05f;
        bool is_rate_axis = false;
        float rate_scale = 250.0f;
        uint16_t rate_current_value = 1500;
        uint16_t rate_init_value = 1500;
        std::chrono::steady_clock::time_point last_update_time;
        float expo = 0.0f;
    };

    struct ButtonMapping {
        int rc_channel = -1;
        uint16_t on_value = 2000;
        uint16_t off_value = 1000;
        std::function<void()> callback = nullptr;
        std::function<void()> hold_callback = nullptr;
        bool was_pressed = false;
        bool is_held = false;
        std::chrono::steady_clock::time_point hold_start_time;
        std::chrono::steady_clock::time_point last_hold_send;
    };

    void updateControllerState();
    void processButton(OpenXRButton button_code, bool pressed);
    float normalizeAxis(float raw_value, const AxisMapping& mapping);
    void updateRCChannels();
    void loadConfigurationMappings();
    void initializeRCChannels();
    void processHoldButtons();

    JoystickState state;
    std::atomic<bool> running{false};
    std::thread update_thread;

    // Previous controller states for button press detection
    controller_t prev_left_controller;
    controller_t prev_right_controller;

    // Mappings
    std::map<int, AxisMapping> axis_mappings;
    std::map<int, ButtonMapping> button_mappings;

    // Raw axis values storage
    std::map<int, float> raw_axis_values;
};

#endif // JOYSTICK_OPENXR_H