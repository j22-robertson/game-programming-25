#version 460

layout (location = 0) out vec2 tex_coords;


const vec2 vertices[] ={
vec2(-1,1),
vec2(-1,-1),
vec2(1,-1),
vec2(1,1)
};

const int indices[]={
0,1,2,
0,2,3
};

const vec2 UVs[] ={
vec2(0.0, 1.0),
vec2(0.0, 0.0),
vec2(1.0, 0.0),
vec2(0.0, 1.0),
vec2(1.0, 1.0),
vec2(1.0, 1.0)
};


void main()
{

  //  vec2 out_uv = vec2((gl_VertexIndex<<1)&2, gl_VertexIndex&2);
    gl_Position= vec4(UVs[gl_VertexIndex]*2.0-1.0,0.0,1.0);


    //gl_Position = vec4(out_uv*2.0f + -1.0f, 0.0f, 1.0f);

    tex_coords = UVs[gl_VertexIndex];

}
