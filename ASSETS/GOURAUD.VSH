#version 430 core
precision mediump float;

in layout (location = 0) vec3 in_position;
in layout (location = 1) vec3 in_color;
in layout (location = 2) vec3 in_texcoord;

out vec3 out_color;
out vec2 out_texcoord;

uniform mat4 proj_matrix;
uniform mat4 view_matrix;
uniform mat4 model_matrix;

void main()
{
	gl_Position = proj_matrix * view_matrix * model_matrix * vec4(in_position, 1.0);
	out_color = in_color;
	out_texcoord = in_texcoord.xy;
}