import cairo
import math
heading = 0
def draw_heading(x, y, step_range, bottom):
    surface = cairo.ImageSurface(cairo.FORMAT_ARGB32, 400, 400)
    ctx = cairo.Context(surface)

    ctx.translate(x, y)

    mf = 1
    if bottom:
        mf = -1

    # Value indicator
    value = math.degrees(heading)

    font_size = 20
    ctx.set_font_size(font_size)

    text_side_border = 5
    text_top_border = 4
    text_width = ctx.text_extents('360')[2]

    length = text_side_border * 2 + text_width  # Total length
    height = text_top_border * 1.5 + font_size + length / 4  # Total height

    ctx.set_source_rgb(0, 0, 0)
    ctx.set_line_width(1)
    ctx.move_to(-length / 2, 0)
    ctx.line_to(length / 2, 0)
    ctx.line_to(length / 2, mf * (text_top_border * 1.5 + font_size))
    ctx.line_to(0, mf * height)
    ctx.line_to(-length / 2, mf * (text_top_border * 1.5 + font_size))
    ctx.close_path()
    ctx.stroke()

    text = round(value)
    ctx.move_to(text_width / 2, mf * (2 * text_top_border + font_size) / 2)
    ctx.show_text(str(text))

    # Scale
    font_size = 16
    ctx.set_font_size(font_size)

    text_border = 2
    border = 4
    step_length = [16, 11, 7]

    ctx.translate(0, mf * (height + border))

    ctx.rectangle(
        (-step_range * 16) / 2,
        0,
        16 * step_range,
        mf * (step_length[0] + 2 * text_border + font_size)
    )
    ctx.clip()

    step_margin = 5
    step_zero_offset = math.ceil(step_range / 2) + step_margin
    step_value_offset = math.floor(value)
    step_offset = value - step_value_offset

    ctx.translate(-(step_zero_offset + step_offset) * 16, 0)

    ctx.move_to(0, 0)
    for i in range(-step_zero_offset + step_value_offset, step_zero_offset + step_value_offset):
        pos_i = abs(i)

        ctx.move_to(0, 0)
        if pos_i % 10 == 0:
            ctx.line_to(0, mf * step_length[0])
        elif pos_i % 5 == 0:
            ctx.line_to(0, mf * step_length[1])
        else:
            ctx.line_to(0, mf * step_length[2])

        if pos_i % 90 == 0 or pos_i % 45 == 0 or pos_i % 10 == 0:
            if pos_i % 360 == 0:
                text = 'N'
            elif pos_i % 45 == 0:
                text = 'NE'
            elif pos_i % 90 == 0:
                text = 'E'
            elif pos_i % 135 == 0:
                text = 'SE'
            elif pos_i % 180 == 0:
                text = 'S'
            elif pos_i % 225 == 0:
                text = 'SW'
            elif pos_i % 270 == 0:
                text = 'W'
            elif pos_i % 315 == 0:
                text = 'NW'
            else:
                text = str(pos_i % 360)

            ctx.move_to(0, mf * (step_length[0] + text_border + font_size / 2))
            ctx.show_text(text)

        ctx.translate(16, 0)

    ctx.stroke()

    surface.write_to_png("heading.png")

draw_heading(200, 200, 20, False)

