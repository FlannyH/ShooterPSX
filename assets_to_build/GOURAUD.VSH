#version 110

in vec3 in_position;
in vec3 in_color;
in vec2 in_texcoord;
in float in_texture;
in vec3 in_normal;

varying out vec3 out_color;
varying out vec2 out_texcoord;
varying out float out_texture;
varying out vec3 out_normal;

uniform mat4 proj_matrix;
uniform mat4 view_matrix;
uniform mat4 model_matrix;
uniform int texture_offset;
uniform int curr_depth_bias;

void main() {
	gl_Position = proj_matrix * view_matrix * model_matrix * vec4(in_position, 1.0);
	gl_Position.x = floor(gl_Position.x/gl_Position.w * 512.0) / 512.0 * gl_Position.w;
	gl_Position.y = floor(gl_Position.y/gl_Position.w * 240.0) / 240.0 * gl_Position.w;
    gl_Position.z += float(curr_depth_bias) / 2048.0;
	out_color = in_color * 2.0;
	out_texcoord = in_texcoord;
	out_normal = in_normal;
	out_texture = in_texture + float(texture_offset);
}
