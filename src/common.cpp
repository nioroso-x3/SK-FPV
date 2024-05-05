#include "common.h"

using namespace sk;
using namespace mavsdk;

mesh_t     plane_mesh;
material_t plane_mat;
pose_t     plane_pose = {{0,0.3,-1.5f}, {0,0,0,1}};
tex_t vid0;

//second screen
mesh_t     plane1_mesh;
material_t plane1_mat;
pose_t     plane1_pose = {{0,-1.0f,-1.2f}, {-0.258819,0,0,0.9659258}};
tex_t vid1;

//HUD
mesh_t     hud_mesh;
material_t hud_mat;
pose_t     hud_pose;
tex_t      hud_tex;



//Double buffers for the screens
cv::Mat buffer0[2];
cv::Mat buffer1[2];
cv::Mat buffer2[2];

//Current buffer being used
char cur_buffer0 = 0;
char cur_buffer1 = 0;
char cur_buffer2 = 0;

uint64_t cnt = 0;


//vehicle position
//attributes are latitude_deg, longitude_deg absolute_altitude_m and relative_altitude_m
Telemetry::Position vh_pos;

//vehicle attitude
//attributes are roll_deg, pitch_deg, and yaw_deg
// positive is bank right, nose up and rotating right
Telemetry::EulerAngle vh_att;

//vehicle batteries 0 and 1
//attributes are id, voltage_v, current_battery_a
Telemetry::Battery vh_bat0;
Telemetry::Battery vh_bat1;

//vehicle airspeed_m_s, throttle_percentage and climb_rate_m_s
Telemetry::FixedwingMetrics vh_fwing;

//vehicle gps info, num_satellites and fix_type
Telemetry::GpsInfo vh_gpsi;

//vehicle raw gps info, velocity_m_s is the ground speed
Telemetry::RawGps vh_gpsr;


//vehicle status messages
//should hold the last status
int status_counter = 0;
Telemetry::StatusText vh_st_text;


