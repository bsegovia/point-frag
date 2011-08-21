#version 330 core
#define POSITION 0

uniform mat4 MVP;
uniform vec4 c;
layout(location = POSITION) in vec3 p;

out block {
  vec4 c;
} Out;

void main() {
  Out.c = c;
  gl_Position = MVP * vec4(p.x, -p.y, p.z, 1.f);
}

