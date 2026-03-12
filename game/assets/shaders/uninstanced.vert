#version 460 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;

out vec2 TexCoords;
out vec3 Normal;
out vec3 FragPos;

uniform mat4 model;
uniform mat4 view;
uniform mat4 perspective;
uniform mat3 normal;

void main()
{
    TexCoords = aTexCoords;
    Normal = normal * aNormal;
    vec4 game_pos = model * vec4(aPos, 1.0f);
    FragPos = vec3(game_pos);
    gl_Position = perspective * view * game_pos;
}