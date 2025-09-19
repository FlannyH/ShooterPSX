#version 140

in vec3 out_position_ws;
in vec3 out_color;
in vec2 out_texcoord;
in vec3 out_normal;
flat in float out_texture;
flat in vec2 out_texture_offset;
flat in vec2 out_texture_size;

out vec4 frag_color;
out vec2 object_picking;

uniform sampler2D tex;
uniform int texture_bound;
uniform int interpolation_mode; // 0 = point, 1 = tri-point filtering, 2 = bilinear filtering
uniform int edge_behavior; // 0 = wrap, 1 = clamp
uniform int vertex_lighting;
uniform float alpha;

uniform int drawing_id;
uniform int drawing_what;

struct Light {
	vec3 direction_position;
	float intensity;
	vec3 color;
	float type;
};

uniform Lights {
    Light lights[256]; // MAX_LIGHT_COUNT
};

void main() {
    vec4 vtx_color = vec4(out_color, alpha);
    vec4 tex_color = vec4(0.0, 0.0, 0.0, 1.0);
    vec4 light_color = vec4(0.0);

    // If a texture is bound to the shader, and if this polygon has a texture
    if (texture_bound == 1 && (abs(out_texture - 255.0) > 0.5)) {
        // Get texture pixel coordinate on the texture atlas
        vec2 uv_normalized = out_texcoord;
        vec2 pixel_coord_within_texture = out_texcoord;

        // Find mip level, and make sure it's not too high as to keep individual textures 1x1
        // the texture allocator guarantees that a texture will be at least the rounded up to
        // a power of 2 based on its smallest axis
        vec2 dx = dFdx(pixel_coord_within_texture);
        vec2 dy = dFdy(pixel_coord_within_texture);
        float rho = max(length(dx), length(dy));
        float lod = max(0.0, log2(rho));
        int max_mip = int(floor(log2(min(out_texture_size.x, out_texture_size.y))));
        int curr_mip = int(clamp(floor(lod), 0.0, float(max_mip)));
        float res_mul = 1.0 / (1 << curr_mip);

        pixel_coord_within_texture *= vec2(res_mul);

        int curr_interpolation_mode = interpolation_mode;
        if (lod > 1.5) curr_interpolation_mode = 2;
        if (curr_interpolation_mode != 0) pixel_coord_within_texture -= 0.5;

        // Get 4 integer sampling positions
        ivec2 texture_size = ivec2(out_texture_size * vec2(res_mul));
        ivec2 texture_offset = ivec2(out_texture_offset * vec2(res_mul));
        float fx = pixel_coord_within_texture.x;
        float fy = pixel_coord_within_texture.y;
        float fx1 = floor(fx);
        float fx2 = fx1 + 1.0;
        float fy1 = floor(fy);
        float fy2 = fy1 + 1.0;
        int x1;
        int x2;
        int y1;
        int y2;

        // Apply edge correction (wrap, clamp)
        if (edge_behavior == 0) { // wrap
            x1 = texture_offset.x + (int(fx1) % texture_size.x);
            x2 = texture_offset.x + (int(fx2) % texture_size.x);
            y1 = texture_offset.y + (int(fy1) % texture_size.y);
            y2 = texture_offset.y + (int(fy2) % texture_size.y);
        }

        else /*if (edge_behavior == 1)*/ { // clamp
            x1 = texture_offset.x + (clamp(int(fx1), 0, texture_size.x - 1));
            x2 = texture_offset.x + (clamp(int(fx2), 0, texture_size.x - 1));
            y1 = texture_offset.y + (clamp(int(fy1), 0, texture_size.y - 1));
            y2 = texture_offset.y + (clamp(int(fy2), 0, texture_size.y - 1));
        }

        // Sample the texture (point, n64, bilinear)
        if (curr_interpolation_mode == 0) { // point sample
            tex_color = texelFetch(tex, ivec2(x1, y1), curr_mip);
        }
        else if (curr_interpolation_mode == 1) { // n64-style tri-point sample
            float frac_x = fx - fx1;
            float frac_y = fy - fy1;
            vec4 sample21 = texelFetch(tex, ivec2(x2, y1), curr_mip);
            vec4 sample12 = texelFetch(tex, ivec2(x1, y2), curr_mip);

            // triangle 11, 21, 12
            if (frac_x + frac_y <= 1.0) {
                vec4 sample11 = texelFetch(tex, ivec2(x1, y1), curr_mip);
                tex_color = sample11 * (1.0 - frac_x - frac_y)
                          + sample21 * (frac_x)
                          + sample12 * (frac_y);
            }
            // triangle 22, 21, 12
            else {
                vec4 sample22 = texelFetch(tex, ivec2(x2, y2), curr_mip);
                tex_color = sample22 * (frac_x + frac_y - 1.0)
                          + sample21 * (1.0 - frac_y)
                          + sample12 * (1.0 - frac_x);
            }
        }
        else if (curr_interpolation_mode == 2) { // bilinear sample
            float frac_x = fx - fx1;
            float frac_y = fy - fy1;
            vec4 sample11 = texelFetch(tex, ivec2(x1, y1), curr_mip);
            vec4 sample21 = texelFetch(tex, ivec2(x2, y1), curr_mip);
            vec4 sample12 = texelFetch(tex, ivec2(x1, y2), curr_mip);
            vec4 sample22 = texelFetch(tex, ivec2(x2, y2), curr_mip);
            vec4 lerp_x_y1 = mix(sample11, sample21, frac_x);
            vec4 lerp_x_y2 = mix(sample12, sample22, frac_x);
            vec4 lerp_y = mix(lerp_x_y1, lerp_x_y2, frac_y);
            tex_color = lerp_y;
        }    

        // Alpha clipping
        if (tex_color.a < 0.5) {
            discard;
        }
    }

    if (alpha < 0.001) {
        discard;
    }


    // Materials with id 254 are occluders; don't render them
    if (abs(out_texture - 254.0) < 0.5)  {
        discard;
    }

    if (vertex_lighting == 0) {
        for (int i = 0; i < 256; ++i) {
            if (abs(lights[i].type - 0.0) < 0.5) continue;
            if (abs(lights[i].type - 1.0) < 0.5) { // Directional
                vec3 add_light_color = clamp(dot(out_normal, -lights[i].direction_position), 0.0, 1.0) * lights[i].color * lights[i].intensity;
                light_color.xyz += add_light_color;
            }
            if (abs(lights[i].type - 2.0) < 0.5) { // Point
                vec3 surface_to_light = (lights[i].direction_position * -8.0) - (out_position_ws / 4096.0);
                vec3 light_dir = normalize(surface_to_light) * vec3(-1.0, -1.0, 1.0);
                float distance2 = dot(surface_to_light, surface_to_light); // todo: is this in the right units?

                vec3 add_light_color = clamp(dot(out_normal, light_dir), 0.0, 1.0) * lights[i].color * lights[i].intensity;
                light_color.xyz += (add_light_color / distance2);
            }
        }
    }

    // Apply lighting
    frag_color.xyz = (vtx_color.xyz + light_color.xyz) * tex_color.xyz;

    // Gamma correction - roughly converts 2.2 gamma to 2.8 gamma, which is closer to how it looks on my ps1
    frag_color.xyz = pow(frag_color.xyz, vec3(2.8 / 2.2));
    frag_color.w = tex_color.a * alpha;

    // For editor
    // todo: fix this
    object_picking.x = float(drawing_id) / 255.0;
    object_picking.y = float(drawing_what) / 255.0;
}
