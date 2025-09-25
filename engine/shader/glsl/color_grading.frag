#version 310 es

#extension GL_GOOGLE_include_directive : enable

#include "constants.h"

layout(input_attachment_index = 0, set = 0, binding = 0) uniform highp subpassInput in_color;

layout(set = 0, binding = 1) uniform sampler2D color_grading_lut_texture_sampler;

layout(location = 0) out highp vec4 out_color;

#define CELLS_PER_ROW 8.0
#define CELLS_PER_COLUM 8.0

void main()
{
    highp ivec2 lut_tex_size = textureSize(color_grading_lut_texture_sampler, 0);
    highp float _COLORS      = float(lut_tex_size.y);
    highp vec4 base_color    = subpassLoad(in_color).rgba;

    // ------------------------------------------------------------------------------

    out_color = base_color;

    // ------------------------------------------------------------------------------

    // highp float cell         = base_color.b * 15.0; // for interpoation
    // highp float cell_l       = floor(base_color.b * 15.0); // len, 15 currently
    // highp float cell_r       = ceil(base_color.b * 15.0);

    // highp float x_half       = 0.5 / 256.0; // read from center 0.5/width
    // highp float y_half       = 0.5 / 16.0; // read from center 0.5/height

    // highp vec2 uv_l          = vec2(
    //     x_half + base_color.r / _COLORS * (15.0 / _COLORS) + cell_l / _COLORS,
    //     y_half + base_color.g * (15.0 / _COLORS)
    // );
    // highp vec2 uv_r          = vec2(
    //     x_half + base_color.r / _COLORS * (15.0 / _COLORS) + cell_r / _COLORS,
    //     y_half + base_color.g * (15.0 / _COLORS)
    // );

    // highp vec4 color_l       = texture(color_grading_lut_texture_sampler, uv_l);
    // highp vec4 color_r       = texture(color_grading_lut_texture_sampler, uv_r);
    // highp vec4 mid_color     = mix(color_l, color_r, fract(cell));

    // out_color = mid_color;

    // ------------------------------------------------------------------------------

    // highp float half_pixel_size = 0.5 / float(lut_tex_size.x * lut_tex_size.y);
    // highp float cell_xSize =  1.0 / CELLS_PER_ROW;
    // highp float cell_ySize =  1.0 / CELLS_PER_COLUM;

    // highp float blue_cell = base_color.b * (CELLS_PER_ROW * CELLS_PER_COLUM - 1.0);
    // highp float xOffset = half_pixel_size + base_color.r * (cell_xSize - (2.0 * half_pixel_size));
    // highp float yOffset = half_pixel_size + base_color.g * (cell_ySize - (2.0 * half_pixel_size));

    // highp vec2 lower_cell, lower_sample, upper_cell, upper_sample;
    
    // lower_cell.y = floor(blue_cell / CELLS_PER_ROW);
    // lower_cell.x = floor(blue_cell) - lower_cell.y * CELLS_PER_COLUM;
    // lower_sample.x = lower_cell.x * cell_xSize + xOffset;
    // lower_sample.y = lower_cell.y * cell_ySize + yOffset;

    // upper_cell.y = floor(ceil(blue_cell) / CELLS_PER_ROW);
    // upper_cell.x = ceil(blue_cell) - upper_cell.y * CELLS_PER_COLUM;
    // upper_sample.x = upper_cell.x * cell_xSize + xOffset;
    // upper_sample.y = upper_cell.y * cell_ySize + yOffset;

    // highp vec3 color = mix(
    //     texture(color_grading_lut_texture_sampler, lower_sample).rgb,
    //     texture(color_grading_lut_texture_sampler, upper_sample).rgb,
    //     fract(blue_cell)
    // );

    // out_color = mix(base_color, vec4(color, 1.0), 1.0);
}
