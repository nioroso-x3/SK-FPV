#include <cairommconfig.h>
#include <cairomm/context.h>
#include <cairomm/surface.h>

void draw_horizon_ladder(const Cairo::RefPtr<Cairo::Context>& cr, double x, double y, double pixel_per_deg) {
    cr->save();  // Save the current state of the context

    // Constants
    double length = 460;
    double space = 80;
    double q = 12;
    double small_length = 26;  // Re-declare length for small dashes

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

    // Begin a new path for small dash lines
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

    cr->set_dash(std::vector<double>(0), 0); // Resetting dash pattern to solid line

    // Restore the context to its original state before translation
    cr->translate(-x, -y - 3 * pixel_per_deg);
    cr->restore();
}

int main() {
    auto surface = Cairo::ImageSurface::create(Cairo::FORMAT_ARGB32, 640, 480);
    auto cr = Cairo::Context::create(surface);

    // Example settings and call
    double pixel_per_deg = 10;  // Example value, adjust as needed
    draw_horizon_ladder(cr, 320, 240, pixel_per_deg);

    // Save to file
    surface->write_to_png("horizon.png");

    return 0;
}
