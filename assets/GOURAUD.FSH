#version 430 core
precision mediump float;

in noperspective vec3 out_color;
in noperspective vec2 out_texcoord;
out vec4 frag_color;

uniform sampler2D tex;

void main()
{
	frag_color = vec4(out_color, 1.0f) * texture(tex, out_texcoord / vec2(255.0f));
}