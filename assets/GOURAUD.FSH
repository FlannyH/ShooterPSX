#version 430 core
precision mediump float;

in noperspective vec3 out_color;
in noperspective vec2 out_texcoord;
out vec4 frag_color;

void main()
{
	frag_color = vec4(out_color, 1.0f);
}