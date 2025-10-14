#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec3 inColor;

layout(location = 0) out vec3 vWorldPos;
layout(location = 1) out vec3 vNormal;
layout(location = 2) out vec3 vAlbedo;

layout(set = 0, binding = 0) uniform CameraUBO {
    mat4 view;
    mat4 proj;
    vec4 camPos;
} uCamera;

void main() {
    vec4 worldPos = vec4(inPosition, 1.0);
    gl_Position = uCamera.proj * uCamera.view * worldPos;
    vWorldPos = inPosition;
    vNormal   = normalize(inNormal);
    vAlbedo   = inColor;
}
