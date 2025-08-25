#version 460
const int MAX_LIGHTS=100;

layout (location = 0) in vec2 texcoord;
layout (location = 1) in mat3 btn_matrix;
layout (location = 4) in vec3 view_position;
layout (location = 5) in vec3 fragment_position;

layout (location = 0) out vec4 FragColor;

layout(std140, set = 3, binding = 0) uniform UniformBlock   {
    float time;
};

struct Light {
    int type;
    vec3 position;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    vec3 direction;
    float constant;
    float linear;
    float quadratic;
    float cutoff;
    float outer_cutoff;
    vec3 padding;
};
layout(std140, set = 3, binding = 1) uniform NumLights   {
    int num_lights;
};

layout (std140, set=3, binding=2) uniform SceneLights{
    Light lights[MAX_LIGHTS];
};

layout(set=2, binding=0)uniform sampler2D albedo_texture;
layout(set=2, binding=1)uniform sampler2D normal_map;
layout(set=2, binding=2)uniform sampler2D roughness_texture;
layout(set=2, binding=3)uniform sampler2D metallic_texture;

vec3 CalculateDirectional(Light directional, vec3 normal, vec3 view,float shine)
{
    vec3 light_direction = normalize(-directional.direction);

    float diff=max(dot(normal,light_direction),0.0);

    vec3 reflection_direction = reflect(-light_direction, normal);

    float spec = pow(max(dot(view,reflection_direction),0.0),shine);

    vec3 ambient = directional.ambient * vec3(texture(albedo_texture,texcoord).rgb);
    vec3 diffuse = directional.diffuse * diff * vec3(texture(albedo_texture,texcoord).rgb);
    vec3 specular = directional.specular*  spec *vec3(texture(metallic_texture,texcoord).rgb);
    return (ambient+diffuse+specular);
}

vec3 CalculatePointLight(Light point_light,mat3 btn_matrix, vec3 normal, vec3 view,vec3 frag_position,float shine)
{
    vec3 direction_to_light =normalize(point_light.direction - frag_position);
    vec3 light_direction= normalize(point_light.position  - frag_position);

    float diff = max(dot(normal,light_direction),0.0);

    vec3 reflection_direction = reflect(-light_direction,normal);

    float spec = pow(dot(view,reflection_direction),shine);

    float distance = length(point_light.position - frag_position);

    float attenuation = 1.0/point_light.constant+point_light.linear*distance+point_light.quadratic*(distance*distance);

    vec3 ambient = point_light.ambient * vec3(texture(albedo_texture,texcoord).rgb);
    vec3 diffuse = point_light.diffuse * diff * vec3(texture(albedo_texture,texcoord).rgb);
    vec3 specular = point_light.specular *  spec * vec3(texture(metallic_texture,texcoord).r);

    ambient *= attenuation;
    diffuse *= attenuation;
    specular *= attenuation;

    return (ambient+diffuse+specular);
}

vec3 CalculateSpotlight(Light spotlight, vec3 direction_to_light, vec3 normal, vec3 view, float shine)
{
    float theta = dot(direction_to_light, normalize(-spotlight.direction));
    float epsilon = spotlight.cutoff - spotlight.outer_cutoff;
    float intensity = clamp((theta-spotlight.outer_cutoff)/epsilon,0.0,1.0);
    float diff = max(dot(normal,direction_to_light),0.0);

    vec3 reflection_direction = reflect(-direction_to_light,normal);
    float spec = pow(dot(view,reflection_direction),shine);

    vec3 diffuse = spotlight.diffuse * diff * vec3(texture(albedo_texture,texcoord).rgb);
    vec3 specular = spotlight.specular *  spec * vec3(texture(metallic_texture,texcoord).rgb);
    vec3 ambient = texture(albedo_texture, texcoord).rgb;

    if(theta>spotlight.cutoff)
    {
        diffuse *= intensity;
        specular *= intensity;
        return (diffuse+specular);
    }
    return ambient*0.1;
}

void main()
{
    vec3 normal = texture(normal_map, texcoord).rgb;
    normal = normal*2.0+1.0;
    normal = normalize(btn_matrix*normal);

    //vec3 light_direction = btn_matrix*normalize(lights[0].direction -fragment_position);
    vec3 view_direction = btn_matrix*normalize(view_position-fragment_position);

    vec3 light_output = vec3(0.0,0.0,0.0);

    for(int i = 0; i < num_lights; i++)
    {
        Light light = lights[i];
        vec3 light_direction =btn_matrix * normalize(light.position -fragment_position);
        light_direction= btn_matrix*normalize(view_position-fragment_position);
        vec3 half_way = normalize(light_direction + view_direction);
        switch(light.type){
            case 0:
                    light_output +=CalculateDirectional(light,normal,view_direction,32);
            case 1:
                    light_output +=CalculateSpotlight(light, light_direction, normal, view_direction,32);
        }
    }


    //FragColor = vec4((ambient+diffuse+specular)*albedo.rgb,1.0);


    FragColor = vec4(light_output,1.0);

   // FragColor = vec4(normalize(light_output),1.0);

  // FragColor = vec4(1.0,1.0,1.0,1.0);
}
