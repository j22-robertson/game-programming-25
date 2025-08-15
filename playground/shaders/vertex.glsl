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
layout (location = 0) out vec2 texcoord;




void main()
{
    gl_Position =camera.projection*camera.view * camera.model * vec4(a_position.xyz, 1.0);
    texcoord= a_tex;
}
