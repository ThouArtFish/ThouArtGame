#version 460 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;
layout (location = 3) in vec4 ModelPosScale;
layout (location = 4) in vec4 ModelAxisRot;
out vec2 TexCoords;
out vec3 Normal;
out vec3 FragPos;
uniform mat4 view;
uniform mat4 perspective;
vec3 axisRotation(vec3 v, vec3 a, float r) {
	return v * cos(r) + cross(a, v) * sin(r) + a * dot(a, v) * (1.0f - cos(r));
}
void main() {
    TexCoords = aTexCoords;
    Normal = axisRotation(aNormal, ModelAxisRot.xyz, ModelAxisRot.w);
    FragPos = axisRotation(aPos, ModelAxisRot.xyz, ModelAxisRot.w) * ModelPosScale.w + ModelPosScale.xyz;
    gl_Position = perspective * view * vec4(FragPos, 1.0f);
}