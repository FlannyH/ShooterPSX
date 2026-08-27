#version 140

in vec3 in_position;

out vec3 out_position_ws;

uniform mat4 proj_matrix;
uniform mat4 view_matrix;
uniform mat4 model_matrix;
uniform int curr_depth_bias;

void main() {
    // local -> world (-> fragment shader)
	vec4 world_pos = model_matrix * vec4(in_position, 1.0);
	out_position_ws = world_pos.xyz;

	// world -> camera -> screen
	gl_Position = proj_matrix * view_matrix * world_pos;
	gl_Position.x = floor(gl_Position.x/gl_Position.w * 512.0) / 512.0 * gl_Position.w;
	gl_Position.y = floor(gl_Position.y/gl_Position.w * 240.0) / 240.0 * gl_Position.w;
    gl_Position.z += float(curr_depth_bias) / 2048.0;
}
