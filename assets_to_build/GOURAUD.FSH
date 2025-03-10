#version 110

in vec3 out_color;
in vec2 out_texcoord;
in float out_texture;
varying out vec4 frag_color;

uniform sampler2D tex;
uniform int texture_bound;
uniform int texture_is_page;
uniform int texture_blend_mode;
uniform int interpolation_mode; // 0 = point, 1 = tri-point filtering, 2 = bilinear filtering
uniform int edge_behavior; // 0 = wrap, 1 = clamp
uniform float alpha;

void main() {
    vec2 pixel_coord;
    vec2 texture_offset;
    float texture_size;

    if (texture_is_page == 0) {
        texture_offset = vec2(
            (floor((out_texture / 4.0) + 0.1) * 64.0), 
            (mod(floor(out_texture + 0.1), 4.0) * 64.0)
        );
        texture_size = 64.0;
        pixel_coord = (out_texcoord / vec2(255.0) * vec2(64.0));
        if (interpolation_mode != 0) { // interpolated solutions need a half pixel offset
            pixel_coord += vec2(0.5, 0.5);
        }
    } 
    else {
        texture_offset = vec2(
            (float(out_texture) * 256.0),
            256.0
        );
        texture_size = 255.0;
        pixel_coord = out_texcoord;
        pixel_coord += vec2(0.5, 0.5);
    }
    frag_color = vec4(out_color, alpha);
    if (texture_bound == 1 && (abs(out_texture - 255.0) > 0.5)) {
        float u = pixel_coord.x;
        float v = pixel_coord.y;
        float u1 = floor(pixel_coord.x);
        float u2 = ceil(pixel_coord.x);
        float v1 = floor(pixel_coord.y);
        float v2 = ceil(pixel_coord.y);
        float u1_edged, u2_edged, v1_edged, v2_edged;

        if (edge_behavior == 0) { // wrap
            u1_edged = mod(u1, texture_size);
            u2_edged = mod(u2, texture_size);
            v1_edged = mod(v1, texture_size);
            v2_edged = mod(v2, texture_size);
            pixel_coord = mod(pixel_coord, texture_size);
        }
        else { // clamp - default
            u1_edged = clamp(u1, -0.5, texture_size - 0.5);
            u2_edged = clamp(u2, -0.5, texture_size - 0.5);
            v1_edged = clamp(v1, -0.5, texture_size - 0.5);
            v2_edged = clamp(v2, -0.5, texture_size - 0.5);
            pixel_coord = clamp(pixel_coord, -0.5, texture_size - 0.5);
        }

        if (interpolation_mode == 2) { // bilinear sampling
            float weight4 = ((u - u1) * (v - v1)) / ((u2 - u1) * (v2 - v1));
            float weight3 = ((u2 - u) * (v - v1)) / ((u2 - u1) * (v2 - v1));
            float weight2 = ((u - u1) * (v2 - v)) / ((u2 - u1) * (v2 - v1));
            float weight1 = ((u2 - u) * (v2 - v)) / ((u2 - u1) * (v2 - v1));
            vec4 sample1 = weight1 * texture2D(tex, (vec2(u1_edged, v1_edged) + texture_offset) / vec2(2048.0, 512.0));
            vec4 sample2 = weight2 * texture2D(tex, (vec2(u2_edged, v1_edged) + texture_offset) / vec2(2048.0, 512.0));
            vec4 sample3 = weight3 * texture2D(tex, (vec2(u1_edged, v2_edged) + texture_offset) / vec2(2048.0, 512.0));
            vec4 sample4 = weight4 * texture2D(tex, (vec2(u2_edged, v2_edged) + texture_offset) / vec2(2048.0, 512.0));
            frag_color *= (sample1 + sample2 + sample3 + sample4);
        }
        else if (interpolation_mode == 1) { // 3-point sampling
            float fu = fract(pixel_coord.x);
            float fv = fract(pixel_coord.y);
            vec4 sample2 = texture2D(tex, (vec2(u2_edged, v1_edged) + texture_offset) / vec2(2048.0, 512.0));
            vec4 sample3 = texture2D(tex, (vec2(u1_edged, v2_edged) + texture_offset) / vec2(2048.0, 512.0));

            if (fu + fv < 1.0) {
                vec4 sample1 = texture2D(tex, (vec2(u1_edged, v1_edged) + texture_offset) / vec2(2048.0, 512.0));
                float weight3 = fv;
                float weight2 = fu;
                float weight1 = 1.0-fu-fv;
                frag_color *= (sample1 * weight1) + (sample2 * weight2) + (sample3 * weight3);
            }
            else {
                vec4 sample4 = texture2D(tex, (vec2(u2_edged, v2_edged) + texture_offset) / vec2(2048.0, 512.0));
                float weight4 = 1.0 - ((1.0-fu) + (1.0-fv));
                float weight3 = 1.0 - fu;
                float weight2 = 1.0 - fv;
                frag_color *= (sample2 * weight2) + (sample3 * weight3) + (sample4 * weight4);
            }
        }
        else { // point sampling - default
            frag_color *= texture2D(tex, (vec2(u, v) + texture_offset) / vec2(2048.0, 512.0));
        }
    } 
    if (frag_color.a < 0.001) {
        discard;
    }
    if (abs(out_texture - 254.0) < 0.5)  {
        discard;
    }
    frag_color.xyz = pow(frag_color.xyz, vec3(2.8 / 2.2)); // closer to how it looks on my ps1
}
