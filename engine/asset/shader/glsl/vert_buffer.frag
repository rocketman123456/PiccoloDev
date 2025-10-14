#version 450

layout(location = 0) in vec3 vWorldPos;
layout(location = 1) in vec3 vNormal;
layout(location = 2) in vec3 vAlbedo;

layout(location = 0) out vec4 outColor;

void main() {
    // Simple Phong shading with a single directional light
    vec3 N = normalize(vNormal);
    vec3 L = normalize(vec3(0.4, 1.0, 0.3));
    vec3 V = normalize(-vWorldPos); // camera at origin in world; acceptable for demo
    vec3 R = reflect(-L, N);

    float ambient = 0.2;
    float diff    = max(dot(N, L), 0.0);
    float spec    = pow(max(dot(R, V), 0.0), 32.0);

    vec3 color = vAlbedo * (ambient + diff) + vec3(0.5) * spec;
    outColor = vec4(color, 1.0);
}
