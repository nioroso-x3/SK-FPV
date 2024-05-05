#include <cairommconfig.h>
#include <cairomm/context.h>
#include <cairomm/surface.h>
#include <iostream>
#include <cmath>

void drawThrottle(const Cairo::RefPtr<Cairo::Context>& cr, double x, double y, double throttle) {
    cr->set_font_size(18);
    cr->set_source_rgb(0, 0, 0); // Set text color to black

    double border = 8;
    double indexLength = 6;
    double range = 1.5 * M_PI;
    double start = 0.5 * M_PI;
    Cairo::TextExtents extents;
    cr->get_text_extents("100%", extents);
  
    double radius = extents.width / 2 + border;
    double angle = start + range * throttle;

    double trX = x + radius + indexLength;
    double trY = y - radius - indexLength;
    cr->save(); // Save current transformation matrix
    cr->translate(trX, trY);


    cr->begin_new_sub_path();
    cr->arc(0, 0, radius, start, angle);
    cr->line_to((radius + indexLength) * cos(angle), (radius + indexLength) * sin(angle));
    cr->stroke();

    cr->set_source_rgba(0, 0, 0, 0.5); // Set stroke color with alpha
    cr->arc(0, 0, radius, angle, start + range);
    cr->stroke();

    std::string v = std::to_string(static_cast<int>(throttle * 100)) + "%";
    cr->get_text_extents(v, extents);
    cr->move_to(-extents.width/2, extents.height/2);
    cr->show_text(v);

    cr->restore(); // Restore previous transformation matrix
}

int main() {
    Cairo::RefPtr<Cairo::ImageSurface> surface = Cairo::ImageSurface::create(Cairo::FORMAT_ARGB32, 200, 200);
    Cairo::RefPtr<Cairo::Context> cr = Cairo::Context::create(surface);

    // Draw throttle indicator
    drawThrottle(cr, 100, 100, 0.5); // Example usage with throttle set to 75%

    surface->write_to_png("throttle.png"); // Save to PNG file

    return 0;
}

