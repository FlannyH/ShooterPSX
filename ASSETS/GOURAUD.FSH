#version 430 core
precision mediump float;

in vec3 out_color;
in vec2 out_texcoord;
out vec4 frag_color;

uniform sampler2D tex;
uniform int texture_bound;

void main()
{
	frag_color = vec4(out_color, 1.0f) * (texture_bound == 1 ? texture(tex, out_texcoord / vec2(255.0f)) : vec4(1.0f, 1.0f, 1.0f, 1.0f));
}