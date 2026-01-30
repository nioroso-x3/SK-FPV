#include "hud.h"
#include <cairommconfig.h>
#include <cairomm/context.h>
#include <cairomm/surface.h>
#include <sstream>
#include <ctime>
#define pix_deg 32
using namespace sk;
using namespace mavsdk;


std::string 
deci(float i,int p){
  std::stringstream s;
  s << std::fixed << std::setprecision(p) << i;
  return s.str();
}

void drawHeading(Cairo::RefPtr<Cairo::Context> cr, 
                 float x, 
                 float y, 
                 int step_range, 
                 float value, //heading from 0 to 360
                 bool bottom) {
    cr->save();
    cr->translate(x, y);
    float mf = 1;
    if (bottom) {
        mf = -1;
    }

    // Value indicator

    float font_size = 28.0f;
    cr->set_font_size(font_size);
    int text = round(value);

    Cairo::TextExtents text_extents;
    cr->get_text_extents(std::to_string(text), text_extents);
    float text_width = text_extents.width;

    float text_side_border = 5;
    float text_top_border = 4;
    float length = text_side_border * 2 + text_width + 16; // Total length
    float height = text_top_border * 1.5 + font_size + length / 4; // Total height

    cr->move_to(-length / 2, 0);
    cr->line_to(length / 2, 0);
    cr->line_to(length / 2, mf * (text_top_border * 1.5 + font_size));
    cr->line_to(0, mf * height);
    cr->line_to(-length / 2, mf * (text_top_border * 1.5 + font_size));
    cr->close_path();
    cr->stroke();


    float tx = text_width / 2;
    float ty = mf * (2 * text_top_border + font_size) / 2;
    cr->move_to(-tx, ty+10);
    cr->show_text(std::to_string(text));

    // Scale
    font_size = 18;
    cr->set_font_size(font_size);

    float text_border = 2;
    float border = 4;
    float step_length[3] = {16, 11, 7};

    cr->translate(0, mf * (height + border));

    cr->rectangle((-step_range * 16) / 2, 0, 16 * step_range, mf * (step_length[0] + 2 * text_border + font_size)+8);
    cr->clip();

    float step_margin = 5;
    float step_zero_offset = ceil(step_range / 2) + step_margin;
    float step_value_offset = floor(value);
    float step_offset = value - step_value_offset;

    cr->translate(-(step_zero_offset + step_offset) * 16, 0);

    cr->move_to(0, 0);
    for (int i = -step_zero_offset + step_value_offset; i < step_zero_offset + step_value_offset; i++) {
        int pos_i = abs(i);
        cr->move_to(0, 0);
        if (pos_i % 10 == 0) {
            cr->line_to(0, mf * step_length[0]);
        } else if (pos_i % 5 == 0) {
            cr->line_to(0, mf * step_length[1]);
        } else {
            cr->line_to(0, mf * step_length[2]);
        }

        if (pos_i % 90 == 0 || pos_i % 45 == 0 || pos_i % 10 == 0) {
            std::string atext;
            pos_i = pos_i % 360;
            bool is_card = true;
            if (pos_i == 0) {
                atext = "N";
            } else if (pos_i == 45) {
                atext = "NE";
            } else if (pos_i == 90 ) {
                atext = "E";
            } else if (pos_i == 135) {
                atext = "SE";
            } else if (pos_i == 180) {
                atext = "S";
            } else if (pos_i == 225) {
                atext = "SW";
            } else if (pos_i == 270) {
                atext = "W";
            } else if (pos_i == 315) {
                atext = "NW";
            } else {
                atext = std::to_string(pos_i % 360);
                is_card=false;
            }
            if (is_card){
            cr->move_to(-5, mf * (step_length[0] + text_border + font_size / 2) +14);
            cr->set_font_size(28);

            }
            else{
            cr->move_to(-5, mf * (step_length[0] + text_border + font_size / 2) + 9);
            cr->set_font_size(22);

            }
            cr->show_text(atext);
        }

        cr->translate(16, 0);
    }

    cr->stroke();
    cr->restore();
}

void drawVerticalScale(Cairo::RefPtr<Cairo::Context> cr, 
                       float                        x, 
                       float                        y, 
                       float                        value, 
                       float                        stepRange, 
                       bool                          right) {
    cr->save();
    cr->translate(x, y);

    float mf = 1.0;
    if (right) {
        mf = -1.0;
    }
    float exampleValue = 9999;
    // value indicator
    float fontSize = 28.0;
    cr->set_font_size(fontSize);

    float textSideBorder = 5.0;
    float textTopBorder = 4.0;
    Cairo::TextExtents textExtents;
    cr->get_text_extents(std::to_string(exampleValue), textExtents);
    float textWidth = textExtents.width;

    float height = fontSize + 2 * textTopBorder;
    float length = textSideBorder * 2 + textWidth + height / 2; // total length

    cr->move_to(0, -height / 2);
    cr->line_to(mf * (textSideBorder * 2 + textWidth), -height / 2);
    cr->line_to(mf * length, 0);
    cr->line_to(mf * (textSideBorder * 2 + textWidth), height / 2);
    cr->line_to(0, height / 2);
    cr->close_path();
    cr->stroke();
    cr->move_to(right ? -textSideBorder - textWidth +35 : textSideBorder + textWidth -110, 8);
    
    std::string str_v = deci(value,1);
    if (str_v.size() < 4) str_v.insert(str_v.begin(), 4 -str_v.size(), ' ');
    
    cr->show_text(str_v);

    // scale |----I----|----I----|----I----|
    fontSize = 18.0;
    cr->set_font_size(fontSize);

    float textBorder = 3.0;
    float border = 4.0;
    float stepLength[] = {16.0, 11.0, 7.0};

    if (!right) {
        cr->move_to(0, 0);
    }

    // space from value indicator
    cr->translate(mf * (length + border), 0);

    // visible step range clip
    //
    //
    //
    int clipr = 0;
    int clipl = 0;
    if (right) clipr= -75;
    if (right) clipl= 50;
    cr->rectangle(clipr, -((stepRange * 10) / 2), mf * stepLength[0] + 2 * textBorder + textExtents.width + clipl, stepRange * 10);
    cr->clip();

    float stepMargin = 5.0; // top and bottom extra steps
    float stepZeroOffset = ceil(stepRange / 2) + stepMargin; // '0' offset from bottom (35.5 -> 18, 35 -> 18)
    float stepValueOffset = floor(value); // 35.5 -> 35
    float stepOffset = value - stepValueOffset; // 35.5 -> 0.5

    cr->translate(0, (stepZeroOffset + stepOffset) * 10); // translate to start position

    for (float i = -stepZeroOffset + stepValueOffset; i < stepZeroOffset + stepValueOffset; i++) {
        cr->move_to(0, 0);
        int mv = 0;
        std::string str_i = std::to_string((int)i);
        if (right && str_i.size() < 4){
          str_i.insert(str_i.begin(), 4 -str_i.size(), ' ');
        }
        switch (abs((int)i) % 10) {
            case 0:
                cr->line_to(mf * stepLength[0], 0);
                if (right) mv = 38;
                cr->move_to(mf * (stepLength[0] + textBorder + mv), 0);
                cr->show_text(str_i);
                break;
            case 5:
                cr->line_to(mf * stepLength[1], 0);
                break;
            default:
                cr->line_to(mf * stepLength[2], 0);
                break;
        }
        cr->translate(0, -10);
    }
    cr->stroke();

    cr->restore();
}

void drawFlightPath(Cairo::RefPtr<Cairo::Context> cr, 
                    float x, 
                    float y) {
    cr->save(); // Save the current state of the Cairo context
    cr->translate(x, y);

    float r = 12; // radius for square

    // Drawing a square
    cr->begin_new_path();
    cr->move_to(r, 0);
    cr->line_to(0, r);
    cr->line_to(-r, 0);
    cr->line_to(0, -r);
    cr->close_path();

    // Defining length of extension lines
    float line = 9;

    // Right line
    cr->move_to(r, 0);
    cr->line_to(r + line, 0);

    // Center top line
    cr->move_to(0, -r);
    cr->line_to(0, -r - line);

    // Left line
    cr->move_to(-r, 0);
    cr->line_to(-r - line, 0);

    cr->stroke(); // Execute all line drawing commands

    cr->restore(); // Restore to the previous state (before translate)
}

void drawPitchLadder(Cairo::RefPtr<Cairo::Context> cr, float x, float y, int value) {
    cr->save(); // Save the current state of the Cairo context
    cr->translate(x, y);

    float length = 200; // total length
    float space = 80; // space between
    float q = 12;

    cr->begin_new_path();

    // Right ladder
    cr->move_to(space / 2, 0);
    cr->line_to(length / 2 - q, 0);
    cr->line_to(length / 2, value > 0 ? q : -q);

    // Left ladder
    cr->move_to(-space / 2, 0);
    cr->line_to(-(length / 2 - q), 0);
    cr->line_to(-length / 2, value > 0 ? q : -q);

    cr->stroke();

    // Setup font scale
    cr->set_font_size(20); // Assuming you have a method or know how to scale it

    // Text alignment setup (Cairo does not support direct text alignment. This has to be manually calculated)
    Cairo::TextExtents te;
    cr->get_text_extents("-90", te);
    float textWidth = te.width;

    // Right text
    std::string rtext = std::to_string(value);
    if (rtext.size() < 4) rtext.insert(rtext.begin(),4 - rtext.size(), ' ');
    cr->move_to(length / 2 + 4 + textWidth -40, (value > 0 ? q / 2 : -q / 2)+5);
    cr->show_text(rtext);

    // Left text
    cr->move_to(-(length / 2 + 4)-50, (value > 0 ? q / 2 : -q / 2) + 5);
    cr->show_text(rtext);

    cr->restore(); // Restore to the previous state (before translate)
}



void drawHorizonLadder(const Cairo::RefPtr<Cairo::Context>& cr, 
                       float                               x, 
                       float                               y, 
                       float                               pixel_per_deg) {
    cr->save();  // Save the current state of the context

    // Constants
    float length = 460;
    float space = 80;
    float q = 12;
    float small_length = 26;  // Re-declare length for small dashes

    // Translate the context to start point
    cr->translate(x, y);

    // Begin drawing the main part of the ladder
    cr->begin_new_path();

    // Draw right side of the horizon ladder
    cr->move_to(space / 2, 0);
    cr->line_to(length / 2 - q, 0);
    cr->line_to(length / 2, q);

    // Draw left side of the horizon ladder
    cr->move_to(-space / 2, 0);
    cr->line_to(-(length / 2 - q), 0);
    cr->line_to(-length / 2, q);

    cr->stroke();  // Apply the strokes

    // Set dashed lines for indicating degrees of pitch
    std::vector<double> dash_pattern = {6.0, 4.0};
    cr->set_dash(dash_pattern, 0);

    // Begin a new path for small dash lines below horizon
    cr->begin_new_path();
    for (int i = 0; i < 3; ++i) {
        cr->translate(0, pixel_per_deg);  // Move down per degree

        // Draw small horizontal lines on the right side
        cr->move_to(space / 2, 0);
        cr->line_to(space / 2 + small_length, 0);

        // Draw small horizontal lines on the left side
        cr->move_to(-space / 2, 0);
        cr->line_to(-(space / 2 + small_length), 0);

        cr->stroke();  // Apply the strokes
    }
    // Restore the context to its original state before translation
    cr->translate(-x, -y - 3 * pixel_per_deg);
    for (int i = 0; i < 3; ++i) {
        cr->translate(0, -pixel_per_deg);  // Move up per degree

        // Draw small horizontal lines on the right side
        cr->move_to(space / 2, 0);
        cr->line_to(space / 2 + small_length, 0);

        // Draw small horizontal lines on the left side
        cr->move_to(-space / 2, 0);
        cr->line_to(-(space / 2 + small_length), 0);

        cr->stroke();  // Apply the strokes
    }

    cr->set_dash(std::vector<double>(0), 0); // Resetting dash pattern to solid line

    // Restore the context to its original state before translation
    cr->translate(-x, -y + 3 * pixel_per_deg);
    cr->restore();
}

void drawThrottle(const Cairo::RefPtr<Cairo::Context>& cr, float x, float y, float throttle) {
    cr->save();
    cr->set_font_size(27);

    float border = 8;
    float indexLength = 6;
    float range = 1.5 * M_PI;
    float start = 0.5 * M_PI;
    Cairo::TextExtents extents;
    cr->get_text_extents("100%", extents);
  
    float radius = extents.width / 2 + border;
    float angle = start + range * throttle;

    float trX = x + radius + indexLength;
    float trY = y - radius - indexLength;
    cr->save(); // Save current transformation matrix
    cr->translate(trX, trY);


    cr->begin_new_sub_path();
    cr->arc(0, 0, radius, start, angle);
    cr->line_to((radius + indexLength) * cos(angle), (radius + indexLength) * sin(angle));
    cr->stroke();

    cr->arc(0, 0, radius, angle, start + range);
    cr->stroke();

    std::string v = std::to_string(static_cast<int>(throttle * 100)) + "%";
    cr->get_text_extents(v, extents);
    cr->move_to(-extents.width/2, extents.height/2);
    cr->show_text(v);

    cr->restore(); // Restore previous transformation matrix
}

void drawStatusMsg(const Cairo::RefPtr<Cairo::Context>& cr, float x, float y, std::string msg){
  cr->save();
  if(status_counter){
    status_counter--;
    cr->set_font_size(40);
    cr->move_to(x,y);
    cr->show_text("MSG->");
    cr->move_to(x+(HUD_w*0.1375),y);
    cr->show_text(msg);
  }
  cr->restore();
}


void drawBatteryStatus(const Cairo::RefPtr<Cairo::Context>& cr, float x, float y, float v, float a){
  cr->save();
  //based on 4S battery, change it to match your vehicle.
  float vmax = 16.8;
  float vmin = 12.8;
  float w = v*a;
  float pct = ((v - vmin)/(vmax-vmin));
  float bar_l = HUD_w/2;
  cr->translate(x,y);
  cr->rectangle(0,-5,5,30);
  cr->fill();
  cr->rectangle(bar_l,-5,5,30);
  cr->fill();
  if(pct > 0.0){
    float l = bar_l*pct;
    cr->rectangle(0,0,l,20);
    cr->fill();
  }
  cr->move_to(bar_l+5,10);
  std::stringstream volts;
  volts << deci(v,2) << " V";
  cr->show_text(volts.str());
  cr->move_to(bar_l+5,30);
  std::stringstream amps;
  amps << deci(a,1) << " A";
  cr->show_text(amps.str());
  cr->restore();
}

void drawSimpleLabel(const Cairo::RefPtr<Cairo::Context>& cr, float x, float y, std::string label, double value, int precision,float s){
  cr->save();
  cr->set_font_size(s);
  Cairo::TextExtents t;
  cr->get_text_extents(label,t);
  cr->translate(x,y);
  cr->move_to(0,0);
  cr->show_text(label);
  cr->move_to(t.width+8,0);
  cr->show_text(deci(value,precision));
  cr->restore();
}

void drawLabel(const Cairo::RefPtr<Cairo::Context>& cr, float x, float y, std::string txt,float s){
  cr->save();
  cr->translate(x,y);
  cr->move_to(0,0);
  cr->set_font_size(s);
  cr->show_text(txt);
  cr->restore();
}

void drawAircraftSymbol(const Cairo::RefPtr<Cairo::Context>& cr, float x, float y){
  cr->save();
  cr->set_line_width(3.0);
  cr->translate(x,y);
  cr->begin_new_path();
  cr->move_to(-25,0);
  cr->line_to(-10,0);
  cr->line_to(-5,6);
  cr->line_to(0,0);
  cr->line_to(5,6);
  cr->line_to(10,0);
  cr->line_to(25,0);
  cr->stroke();
  cr->restore();
}

void drawHomeDirection(const Cairo::RefPtr<Cairo::Context>& cr, float x, float y, float heading, int s){
  cr->save();
  cr->set_line_width(2.0);
  cr->translate(x,y);
  cr->rotate(heading * (M_PI/180.0));
  cr->move_to(0   , 3*s);
  cr->line_to(0   ,-3*s);
  cr->move_to(-2*s,-1*s);
  cr->line_to(0   ,-3*s);
  cr->move_to(2*s ,-1*s);
  cr->line_to(0   ,-3*s);
  cr->stroke();
  cr->restore();
}

void drawDateTime(const Cairo::RefPtr<Cairo::Context>& cr, float x, float y, uint64_t time, float size){
  uint64_t unix_time = time/1000000;
  std::tm* time_info = std::localtime((time_t*)&unix_time);
  char tbuffer[128];
  std::strftime(tbuffer,sizeof(tbuffer), "%H:%M:%S %Y/%m/%d", time_info);
  drawLabel(cr, x+16, y+24, std::string(tbuffer),size);
}

void drawLTERadio(const Cairo::RefPtr<Cairo::Context>& cr, float x, float y, uint8_t s, uint8_t fr, uint8_t t, uint8_t q){
  std::string label("UNK");
  if (t == 1) label = std::string("GSM");
  if (t == 2) label = std::string("CDM");
  if (t == 3) label = std::string("WCD");
  if (t == 4) label = std::string("LTE");
  int s_perc = -1;
  if (q < 255) s_perc = int(100 * (q/254.0));
  if (s == 12){
     drawSimpleLabel(cr,x,y, label + std::string("%"), s_perc, 0, 30.0f);
  }
  else{
     drawSimpleLabel(cr,x,y, label + std::to_string(s), fr, 0, 30.0f);
  }
}

void drawAoA_SSA(const Cairo::RefPtr<Cairo::Context>& cr,
                 float x, float y, 
                 float aoa, float ssa){

  cr->save();
  cr->set_line_width(2.5);
  cr->translate(x,y);
  cr->begin_new_path();
  cr->arc(ssa*pix_deg,aoa*pix_deg, HUD_w/100,0, 2*M_PI);
  cr->stroke();
  cr->restore();
}

void draw_cairo_hud()
{
  
  auto surface = Cairo::ImageSurface::create(Cairo::FORMAT_ARGB32, HUD_w, HUD_h);

  auto cr = Cairo::Context::create(surface);
  cr->save(); // Save the initial state of the Cairo context
  while(true){
    float roll = std::isnan(vh_att.roll_deg) ? 0.0 : -vh_att.roll_deg;
    float pitch = std::isnan(vh_att.pitch_deg) ? 0.0 : vh_att.pitch_deg;
    float head = std::isnan(vh_att.yaw_deg) ? 0.0 : vh_att.yaw_deg ;
    if (head < 0.0) head += 360;
    float alt_a = std::isnan(vh_pos.absolute_altitude_m) ? 0.0 : vh_pos.absolute_altitude_m;
    float alt_r = std::isnan(vh_pos.relative_altitude_m) ? 0.0 : vh_pos.relative_altitude_m;
    double lat_d = std::isnan(vh_pos.latitude_deg) ? 0.0 : vh_pos.latitude_deg;
    double lon_d = std::isnan(vh_pos.longitude_deg) ? 0.0 : vh_pos.longitude_deg;
    double lat_h = std::isnan(home_pos.latitude_deg) ? 0.0 : home_pos.latitude_deg;
    double lon_h = std::isnan(home_pos.longitude_deg) ? 0.0 : home_pos.longitude_deg;
    float home_h = get_heading(lon_d,lat_d,lon_h,lat_h);
    home_h -= head;
    if (home_h < 0.0) home_h += 360;
    float home_d = get_distance(lon_d,lat_d,lon_h,lat_h);
    float as = std::isnan(vh_fwing.airspeed_m_s) ? 0.0 : vh_fwing.airspeed_m_s*3.6; //km/h
    float thr = std::isnan(vh_fwing.throttle_percentage) ? 0.0 : vh_fwing.throttle_percentage;
    float gs = std::isnan(vh_gpsr.velocity_m_s) ? 0.0 : vh_gpsr.velocity_m_s*3.6; //km/h
    std::string lastMsg = vh_st_text.text;
    float v0 = std::isnan(vh_bat0.voltage_v) ? 0.0 : vh_bat0.voltage_v;
    float v1 = std::isnan(vh_bat1.voltage_v) ? 0.0 : vh_bat1.voltage_v;
    float a0 = std::isnan(vh_bat0.current_battery_a) ? 0.0 : vh_bat0.current_battery_a;
    float a1 = std::isnan(vh_bat1.current_battery_a) ? 0.0 : vh_bat1.current_battery_a;
    int n_sats = vh_gpsi.num_satellites; 
    std::string mode = vh_fmode;
    float radio_rssi = wfb_rssi;
    float rc_pct = std::isnan(vh_rc.signal_strength_percent) ? 0.0 : 100*vh_rc.signal_strength_percent/254;
    float rngfnd = std::isnan(vh_rngfnd.current_distance_m) ? 0.0 : vh_rngfnd.current_distance_m;
    uint8_t c_lte_stat = lte_stat;
    uint8_t c_lte_f_reason = lte_f_reason;
    uint8_t c_lte_type = lte_type;
    uint8_t c_lte_qual = lte_qual;
    float c_aoa = aoa;
    float c_ssa = ssa;
    bool rc_mode = rc_override;

    /*int wfb_rx_avg_rssi = 0;
    int video_streams = 0;
    //calculate average rssi
    for(const auto& pair : wfb_rx){
       const std::string &key = pair.first;
       if (key.find("video") != std::string::npos){
           const std::vector<int> &values = pair.second;
           if (values[0] == -1000) continue;
           video_streams++;
           wfb_rx_avg_rssi += values[0];
           //std::cout << key << " " << values[0] << " " << values[1] << " " << values[2] << std::endl;
       }
    }
    if (video_streams != 0)
    wfb_rx_avg_rssi /= video_streams;*/


    //set font seize
    cr->select_font_face("@cairo:monospace",Cairo::FONT_SLANT_NORMAL,Cairo::FONT_WEIGHT_BOLD);
    cr->set_font_size(24.5);
    
    //clear canvas
    cr->save();
    cr->set_source_rgba(0,0,0,0);
    cr->set_operator(Cairo::OPERATOR_SOURCE);
    cr->paint();    // fill image with the color
    cr->restore();
    //set line and color
    cr->set_source_rgba(1.0f, 1.0f, 1.0f, 0.98f);
    cr->set_line_width(4.8);

    //begin drawing elements
    //drawFlightPath(cr,HUD_w/2.0,HUD_h/2.0); need to figure out how to calculate the projection to the hud
    drawAircraftSymbol(cr,HUD_w/2.0,HUD_h/2.0);
    drawAoA_SSA(cr,HUD_w/2.0,HUD_h/2.0,aoa,ssa);
    drawHeading(cr,HUD_w/2.0,5,60,head,false); 
    drawVerticalScale(cr,4,HUD_h/2.0,as,40,false);
    drawSimpleLabel(cr,0,(HUD_h/2.0)+50,"GS",gs,1,30.0f);
    drawVerticalScale(cr,HUD_w-4,HUD_h/2.0,alt_r,40,true);
    drawSimpleLabel(cr,(HUD_w/32)*27,(HUD_h/32)*11,"SATS",n_sats,0,30.0f);
    drawThrottle(cr,(HUD_w/24)*4,(HUD_h/12)*12,thr);
    drawBatteryStatus(cr,(HUD_w/4),(HUD_h/12)*11,v0,a0);
    drawLabel(cr,0,(HUD_h/32)*24,mode,32);
    drawStatusMsg(cr,(HUD_w/16),(HUD_h/10)*8,lastMsg);
    drawLTERadio(cr,(HUD_w/32)*27,(HUD_h/32)*12,c_lte_stat, c_lte_f_reason, c_lte_type, c_lte_qual);
    drawSimpleLabel(cr,(HUD_w/32)*27,(HUD_h/32)*13,"WFB",radio_rssi,0,30.0f);
    drawSimpleLabel(cr,(HUD_w/32)*27,(HUD_h/32)*14,"RC%",rc_pct,0,30.0f);
    drawSimpleLabel(cr, (HUD_w/32)*27, (HUD_h/2.0)+50,"RFALT",rngfnd,1,30.0f);
    if(home_d < 1000) drawSimpleLabel(cr,(HUD_w/32)*27,(HUD_h/2)+100,"H(m)",home_d,0,30.0f);
    else drawSimpleLabel(cr,(HUD_w/32)*27,(HUD_h/2)+100,"H(km)",home_d/1000,1,30.0f);
    drawHomeDirection(cr, (HUD_w/32)*27+16,(HUD_h/2)+128,home_h,5);
    drawDateTime(cr, (HUD_w/32)*12, (HUD_h/32)*31, gps_time, 30.0f);
    if(rc_mode) drawLabel(cr,0,(HUD_h/32)*32,"RC OVERRIDE",30);


    //translating, rolling and centering the canvas for the pitch ladder.
    cr->translate(HUD_w/2.0,HUD_h/2.0);

    // Roll transformation
    cr->rotate(roll * (M_PI/180.0));

    // Pitch transformation
    cr->translate(0, pitch * pix_deg);

    // Draw artificial horizon ladder
    drawHorizonLadder(cr,0,0,pix_deg);
    int pitchDegStep = 5;
    // Top ladders
    for (int deg = pitchDegStep; deg <= 90; deg += pitchDegStep) {
        drawPitchLadder(cr, 0, -(deg * pix_deg), deg);
    }

    // Bottom ladders
    for (int deg = -pitchDegStep; deg >= -90; deg -= pitchDegStep) {
        drawPitchLadder(cr, 0, -(deg * pix_deg), deg);
    }

    cr->restore(); // Restore the initial state
    //write data to hud texture at 50 fps
    tex_set_colors(hud_tex,HUD_w,HUD_h,(void*)surface->get_data());
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
  }
}


