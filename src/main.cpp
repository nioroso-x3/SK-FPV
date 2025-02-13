#include "common.h"
#include "hud.h"
#include "mavlink_setup.h"
#include "stabilization.h"
#include "mapping.h"
#include "wfb_stats.h"

using namespace sk;
using namespace mavsdk;

//video output
cv::VideoWriter output;
//callback needed for the video rendering function
void scr_callback(color32* buffer, int w, int h, void *ctx){
    cv::Mat out(w, h, CV_8UC4, buffer);
    cv::cvtColor(out,out,cv::COLOR_RGBA2BGR);
    if (output.isOpened())
    output << out;
}

void mavlink_thread()
{
  start_mavlink_thread();
  std::cerr << "Mavlink thread exited" << std::endl;
}

void draw_hud()
{
  std::cerr << "Starting HUD thread" << std::endl;
  draw_cairo_hud();
  std::cerr << "HUD thread exited" << std::endl;
}

void map_thread(){
  std::cerr << "Starting Map thread" << std::endl;
  cv::Rect roi(0,0,MAP_W,MAP_H);
  cv::Mat cur_loc;
  cv::Mat mask(MAP_W,MAP_H,CV_8UC4,cv::Scalar(0,0,0,255));
  int cx = MAP_W/2 - 1;
  int cy = MAP_H/2 - 1;
  //draw triangle representing vehicle
  cv::Point p1( cx   , cy-10);
  cv::Point p2( cx-10, cy+15);
  cv::Point p3( cx+10, cy+15);
  cv::Scalar color(128,255,128,255);
  //draw circular transparent mask
  cv::circle(mask,cv::Point(cx,cy), cx , cv::Scalar(0,0,0,0), -1, cv::LINE_AA); 
  //set to your API key and style JSON
  maplibre maps(std::string(""),std::string("file://./style.json"));
  //load the circle and home markers
  maps.add_icon(std::string("circle"),std::string("circle.png"));
  maps.add_icon(std::string("home"),std::string("home.png"));
  while(true){
    cv::Mat output(1536,2048,CV_8UC4,cv::Scalar(0,0,0,0));
    //set home in the map
    int home_set = maps.set_home(home_pos.latitude_deg,home_pos.longitude_deg);
    if (maps.get_map(&cur_loc,vh_pos.latitude_deg,vh_pos.longitude_deg,vh_att.yaw_deg,map_zoom) == 0){
      cv::line(cur_loc,p1,p2,color,2,cv::LINE_AA);
      cv::line(cur_loc,p1,p3,color,2,cv::LINE_AA);
      cv::line(cur_loc,p2,p3,color,2,cv::LINE_AA);
      cur_loc = cur_loc - mask;
      cur_loc.copyTo(output(roi));
      tex_set_colors(vid6,output.cols,output.rows,(void*)output.datastart);
    }
    //mavlink gps stream at most at 10Hz, so 20 fps is enough.
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
  }
  std::cerr << "Map thread exited" << std::endl;
}

void wfbgs_thread(){
  wfb_stats wfb;
  std::cout << "WFB rx stats thread started" << std::endl;
  while(true){

    int openres =  wfb.open("127.0.0.1",8003);
    if (openres != 0){
      std::this_thread::sleep_for(std::chrono::milliseconds(2000));
      continue; 
    }
    wfb.start(wfb_rx);
  }
  std::cout << "WFB rx stats thread finished" << std::endl;
}

int main(int argc, char* argv[]) {

  //initialize stereokit
  sk_settings_t settings = {};
	
  settings.app_name           = "SK_FPV";
	settings.assets_folder      = "Assets";
	settings.display_preference = display_mode_mixedreality;
	
  if (!sk_init(settings))
		return 1;
  
  //setup map screen, 1:1 ratio
  plane6_mesh = mesh_gen_plane({1.333*0.78,1*0.78},{0,0,1},{0,1,0});
  plane6_mat = material_copy_id(default_id_material_unlit);
  vid6 = tex_create(tex_type_image_nomips,tex_format_rgba32);
  tex_set_address(vid6, tex_address_clamp);

  //setup hud
  hud_mesh = mesh_gen_plane({1.5f*hud_s,hud_s},{0,0,1},{0,1,0});
  hud_mat = material_copy_id(default_id_material_unlit);
  hud_tex = tex_create(tex_type_image_nomips,tex_format_rgba32);
  tex_set_address(hud_tex, tex_address_clamp);

  
  //setup textures for map and hud
  material_set_texture(plane6_mat,"diffuse",vid6);
  material_set_transparency(plane6_mat,transparency_blend);
  material_set_texture(hud_mat,"diffuse",hud_tex);
  material_set_transparency(hud_mat,transparency_blend);

  std::cout << "Starting threads" << std::endl;
  std::thread t4(&draw_hud);
  std::thread t5(&mavlink_thread);
  std::thread t6(&map_thread);
  std::thread t7(&wfbgs_thread);
  t4.detach();
  t5.detach();
  t6.detach();
  t7.detach();
 
  //hide hands, we are not using them right now.
  for (size_t h = 0; h < handed_max; h++) {
      input_hand_visible((handed_)h,false);
  }
  //disable skybox 
  render_enable_skytex(false);
  render_set_cam_root(matrix_t({0,0,1.6}));

  //can also stream over network, just add a tee and udp rtp stream.
  bool output_video = output.open("appsrc do-timestamp=true is-live=true ! queue leaky=1 ! videoconvert n-threads=2 ! videorate ! video/x-raw,format=NV12,framerate=30/1 ! "
		                  "queue ! tee name=raw raw. ! "
		                  "queue ! vaapivp9enc rate-control=2 bitrate=40000 keyframe-period=1 quality-level=7 ! vp9parse ! rtpvp9pay mtu=60000 ! multiudpsink clients=127.0.0.1:7600 raw. ! "
				  "queue ! vaapivp9enc rate-control=2 bitrate=4000  keyframe-period=30 quality-level=7 ! vp9parse ! rtpvp9pay mtu=1400 ! multiudpsink clients=127.0.0.1:7601", 
		                  //"queue ! vaapih264enc bitrate=25000 keyframe-period=60 rate-control=2 num-slices=4 ! h264parse config-interval=-1 ! rtph264pay mtu=1400 ! multiudpsink clients=127.0.0.1:7600 raw. ! "
				  //"queue ! vaapih264enc bitrate=2500  keyframe-period=60 rate-control=2 num-slices=4 ! h264parse config-interval=-1 ! rtph264pay mtu=1400 ! multiudpsink clients=127.0.0.1:7601", 
				  cv::CAP_GSTREAMER, 0, 30, cv::Size(1920,1080), true);
  if (!output_video) std::cout << "Output stream failed to start" << std::endl;
  //load video surfaces
  vsurfaces.load_file("./cam.json");
  //display video surfaces names
  for(std::string name : vsurfaces.list_names()) std::cout << "Stream name:" << " " <<name << "\n";
  
  std::cout << "Starting main loop" << std::endl;
  sk_run([]() {
    //videocontainer planes
      for(std::string name : vsurfaces.list_names()){
        mesh_t    &mesh = vsurfaces.mesh_ts[name];
        material_t &mat = vsurfaces.material_ts[name];
        pose_t    &pose = vsurfaces.pose_ts[name];
        ui_handle_begin(name.c_str(), pose, mesh_get_bounds(mesh), false);
        render_add_mesh(mesh, mat, matrix_identity);
        ui_handle_end();
      }
      //HUD plane
      //setup hud position and orientation. HUD is always 1 unit in front of camera, facing it.
      vec3 at = input_head()->position + input_head()->orientation * vec3_forward * 1.0f;
      quat ori = quat_lookat(input_head()->position,at);
      ui_handle_begin("HUD", hud_pose, mesh_get_bounds(hud_mesh), false);
      render_add_mesh(hud_mesh, hud_mat, matrix_trs(at,ori,vec3_one));
      ui_handle_end();
      //map window
      ui_handle_begin("Plane6", plane6_pose, mesh_get_bounds(plane6_mesh), false);
      render_add_mesh(plane6_mesh, plane6_mat, matrix_trs(at,ori,vec3_one));
      ui_handle_end();
        
        //on cv1 around 54 degrees looks close to what is seen in the headset. you can play around with the resolution and fov.
    if(cnt % 3 ==0)
      render_screenshot_capture(&scr_callback, *input_head(), 1920, 1080, 54, tex_format_rgba32, NULL); 
        
        //print some debug stuff if needed
    if(cnt % 100 == 0){
            //std::cout << hud_pose.orientation.x << " " << hud_pose.orientation.y << " " << hud_pose.orientation.z << " " << hud_pose.orientation.w << std::endl << std::endl;
            //std::cout << hud_pose.position.x << " " << hud_pose.position.y << " " << hud_pose.position.z << std::endl << std::endl;
            //std::cout << vh_att.roll_deg << " " << vh_att.pitch_deg << std::endl;
            //std::cout << ori.x << " " << ori.y << " " << ori.z << " "<< ori.w << std::endl;
    }    
    cnt++;
  });
  t4.join();
  t5.join();
  t6.join();
  t7.join();
  return 0;
}
