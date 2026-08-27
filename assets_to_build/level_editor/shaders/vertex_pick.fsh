#version 330

in vec3 out_position_ws;

layout(location = 0) out vec4 frag_color;
layout(location = 1) out vec2 object_picking;
layout(location = 2) out vec3 vertex_picking;

uniform int drawing_id;
uniform int drawing_what;

void main() {
    // make circle shape around vertex instead of square
    vec2 p = gl_PointCoord * 2.0 - 1.0;
    if (dot(p, p) > 1.0)
        discard;

    vertex_picking = out_position_ws;

    // todo(gouraud_obj_picking_fix): desc: verify object picking correctness
    object_picking.x = float(drawing_id) / 255.0;
    object_picking.y = float(drawing_what) / 255.0;
}
