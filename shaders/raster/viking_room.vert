#version 450

layout(set = 0, binding = 0) uniform UniformBufferObject {
    mat4 view;
    mat4 proj;
} ubo;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec2 inTexCoord;

layout(location = 0) out vec2 fragUV;

layout( push_constant ) uniform constants
{
	mat4 renderMatrix;
} Mesh;

void main() {
	mat4 transformMatrix = ubo.proj * ubo.view * Mesh.renderMatrix;
    gl_Position = transformMatrix * vec4(inPosition, 1.0);
    fragUV = inTexCoord;
}