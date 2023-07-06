#version 430 core
precision mediump float;

in vec3 out_color;
in vec2 out_texcoord;
in float out_texture;
out vec4 frag_color;

uniform sampler3D tex;
uniform int texture_bound;

void main()
{
	frag_color = vec4(out_color, 1.0f) * (texture_bound == 1 ? texture(tex, vec3(out_texcoord / vec2(255.0f), out_texture / 255.f)) : vec4(1.0f, 1.0f, 1.0f, 1.0f));
}