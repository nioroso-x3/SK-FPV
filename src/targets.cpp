#include "targets.h"

void draw_box(float   x1f, 
              float   y1f, 
              float   x2f, 
              float   y2f, 
              int     cls, 
              float   p, 
              cv::Mat &img){
    int w = img.cols;
    int h = img.rows;
    int x1 = x1f*w;
    int y1 = y1f*h;
    int x2 = x2f*w;
    int y2 = y2f*h;
    cv::Scalar color = cv::Scalar(255,255,255,255);
    cv::rectangle(img, cv::Point(x1, y1), cv::Point(x2, y2),color,2);
    /*
    char text[256];
    sprintf(text, "%d %.1f%%", cls, p * 100);
        //No need to draw labels
    int baseLine = 0;
    cv::Size label_size = cv::getTextSize(text, cv::FONT_HERSHEY_SIMPLEX, 0.5, 1, &baseLine);

    int x = x1;
    int y = y1 - label_size.height - baseLine;
    if (y < 0) y = 0;
    if (x + label_size.width > img.cols) x = img.cols - label_size.width;

    cv::rectangle(img, cv::Rect(cv::Point(x, y), cv::Size(label_size.width, label_size.height + baseLine)), cv::Scalar(255, 255, 255, 255), -1);

    cv::putText(img, text, cv::Point(x, y + label_size.height), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 0, 0, 0));
    */

}

void listen_zmq(const std::string        &bind_address, 
                std::map<std::string,cv::Mat> &overlays,
                std::map<std::string,cv::Size> &vsizes) {

    zmq::context_t context(1);

    zmq::socket_t socket(context, zmq::socket_type::sub);
    socket.bind(bind_address.c_str());

    socket.set(zmq::sockopt::subscribe, "");

    std::cout << "ZMQ: listening on " << bind_address << "\n";
    uint64_t count = 0;
    std::map<std::string,cv::Mat> temp;
    std::map<std::string,uint64_t> last_ts;
    while (true) {
        count++;
        zmq::message_t msg;
        auto ret = socket.recv(msg, zmq::recv_flags::none);

        // Unpack with msgpack
        try {
            msgpack::object_handle oh = msgpack::unpack(static_cast<const char*>(msg.data()), msg.size());
            msgpack::object obj = oh.get();

            std::vector<msgpack::object> vec;
            obj.convert(vec);

            if (vec.size() != 8) {
                std::cerr << "Invalid message format (expected 8 elements)\n";
                continue;
            }

            std::string name;
            uint64_t ts;
            float x1, y1, x2, y2; //ratios
            uint8_t t;
            float p;

            vec[0].convert(name);
            vec[1].convert(ts);
            vec[2].convert(x1);
            vec[3].convert(y1);
            vec[4].convert(x2);
            vec[5].convert(y2);
            vec[6].convert(t);
            vec[7].convert(p);

            if (vsizes.find(name) == vsizes.end()) {
                std::cout << "Overlay " << name << " not found\n";

                continue; //received data for a nonexisting video input

            }
            if (last_ts.find(name) == last_ts.end()){
                //initialize the final and temp overlays
                std::cout << "Initializing new overlay\n";
                std::cout << name << " " << vsizes[name].width << " " << vsizes[name].height << "\n";
                overlays[name] = cv::Mat::zeros(vsizes[name].height,vsizes[name].width,CV_8UC4);
                temp[name] = overlays[name].clone();
                last_ts[name] = ts;
            }
            //new data is arriving or no objects detected in the last frame. copy the current temp overlay to the main overlay and clear the temp overlay
            if (ts > last_ts[name] || t == 255){
                temp[name].copyTo(overlays[name]);
                temp[name].setTo(cv::Scalar(0,0,0,0));
                last_ts[name] = ts;
            }
            if (ts < last_ts[name]){ //skip data arriving late.

                 std::cout << "Skipping old data for cam " << name << "\n";
                 continue; 
            }             
            //draw boxes to the temp overlay
            draw_box(x1,y1,x2,y2,t,p,temp[name]);
            

        } catch (const std::exception& ex) {
            std::cerr << "Failed to parse msgpack message: " << ex.what() <<  " " << count << std::endl;
            continue;
        }
    }
}
