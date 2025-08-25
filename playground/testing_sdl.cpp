
//
// Created by James Robertson on 05/08/2025.
#define SDL_MAIN_USE_CALLBACKS
#include <filesystem>
#include <memory>
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>
#include <SDL3/SDL_main.h>
#include <SDL3/SDL.h>
#include <iostream>

#include "code/AssetLib/glTF2/glTF2Exporter.h"
#include "Rendering/RenderUtils.h"



#include "Components/CoreComponents.h"


// TODO: Add light uniform buffers, light types and a new pipeline for debugging lights
struct TestUBO{
    glm::vec4 color;
};


struct TimeStep {
    float time;
};

struct CameraUniform {
    glm::vec3 position;
    float padding = 0.0;
    glm::mat4 view;
    glm::mat4 projection;
};

struct Camera {
    glm::vec3 velocity;
    glm::vec3 position;

    float lx = 0.0;
    float ly = 0.0;

    float pitch {0.0f};
    float yaw {0.f};
    glm::mat4 Rotation_Matrix() const
    {

        glm::quat pitchRotation = glm::angleAxis(pitch, glm::vec3{ 1.f,0.0f,0.0f });

        glm::quat yawRotation = glm::angleAxis(yaw, glm::vec3{ 0.f,-1.f,0.f });

        return glm::toMat4(yawRotation) * glm::toMat4(pitchRotation);
    }

    glm::mat4 View_Matrix() const
    {
        const glm::mat4 cameraTranslation = glm::translate(glm::mat4(1.0f), position);
        const glm::mat4 cameraRotation = Rotation_Matrix();
        return glm::inverse(cameraTranslation * cameraRotation);
    }
    void Update_View(float dt)
    {
        const glm::mat4 rot = Rotation_Matrix();

        position += glm::vec3(rot * glm::vec4(velocity *  0.005f, 0.0f));
    }

};


/***
 *TODO: Deferred rendering
 *TODO: PBR lighting
 */



std::uint32_t GBUFFER_POSITION;
std::uint32_t GBUFFER_ALBEDO;
std::uint32_t GBUFFER_NORMAL;
std::uint32_t GBUFFER_METALLICROUGHNESS;

SDL_Window* window = nullptr;
SDL_GPUDevice* device = nullptr;

std::vector<SDL_GPUBuffer*> vertex_buffer_storage;
std::vector<SDL_GPUBuffer*> index_buffer_storage;
std::vector<std::uint32_t> indices_length_storage;


SDL_GPUBuffer* vertex_buffer;
SDL_GPUBuffer* index_buffer;
SDL_GPUBuffer* instance_buffer;
SDL_GPUTransferBuffer* instance_transfer_buffer;

SDL_GPUTexture* texture;
SDL_GPUSampler* sampler;

SDL_GPUShader* fragment_shader = nullptr;


SDL_GPUTexture* depth_texture;

std::map<std::string, Model> models;
std::queue<Model> unloaded_models;

std::vector<Transform> transforms;

GraphicsResources graphics_resources;


SDL_GPUShader* grid_fragment_shader = nullptr;

SDL_GPUShader* grid_vertex_shader = nullptr;

SDL_GPUShader* geometry_fragment_shader=nullptr;
SDL_GPUShader* geometry_vertex_shader=nullptr;

SDL_GPUGraphicsPipeline* graphics_pipeline = nullptr;
SDL_GPUGraphicsPipeline* grid_pipeline = nullptr;

static TestUBO testUBO = {glm::vec4(1.0,0.0,0.0,1.0)};

static TimeStep sim_time{};

static Camera camera;
//static CameraUniform camera_uniform;
int mesh_count = 0;

std::vector<Light> scene_lights;

void Load_Texture2DFromFile(SDL_GPUDevice* device, const std::string& filename) {

    int width, height, channels;
    unsigned char* data = stbi_load(filename.c_str(), &width, &height, &channels, 4);
    if (data == nullptr) {
        SDL_Log(("Unable to load texture: "+filename + "\n").c_str());
        return;
    }

    SDL_GPUTextureCreateInfo texture_info{};

    texture_info.width = width;
    texture_info.height = height;
    texture_info.layer_count_or_depth = 1;
    texture_info.type = SDL_GPU_TEXTURETYPE_2D;
    texture_info.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER;
    texture_info.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM_SRGB;
    texture_info.num_levels = 1;

   // auto* texture = SDL_CreateGPUTexture(device, &texture_info);
    texture = SDL_CreateGPUTexture(device, &texture_info);

    SDL_GPUTransferBufferCreateInfo buffer_info{};
    buffer_info.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    buffer_info.size = (std::uint32_t)(4*width*height);
    buffer_info.props = 0;


    SDL_GPUCommandBuffer* command_buffer = SDL_AcquireGPUCommandBuffer(device);

    SDL_GPUCopyPass* image_copy_pass = SDL_BeginGPUCopyPass(command_buffer);


    auto* transfer_buffer = SDL_CreateGPUTransferBuffer(device, &buffer_info);




    SDL_GPUTextureTransferInfo transfer_info{};
    transfer_info.transfer_buffer = transfer_buffer;
    transfer_info.pixels_per_row = width;
    transfer_info.offset = 0;
    transfer_info.rows_per_layer = 1;


    SDL_GPUTextureRegion texture_region{};
    texture_region.texture = texture;
    texture_region.d = 0;
    texture_region.h = height;
    texture_region.w = width;
    texture_region.layer = 0;
    texture_region.mip_level = 0;
    texture_region.x=0;
    texture_region.y=0;
    texture_region.z=0;

    void* mapped_data = SDL_MapGPUTransferBuffer(device, transfer_buffer,false);

    SDL_memcpy(mapped_data, data, 4*width*height);

    SDL_UnmapGPUTransferBuffer(device, transfer_buffer);


    SDL_UploadToGPUTexture(image_copy_pass,&transfer_info,&texture_region, true);

  //  SDL_UploadToGPUBuffer(ib_copy_pass, &transfer_location, &buffer_region, true);
    SDL_EndGPUCopyPass(image_copy_pass);

    SDL_SubmitGPUCommandBuffer(command_buffer);



}



std::uint32_t Load_GBufferTexture(SDL_GPUDevice* device, std::string& name) {

    int width, height, channels;

    width = 960;
    height = 540;

    SDL_GPUTextureCreateInfo texture_info{};
    std::vector<glm::vec4> texture_data;
    auto texture_size = width*height;
    for (int i = 0; i < texture_size; i++) {
        texture_data.push_back(glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));
    }


    texture_info.width = width;
    texture_info.height = height;
    texture_info.layer_count_or_depth = 1;
    texture_info.type = SDL_GPU_TEXTURETYPE_2D;
    texture_info.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER;
    texture_info.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM_SRGB;
    texture_info.num_levels = 1;

   // auto* texture = SDL_CreateGPUTexture(device, &texture_info);
    texture = SDL_CreateGPUTexture(device, &texture_info);

    SDL_GPUTransferBufferCreateInfo buffer_info{};
    buffer_info.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    buffer_info.size = (std::uint32_t)(4*width*height);
    buffer_info.props = 0;


    SDL_GPUCommandBuffer* command_buffer = SDL_AcquireGPUCommandBuffer(device);

    SDL_GPUCopyPass* image_copy_pass = SDL_BeginGPUCopyPass(command_buffer);


    auto* transfer_buffer = SDL_CreateGPUTransferBuffer(device, &buffer_info);




    SDL_GPUTextureTransferInfo transfer_info{};
    transfer_info.transfer_buffer = transfer_buffer;
    transfer_info.pixels_per_row = width;
    transfer_info.offset = 0;
    transfer_info.rows_per_layer = 1;


    SDL_GPUTextureRegion texture_region{};
    texture_region.texture = texture;
    texture_region.d = 0;
    texture_region.h = height;
    texture_region.w = width;
    texture_region.layer = 0;
    texture_region.mip_level = 0;
    texture_region.x=0;
    texture_region.y=0;
    texture_region.z=0;

    void* mapped_data = SDL_MapGPUTransferBuffer(device, transfer_buffer,false);

    SDL_memcpy(mapped_data, texture_data.data(), 4*width*height);

    SDL_UnmapGPUTransferBuffer(device, transfer_buffer);


    SDL_UploadToGPUTexture(image_copy_pass,&transfer_info,&texture_region, true);

  //  SDL_UploadToGPUBuffer(ib_copy_pass, &transfer_location, &buffer_region, true);
    SDL_EndGPUCopyPass(image_copy_pass);

    SDL_SubmitGPUCommandBuffer(command_buffer);

    return graphics_resources.texture_map.Insert_Resource(std::unique_ptr<GPUTexture, ResourceDeleter<GPUTexture>>(new GPUTexture(texture),ResourceDeleter<GPUTexture>(device)),name);
}

void Initialize_GBuffer(SDL_GPUDevice* device) {
    std::string gbuffer_position = "GBUFFER_POSITION";
    std::string gbuffer_normal = "GBUFFER_NORMAL";
    std::string gbuffer_albedo = "GBUFFER_ALBEDO";
    std::string gbuffer_metallic_roughness = "GBUFFER_METALLICROUGHNESS";

    GBUFFER_POSITION =Load_GBufferTexture(device,gbuffer_position);
    GBUFFER_ALBEDO =Load_GBufferTexture(device,gbuffer_albedo);
    GBUFFER_NORMAL =Load_GBufferTexture(device,gbuffer_normal);
    GBUFFER_METALLICROUGHNESS =Load_GBufferTexture(device,gbuffer_metallic_roughness);
}

void Load_GeometryPipeline(SDL_GPUDevice* device) {

    size_t vertex_shader_size = 0;
    void* vertex_code = SDL_LoadFile("../playground/shaders/geometry_vertex.spv", &vertex_shader_size);

    SDL_GPUShaderCreateInfo vertex_shader_info{};
    vertex_shader_info.code = (Uint8*)vertex_code; //convert to an array of bytes
    vertex_shader_info.code_size = vertex_shader_size;
    vertex_shader_info.entrypoint = "main";
    vertex_shader_info.format = SDL_GPU_SHADERFORMAT_SPIRV; // loading .spv shaders
    vertex_shader_info.stage = SDL_GPU_SHADERSTAGE_VERTEX; // vertex shader
    vertex_shader_info.num_samplers = 0;
    vertex_shader_info.num_storage_buffers = 0;
    vertex_shader_info.num_storage_textures = 0;
    vertex_shader_info.num_uniform_buffers = 1;

    geometry_vertex_shader = SDL_CreateGPUShader(device, &vertex_shader_info);

    SDL_free(vertex_code);


    size_t fragment_shader_size = 0;
    void* fragment_code = SDL_LoadFile("../playground/shaders/geometry_fragment.spv",&fragment_shader_size);

    SDL_GPUShaderCreateInfo fragment_shader_info{};
    fragment_shader_info.code = (Uint8*)fragment_code;
    fragment_shader_info.code_size = fragment_shader_size;
    fragment_shader_info.entrypoint = "main";
    fragment_shader_info.format = SDL_GPU_SHADERFORMAT_SPIRV;
    fragment_shader_info.stage = SDL_GPU_SHADERSTAGE_FRAGMENT;
    fragment_shader_info.num_samplers = 0;
    fragment_shader_info.num_storage_buffers =  0;
    fragment_shader_info.num_storage_textures = 0;
    fragment_shader_info.num_uniform_buffers = 0;
    geometry_fragment_shader = SDL_CreateGPUShader(device, &fragment_shader_info);
    SDL_free(fragment_code);


    SDL_GPUGraphicsPipelineCreateInfo geometry_pipeline_info{};
    geometry_pipeline_info.vertex_shader = geometry_vertex_shader;
    geometry_pipeline_info.fragment_shader = geometry_fragment_shader;

   /*
    grid_pipeline_info.vertex_shader = grid_vertex_shader;
    grid_pipeline_info.fragment_shader = grid_fragment_shader;

    grid_pipeline_info.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;

    grid_pipeline_info.vertex_input_state.num_vertex_buffers = 0;
    grid_pipeline_info.vertex_input_state.vertex_buffer_descriptions = nullptr;

    grid_pipeline_info.vertex_input_state.num_vertex_attributes=0;
    grid_pipeline_info.vertex_input_state.vertex_attributes = nullptr;

    grid_pipeline_info.rasterizer_state.cull_mode = SDL_GPU_CULLMODE_NONE;
    grid_pipeline_info.rasterizer_state.front_face = SDL_GPU_FRONTFACE_COUNTER_CLOCKWISE;

    grid_pipeline_info.depth_stencil_state.enable_depth_test = false;
    grid_pipeline_info.depth_stencil_state.enable_depth_write = false;
    grid_pipeline_info.target_info.has_depth_stencil_target = false;

   grid_pipeline_info.target_info.depth_stencil_format = SDL_GPU_TEXTUREFORMAT_D24_UNORM_S8_UINT;



    SDL_GPUColorTargetDescription color_target_descriptions[1];

    color_target_descriptions[0] = {};
    color_target_descriptions[0].format = SDL_GetGPUSwapchainTextureFormat(device, window);

    color_target_descriptions[0].blend_state.enable_blend = true;
    color_target_descriptions[0].blend_state.color_blend_op = SDL_GPU_BLENDOP_ADD;
    color_target_descriptions[0].blend_state.dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ZERO;
    color_target_descriptions[0].blend_state.src_color_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA;
    color_target_descriptions[0].blend_state.alpha_blend_op = SDL_GPU_BLENDOP_ADD;
    color_target_descriptions[0].blend_state.src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ZERO;
    color_target_descriptions[0].blend_state.dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA;



    grid_pipeline_info.target_info.num_color_targets = 1;
    grid_pipeline_info.target_info.color_target_descriptions = color_target_descriptions;

    grid_pipeline = SDL_CreateGPUGraphicsPipeline(device,&grid_pipeline_info);
    if (grid_pipeline == nullptr) {
        SDL_Log("Grid Pipeline has failed");
    } */
}


void Update_Camera(CameraUniform& camera_uniform) {


    static auto startTime = std::chrono::high_resolution_clock::now();

    auto currentTime = std::chrono::high_resolution_clock::now();
    float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

    camera.Update_View(time);

    camera_uniform.view = camera.View_Matrix();

    Transform transform{};
    transform.Rotate(glm::vec3(0.0f, 0.0f, 1.0f),time*90);
    transform.Get_Scale() = glm::vec3(1.0f, 1.0f, 1.0f);
    transform.Get_Position() = glm::vec3(time*1.0,1.0,1.0);

    camera_uniform.position = camera.position;
    camera_uniform.projection = glm::perspective(glm::radians(70.0f), (float)960 / (float)540, 0.1f, 1000.0f);

    scene_lights[1].position = camera.position;
    camera_uniform.projection[1][1] *= -1;
}



void Load_GridShaders(SDL_GPUDevice* device) {

    size_t vertex_shader_size = 0;
    void* vertex_code = SDL_LoadFile("../playground/shaders/grid_vertex.spv", &vertex_shader_size);

    SDL_GPUShaderCreateInfo vertex_shader_info{};
    vertex_shader_info.code = (Uint8*)vertex_code; //convert to an array of bytes
    vertex_shader_info.code_size = vertex_shader_size;
    vertex_shader_info.entrypoint = "main";
    vertex_shader_info.format = SDL_GPU_SHADERFORMAT_SPIRV; // loading .spv shaders
    vertex_shader_info.stage = SDL_GPU_SHADERSTAGE_VERTEX; // vertex shader
    vertex_shader_info.num_samplers = 0;
    vertex_shader_info.num_storage_buffers = 0;
    vertex_shader_info.num_storage_textures = 0;
    vertex_shader_info.num_uniform_buffers = 1;

    grid_vertex_shader = SDL_CreateGPUShader(device, &vertex_shader_info);

    SDL_free(vertex_code);


    size_t fragment_shader_size = 0;
    void* fragment_code = SDL_LoadFile("../playground/shaders/grid_fragment.spv",&fragment_shader_size);

    SDL_GPUShaderCreateInfo fragment_shader_info{};
    fragment_shader_info.code = (Uint8*)fragment_code;
    fragment_shader_info.code_size = fragment_shader_size;
    fragment_shader_info.entrypoint = "main";
    fragment_shader_info.format = SDL_GPU_SHADERFORMAT_SPIRV;
    fragment_shader_info.stage = SDL_GPU_SHADERSTAGE_FRAGMENT;
    fragment_shader_info.num_samplers = 0;
    fragment_shader_info.num_storage_buffers =  0;
    fragment_shader_info.num_storage_textures = 0;
    fragment_shader_info.num_uniform_buffers = 0;
   grid_fragment_shader = SDL_CreateGPUShader(device, &fragment_shader_info);
    SDL_free(fragment_code);


}


void Load_GridPipeline(SDL_GPUDevice* device) {

    Load_GridShaders(device);

    SDL_GPUGraphicsPipelineCreateInfo grid_pipeline_info{};

    grid_pipeline_info.vertex_shader = grid_vertex_shader;
    grid_pipeline_info.fragment_shader = grid_fragment_shader;

    grid_pipeline_info.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;

    grid_pipeline_info.vertex_input_state.num_vertex_buffers = 0;
    grid_pipeline_info.vertex_input_state.vertex_buffer_descriptions = nullptr;

    grid_pipeline_info.vertex_input_state.num_vertex_attributes=0;
    grid_pipeline_info.vertex_input_state.vertex_attributes = nullptr;

    grid_pipeline_info.rasterizer_state.cull_mode = SDL_GPU_CULLMODE_NONE;
    grid_pipeline_info.rasterizer_state.front_face = SDL_GPU_FRONTFACE_COUNTER_CLOCKWISE;

    grid_pipeline_info.depth_stencil_state.enable_depth_test = false;
    grid_pipeline_info.depth_stencil_state.enable_depth_write = false;
    grid_pipeline_info.target_info.has_depth_stencil_target = false;

   grid_pipeline_info.target_info.depth_stencil_format = SDL_GPU_TEXTUREFORMAT_D24_UNORM_S8_UINT;



    SDL_GPUColorTargetDescription color_target_descriptions[1];

    color_target_descriptions[0] = {};
    color_target_descriptions[0].format = SDL_GetGPUSwapchainTextureFormat(device, window);

    color_target_descriptions[0].blend_state.enable_blend = true;
    color_target_descriptions[0].blend_state.color_blend_op = SDL_GPU_BLENDOP_ADD;
    color_target_descriptions[0].blend_state.dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ZERO;
    color_target_descriptions[0].blend_state.src_color_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA;
    color_target_descriptions[0].blend_state.alpha_blend_op = SDL_GPU_BLENDOP_ADD;
    color_target_descriptions[0].blend_state.src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ZERO;
    color_target_descriptions[0].blend_state.dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA;



    grid_pipeline_info.target_info.num_color_targets = 1;
    grid_pipeline_info.target_info.color_target_descriptions = color_target_descriptions;

    grid_pipeline = SDL_CreateGPUGraphicsPipeline(device,&grid_pipeline_info);
    if (grid_pipeline == nullptr) {
        SDL_Log("Grid Pipeline has failed");
    }


}

void Create_InstanceBuffer(SDL_GPUDevice* device) {
    SDL_GPUBufferCreateInfo buffer_create_info{};

    const std::uint32_t instance_buffer_size = sizeof(glm::mat4)*transforms.size();

    const auto command_buffer = SDL_AcquireGPUCommandBuffer(device);

    buffer_create_info.size = instance_buffer_size;
    buffer_create_info.usage = SDL_GPU_BUFFERUSAGE_VERTEX;
    instance_buffer = SDL_CreateGPUBuffer(device, &buffer_create_info);

    SDL_GPUTransferBufferCreateInfo transfer_buffer_info{};
    transfer_buffer_info.size = instance_buffer_size;
    transfer_buffer_info.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    instance_transfer_buffer = SDL_CreateGPUTransferBuffer(device, &transfer_buffer_info);



    glm::mat4* data = (glm::mat4*)SDL_MapGPUTransferBuffer(device, instance_transfer_buffer, true);


    SDL_GPUCopyPass* copy_pass = SDL_BeginGPUCopyPass(command_buffer);
    auto model_matrices = std::vector<glm::mat4>();

    for (auto& transform : transforms) {
        model_matrices.emplace_back(transform.Get_TransformMatrix());
    }

    SDL_memcpy(data,model_matrices.data(),instance_buffer_size);
    SDL_UnmapGPUTransferBuffer(device, instance_transfer_buffer);
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////



    SDL_GPUTransferBufferLocation transfer_location{};
    transfer_location.transfer_buffer = instance_transfer_buffer;
    transfer_location.offset = 0;

    SDL_GPUBufferRegion buffer_region{};
    buffer_region.buffer = instance_buffer;
    buffer_region.size = instance_buffer_size;
    buffer_region.offset = 0;

    SDL_UploadToGPUBuffer(copy_pass, &transfer_location, &buffer_region, true);

    SDL_EndGPUCopyPass(copy_pass);

    SDL_SubmitGPUCommandBuffer(command_buffer);

  //  SDL_ReleaseGPUTransferBuffer(device,transfer_buffer);
}

void Update_InstanceBuffer() {
    //TODO: Change this function to update fixed parts of the instance buffer so that different instances of different models can be rendered with the same binding?
    //TODO: Make this function work with all transforms that can be rendered within a scene


    const std::uint32_t instance_buffer_size = sizeof(glm::mat4)*transforms.size();

    const auto command_buffer = SDL_AcquireGPUCommandBuffer(device);


    glm::mat4* data = (glm::mat4*)SDL_MapGPUTransferBuffer(device, instance_transfer_buffer, true);


    SDL_GPUCopyPass* copy_pass = SDL_BeginGPUCopyPass(command_buffer);
    auto model_matrices = std::vector<glm::mat4>();

    for (auto transform : transforms) {
        model_matrices.emplace_back(transform.Get_TransformMatrix());
    }

    SDL_memcpy(data,model_matrices.data(),instance_buffer_size);
    SDL_UnmapGPUTransferBuffer(device, instance_transfer_buffer);
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////



    SDL_GPUTransferBufferLocation transfer_location{};
    transfer_location.transfer_buffer = instance_transfer_buffer;
    transfer_location.offset = 0;

    SDL_GPUBufferRegion buffer_region{};
    buffer_region.buffer = instance_buffer;
    buffer_region.size = instance_buffer_size;
    buffer_region.offset = 0;

    SDL_UploadToGPUBuffer(copy_pass, &transfer_location, &buffer_region, true);

    SDL_EndGPUCopyPass(copy_pass);

    SDL_SubmitGPUCommandBuffer(command_buffer);

}

SDL_AppResult SDL_AppInit(void **appstate, int argc, char **argv)
{
    // create a window
    window = SDL_CreateWindow("Goodbye, Triangle!", 960, 540, SDL_WINDOW_RESIZABLE);

    device = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV, false, NULL);

    SDL_ClaimWindowForGPUDevice(device, window);


    //Mesh_UploadToGPU(device, square_vertices, indices);


    Light directional_light = {};
    directional_light.type = 0;
    directional_light.position = glm::vec3(3.0,9.0,3.0);
    directional_light.ambient = glm::vec3(1.0,1.0,1.0);
    directional_light.specular = glm::vec3(1.0,1.0,1.0);
    directional_light.diffuse = glm::vec3(1.0,1.0,1.0);
    directional_light.direction = glm::vec3(3.0,40.0,3.0);
    scene_lights.push_back(directional_light);

    Light spotlight = {};
    spotlight.type =1;
    spotlight.position = glm::vec3(0.0,0.0,1.0);
    spotlight.ambient = glm::vec3(1.0,1.0,1.0);
    spotlight.specular = glm::vec3(1.0,1.0,1.0);
    spotlight.diffuse = glm::vec3(1.0,1.0,1.0);
    spotlight.direction = glm::vec3(0.1,0.1,1.0);
    spotlight.cutoff = glm::cos(glm::radians(12.5f));
    spotlight.outer_cutoff = glm::cos(glm::radians(17.0f));
    scene_lights.push_back(spotlight);



    SDL_GPUTextureCreateInfo depth_texture_info = {};
    depth_texture_info.width = 960.0f;
    depth_texture_info.height = 540.0f;
    depth_texture_info.layer_count_or_depth = 1;
   depth_texture_info.num_levels = 1;
    depth_texture_info.format = SDL_GPU_TEXTUREFORMAT_D24_UNORM_S8_UINT;
   depth_texture_info.usage = SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET;
    depth_texture = SDL_CreateGPUTexture(device, &depth_texture_info);

   // Mesh_UploadToGPU(device, cube_vertices,cube_indices);

    SDL_GPUCommandBuffer* command_buffer = SDL_AcquireGPUCommandBuffer(device);
    std::map<std::string, std::unique_ptr<Texture2D>> texture_map;
    Model model = Load_ModelDataFromFile(device,
        command_buffer ,
        "../playground/Models/ornate_mirror/scene.gltf",
        graphics_resources);
    SDL_SubmitGPUCommandBuffer(command_buffer);

    models.insert(std::pair("CUBE",model));
/*
    while (!unloaded_models.empty()) {
        auto unloaded_model = unloaded_models.front();
        for (auto& mesh : unloaded_model.mesh_storage) {
           // Mesh_UploadToGPU(device,command_buffer,graphics_resources,mesh);
        }
        unloaded_models.pop();
        models.insert(std::pair("CUBE",unloaded_model));
    }*/



    //Load_Texture2DFromFile(device,"../playground/Textures/red-clay-wall-albedo.png" );

    Load_Texture2DFromFile(device,"../playground/Models/ornate_mirror/textures/Mirror_material_baseColor.jpeg" );
    size_t vertex_shader_size = 0;
    void* vertex_code = SDL_LoadFile("../playground/shaders/vertex.spv", &vertex_shader_size);

    SDL_GPUShaderCreateInfo vertex_shader_info{};
    vertex_shader_info.code = (Uint8*)vertex_code; //convert to an array of bytes
    vertex_shader_info.code_size = vertex_shader_size;
    vertex_shader_info.entrypoint = "main";
    vertex_shader_info.format = SDL_GPU_SHADERFORMAT_SPIRV; // loading .spv shaders
    vertex_shader_info.stage = SDL_GPU_SHADERSTAGE_VERTEX; // vertex shader
    vertex_shader_info.num_samplers = 0;
    vertex_shader_info.num_storage_buffers = 0;
    vertex_shader_info.num_storage_textures = 0;
    vertex_shader_info.num_uniform_buffers = 1;
    SDL_GPUShader* vertex_shader = SDL_CreateGPUShader(device, &vertex_shader_info);

    SDL_free(vertex_code);

    size_t fragment_shader_size = 0;
    void* fragment_code = SDL_LoadFile("../playground/shaders/fragment.spv",&fragment_shader_size);

    SDL_GPUSamplerCreateInfo  sampler_create_info{};

    sampler_create_info.min_filter = SDL_GPU_FILTER_LINEAR;
    sampler_create_info.mag_filter = SDL_GPU_FILTER_LINEAR;
    sampler_create_info.mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_LINEAR;
    sampler_create_info.address_mode_u= SDL_GPU_SAMPLERADDRESSMODE_REPEAT;
    sampler_create_info.address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_REPEAT;
    sampler_create_info.address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_REPEAT;
    sampler_create_info.enable_anisotropy = false;
    sampler_create_info.max_anisotropy = 0.0f;
    sampler_create_info.enable_compare = false;
    sampler_create_info.compare_op = SDL_GPU_COMPAREOP_NEVER;
    sampler_create_info.min_lod = -1000.0f;
    sampler_create_info.max_lod = 1000.0f;

    sampler = SDL_CreateGPUSampler(device,&sampler_create_info);

    SDL_GPUShaderCreateInfo fragment_shader_info{};
    fragment_shader_info.code = (Uint8*)fragment_code;
    fragment_shader_info.code_size = fragment_shader_size;
    fragment_shader_info.entrypoint = "main";
    fragment_shader_info.format = SDL_GPU_SHADERFORMAT_SPIRV;
    fragment_shader_info.stage = SDL_GPU_SHADERSTAGE_FRAGMENT;
    fragment_shader_info.num_samplers = 4;
    fragment_shader_info.num_storage_buffers = 0;
    fragment_shader_info.num_storage_textures = 0;
    fragment_shader_info.num_uniform_buffers = 3;
    fragment_shader = SDL_CreateGPUShader(device, &fragment_shader_info);

    SDL_free(fragment_code);

    Load_GridPipeline(device);

    SDL_GPUGraphicsPipelineCreateInfo graphics_pipeline_info{};

    graphics_pipeline_info.vertex_shader = vertex_shader;
    graphics_pipeline_info.fragment_shader = fragment_shader;

    graphics_pipeline_info.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;

    SDL_GPUVertexBufferDescription vertex_buffer_descriptions[2];

    vertex_buffer_descriptions[0].slot = 0;
    vertex_buffer_descriptions[0].input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX;
    vertex_buffer_descriptions[0].instance_step_rate = 0;
    vertex_buffer_descriptions[0].pitch = sizeof(Vertex);

    vertex_buffer_descriptions[1].slot = 1;
    vertex_buffer_descriptions[1].input_rate = SDL_GPU_VERTEXINPUTRATE_INSTANCE;
    vertex_buffer_descriptions[1].pitch = sizeof(glm::mat4x4);
    vertex_buffer_descriptions[1].instance_step_rate= 1;

    graphics_pipeline_info.vertex_input_state.num_vertex_buffers = 2;
    graphics_pipeline_info.vertex_input_state.vertex_buffer_descriptions = vertex_buffer_descriptions;

    SDL_GPUVertexAttribute vertex_attributes[9];

    vertex_attributes[0].buffer_slot=0;
    vertex_attributes[0].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3;
    vertex_attributes[0].location = 0;
    vertex_attributes[0].offset = 0;

    vertex_attributes[1].buffer_slot=0;
    vertex_attributes[1].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3;
    vertex_attributes[1].location = 1;
    vertex_attributes[1].offset = sizeof(float)*3;

    vertex_attributes[2].buffer_slot=0;
    vertex_attributes[2].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2;
    vertex_attributes[2].location = 2;
    vertex_attributes[2].offset = sizeof(float)*6;

    vertex_attributes[3].buffer_slot=0;
    vertex_attributes[3].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3;
    vertex_attributes[3].location = 3;
    vertex_attributes[3].offset = sizeof(float)*8;

    vertex_attributes[4].buffer_slot=0;
    vertex_attributes[4].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3;
    vertex_attributes[4].location = 4;
    vertex_attributes[4].offset = sizeof(float)*11;

    vertex_attributes[5].buffer_slot = 1;
    vertex_attributes[5].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4;
    vertex_attributes[5].location = 5;
    vertex_attributes[5].offset = 0;

    vertex_attributes[6].buffer_slot = 1;
    vertex_attributes[6].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4;
    vertex_attributes[6].location = 6;
    vertex_attributes[6].offset =sizeof(float)*4;

    vertex_attributes[7].buffer_slot = 1;
    vertex_attributes[7].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4;
    vertex_attributes[7].location = 7;
    vertex_attributes[7].offset = sizeof(float)*8;

    vertex_attributes[8].buffer_slot = 1;
    vertex_attributes[8].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4;
    vertex_attributes[8].location = 8;
    vertex_attributes[8].offset = sizeof(float)*12;

    graphics_pipeline_info.vertex_input_state.num_vertex_attributes=9;
    graphics_pipeline_info.vertex_input_state.vertex_attributes = vertex_attributes;

    graphics_pipeline_info.rasterizer_state.cull_mode = SDL_GPU_CULLMODE_BACK;
    graphics_pipeline_info.rasterizer_state.front_face = SDL_GPU_FRONTFACE_CLOCKWISE;

    SDL_GPUColorTargetDescription color_target_descriptions[1];

    color_target_descriptions[0] = {};
    color_target_descriptions[0].format = SDL_GetGPUSwapchainTextureFormat(device, window);

    graphics_pipeline_info.target_info.num_color_targets = 1;

    graphics_pipeline_info.target_info.color_target_descriptions = color_target_descriptions;

    graphics_pipeline_info.depth_stencil_state.enable_depth_test = true;
    graphics_pipeline_info.depth_stencil_state.enable_depth_write = true;
    graphics_pipeline_info.depth_stencil_state.compare_op = SDL_GPU_COMPAREOP_LESS;
    graphics_pipeline_info.target_info.has_depth_stencil_target = true;

    graphics_pipeline_info.target_info.depth_stencil_format = SDL_GPU_TEXTUREFORMAT_D24_UNORM_S8_UINT;



    graphics_pipeline = SDL_CreateGPUGraphicsPipeline(device,&graphics_pipeline_info);

    Transform transform{};
    transform.Get_Scale()= glm::vec3(1.0,1.0,1.0);
    transform.Get_Position() = glm::vec3(0.0,0.0,0.0);

    Transform test_transform{};
    test_transform.Get_Scale()= glm::vec3(2.0,2.0,2.0);
    test_transform.Get_Position()= glm::vec3(6.0,0.0,6.0);
    transforms.push_back(transform);
    transforms.push_back(test_transform);
    Create_InstanceBuffer(device);

    SDL_ReleaseGPUShader(device, vertex_shader);
    SDL_ReleaseGPUShader(device, fragment_shader);

   // Update_Camera();

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void *appstate)
{
    SDL_GPUCommandBuffer* command_buffer = SDL_AcquireGPUCommandBuffer(device);

    SDL_GPUTexture* swapchain_texture = nullptr;

    uint32_t width = 960;
    uint32_t height = 540;
    SDL_WaitAndAcquireGPUSwapchainTexture(command_buffer, window, &swapchain_texture, &width, &height);
    SDL_Rect scissor;
    scissor.x = 0;
    scissor.y = 0;
    scissor.w = 960;
    scissor.h = 540;

    SDL_GPUViewport viewport {};
    viewport.min_depth = 0;
    viewport.max_depth = 1;
    viewport.x = 0;
    viewport.y = 0;
    viewport.w = 960;
    viewport.h = 540;




    SDL_GPUDepthStencilTargetInfo depth_stencil_target_info{};
    depth_stencil_target_info.texture = depth_texture;
    depth_stencil_target_info.clear_depth = 1.0f;
    depth_stencil_target_info.load_op = SDL_GPU_LOADOP_CLEAR;
    depth_stencil_target_info.store_op = SDL_GPU_STOREOP_STORE;

    if (swapchain_texture==nullptr) {
        SDL_SubmitGPUCommandBuffer(command_buffer);
        return SDL_APP_CONTINUE;
    }

    SDL_GPUColorTargetInfo color_target_info{};
    color_target_info.clear_color = {0.f, 0.0f,0.0f, .0f};
    color_target_info.load_op = SDL_GPU_LOADOP_CLEAR;
    color_target_info.store_op = SDL_GPU_STOREOP_STORE;
    color_target_info.texture = swapchain_texture;


    CameraUniform camera_uniform{};
    Update_Camera(camera_uniform);


   // printMatrix(camera_uniform.view,"View: \n");
    //printMatrix(camera_uniform.projection,"Projection: \n");


    SDL_GPURenderPass* render_pass = SDL_BeginGPURenderPass(command_buffer, &color_target_info,1, &depth_stencil_target_info);
    ///SDL_GPURenderPass* render_pass = SDL_BeginGPURenderPass(command_buffer, &color_target_info,1, NULL);
    SDL_SetGPUScissor(render_pass,&scissor);
    SDL_SetGPUViewport(render_pass,&viewport);

    SDL_BindGPUGraphicsPipeline(render_pass,grid_pipeline);
    SDL_PushGPUVertexUniformData(command_buffer,0,&camera_uniform,sizeof(CameraUniform));
    //SDL_PushGPUVertexUniformData()

    SDL_DrawGPUPrimitives(render_pass,6,1,0,0);



    SDL_BindGPUGraphicsPipeline(render_pass,graphics_pipeline);
/*
    SDL_GPUTextureSamplerBinding texture_sampler_binding[1]{};
    texture_sampler_binding[0].sampler = sampler;
    texture_sampler_binding[0].texture = texture;
*/


    SDL_PushGPUVertexUniformData(command_buffer,0,&camera_uniform,sizeof(CameraUniform));


    sim_time.time = SDL_GetTicksNS()/1e9f;

    auto move = glm::vec3(sim_time.time*1.0,0.0,0.0);
    //transforms.front().Translate(move);
    Update_InstanceBuffer();

    std::int32_t num_lights = scene_lights.size();

    SDL_PushGPUFragmentUniformData(command_buffer,0,&sim_time,sizeof(TimeStep));
    SDL_PushGPUFragmentUniformData(command_buffer, 1, &num_lights, sizeof(std::int32_t));
    SDL_PushGPUFragmentUniformData(command_buffer,2,scene_lights.data(),sizeof(Light)*scene_lights.size());

    for (const auto& mesh : models["CUBE"].mesh_storage) {
        SDL_GPUTextureSamplerBinding texture_sampler_binding[4]{};
        texture_sampler_binding[0].sampler = sampler;
        texture_sampler_binding[0].texture = graphics_resources.texture_map.Get_Resource(mesh.material.albedo)->Get_GPUTexture();
        texture_sampler_binding[1].sampler = sampler;
        texture_sampler_binding[1].texture = graphics_resources.texture_map.Get_Resource(mesh.material.normal)->Get_GPUTexture();

        texture_sampler_binding[2].sampler = sampler;
        texture_sampler_binding[2].texture = graphics_resources.texture_map.Get_Resource(mesh.material.roughness)->Get_GPUTexture();

        texture_sampler_binding[3].sampler = sampler;
        texture_sampler_binding[3].texture = graphics_resources.texture_map.Get_Resource(mesh.material.metallic)->Get_GPUTexture();



        SDL_BindGPUFragmentSamplers(render_pass,0,texture_sampler_binding,4);
        SDL_GPUBufferBinding buffer_bindings[2];

        buffer_bindings[0].buffer = graphics_resources.vertex_buffer_map.Get_Resource(mesh.v_buffer_id)->Get_GPUBuffer();
        buffer_bindings[0].offset = 0;
        buffer_bindings[1].buffer = instance_buffer;
        buffer_bindings[1].offset = 0;

        SDL_GPUBufferBinding ib_buffer_bindings[1];
        ib_buffer_bindings[0].buffer = graphics_resources.index_buffer_map.Get_Resource(mesh.i_buffer_id)->Get_GPUBuffer();
        ib_buffer_bindings[0].offset=0;


        SDL_BindGPUVertexBuffers(render_pass,0,buffer_bindings,2);
        SDL_BindGPUIndexBuffer(render_pass, ib_buffer_bindings, SDL_GPU_INDEXELEMENTSIZE_16BIT);

        SDL_DrawGPUIndexedPrimitives(render_pass,mesh.indices.size(),transforms.size(),0,0,0);

    }



    // DRAW HERE

    SDL_EndGPURenderPass(render_pass);

    SDL_SubmitGPUCommandBuffer(command_buffer);

    return SDL_APP_CONTINUE;
}


SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event)
{
    // close the window on request
    if (event->type == SDL_EVENT_WINDOW_CLOSE_REQUESTED)
    {
        return SDL_APP_SUCCESS;
    }
    if (event->key.down) {
        switch (event->key.scancode){
            case SDL_SCANCODE_W:
                camera.velocity.z =-1;
                break;
            case SDL_SCANCODE_A:
                camera.velocity.x =-1;
                break;
            case SDL_SCANCODE_S:
                camera.velocity.z =1;
                break;
            case SDL_SCANCODE_D:
                camera.velocity.x =1;
                break;
            case SDL_SCANCODE_R:
                camera.velocity.y =1;
            default:
                break;
        }
    }
    else if (!event->key.down){
        switch (event->key.scancode){
            case SDL_SCANCODE_W:
                camera.velocity.z =0;
                break;
            case SDL_SCANCODE_A:
                camera.velocity.x =0;
                break;
            case SDL_SCANCODE_S:
                camera.velocity.z =0;
                break;
            case SDL_SCANCODE_D:
                camera.velocity.x =0;
                break;
            default:
                break;
        }
    }

    if (event->type== SDL_EVENT_MOUSE_MOTION) {

        auto mx = event->motion.x;
        auto my = event->motion.y;
        float xoffset = mx - camera.lx;

        float yoffset = camera.ly - my;



        camera.yaw += xoffset/200.f;
        camera.pitch -= yoffset/200.f;

        camera.lx = mx;

        camera.ly = my;
       // SDL_Log("Mouse moved");
    }


    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void *appstate, SDL_AppResult result)
{
    SDL_ReleaseGPUShader(device, grid_vertex_shader);
    SDL_ReleaseGPUShader(device, grid_fragment_shader);

    SDL_ReleaseGPUBuffer(device,instance_buffer);

    SDL_ReleaseGPUTransferBuffer(device,instance_transfer_buffer);

    for (auto buffer : vertex_buffer_storage) {
        SDL_ReleaseGPUBuffer(device, buffer);
    }
    for (auto buffer : index_buffer_storage) {
        SDL_ReleaseGPUBuffer(device, buffer);
    }
    graphics_resources.vertex_buffer_map.resources.clear();
    graphics_resources.index_buffer_map.resources.clear();
    graphics_resources.texture_map.resources.clear();

    SDL_ReleaseGPUBuffer(device, index_buffer);
    SDL_ReleaseGPUBuffer(device,vertex_buffer);
    SDL_ReleaseGPUSampler(device,sampler);
    SDL_ReleaseGPUTexture(device, texture);
   SDL_ReleaseGPUTexture(device, depth_texture);
    SDL_ReleaseGPUGraphicsPipeline(device,grid_pipeline);
    SDL_ReleaseGPUGraphicsPipeline(device,graphics_pipeline);

    SDL_DestroyWindow(window);
    SDL_DestroyGPUDevice(device);

}
/*
int main(void)
{
    const char* window_title = "Testing SDL";
    SDL_Log("Starting %s\n", window_title);

    SDL_Window* window;
    SDL_Renderer* renderer;

    constexpr int resolution_width = 800;
    constexpr int resolution_height = 600;


    SDL_GPUDevice* device = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV|SDL_GPU_SHADERFORMAT_MSL,true,NULL);

    SDL_DestroyGPUDevice(device);




    SDL_CreateWindowAndRenderer(
        window_title,
        resolution_width,
        resolution_height,
        SDL_WINDOW_RESIZABLE,
        &window,&renderer
        );

    bool quit = false;

    SDL_Event event;
    while (!quit) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                quit = true;
            }


            SDL_SetRenderDrawColor(renderer, 0x0c, 0x42, 0xA1, 0x00);
           // SDL_RenderTexture(renderer,1,1,1);
            SDL_RenderClear(renderer);

            SDL_SetRenderDrawColor(renderer, 0xFF, 0xFF, 0xFF, 0xFF);
            SDL_RenderDebugText(renderer, 370, 290, "Testin SDL");

            SDL_RenderPresent(renderer);
            //SDL_Texture texture;
           // texture.

            SDL_Delay(16);

        }
    }
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}*/
//