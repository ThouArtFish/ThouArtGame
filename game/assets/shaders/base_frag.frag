#version 460 core

struct Light {
    vec4 a;
    vec4 b;
};

layout(std430, binding = 0) buffer readonly scene_lights {
    int lights_size;
    Light lights[];
};

out vec4 FragColour;

in vec2 TexCoords;
in vec3 Normal;
in vec3 FragPos;

uniform float ambience = 0.1f;
uniform float spec_mod;
uniform float spec_exp;
uniform bool spec_map;
uniform sampler2D diffuse1;
uniform sampler2D specular1;

void main() {   
    FragColour = texture(diffuse1, TexCoords); 
}