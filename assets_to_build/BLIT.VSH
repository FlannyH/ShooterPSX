#version 330 core

const vec2 vertices[3] = vec2[](
    vec2(-1.0, -1.0), 
    vec2( 3.0, -1.0), 
    vec2(-1.0,  3.0)  
);
const vec2 uvs[3] = vec2[](
    vec2(0.0, 0.0), 
    vec2(2.0, 0.0), 
    vec2(0.0, 2.0)  
);
out vec2 uv;

void main() {
    gl_Position = vec4(vertices[gl_VertexID], 0.0, 1.0);
    uv = uvs[gl_VertexID];
}
