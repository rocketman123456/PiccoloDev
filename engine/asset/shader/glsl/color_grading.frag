#version 310 es

#extension GL_GOOGLE_include_directive : enable

#include "constants.h"

layout(input_attachment_index = 0, set = 0, binding = 0) uniform highp subpassInput in_color;

layout(set = 0, binding = 1) uniform sampler2D color_grading_lut_texture_sampler;

layout(location = 0) out highp vec4 out_color;

#define CELL_SIZE 16.0
#define IMAGE_WIDTH 256.0
#define IMAGE_HEIGHT 16.0

// #define CELL_SIZE 32.0
// #define IMAGE_WIDTH 1024.0
// #define IMAGE_HEIGHT 32.0

void main()
{
    highp ivec2 lut_tex_size = textureSize(color_grading_lut_texture_sampler, 0);
    // highp float _COLORS      = float(lut_tex_size.y);
    highp vec4 base_color    = subpassLoad(in_color).rgba;

    // Clamp input color to avoid out-of-bounds sampling
    highp vec3 clamped_color = clamp(base_color.rgb, 0.0, 1.0);

    // ------------------------------------------------------------------------------

    // out_color = base_color;

    // ------------------------------------------------------------------------------

    highp float cell     = clamped_color.b * (CELL_SIZE - 1.0); // for interpoation
    highp float cell_l   = floor(cell); // len, 15 currently
    highp float cell_r   = ceil(cell);

    highp float x_half   = 0.5 / float(lut_tex_size.x); // read from center 0.5/width
    highp float y_half   = 0.5 / float(lut_tex_size.y); // read from center 0.5/height

    highp vec2 uv_l = vec2(
        x_half + (clamped_color.r * (CELL_SIZE - 1.0) / float(lut_tex_size.y) + cell_l) / float(lut_tex_size.y),
        y_half + (clamped_color.g * (CELL_SIZE - 1.0)) / float(lut_tex_size.y)
    );
    
    highp vec2 uv_r = vec2(
        x_half + (clamped_color.r * (CELL_SIZE - 1.0) / float(lut_tex_size.y) + cell_r) / float(lut_tex_size.y),
        y_half + (clamped_color.g * (CELL_SIZE - 1.0)) / float(lut_tex_size.y)
    );

    highp vec4 color_l   = texture(color_grading_lut_texture_sampler, uv_l);
    highp vec4 color_r   = texture(color_grading_lut_texture_sampler, uv_r);
    highp vec4 mid_color = mix(color_l, color_r, fract(cell));

    // out_color = base_color;
    // out_color = mid_color;
    // out_color = vec4(mid_color.rgb, base_color.a);
    out_color = vec4(clamped_color.rgb, base_color.a);
}
