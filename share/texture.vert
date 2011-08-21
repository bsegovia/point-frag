#version 330 core
#define POSITION 0
#define TEXCOORD 1

uniform mat4 MVP;

layout(location = POSITION) in vec3 p;
layout(location = TEXCOORD) in vec2 t;

out block {
  vec2 t;
} Out;

void main() {	
  Out.t = t;
  gl_Position = MVP * vec4(p.x, -p.y, p.z, 1.f);
}

