@vs vs
in vec2 position;
in vec2 texcoord0;
in vec4 color0;
out vec2 uv;
out vec4 color;
void main() {
    gl_Position = vec4(position * vec2(2.0, -2.0) + vec2(-1.0, +1.0), 0.0, 1.0);
    uv = texcoord0;
    color = color0;
}
@end

@fs fs
layout(binding=0) uniform texture2D tex;
layout(binding=0) uniform sampler smp;
in vec2 uv;
in vec4 color;
out vec4 frag_color;
void main() {
    frag_color = texture(sampler2D(tex, smp), uv).xxxx * color;
}
@end

@program shd vs fs
