#include "mavlink_setup.h"
using namespace mavsdk;

void start_mavlink_thread(){
  bool found_wfb = false;
  std::cerr << "Started mavlink setup thread" << std::endl;
  Mavsdk mavsdk{Mavsdk::Configuration{Mavsdk::ComponentType::GroundStation}};
  ConnectionResult connection_result = mavsdk.add_any_connection("udp://0.0.0.0:14551");

  if (connection_result != ConnectionResult::Success) {
      std::cerr << "Connection failed: " << connection_result << '\n';
  }

  auto system = mavsdk.first_autopilot(3.0);
  while(!system) {
      std::cerr << "Connection timeout, waiting 30 seconds " << std::endl;
      system = mavsdk.first_autopilot(30.0);
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
  while(true){
    //wait for wfb system id to start sending data
    //
    auto compids = system.value().component_ids();
    for (int i = 0; i < compids.size(); ++i){
      std::cout << "comp ids: " << int(compids[i]) << std::endl;
    }
    if (!found_wfb){
      auto systems = mavsdk.systems();
      std::cout << "num systems: " << systems.size() << std::endl;
      for (int i = 0; i < systems.size(); ++i){
        std::cout << "sys id: " << systems[i]->get_system_id() << std::endl;
        if (systems[i]->get_system_id() == 3)
          std::cout << "mavlink system found" << std::endl;
          //found the wfb link system
          auto wfbpass = mavsdk::MavlinkPassthrough{systems[i]};
          //mavlink stream stats
          wfbpass.subscribe_message(MAVLINK_MSG_ID_RADIO_STATUS,[](const mavlink_message_t &msg){
            std::cout << msg.sysid << std::endl;
            if ((msg.sysid == 3) || (msg.compid == 68)) {
              wfb_rssi = (int8_t)mavlink_msg_radio_status_get_rssi(&msg);
              wfb_errors = mavlink_msg_radio_status_get_rxerrors(&msg);
              wfb_fec_fixed = mavlink_msg_radio_status_get_fixed(&msg);
              wfb_flags = mavlink_msg_radio_status_get_remnoise(&msg);
            }
            std::cout << wfb_rssi << std::endl;
            std::cout << wfb_errors << std::endl;
            std::cout << wfb_fec_fixed << std::endl;
            std::cout << wfb_flags << std::endl;
          });
        found_wfb = true;
      }
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
  }




}

