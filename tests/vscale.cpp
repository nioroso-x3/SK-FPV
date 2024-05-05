#include <cairommconfig.h>
#include <cairomm/context.h>
#include <cairomm/surface.h>

void drawVerticalScale(Cairo::RefPtr<Cairo::Context> cr, double x, double y, double value, double exampleValue, double stepRange, bool right) {
    cr->save();
    cr->translate(x, y);

    double mf = 1.0;
    if (right) {
        mf = -1.0;
    }

    // value indicator
    double fontSize = 20.0;
    cr->set_font_size(fontSize);

    double textSideBorder = 5.0;
    double textTopBorder = 4.0;
    Cairo::TextExtents textExtents;
    cr->get_text_extents(std::to_string(exampleValue), textExtents);
    double textWidth = textExtents.width;

    double height = fontSize + 2 * textTopBorder;
    double length = textSideBorder * 2 + textWidth + height / 2; // total length

    cr->move_to(0, -height / 2);
    cr->line_to(mf * (textSideBorder * 2 + textWidth), -height / 2);
    cr->line_to(mf * length, 0);
    cr->line_to(mf * (textSideBorder * 2 + textWidth), height / 2);
    cr->line_to(0, height / 2);
    cr->close_path();
    cr->stroke();
    cr->move_to(right ? -textSideBorder - textWidth +40 : textSideBorder + textWidth -90, 8);
    
    std::string str_v = std::to_string((int)value);
    if (str_v.size() < 4) str_v.insert(str_v.begin(), 4 -str_v.size(), ' ');
    
    cr->show_text(str_v);

    // scale |----I----|----I----|----I----|
    fontSize = 16.0;
    cr->set_font_size(fontSize);

    double textBorder = 3.0;
    double border = 4.0;
    double stepLength[] = {16.0, 11.0, 7.0};

    if (!right) {
        cr->move_to(0, 0);
    }

    // space from value indicator
    cr->translate(mf * (length + border), 0);

    // visible step range clip
    //
    //
    //
    cr->get_text_extents("9999",textExtents);
    int clipr = 0;
    int clipl = 0;
    if (right) clipr= -75;
    if (right) clipl= 50;
    cr->rectangle(clipr, -((stepRange * 10) / 2), mf * stepLength[0] + 2 * textBorder + textExtents.width + clipl, stepRange * 10);
    cr->clip();

    double stepMargin = 5.0; // top and bottom extra steps
    double stepZeroOffset = ceil(stepRange / 2) + stepMargin; // '0' offset from bottom (35.5 -> 18, 35 -> 18)
    double stepValueOffset = floor(value); // 35.5 -> 35
    double stepOffset = value - stepValueOffset; // 35.5 -> 0.5

    cr->translate(0, (stepZeroOffset + stepOffset) * 10); // translate to start position

    for (double i = -stepZeroOffset + stepValueOffset; i < stepZeroOffset + stepValueOffset; i++) {
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

int main() {
    auto surface = Cairo::ImageSurface::create(Cairo::FORMAT_ARGB32, 800, 600);
    auto cr = Cairo::Context::create(surface);

    // Example settings and call
    double pixel_per_deg = 10;  // Example value, adjust as needed
    drawVerticalScale(cr, 10, 300, 1000,9999,41,false);

    // Save to file
    drawVerticalScale(cr, 790, 300,1001,9999,41,true);
    surface->write_to_png("vscale.png");

    return 0;
}
