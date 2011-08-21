#version 330 core
#define POSITION	0
#define TEXCOORD  1
#define COLOR     2
#define NORMAL    3
#define TANGENT   4

layout(location = POSITION) in vec3 p;
layout(location = NORMAL)   in vec3 n;
layout(location = COLOR)    in vec3 c;
layout(location = TANGENT)  in vec3 t;

out block
{
  vec3 n, c, t;
} Out;


void main()
{	
  Out.n = 2.f*n-1.f;
  Out.t = t;
  Out.c = c;
  gl_Position = vec4(p, 1.f);
}

