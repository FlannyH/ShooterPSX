#version 140

const float PI = 3.14159265358979;

uniform sampler2D tex;
uniform float time_since_start;
in vec2 uv;
out vec4 frag_color;

const float carrier_cycles = 212.0;
const float dot_crawl_strength = 0.0;
const float scanline_intensity = 0.0;
const float contrast = 1.0;
const float brightness = 0.0;

// x = signal, y = phase
vec2 composite(vec2 uv_) {
    vec3 col = texture(tex, uv_).rgb;
    col = floor(col * 31.0) / 31.0;

    vec3 ycc = vec3(
        (0.299000 * col.r) + (0.587000 * col.g) + (0.114000 * col.b),
        -(0.168746 * col.r) - (0.331264 * col.g) + (0.500000 * col.b),
        (0.500000 * col.r) - (0.418688 * col.g) - (0.081312 * col.b)
    );

    float line = floor(uv_.y * 240);
    float x = 2.0 * PI * carrier_cycles * uv_.x;
    float y = (1.0 * PI * line);
    float line_phase = time_since_start * 60;

    float signal = ycc.x + (ycc.y * cos(x+y+line_phase)) + (ycc.z * sin(x+y+line_phase));
    return vec2(signal, x+y+line_phase);
}

vec3 apply_composite(vec2 uv_snap) {
    const float half_period = 0.5 / carrier_cycles;
    const float quarter_period = 0.25 / carrier_cycles;

    float y = 0;
    float u = 0;
    float v = 0;

    // get luma first - we need it to demodulate chroma
    float weight = 0.0;
    for (int i = -1; i <= 1; ++i) {

        float x = uv_snap.x + (float(i) * half_period);
        vec2 comp_result = composite(vec2(x, uv_snap.y));
        float signal = comp_result.x;
        float phase = comp_result.y;
        float curr_weight = 0.25;
        if (i == 0) {
            curr_weight = 0.5;
        }
        y += signal * curr_weight;
        weight += curr_weight;
    }
    y /= weight;

    // get chroma
    for (int i = -2; i <= 2; ++i) {
        float x = uv_snap.x + (float(i) * quarter_period);
        vec2 comp_result = composite(vec2(x, uv_snap.y));
        float signal = comp_result.x;
        float phase = comp_result.y;
        float chroma = (signal - y);
        u += chroma * cos(phase);
        v += chroma * sin(phase);
    }
    u /= 4.0;
    v /= 4.0;

    vec3 regular_rgb = texture(tex, uv_snap).rgb;

    vec3 composite_rgb = vec3(
        y + (1.402 * v),
        y - (0.344136 * u) - (0.714136 * v),
        y + (1.772 * u)
    );

    float scanline_strength = 1.0 - (abs(uv_snap.y - uv.y) * scanline_intensity * 240);
    return scanline_strength * mix(regular_rgb, composite_rgb, dot_crawl_strength);
}

void main() {
    vec2 uv_snap = uv;
    uv_snap.y = (floor(uv_snap.y * 240) + 0.5) / 240;

    vec3 composite_rgb = apply_composite(uv_snap);
    vec3 mapped_rgb = (composite_rgb * contrast) + brightness;

    frag_color.rgb = mapped_rgb;
}
