#version 460 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;
layout (location = 3) in mat4 aModelMatrix;

out vec2 TexCoords;
out vec3 Normal;
out vec3 FragPos;

uniform mat4 view;
uniform mat4 perspective;

void main()
{
    TexCoords = aTexCoords;
    Normal = normalize(mat3(aModelMatrix) * aNormal);
    vec4 game_pos = aModelMatrix * vec4(aPos, 1.0f);
    FragPos = game_pos.xyz;
    gl_Position = perspective * view * game_pos;
}