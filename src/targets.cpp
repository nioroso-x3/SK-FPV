#include "targets.h"

using namespace std::chrono_literals;
cv::Scalar get_color_p(float value) {
    // Clamp value between 0 and 1
    value = std::clamp(value, 0.0f, 1.0f);

    uchar r, g, b = 0;
    if (value < 0.5f) {
        // Interpolate between red and yellow
        float t = value / 0.5f;
        r = 255;
        g = (255 * t);
    } else {
        // Interpolate between yellow and green
        float t = (value - 0.5f) / 0.5f;
        r = (255 * (1.0f - t));
        g = 255;
    }

    return cv::Scalar(r, g, b, 255); // OpenCV uses BGRA format
}

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
    cv::Scalar color = cv::Scalar(10,255,10,255);
    if (cls != 0) color = cv::Scalar(255,10,10,255);

    cv::rectangle(img, cv::Point(x1, y1), cv::Point(x2, y2),color,2);
        
    char text[256];
    if(cls == 0) sprintf(text, "HUMAN %.1f%%", p * 100);
    if(cls > 0 && cls <= 8) sprintf(text, "VEHIC %.1f%%", p * 100);
    if(cls > 8) sprintf(text, "????? %.1f%%", p * 100);
   
    int baseLine = 0;
    cv::Size label_size = cv::getTextSize(text, cv::FONT_HERSHEY_PLAIN, 0.5, 1, &baseLine);

    int x = x1;
    int y = y1 - label_size.height - baseLine;
    if (y < 0) y = 0;
    if (x + label_size.width > img.cols) x = img.cols - label_size.width;

    cv::rectangle(img, cv::Rect(cv::Point(x, y), cv::Size(label_size.width, label_size.height + baseLine)), color, -1);

    cv::putText(img, text, cv::Point(x, y + label_size.height), cv::FONT_HERSHEY_PLAIN, 0.5, cv::Scalar(0, 0, 0, 255));
}

void draw_target(float   x1f, 
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
    //calculate proportional marker size and line width
    int fw = w/32;
    int lw = w/480;

    //get detection box center
    int cx = (x1+x2) / 2;
    int cy = (y1+y2) / 2;
    
    //Set color according to p value
    cv::Scalar color = get_color_p(p);
    
     
    //mark humans with triangles
    if (cls == 0){
        cv::Point p1(cx, cy+0.6666*fw);
        cv::Point p2(cx+fw/2, cy-0.3333*fw);
        cv::Point p3(cx-fw/2, cy-0.3333*fw);
        cv::line(img, p1, p2, color, lw);
        cv::line(img, p2, p3, color, lw);
        cv::line(img, p3, p1, color, lw);
    }
    //mark vehicles with boxes
    else{
       cv::rectangle(img, cv::Rect(cv::Point(cx-fw/2,cy-fw/2), cv::Point(cx+fw/2,cy+fw/2)), color, lw);
    }
    //cv::circle(img,cv::Point(cx,cy),fw/2,color, 0, 2);
}

void listen_zmq(const std::string        &bind_address, 
                std::map<std::string,cv::Mat> &overlays,
                std::map<std::string,cv::Size> &vsizes) {

    zmq::context_t context(1);

    zmq::socket_t socket(context, zmq::socket_type::pull);
    socket.bind(bind_address.c_str());

    std::cout << "ZMQ: listening on " << bind_address << "\n";
    uint64_t count = 0;
    std::map<std::string,cv::Mat> temp;
    std::map<std::string,uint64_t> last_ts;
    while (true) {
        count++;
        zmq::pollitem_t items[] = { {socket, 0, ZMQ_POLLIN, 0}  };
        zmq::poll(items, 1, 1s);
        //clear overlays if no messages
        if (!(items[0].revents & ZMQ_POLLIN)){

            for (auto it = overlays.begin(); it != overlays.end(); ++it){
                it->second.setTo(cv::Scalar(0,0,0,0));
            }
            continue;
        }
        //messages available, parse them
        zmq::message_t msg;
        auto ret = socket.recv(msg, zmq::recv_flags::none);

        // Unpack with msgpack
        try {
            msgpack::object_handle oh = msgpack::unpack(static_cast<const char*>(msg.data()), msg.size());
            msgpack::object obj = oh.get();

            std::vector<msgpack::object> vec;
            obj.convert(vec);

            if (vec.size() != 8) {
                std::cerr << "ZMQ: Invalid message format (expected 8 elements)\n";
                continue;
            }

            std::string name;
            uint64_t ts;
            float x1, y1, x2, y2; //detection box as proportional ratios of the image
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
                std::cout << "ZMQ: Overlay " << name << " not found\n";

                continue; //received data for a nonexisting video input

            }
            if (last_ts.find(name) == last_ts.end()){
                //initialize the final and temp overlays
                std::cout << "ZMQ: Initializing new overlay\n";
                std::cout << "  " << name << " " << vsizes[name].width << " " << vsizes[name].height << "\n";
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
                 continue; 
            }             
            //draw boxes to the temp overlay
            if(t != 255){
                draw_target(x1,y1,x2,y2,t,p,temp[name]);
                //std::cout << "ZMQ: " <<  name <<  " " << ts << " " << t << " " << p << "\n";
            }
            

        } 
        catch (const std::exception& ex) {
            std::cerr << "ZMQ: Failed to parse msgpack message: " << ex.what() <<  " " << count << std::endl;
            continue;
        }
    }

}
