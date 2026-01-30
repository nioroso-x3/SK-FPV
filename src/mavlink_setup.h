#ifndef MAVLINK_SETUP_H
#define MAVLINK_SETUP_H
#include "common.h"
#define WFB_RADIO_SYSTEM_ID 3

void start_mavlink_thread();
void start_rc_override_thread();

// Flight mode functions
void set_flight_mode_manual();
void set_flight_mode_stabilize();
void set_flight_mode_fbwa();
void set_flight_mode_cruise();
void set_flight_mode_auto();
void set_flight_mode_rtl();
void set_flight_mode_loiter();
void set_flight_mode_guided();

// Global MAVLink passthrough for RC override
extern std::shared_ptr<mavsdk::MavlinkPassthrough> g_mavlink_passthrough;

#endif
