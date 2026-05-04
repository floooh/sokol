@vs offscreen_vs
layout(binding=0) uniform offscreen_vs_params {
    vec2 uv_offset;
    vec2 uv_scale;
};
out vec2 uv;

void main() {
    float x = (gl_VertexIndex & 1) != 0 ? 0.0 : 1.0;
    float y = (gl_VertexIndex & 2) != 0 ? 1.0 : 0.0;
    gl_Position = vec4(vec2(x, y) * 2.0 - 1.0, 0.5, 1.0);
    uv = (vec2(x, 1.0 - y) * uv_scale) + uv_offset;
}
@end
@fs rgba8_fs
layout(binding=0) uniform texture2D fb_tex;
layout(binding=0) uniform sampler smp;
in vec2 uv;
out vec4 frag_color;
void main() {
    frag_color = texture(sampler2D(fb_tex, smp), uv);
}
@end
@program rgba8 offscreen_vs rgba8_fs

// offscreen shader with color palette decoding
@fs palette8_fs
layout(binding=0) uniform texture2D fb_tex;
layout(binding=1) uniform texture2D pal_tex;
layout(binding=0) uniform sampler smp;
in vec2 uv;
out vec4 frag_color;
void main() {
    float pix = texture(sampler2D(fb_tex, smp), uv).x;
    frag_color = vec4(texture(sampler2D(pal_tex, smp), vec2(pix,0)).xyz, 1.0);
}
@end
@program palette8 offscreen_vs palette8_fs

@vs render_vs
@glsl_options flip_vert_y
layout(binding=0) uniform render_vs_params {
    int rotate;
};
out vec2 uv;

void main() {
    vec2 in_pos, in_uv;
    in_pos.x = (gl_VertexIndex & 1) != 0 ? 0.0 : 1.0;
    in_pos.y = (gl_VertexIndex & 2) != 0 ? 1.0 : 0.0;
    if (rotate == 0) {
        in_uv.x = in_pos.x;
        in_uv.y = 1.0 - in_pos.y;
    } else {
        in_uv.x = in_pos.y;
        in_uv.y = in_pos.x;
    }
    gl_Position = vec4(in_pos*2.0-1.0, 0.5, 1.0);
    uv = in_uv;
}
@end
@fs render_fs
layout(binding=0) uniform texture2D tex;
layout(binding=0) uniform sampler smp;
in vec2 uv;
out vec4 frag_color;

void main() {
    frag_color = vec4(texture(sampler2D(tex, smp), uv).xyz, 1.0);
}
@end
@program render render_vs render_fs
