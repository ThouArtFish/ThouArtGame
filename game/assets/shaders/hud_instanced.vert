#version 460 core

layout (location = 0) in vec2 Base;
layout (location = 1) in vec4 QuadData;
layout (location = 2) in vec4 TexelData;
layout (location = 3) in int TexInd;

out vec2 TexCoord;
flat out int tex_index;

void main() 
{
	tex_index = TexInd;
	TexCoord = Base * TexelData.zw + TexelData.xy;
	gl_Position = vec4(vec2(Base.x, Base.y - 1.0f) * QuadData.zw + QuadData.xy, 0.0f, 1.0f);
}