#version 450

layout(set = 0, binding = 0) uniform UniformBufferObject {
    mat4 view;
    mat4 proj;
} ubo;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec2 inTexCoord;

layout(location = 0) out vec3 fragColor;

layout( push_constant ) uniform constants
{
	mat4 renderMatrix;
} Mesh;

void main() {
    float outlineWidth = 1.10;
	
	mat4 scaleMatrix = mat4(0.0);
    scaleMatrix[0][0] = outlineWidth;
    scaleMatrix[1][1] = outlineWidth;
    scaleMatrix[2][2] = outlineWidth;
	scaleMatrix[3][3] = 1;
    mat4 scaledRenderMatrix = Mesh.renderMatrix * scaleMatrix;


	mat4 transformMatrix = ubo.proj * ubo.view * scaledRenderMatrix;
    gl_Position = transformMatrix * vec4(inPosition, 1.0);
}