#version 330 core
#define POSITION	0
#define TEXCOORD	1
#define COLOR		3
#define FRAG_COLOR	0

uniform mat4 MVP;

layout(location = POSITION) in vec2 p;
layout(location = TEXCOORD) in vec2 t;

out block
{
  vec2 t;
} Out;

void main()
{	
  Out.t = t;
  gl_Position = MVP * vec4(p,0.f,1.f);
}


