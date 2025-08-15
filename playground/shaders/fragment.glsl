#version 460

layout (location = 0) in vec2 texcoord;
layout (location = 0) out vec4 FragColor;

layout(std140, set = 3, binding = 0) uniform UniformBlock   {
    float time;
};

layout(set=2, binding=0)uniform sampler2D test_texture;

void main()
{
    FragColor = texture(test_texture, texcoord);
  // FragColor = vec4(1.0,1.0,1.0,1.0);
}
