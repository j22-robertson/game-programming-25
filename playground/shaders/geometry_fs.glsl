#version 450
layout (location = 0) in vec2 texcoord;
layout (location = 1) in mat3 btn_matrix;
layout (location = 4) in vec3 fragment_position;

layout(location=0) out vec4 frag_albedo;
layout(location =1) out vec4 frag_normal;
layout(location=2) out vec4 frag_position;
layout(location =3) out vec4 frag_metallic_roughness;


layout(set=2, binding=0)uniform sampler2D albedo_texture;
layout(set=2, binding=1)uniform sampler2D normal_map;
layout(set=2, binding=2)uniform sampler2D roughness_texture;
layout(set=2, binding=3)uniform sampler2D metallic_texture;

void main() {
    vec3 normal = texture(normal_map, texcoord).rgb;
    normal = normal*2.0-1.0;
    normal = normalize(btn_matrix*normal);


    frag_position=vec4(fragment_position.xyz,1.0);
    frag_albedo = vec4(texture(albedo_texture, texcoord).rgb,1.0);
    frag_normal = vec4(normal.xyz,1.0);
    frag_metallic_roughness = vec4(texture(metallic_texture,texcoord).r,texture(roughness_texture,texcoord).g,texture(metallic_texture,texcoord).b,1.0);
}