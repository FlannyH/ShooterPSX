#version 460

in vec3 in_position;
in vec3 in_color;
in vec2 in_texcoord;
in float in_texture;
in vec3 in_normal;

out vec3 out_position_ws;
out vec3 out_color;
out vec2 out_texcoord;
out vec3 out_normal;
flat out float out_texture;
flat out vec2 out_texture_offset;
flat out vec2 out_texture_size;

uniform sampler2D tex;
uniform isampler2D tex_meta;
uniform mat4 proj_matrix;
uniform mat4 view_matrix;
uniform mat4 model_matrix;
uniform int curr_depth_bias;
uniform int vertex_lighting;
uniform int tex_category;

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
	// find texture offset
	ivec4 tex_meta_sample = texelFetch(tex_meta, ivec2(int(in_texture), tex_category), 0);
	out_texture_offset = vec2(tex_meta_sample.x, tex_meta_sample.y);
	out_texture_size = vec2(tex_meta_sample.z, tex_meta_sample.w);

	vec4 world_pos = model_matrix * vec4(in_position, 1.0);
	out_position_ws = world_pos.xyz;
	gl_Position = proj_matrix * view_matrix * world_pos;
	gl_Position.x = floor(gl_Position.x/gl_Position.w * 512.0) / 512.0 * gl_Position.w;
	gl_Position.y = floor(gl_Position.y/gl_Position.w * 240.0) / 240.0 * gl_Position.w;
    gl_Position.z += float(curr_depth_bias) / 2048.0;
	out_color = in_color * 2.0;
	out_texcoord = in_texcoord;
	out_normal = in_normal;
	out_texture = in_texture;
	
	if (vertex_lighting != 0) {
		for (int i = 0; i < 256; ++i) {
			if (abs(lights[i].type - 0.0) < 0.5) continue;
			if (abs(lights[i].type - 1.0) < 0.5) { // Directional
				vec3 add_light_color = clamp(dot(out_normal, -lights[i].direction_position), 0.0, 1.0) * lights[i].color * lights[i].intensity;
				out_color.xyz += add_light_color;
			}
			if (abs(lights[i].type - 2.0) < 0.5) { // Point
				vec3 surface_to_light = (lights[i].direction_position * -8.0) - (out_position_ws / 4096.0);
				vec3 light_dir = normalize(surface_to_light) * vec3(-1.0, -1.0, 1.0);
				float distance2 = dot(surface_to_light, surface_to_light); // todo: is this in the right units?

				vec3 add_light_color = clamp(dot(out_normal, light_dir), 0.0, 1.0) * lights[i].color * lights[i].intensity;
				out_color.xyz += (add_light_color / distance2);
			}
		}
	}
}
