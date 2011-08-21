#version 330 core

#if 1
layout(points) in;
layout(triangle_strip, max_vertices = 4) out;
in block {
  vec3 n, c, t;
} In[1];

uniform mat4 MVP;
uniform float size;

out block {
  vec3 n, c;
  vec2 t;
} Out;

void main() {
  float tn = dot(vec3(MVP[0][3], MVP[1][3], MVP[2][3]), In[0].n);
  //if (tn < 0)
  {
    vec3 t = size * In[0].t - 0.5f*size;
    vec3 b = cross(In[0].n,t);
    vec4 p = MVP * gl_in[0].gl_Position;
    vec4 u = MVP * vec4(t,0.f);
    vec4 v = MVP * vec4(b,0.f);
    gl_Position = p - u - v;
    Out.n = In[0].n;
    Out.c = In[0].c;
    Out.t = vec2(0.f,0.f);
    EmitVertex();
    gl_Position = p + u - v;
    Out.n = In[0].n;
    Out.c = In[0].c;
    Out.t = vec2(0.f,1.f);
    EmitVertex();
    gl_Position = p - u + v;
    Out.n = In[0].n;
    Out.c = In[0].c;
    Out.t = vec2(1.f,0.f);
    EmitVertex();
    gl_Position = p + u + v;
    Out.n = In[0].n;
    Out.c = In[0].c;
    Out.t = vec2(1.f,1.f);
    EmitVertex();
    EndPrimitive();
  }
}
#else
layout(points) in;
//layout(triangle_strip, max_vertices = 3) out;
layout(points, max_vertices = 1) out;
in block {
  vec3 n, c, t;
} In[1];

uniform mat4 MVP;
uniform float size;

out block {
  vec3 n, c;
} Out;

void main() {
  vec4 tn = MVP * vec4(In[0].n, 0.f);
  if (tn.w < 0) {
  vec3 b = cross(In[0].n,In[0].t);
  gl_Position = MVP * gl_in[0].gl_Position;
//  vec4 d = MVP * vec4(size,size,size,0.f);
  gl_PointSize = 2.f;//min(5.f, length(d.xyz / d.w));
  Out.n = In[0].n;
  Out.c = In[0].c;
  EmitVertex();
#if 0
  gl_Position = MVP * (gl_in[0].gl_Position + vec4(In[0].t*size,0.f));
  Out.n = In[0].n;
  Out.c = In[0].c;
  EmitVertex();
  gl_Position = MVP * (gl_in[0].gl_Position + vec4(b*size,0.f));
  Out.n = In[0].n;
  Out.c = In[0].c;
  EmitVertex();
  EndPrimitive();
#endif
  }
}

#endif

