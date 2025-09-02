#version 460
layout(std140, set=1, binding=0) uniform UniformBufferObject
{
    vec3 view_position;
    float padding;
    mat4 view;
    mat4 projection;
} camera;



layout (location = 0) in vec3 a_position;
layout (location = 1) in vec3 a_normal;
layout (location = 2) in vec3 a_tangent;
layout (location = 3) in vec3 a_bitangent;
layout (location = 4) in vec2 a_tex;
layout (location = 5) in vec4 model_matrix_x;
layout (location = 6) in vec4 model_matrix_y;
layout (location = 7) in vec4 model_matrix_z;
layout (location = 8) in vec4 model_matrix_w;

layout (location = 0) out vec3 fragcolor;





void main()
{
    mat4 model_matrix = mat4(model_matrix_x, model_matrix_y, model_matrix_z, model_matrix_w);
    vec3 tangent = normalize(vec3(model_matrix*vec4(a_tangent,0.0)));
    vec3 bitangent = normalize(vec3(model_matrix*vec4(a_bitangent,0.0)));
    vec3 normal = normalize(vec3(model_matrix*vec4(a_normal,0.0)));
    fragcolor = vec3(0.5,0.5,0.0);
    mat3 TBN = mat3(tangent,bitangent,normal);
    vec3 fragment_position = vec3(model_matrix*vec4(a_position,1.0));

    gl_Position =camera.projection*camera.view * model_matrix * vec4(fragment_position.xyz, 1.0);

}