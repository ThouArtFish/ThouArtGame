#version 460 core

out vec4 FragColour;
in vec2 TexCoords;
in vec3 Normal;
in vec3 FragPos;
uniform int diff_tex_num;
uniform vec3 colour;
uniform sampler2D diff_texs[16];
void main() {
    if (diff_tex_num == 0)
	FragColour = vec4(colour, 1.0f);
    else
	FragColour = glm::vec4(1.0f);
	float factor = 1.0f / ((float) diff_tex_num);
	for (int i = 0; i < diff_tex_num; i++) {
		FragColour = mix(FragColour, texture(diff_texs[i], TexCoords), factor);
	}
}