#version 460

layout (location = 0) in vec2 texcoord;
layout (location = 0) out vec4 FragColor;

layout(std140, set = 3, binding = 0) uniform UniformBlock   {
    float time;
};

layout(set=2, binding=0)uniform sampler2D albedo_texture;
layout(set=2, binding=1)uniform sampler2D normal_map;
layout(set=2, binding=2)uniform sampler2D roughness_texture;
layout(set=2, binding=3)uniform sampler2D metallic_texture;

void main()
{
    FragColor = texture(normal_map, texcoord);
  // FragColor = vec4(1.0,1.0,1.0,1.0);
}
