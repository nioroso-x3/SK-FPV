#include <cairommconfig.h>
#include <cairomm/context.h>
#include <cairomm/surface.h>
#include <cmath> // For rounding the value

void draw_pitch_ladder(const Cairo::RefPtr<Cairo::Context>& cr, double x, double y, double value) {
    cr->save();
    cr->translate(x, y);

    double length = 200; // total length
    double space = 80; // space between ladders
    double q = 12;

    // Begin path for drawing the ladders
    cr->begin_new_path();

    // Right ladder
    cr->move_to(space / 2, 0);
    cr->line_to(length / 2 - q, 0);
    cr->line_to(length / 2, value > 0 ? q : -q);

    // Left ladder
    cr->move_to(-space / 2, 0);
    cr->line_to(-(length / 2 - q), 0);
    cr->line_to(-length / 2, value > 0 ? q : -q);

    cr->stroke(); // Apply the stroke to draw the lines

    // Setting font size and properties
    cr->set_font_size(16);
    cr->select_font_face("Sans", Cairo::FONT_SLANT_NORMAL, Cairo::FONT_WEIGHT_NORMAL);
    cr->set_font_size(16);

    double textBorder = 4;
    Cairo::TextExtents extents;
    cr->get_text_extents("-90", extents);
    double textWidth = extents.width;

    // Draw right text
    cr->move_to(length / 2 + textBorder + textWidth, value > 0 ? q / 2 : -q / 2);
    cr->show_text(std::to_string((int)value));

    // Draw left text
    cr->move_to(-(length / 2 + textBorder), value > 0 ? q / 2 : -q / 2);
    cr->show_text(std::to_string((int)value));

    // Restore the original state (removes translation)
    cr->restore();
}

int main() {
    auto surface = Cairo::ImageSurface::create(Cairo::FORMAT_ARGB32, 640, 480);
    auto cr = Cairo::Context::create(surface);

    // Example settings and call
    draw_pitch_ladder(cr, 320, 240, 45); // Example value

    // Save to file
    surface->write_to_png("output.png");

    return 0;
}

