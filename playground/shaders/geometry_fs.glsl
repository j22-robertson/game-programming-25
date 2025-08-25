#version 450
layout (location = 0) in vec2 texcoord;
layout (location = 1) in mat3 btn_matrix;
layout (location = 4) in vec3 view_position;
layout (location = 5) in vec3 fragment_position;

layout(binding =0) out vec4 frag_position;
layout(binding =1) out vec4 frag_albedo;
layout(binding =2) out vec4 frag_normal;
layout(binding =3) out vec4 frag_metallic_roughness;


layout(set=2, binding=0)uniform sampler2D albedo_texture;
layout(set=2, binding=1)uniform sampler2D normal_map;
layout(set=2, binding=2)uniform sampler2D roughness_texture;
layout(set=2, binding=3)uniform sampler2D metallic_texture;

void main() {

}