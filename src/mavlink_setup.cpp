#include "mavlink_setup.h"
#include "joystick_input.h"
#include <algorithm>
using namespace mavsdk;

// Global MAVLink passthrough for RC override
std::shared_ptr<mavsdk::MavlinkPassthrough> g_mavlink_passthrough = nullptr;

// Helper function to find mode index in fmodes array
int get_flight_mode_index(const std::string& mode_name) {
    for (int i = 0; i < 25; i++) { // fmodes array size
        if (fmodes[i] == mode_name) {
            return i;
        }
    }
    return -1; // Mode not found
}

// Helper function to check if RC disable switch is active
bool is_rc_disable_switch_active() {
    if (!g_joystick_config) return false;

    const auto& settings = g_joystick_config->getSettings();
    if (settings.rc_disable_switch_channel < 0 || settings.rc_disable_switch_channel >= 16) {
        return false; // Disable switch not configured or out of range
    }

    // Check if the specified RC channel is above threshold
    extern JoystickState g_joystick_state;
    uint16_t channel_value = g_joystick_state.rc_channels[settings.rc_disable_switch_channel];
    return channel_value > settings.rc_disable_switch_threshold;
}

// Helper function to send flight mode command
void send_flight_mode_command(const std::string& mode_name) {
    if (!g_mavlink_passthrough) return;

    // Check RC disable switch first (master kill switch)
    if (is_rc_disable_switch_active()) {
        std::cout << "Flight mode command blocked - RC disable switch is active" << std::endl;
        return;
    }

    // Only allow flight mode commands when RC override is active
    if (!rc_override) {
        std::cout << "Flight mode command blocked - RC override not active" << std::endl;
        return;
    }

    int mode_index = get_flight_mode_index(mode_name);
    if (mode_index >= 0) {
        g_mavlink_passthrough->queue_message([=](MavlinkAddress mavlink_address, uint8_t channel) {
            mavlink_message_t msg;
            mavlink_msg_set_mode_pack(
                mavlink_address.system_id,
                mavlink_address.component_id,
                &msg,
                1,  // target_system
                MAV_MODE_FLAG_CUSTOM_MODE_ENABLED,
                mode_index
            );
            return msg;
        });
        std::cout << "Setting flight mode: " << mode_name << " (mode " << mode_index << ")" << std::endl;
    }
}

void start_mavlink_thread(){
  bool found_wfb = false;
  std::cerr << "MAVLINK: Started mavlink setup thread" << std::endl;
  Mavsdk mavsdk{Mavsdk::Configuration{Mavsdk::ComponentType::GroundStation}};
  ConnectionResult connection_result = mavsdk.add_any_connection("udp://0.0.0.0:14551");

  if (connection_result != ConnectionResult::Success) {
      std::cerr << "MAVLINK: Connection failed: " << connection_result << '\n';
  }

  auto system = mavsdk.first_autopilot(3.0);
  while(!system) {
      std::cerr << "MAVLINK: connection timeout, waiting 10 seconds." << std::endl;
      system = mavsdk.first_autopilot(3.0);
      std::this_thread::sleep_for(std::chrono::milliseconds(10000));
  }


   // Instantiate plugins.
  auto telemetry = mavsdk::Telemetry{system.value()};
  auto passthrough = mavsdk::MavlinkPassthrough{system.value()};

  // Store passthrough globally for RC override
  g_mavlink_passthrough = std::make_shared<mavsdk::MavlinkPassthrough>(system.value());
  telemetry.subscribe_position([](Telemetry::Position position) {
     vh_pos = position;
  });
  telemetry.subscribe_attitude_euler([](Telemetry::EulerAngle att){
     vh_att = att;
  });
  telemetry.subscribe_battery([](Telemetry::Battery bat){
     if (bat.id == 0){
        vh_bat0 = bat;
     }
     if (bat.id == 1){
        vh_bat1 = bat;
     }
  });
  telemetry.subscribe_fixedwing_metrics([](Telemetry::FixedwingMetrics fwing){
     vh_fwing = fwing;
  });
  telemetry.subscribe_gps_info([](Telemetry::GpsInfo gpsi){
     vh_gpsi = gpsi;
  });
  telemetry.subscribe_raw_gps([](Telemetry::RawGps gpsr){
     vh_gpsr = gpsr;
  });
 

  telemetry.subscribe_status_text([](Telemetry::StatusText st_text){
     if (st_text.text != vh_st_text.text){
       vh_st_text = st_text;
       status_counter = 50*15; //show message for 15s with HUD running at 50 fps
     }
  });

  telemetry.subscribe_rc_status([](Telemetry::RcStatus rc_st){
    vh_rc = rc_st;
  });

  //should put an if here to choose from Px4 or ardupilot but I dont use PX4
  /*telemetry.subscribe_flight_mode([](Telemetry::FlightMode fmode){
    int idx = (int)fmode;
    if (idx < 18) vh_fmode = fmodes[(int)fmode];
    else vh_fmode = "Unknown";
    std::cout << vh_fmode  << " " << idx << std::endl;
  });*/
  //PX4 flight modes are different from ardupilot
  passthrough.subscribe_message(MAVLINK_MSG_ID_HEARTBEAT,[](const mavlink_message_t &msg){
    mavlink_heartbeat_t heartbeat;
    mavlink_msg_heartbeat_decode(&msg,&heartbeat);
    if ((0 <= heartbeat.custom_mode) && (heartbeat.custom_mode < 25)) vh_fmode = fmodes[heartbeat.custom_mode];
    else vh_fmode = "UNKNOWN";
  });

  telemetry.subscribe_home([](Telemetry::Position position){
    home_pos = position;
  });
 
  telemetry.subscribe_distance_sensor([](Telemetry::DistanceSensor sensor){
      vh_rngfnd = sensor;
  });
  //System Time
  passthrough.subscribe_message(MAVLINK_MSG_ID_SYSTEM_TIME,[](const mavlink_message_t &msg){
    mavlink_system_time_t systemtime;
    mavlink_msg_system_time_decode(&msg,&systemtime);
    gps_time = systemtime.time_unix_usec;
  });
  //AoA and SSA
  passthrough.subscribe_message(MAVLINK_MSG_ID_AOA_SSA,[](const mavlink_message_t &msg){
    mavlink_aoa_ssa_t aoa_ssa;
    mavlink_msg_aoa_ssa_decode(&msg,&aoa_ssa);
    aoa = aoa_ssa.AOA;
    ssa = aoa_ssa.SSA;
  });




  //debugging packets from the source
 /*
  mavsdk.intercept_incoming_messages_async([](mavlink_message_t &msg) -> bool{
    if (msg.compid == 68){
    
      std::cout << int(msg.msgid) << std::endl;
      std::cout << int(msg.sysid) << std::endl;
    }
    return true;

  });*/
  //wait for the wfb radio system to appear 
  auto systems = mavsdk.systems();
  int radio_idx = -1;
  while(radio_idx == -1){
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    systems = mavsdk.systems();
    for (int i = 0; i < systems.size(); ++i){
      if (systems[i]->get_system_id() == WFB_RADIO_SYSTEM_ID){
        radio_idx = i;
        break;
      }
    }
  }
  
  auto ps = mavsdk::MavlinkPassthrough{systems[radio_idx]};
  ps.subscribe_message(MAVLINK_MSG_ID_RADIO_STATUS,[](const mavlink_message_t &msg){
    if (msg.compid == MAV_COMP_ID_TELEMETRY_RADIO) {
      wfb_rssi = (int8_t)mavlink_msg_radio_status_get_rssi(&msg);
      wfb_errors = mavlink_msg_radio_status_get_rxerrors(&msg);
      wfb_fec_fixed = mavlink_msg_radio_status_get_fixed(&msg);
      wfb_flags = mavlink_msg_radio_status_get_remnoise(&msg);
    }
  });
  
  
  ps.subscribe_message(MAVLINK_MSG_ID_CELLULAR_STATUS,[](const mavlink_message_t &msg){
    lte_stat = (uint8_t)mavlink_msg_cellular_status_get_status(&msg);
    lte_f_reason = (uint8_t)mavlink_msg_cellular_status_get_failure_reason(&msg);
    lte_type = (uint8_t)mavlink_msg_cellular_status_get_type(&msg);
    lte_qual = (uint8_t)mavlink_msg_cellular_status_get_quality(&msg);

  });
  
  /*
  auto ps1 = mavsdk::MavlinkPassthrough{systems[0]};
  ps1.subscribe_message(MAVLINK_MSG_ID_RC_CHANNELS,[](const mavlink_message_t &msg){
    mavlink_rc_channels_t *rc_channels;
    mavlink_msg_rc_channels_decode(&msg,rc_channels);
    if (rc_channels->chancount >= 12){
      cam_selector = rc_channels->chan7_raw; 
      std::cout << "Channel 7: " << cam_selector << std::endl;
    }

  });
  */
  while(true){
/*    std::cout << int(wfb_rssi) << std::endl;
    std::cout << int(wfb_errors) << std::endl;
    std::cout << int(wfb_fec_fixed) << std::endl;
    std::cout << int(wfb_flags) << std::endl;*/
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
  }
}

void start_rc_override_thread() {
  std::cout << "Starting RC override thread" << std::endl;

  // Wait for MAVLink passthrough to be ready
  while (!g_mavlink_passthrough) {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }

  // Get update rate from config (default 20Hz = 50ms)
  int update_interval_ms = 50;
  if (g_joystick_config && g_joystick_config->getSettings().enable_rc_override) {
    update_interval_ms = 1000 / g_joystick_config->getSettings().update_rate_hz;
  }

  std::cout << "RC override running at " << (1000.0f / update_interval_ms) << " Hz" << std::endl;

  auto last_update = std::chrono::steady_clock::now();

  while (true) {
    // Check RC disable switch first (master kill switch)
    if (is_rc_disable_switch_active()) {
      // Force RC override OFF when disable switch is active
      if (g_joystick_config && g_joystick_config->getSettings().enable_rc_override) {
        g_joystick_config->setRcOverride(false);
        std::cout << "RC disable switch active - forcing RC override OFF" << std::endl;
      }
    }

    // Check if RC override is enabled and joystick is connected
    if (g_joystick_config &&
        g_joystick_config->getSettings().enable_rc_override &&
        g_joystick_state.connected &&
        !is_rc_disable_switch_active()) {

      // Send RC override message using queue_message with lambda
      try {
        g_mavlink_passthrough->queue_message([&](MavlinkAddress mavlink_address, uint8_t channel) {
          mavlink_message_t msg;

          // Pack RC channels override message
          mavlink_msg_rc_channels_override_pack(
            mavlink_address.system_id,
            mavlink_address.component_id,
            &msg,
            1,  // target_system (autopilot)
            MAV_COMP_ID_AUTOPILOT1,  // target_component
            g_joystick_state.rc_channels[0],   // chan1_raw (Roll)
            g_joystick_state.rc_channels[1],   // chan2_raw (Pitch)
            g_joystick_state.rc_channels[2],   // chan3_raw (Throttle)
            g_joystick_state.rc_channels[3],   // chan4_raw (Yaw)
            g_joystick_state.rc_channels[4],   // chan5_raw (Aux1)
            g_joystick_state.rc_channels[5],   // chan6_raw (Aux2)
            g_joystick_state.rc_channels[6],   // chan7_raw (Aux3)
            g_joystick_state.rc_channels[7],   // chan8_raw (Aux4)
            0,  // chan9_raw (unused)
            0,  // chan10_raw (unused)
            0,  // chan11_raw (unused)
            0,  // chan12_raw (unused)
            0,  // chan13_raw (unused)
            0,  // chan14_raw (unused)
            0,  // chan15_raw (unused)
            0,  // chan16_raw (unused)
            0,  // chan17_raw (unused)
            0   // chan18_raw (unused)
          );

          return msg;
        });
      } catch (const std::exception& e) {
        // Only log occasional failures to avoid spam
        static int error_count = 0;
        if (error_count++ % 100 == 0) {
          std::cerr << "Failed to send RC override: " << e.what() << std::endl;
        }
      }
    }

    // Precise timing control
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_update);

    if (elapsed.count() < update_interval_ms) {
      std::this_thread::sleep_for(std::chrono::milliseconds(update_interval_ms - elapsed.count()));
    }

    last_update = std::chrono::steady_clock::now();
  }

  std::cout << "RC override thread exited" << std::endl;
}

// Flight mode functions
void set_flight_mode_manual() { send_flight_mode_command("MANUAL"); }
void set_flight_mode_stabilize() { send_flight_mode_command("STABILIZE"); }
void set_flight_mode_fbwa() { send_flight_mode_command("FBWA"); }
void set_flight_mode_cruise() { send_flight_mode_command("CRUISE"); }
void set_flight_mode_auto() { send_flight_mode_command("Auto"); }
void set_flight_mode_rtl() { send_flight_mode_command("RTL"); }
void set_flight_mode_loiter() { send_flight_mode_command("Loiter"); }
void set_flight_mode_guided() { send_flight_mode_command("Guided"); }

