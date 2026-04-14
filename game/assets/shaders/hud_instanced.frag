#version 460 core

out vec4 FragColour;

in vec2 TexCoord;
flat in int tex_index;

uniform sampler2D texture_indices[16];

void main() 
{
	vec4 colour = texture(texture_indices[tex_index], TexCoord);
	if (colour.w < 0.01f)
		discard;
	FragColour = colour;
}