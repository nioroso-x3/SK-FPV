#include <cairommconfig.h>
#include <cairomm/context.h>
#include <cairomm/surface.h>
#include <iostream>
#include <cmath>

void draw_heading(double x, double y, int step_range, bool bottom) {
    auto surface = Cairo::ImageSurface::create(Cairo::FORMAT_ARGB32, 640, 480);
    auto ctx = Cairo::Context::create(surface);

    ctx->translate(x, y);

    double mf = 1;
    if (bottom) {
        mf = -1;
    }

    // Value indicator
    double heading = M_PI/2; // Assuming you have the heading value
    double value = heading * (180 / M_PI);

    double font_size = 20;
    ctx->set_font_size(font_size);
    int text = round(value);

    Cairo::TextExtents text_extents;
    ctx->get_text_extents(std::to_string(text), text_extents);
    double text_width = text_extents.width;

    double text_side_border = 5;
    double text_top_border = 4;
    double length = text_side_border * 2 + text_width; // Total length
    double height = text_top_border * 1.5 + font_size + length / 4; // Total height

    ctx->set_source_rgb(0, 0, 0);
    ctx->set_line_width(1);
    ctx->move_to(-length / 2, 0);
    ctx->line_to(length / 2, 0);
    ctx->line_to(length / 2, mf * (text_top_border * 1.5 + font_size));
    ctx->line_to(0, mf * height);
    ctx->line_to(-length / 2, mf * (text_top_border * 1.5 + font_size));
    ctx->close_path();
    ctx->stroke();


    float tx = text_width / 2;
    float ty = mf * (2 * text_top_border + font_size) / 2;
    ctx->move_to(-tx, ty+10);
    ctx->show_text(std::to_string(text));

    // Scale
    font_size = 16;
    ctx->set_font_size(font_size);

    double text_border = 2;
    double border = 4;
    double step_length[3] = {16, 11, 7};

    ctx->translate(0, mf * (height + border));

    ctx->rectangle((-step_range * 16) / 2, 0, 16 * step_range, mf * (step_length[0] + 2 * text_border + font_size));
    ctx->clip();

    double step_margin = 5;
    double step_zero_offset = ceil(step_range / 2) + step_margin;
    double step_value_offset = floor(value);
    double step_offset = value - step_value_offset;

    ctx->translate(-(step_zero_offset + step_offset) * 16, 0);

    ctx->move_to(0, 0);
    for (int i = -step_zero_offset + step_value_offset; i < step_zero_offset + step_value_offset; i++) {
        int pos_i = abs(i);
        std::cout << pos_i << std::endl;
        ctx->move_to(0, 0);
        if (pos_i % 10 == 0) {
            ctx->line_to(0, mf * step_length[0]);
        } else if (pos_i % 5 == 0) {
            ctx->line_to(0, mf * step_length[1]);
        } else {
            ctx->line_to(0, mf * step_length[2]);
        }

        if (pos_i % 90 == 0 || pos_i % 45 == 0 || pos_i % 10 == 0) {
            std::string atext;
            pos_i = pos_i % 360;
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
            }

            ctx->move_to(-5, mf * (step_length[0] + text_border + font_size / 2));
            ctx->show_text(atext);
        }

        ctx->translate(16, 0);
    }

    ctx->stroke();

    surface->write_to_png("heading.png");
}

int main() {
    draw_heading(320, 240, 20, false);
    return 0;
}
