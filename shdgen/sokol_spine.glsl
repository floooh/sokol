@vs vs
layout(binding=0) uniform vs_params {
    mat4 mvp;
};
in vec2 position;
in vec2 texcoord0;
in vec4 color0;
out vec2 uv;
out vec4 color;
void main() {
    gl_Position = mvp * vec4(position, 0.0, 1.0);
    uv = texcoord0;
    color = color0;
}
@end

@fs fs
layout(binding=0) uniform texture2D tex;
layout(binding=0) uniform sampler smp;
layout(binding=1) uniform fs_params {
    float pma;
};
in vec2 uv;
in vec4 color;
out vec4 frag_color;
void main() {
    vec4 c0 = texture(sampler2D(tex, smp), uv) * color;
    vec4 c1 = vec4(c0.rgb * c0.a, c0.a) * color;
    frag_color = mix(c0, c1, pma);
}
@end

@program shd vs fs
