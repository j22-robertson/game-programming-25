#version 460



layout (location = 0) in vec3 a_position;

layout (location = 1) in vec2 a_tex;

layout (location = 1) out vec2 tex_coords;





void main()
{
    vec2 out_uv = vec2((gl_VertexIndex<<1)&2, gl_VertexIndex&2);

    gl_Position = vec4(out_uv*2.0f + -1.0f, 0.0f, 1.0f);

    tex_coords = out_uv;

}
