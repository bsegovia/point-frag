#version 330 core
#define POSITION	0
#define TEXCOORD	1
#define COLOR		3
#define FRAG_COLOR	0

uniform sampler2D Diffuse;

in block
{
	vec2 t;
} In;

layout(location = FRAG_COLOR, index = 0) out vec4 c;

void main()
{
  c = texture(Diffuse, In.t);
  c /= c.w;
  c.xyz = vec3(abs(dot(normalize(c.xyz), normalize(vec3(1,1,1)))));
  //c = vec4(In.t.x,In.t.y,0,0);
}


