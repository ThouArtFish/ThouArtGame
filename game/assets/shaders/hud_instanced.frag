#version 460 core

out vec4 FragColour;
in vec2 TexCoord;
flat in int tex_index;
uniform sampler2D diff_texs[16];
void main() 
{
	FragColour = texture(diff_texs[tex_index], TexCoord);
	if (FragColour.w < 0.01f)
		discard;
}