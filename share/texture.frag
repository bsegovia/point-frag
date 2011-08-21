#version 330 core
#define FRAG_COLOR  0

uniform sampler2D Diffuse;
in block {
vec2 t;
} In;

layout(location = FRAG_COLOR, index = 0) out vec4 c;
void main() {
  c = texture(Diffuse, In.t);
}

