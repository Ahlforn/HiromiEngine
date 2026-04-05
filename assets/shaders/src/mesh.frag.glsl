#version 450

layout(location = 0) in vec3 v_normal;
layout(location = 1) in vec2 v_uv;

layout(set = 2, binding = 0) uniform sampler2D u_texture;

layout(location = 0) out vec4 out_color;

void main() {
    // Simple diffuse lighting from a fixed light direction.
    const vec3 light_dir = normalize(vec3(0.5, 1.0, 0.5));
    float diffuse = max(dot(normalize(v_normal), light_dir), 0.1);

    vec4 base = texture(u_texture, v_uv);
    out_color = vec4(base.rgb * diffuse, base.a);
}
