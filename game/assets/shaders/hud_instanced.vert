#version 460 core

layout (location = 0) in vec2 Base;
layout (location = 1) in vec2 QuadTrans;
layout (location = 2) in vec2 QuadScale;
layout (location = 3) in vec2 TexelTrans;
layout (location = 4) in vec2 TexelScale;
layout (location = 5) in int TexInd;

out vec2 TexCoord;
flat out int tex_index;

void main() 
{
	TexCoord = Base * TexelScale + TexelTrans;
	tex_index = TexInd;
	gl_Position = vec4(vec2(Base.x, Base.y - 1.0f) * QuadScale + QuadTrans, 0.0f, 1.0f);
}