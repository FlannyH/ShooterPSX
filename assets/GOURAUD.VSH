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

void main()
{
	gl_Position = proj_matrix * view_matrix * model_matrix * vec4(in_position, 1.0);
	out_color = in_color;
	out_texcoord = in_texcoord.xy;
	out_texture = in_texture + float(texture_offset);
}