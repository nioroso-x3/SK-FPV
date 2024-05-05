#include <cairommconfig.h>
#include <cairomm/context.h>
#include <cairomm/surface.h>

void drawPitchLadder(Cairo::RefPtr<Cairo::Context> cr, double x, double y, int value) {
    cr->save(); // Save the current state of the Cairo context
    cr->translate(x, y);

    double length = 200; // total length
    double space = 80; // space between
    double q = 12;

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
    cr->select_font_face("Sans", Cairo::FONT_SLANT_NORMAL, Cairo::FONT_WEIGHT_NORMAL);
    cr->set_font_size(16); // Assuming you have a method or know how to scale it

    // Text alignment setup (Cairo does not support direct text alignment. This has to be manually calculated)
    Cairo::TextExtents te;
    cr->get_text_extents("-90", te);
    double textWidth = te.width;

    // Right text
    std::string rtext = std::to_string(value);
    if (rtext.size() < 4) rtext.insert(rtext.begin(),4 - rtext.size(), ' ');
    cr->move_to(length / 2 + 4 + textWidth -20, (value > 0 ? q / 2 : -q / 2)+5);
    cr->show_text(rtext);

    // Left text
    cr->move_to(-(length / 2 + 4)-43, (value > 0 ? q / 2 : -q / 2) + 5);
    cr->show_text(rtext);

    cr->restore(); // Restore to the previous state (before translate)
}

int main() {
    auto surface = Cairo::ImageSurface::create(Cairo::FORMAT_ARGB32, 640, 480);
    auto cr = Cairo::Context::create(surface);

    // Example settings and call
    drawPitchLadder(cr, 320, 150, 30);
    drawPitchLadder(cr, 320, 180, 20);
    drawPitchLadder(cr, 320, 210, 10);
    drawPitchLadder(cr, 320, 270, -10);
    drawPitchLadder(cr, 320, 300, -20);
    drawPitchLadder(cr, 320, 330, -30);

    // Save to file
    surface->write_to_png("pitchladder.png");

    return 0;
}
