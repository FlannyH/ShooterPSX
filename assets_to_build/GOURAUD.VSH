#version 430 core
precision mediump float;

in layout (location = 0) vec3 in_position;
in layout (location = 1) vec3 in_color;
in layout (location = 2) vec2 in_texcoord;
in layout (location = 3) float in_texture;

out vec3 out_color;
out vec2 out_texcoord;
out float out_texture;

uniform mat4 proj_matrix;
uniform mat4 view_matrix;
uniform mat4 model_matrix;
uniform int texture_offset;
uniform int curr_depth_bias;

void main()
{
	gl_Position = proj_matrix * view_matrix * model_matrix * vec4(in_position, 1.0);
	gl_Position.x = floor(gl_Position.x/gl_Position.w * 512.0) / 512.0 * gl_Position.w;
	gl_Position.y = floor(gl_Position.y/gl_Position.w * 240.0) / 240.0 * gl_Position.w;
    gl_Position.z += float(curr_depth_bias) / 2048.0;
	out_color = in_color * 2;
	out_texcoord = in_texcoord.xy;
	out_texture = in_texture + float(texture_offset);
}