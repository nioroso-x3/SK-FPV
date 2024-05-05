#include <cairommconfig.h>
#include <cairomm/context.h>
#include <cairomm/surface.h>


void drawFlightPath(Cairo::RefPtr<Cairo::Context> cr, double x, double y) {
    cr->save(); // Save the current state of the Cairo context
    cr->translate(x, y);

    double r = 12; // radius for square

    // Drawing a square
    cr->begin_new_path();
    cr->move_to(r, 0);
    cr->line_to(0, r);
    cr->line_to(-r, 0);
    cr->line_to(0, -r);
    cr->close_path();

    // Defining length of extension lines
    double line = 9;

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

int main() {
    auto surface = Cairo::ImageSurface::create(Cairo::FORMAT_ARGB32, 800, 600);
    auto cr = Cairo::Context::create(surface);

    // Example settings and call
    double pixel_per_deg = 10;  // Example value, adjust as needed
    drawFlightPath(cr, 400,300);

    surface->write_to_png("fpath.png");

    return 0;
}
