#version 430 core
precision mediump float;

in vec3 out_color;
in vec2 out_texcoord;
in float out_texture;
out vec4 frag_color;

uniform sampler2D tex;
uniform int texture_bound;
uniform int texture_is_page;
uniform int texture_blend_mode;
uniform float alpha;

void main()
{
    vec2 pixel_coord;

    if (texture_is_page == 0) {
        vec2 texture_offset = vec2(
            (floor((out_texture / 4) + 0.1f) * 64), 
            (mod(floor(out_texture + 0.1), 4) * 64)
        );
        pixel_coord = texture_offset + (out_texcoord / vec2(4.0f));
    } 
    else {
        pixel_coord = vec2(
            (float(out_texture) * 256.0) + out_texcoord.x,
            256.0 + out_texcoord.y
        );
    }
    frag_color = vec4(out_color, alpha);
    if (texture_bound == 1 && (abs(out_texture - 255.f) > 0.5f)) {
        frag_color *= texture(tex, vec2(pixel_coord / vec2(2048.0f, 512.0f)));
    } 
    if (frag_color.a < 0.001f) {
        discard;
    }
    if (abs(out_texture - 254.0f) < 0.5f)  {
        discard;
    }
    frag_color.xyz = pow(frag_color.xyz, vec3(2.8 / 2.2)); // closer to how it looks on my ps1
}