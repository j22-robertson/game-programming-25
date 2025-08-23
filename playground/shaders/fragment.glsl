#version 460

layout (location = 0) in vec2 texcoord;
layout (location = 1) in mat3 btn_matrix;
layout (location = 4) in vec3 view_position;
layout (location = 5) in vec3 fragment_position;

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
    vec3 light_pos = vec3(-3.0,3.0,-3.0);


    vec3 normal = texture(normal_map, texcoord).rgb;
    normal = normal*2.0+1.0;
    normal = normalize(btn_matrix*normal);

    vec3 light_direction = btn_matrix*normalize(light_pos-fragment_position);
    vec3 view_direction = btn_matrix*normalize(view_position-fragment_position);
    vec3 half_way = normalize(light_direction + view_direction);

    float spec  = pow(max(dot(normal,half_way),0.0),32.0);
    vec3 specular = vec3(1.0,1.0,1.0) * spec;

    float ambient_strength = 0.5;
    vec3 ambient = ambient_strength * vec3(1.0,1.0,1.0);



    float diff = max(dot(normal,light_direction),0.0);
    vec3 diffuse = diff * vec3(1.0,1.0,1.0);


    vec3 temp = btn_matrix* vec3(1.0,1.0,1.0);
    vec4 albedo = texture(albedo_texture,texcoord);

    FragColor = vec4((ambient+diffuse+specular)*albedo.rgb,1.0);

  // FragColor = vec4(1.0,1.0,1.0,1.0);
}
