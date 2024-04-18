#include <stereokit.h>
#include <stereokit_ui.h>
#include <opencv2/opencv.hpp>
#include <iostream> 
#include <thread>
using namespace sk;


//pose_t controls location, X (left/right), Y(up/down), Z(front/back). The second argument is a quartenion, you can search for calculators if you want to change the screen angle and rotation.

//Globals so all threads can access them

//first screen
mesh_t     plane_mesh;
material_t plane_mat;
pose_t     plane_pose = {{0,0.12,-1.5f}, {0,0,0,1}};
tex_t vid0;

//second screen
mesh_t     plane1_mesh;
material_t plane1_mat;
pose_t     plane1_pose = {{0,-1.0f,-1.2f}, {-0.258819,0,0,0.9659258}};
tex_t vid1;

//Double buffers for the screens
cv::Mat buffer0[2];
cv::Mat buffer1[2];
cv::Mat buffer2[2];


//Current buffer being used
char cur_buffer0 = 0;
char cur_buffer1 = 0;
char cur_buffer2 = 0;

uint64_t cnt = 0;


cv::VideoCapture cap0;
cv::VideoCapture cap1;
cv::VideoCapture cap2;

model_t test;


//should make this only one function but whatever
void frame_grabber0()
{
  while(true){
    cap0.read(buffer0[!cur_buffer0]);
    cur_buffer0 = !cur_buffer0;
  }
}

void frame_grabber1()
{
  while(true){
    cap1.read(buffer1[!cur_buffer1]);
    cur_buffer1 = !cur_buffer1;
  }
}


model_t main_screen;

int main(void) {
  
  sk_settings_t settings = {};
	
  settings.app_name           = "SKNativeTemplate";
	settings.assets_folder      = "Assets";
	settings.display_preference = display_mode_mixedreality;
	
  if (!sk_init(settings))
		return 1;

  //setup first screen, we generate a plane with 16:9 ratio
  plane_mesh = mesh_gen_plane({2.5,1.406},{0,0,1},{0,1,0});
  plane_mat = material_copy_id(default_id_material_unlit);
  vid0 = tex_create(tex_type_image_nomips,tex_format_rgba32);
  tex_set_address(vid0, tex_address_clamp);

  
  //setup second screen, we generate a plane with a 4:3 ratio
  plane1_mesh = mesh_gen_plane({1.25f,1},{0,0,1},{0,1,0});
  plane1_mat = material_copy_id(default_id_material_unlit);
  vid1 = tex_create(tex_type_image_nomips,tex_format_rgba32);
  tex_set_address(vid1, tex_address_clamp);
  

  //hide hands, we are not using them right now.
  for (size_t h = 0; h < handed_max; h++) {
      input_hand_visible((handed_)h,false);
  }
  
  //setup textures
  material_set_texture(plane_mat,"diffuse",vid0);
  material_set_texture(plane1_mat,"diffuse",vid1);
  

  //Start your streams first, or the app will block waiting for them to open.
  //You can use vaapih264dec and vaapipostprocess to convert to RGBA in the GPU on AMD cards. Sterokit only supports RGBA32 textures
  std::cout << "Starting video 1" << std::endl;
  bool cap0_open = cap0.open("udpsrc port=5600 caps=application/x-rtp, media=video,clock-rate=90000, encoding-name=H264 ! queue ! rtph264depay ! avdec_h264 ! videoconvert ! video/x-raw,format=RGBA ! appsink max-buffers=2 drop=true sync=false",cv::CAP_GSTREAMER);
  
  if(!cap0.isOpened()){
      std::cout << "Error opening video 1" << std::endl;
      return -1;
  }

  std::cout << "Starting video 2" << std::endl;

  bool cap1_open = cap1.open("udpsrc port=5601 caps=application/x-rtp, media=video,clock-rate=90000, encoding-name=H264 ! queue ! rtph264depay ! avdec_h264 ! videoconvert ! video/x-raw,format=RGBA ! appsink max-buffers=2 drop=true sync=false",cv::CAP_GSTREAMER);
  if(!cap1.isOpened()){
      std::cout << "Error opening video 2" << std::endl;
      return -1;
  }
  
  std::cout << "Starting frame threads" << std::endl;

  std::thread t0(&frame_grabber0);
  std::thread t1(&frame_grabber1);
  t0.detach();
  t1.detach();
  std::cout << "Starting main loop" << std::endl;
  sk_run([]() {
        cv::Mat& frame0 = buffer0[cur_buffer0];
        cv::Mat& frame1 = buffer1[cur_buffer1];
        if(!frame0.empty()){
          tex_set_colors(vid0,frame0.cols,frame0.rows,(void*)frame0.datastart);
        }
  
        if(!frame1.empty()){
          tex_set_colors(vid1,frame1.cols,frame1.rows,(void*)frame1.datastart);
        }

        ui_handle_begin("Plane", plane_pose, mesh_get_bounds(plane_mesh), false);
        render_add_mesh(plane_mesh, plane_mat, matrix_identity);
        ui_handle_end();
        
        ui_handle_begin("Plane1", plane1_pose, mesh_get_bounds(plane1_mesh), false);
        render_add_mesh(plane1_mesh, plane1_mat, matrix_identity);
        ui_handle_end();

          
        
        cnt++;

	});
  t0.join();
  t1.join();
  cap0.release();
  cap1.release();
  //cap2.release();
  return 0;
}
