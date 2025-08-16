#version 460
layout(std140, set=1, binding=0) uniform UniformBufferObject
{
  mat4 model;
  mat4 view;
  mat4 projection;
} camera;


layout (location = 0) in vec3 a_position;
layout (location = 1) in vec3 a_normal;
layout (location = 2) in vec2 a_tex;
layout (location = 3) in vec4 model_matrix_x;
layout (location = 4) in vec4 model_matrix_y;
layout (location = 5) in vec4 model_matrix_z;
layout (location = 6) in vec4 model_matrix_w;
layout (location = 0) out vec2 texcoord;




void main()
{
    mat4 model_matrix = mat4(model_matrix_x, model_matrix_y, model_matrix_z, model_matrix_w);
    gl_Position =camera.projection*camera.view * model_matrix * vec4(a_position.xyz, 1.0);
    texcoord= a_tex;
}
