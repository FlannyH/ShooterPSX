#version 140

in vec3 out_position_ws;
in vec3 out_color;
in vec2 out_texcoord;
in float out_texture;
in vec3 out_normal;
out vec4 frag_color;
out vec2 object_picking;

uniform sampler2D tex;
uniform int texture_bound;
uniform int texture_is_page;
uniform int texture_blend_mode;
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
    vec2 pixel_coord;
    vec2 texture_offset;
    float texture_size;
    vec4 vtx_color = vec4(out_color, alpha);
    vec4 tex_color = vec4(0.0);
    vec4 light_color = vec4(0.0);

    if (texture_is_page == 0) {
        texture_offset = vec2(
            (floor((out_texture / 4.0) + 0.1) * 64.0), 
            (mod(floor(out_texture + 0.1), 4.0) * 64.0)
        );
        texture_size = 64.0;
        pixel_coord = (out_texcoord / vec2(256.0) * vec2(64.0));
    } 
    else {
        texture_offset = vec2(
            (float(out_texture) * 256.0),
            256.0
        );
        texture_size = 255.0;
        pixel_coord = out_texcoord;
    }
    if (interpolation_mode != 0) { // interpolated solutions need a half pixel offset
        pixel_coord += vec2(0.5, 0.5);
    }
    if (texture_bound == 1 && (abs(out_texture - 255.0) > 0.5)) {
        float u = pixel_coord.x;
        float v = pixel_coord.y;
        float u1 = floor(pixel_coord.x);
        float u2 = ceil(pixel_coord.x);
        float v1 = floor(pixel_coord.y);
        float v2 = ceil(pixel_coord.y);
        float u_edged, v_edged, u1_edged, u2_edged, v1_edged, v2_edged;

        if (edge_behavior == 0) { // wrap
            u_edged = mod(u, texture_size);
            v_edged = mod(v, texture_size);
            u1_edged = mod(u1, texture_size);
            u2_edged = mod(u2, texture_size);
            v1_edged = mod(v1, texture_size);
            v2_edged = mod(v2, texture_size);
            pixel_coord = mod(pixel_coord, texture_size);
        }
        else if (interpolation_mode != 0) { // clamp for interpolated
            u_edged = clamp(u, -0.5, texture_size - 0.5);
            v_edged = clamp(v, -0.5, texture_size - 0.5);
            u1_edged = clamp(u1, -0.5, texture_size - 0.5);
            u2_edged = clamp(u2, -0.5, texture_size - 0.5);
            v1_edged = clamp(v1, -0.5, texture_size - 0.5);
            v2_edged = clamp(v2, -0.5, texture_size - 0.5);
            pixel_coord = clamp(pixel_coord, -0.5, texture_size - 0.5);
        }
        else { // clamp for point sampled
            u_edged = clamp(u, 0.0, texture_size);
            v_edged = clamp(v, 0.0, texture_size);
            u1_edged = clamp(u1, 0.0, texture_size);
            u2_edged = clamp(u2, 0.0, texture_size);
            v1_edged = clamp(v1, 0.0, texture_size);
            v2_edged = clamp(v2, 0.0, texture_size);
            pixel_coord = clamp(pixel_coord, 0.0, texture_size);
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
            tex_color = (sample1 + sample2 + sample3 + sample4);
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
                tex_color = (sample1 * weight1) + (sample2 * weight2) + (sample3 * weight3);
            }
            else {
                vec4 sample4 = texture2D(tex, (vec2(u2_edged, v2_edged) + texture_offset) / vec2(2048.0, 512.0));
                float weight4 = 1.0 - ((1.0-fu) + (1.0-fv));
                float weight3 = 1.0 - fu;
                float weight2 = 1.0 - fv;
                tex_color = (sample2 * weight2) + (sample3 * weight3) + (sample4 * weight4);
            }
        }
        else { // point sampling - default
            tex_color = texture2D(tex, (vec2(u, v) + texture_offset) / vec2(2048.0, 512.0));
        }
    } 
    if (tex_color.a < 0.001) {
        discard;
    }
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

    frag_color.xyz = (vtx_color.xyz + light_color.xyz) * tex_color.xyz;
    frag_color.xyz = pow(frag_color.xyz, vec3(2.8 / 2.2)); // closer to how it looks on my ps1
    frag_color.w = 1.0;
    object_picking.x = float(drawing_id) / 255.0;
    object_picking.y = float(drawing_what) / 255.0;
}
