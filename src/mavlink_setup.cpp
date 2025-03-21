#include "mavlink_setup.h"
using namespace mavsdk;

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
  
  passthrough.subscribe_message(MAVLINK_MSG_ID_SYSTEM_TIME,[](const mavlink_message_t &msg){
    mavlink_system_time_t systemtime;
    mavlink_msg_system_time_decode(&msg,&systemtime);
    gps_time = systemtime.time_unix_usec;
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

