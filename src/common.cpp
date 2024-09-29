#include "common.h"

using namespace sk;
using namespace mavsdk;


 //vector with the flightmodes
std::string fmodes_PX4[] ={
    "Unknown",
    "FBWA",
    "Autotune",
    "Guided",
    "Ready",
    "Takeoff",
    "Hold",
    "Mission",
    "RTL",
    "Land",
    "Offboard",
    "FollowMe",
    "Manual",
    "Altctl",
    "Posctl",
    "Acro",
    "Rattitude",
    "Stabilized"};


std::string fmodes[] ={
"MANUAL",
"CIRCLE",
"STABILIZE",
"TRAINING",
"ACRO",
"FBWA",
"FBWB",
"CRUISE",
"AUTOTUNE",
"UNKNOWN_9",
"Auto",
"RTL",
"Loiter",
"TAKEOFF",
"AVOID_ADSB",
"Guided",
"QSTABILIZE",
"QHOVER",
"QLOITER",
"QLAND",
"QRTL",
"QAUTOTUNE",
"QACRO",
"THERMAL",
"LOITERQLAND"};




mesh_t     plane_mesh;
material_t plane_mat;
pose_t     plane_pose = {{0.0f,0.2,-2.0f}, {0,0,0,1}};
tex_t vid0;

//second screen
mesh_t     plane1_mesh;
material_t plane1_mat;
pose_t     plane1_pose = {{0.0f,-1.1f,-1.2f}, {-0.258819,0.0,0.0,0.9659258}};
tex_t vid1;

//ground stereo cameras
mesh_t     plane2_mesh;
material_t plane2_mat;
pose_t     plane2_pose = {{1.5f,-1.1f,-1.1f}, {-0.2521877,-0.3782815,0,0.8906764}};
//pose_t     plane2_pose = {{0,-1.7f,-0.45f}, {-0.7071068,0,0,0.7071068}};
tex_t vid2;

//ground stereo cameras
mesh_t     plane3_mesh;
material_t plane3_mat;
pose_t     plane3_pose = {{-1.5f,-1.1f,-1.1f}, {-0.2521877,0.3782815,0,0.8906764}};
//pose_t     plane2_pose = {{0,-1.7f,-0.45f}, {-0.7071068,0,0,0.7071068}};
tex_t vid3;


//left window
mesh_t     plane4_mesh;
material_t plane4_mat;
pose_t     plane4_pose = {{-2.2f,0.2f,0.3f}, {0.0,0.7071068,0.0,0.7071068}};
tex_t vid4;


//right window
mesh_t     plane5_mesh;
material_t plane5_mat;
pose_t     plane5_pose = {{2.2f,0.2f,0.3f}, {0.0,-0.7071608,0.0,0.7071068}};
tex_t      vid5;

bool run_stab = false;
bool gnd_cam_color = true;

//cubemap for skybox
tex_t sky;

//HUD
mesh_t     hud_mesh;
material_t hud_mat;
pose_t     hud_pose;
tex_t      hud_tex;

//map screen
mesh_t     plane6_mesh;
material_t plane6_mat;
pose_t     plane6_pose = {{-1.7f,1.0f,-1.99f}, {0,0,0,1}};
tex_t      vid6;
double     map_zoom = 16;



float p1s = 2.2f;
float p2s = 1.0f;
float p3s = 1.0f;
float pWs = 2.2f;
float hud_s = 0.5f;

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

//rc signal strenght
Telemetry::RcStatus vh_rc;

//flight mode
std::string vh_fmode = "UNKNOWN";

//home position
Telemetry::Position home_pos;


int8_t wfb_rssi = 0;
uint16_t wfb_errors = 0;
uint16_t wfb_fec_fixed = 0;
int8_t wfb_flags = 0;

Telemetry::DistanceSensor vh_rngfnd;

std::map<std::string,std::vector<int>> wfb_rx; 

uint64_t gps_time = 0;

float get_heading(float lon1, float lat1, float lon2, float lat2) {
    lon1 *= M_PI/180.0;
    lon2 *= M_PI/180.0;
    lat1 *= M_PI/180.0;
    lat2 *= M_PI/180.0;
    float y = sin(lon2 - lon1) * cos(lat2);
    float x = (cos(lat1) * sin(lat2)) - ( sin(lat1) * cos(lat2) * cos(lon2 - lon1));
    float o = atan2(y,x);
    return fmodf(o*180/M_PI + 360,360);
}

float get_distance(float lon1, float lat1, float lon2, float lat2) {
    const float R = 6371000.0; // Earth's radius in meters
    float dLat = (lat2 - lat1) * M_PI / 180.0;
    float dLon = (lon2 - lon1) * M_PI / 180.0;
    
    float a = sin(dLat / 2) * sin(dLat / 2) +
              cos(lat1 * M_PI / 180.0) * cos(lat2 * M_PI / 180.0) *
              sin(dLon / 2) * sin(dLon / 2);
    float c = 2 * atan2(sqrt(a), sqrt(1 - a));
    
    return R * c; // Distance in meters
}
