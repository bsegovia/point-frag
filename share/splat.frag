#version 330 core
#define POSITION	0
#define TEXCOORD  1
#define COLOR     2
#define NORMAL    3
#define FRAG_COLOR	0

in block
{
  vec3 n, c;
  vec2 t;
} In;

layout(location = FRAG_COLOR, index = 0) out vec4 Color;

uniform sampler2D Gaussian;

void main()
{
  Color.w = texture(Gaussian, In.t).x;
  if (Color.w == 0.f)
    discard;
  Color.xyz = In.n;
  //Color.xyz = vec3(abs(dot(In.n, normalize(vec3(1,1,1)))));
  Color.w = 1.f;
  Color.xyz *= Color.w;
}

