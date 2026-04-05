#version 450

layout(location = 0) in vec2 v_uv;
layout(location = 1) in vec4 v_color;

// Sampler (binding 0 in fragment stage)
layout(set = 2, binding = 0) uniform sampler2D u_texture;

layout(location = 0) out vec4 out_color;

void main() {
    // If texture handle is null (null check not possible in GLSL; the sampler
    // will sample a 1x1 white texture created by the engine for untextured draws).
    vec4 tex_color = texture(u_texture, v_uv);
    out_color = tex_color * v_color;
}
