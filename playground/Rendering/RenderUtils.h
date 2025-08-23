//
// Created by James Robertson on 13/08/2025.
//

#ifndef GP25_EXERCISES_RENDERUTILS_H
#define GP25_EXERCISES_RENDERUTILS_H
#include <vector>

#include "SDL3/SDL_gpu.h"
#include "RenderData.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

// TODO: Look into the advantages and disadvantages of batching uploads to the GPU in a single transfer buffer for each batch of uploads.
/** TODO: Look into the separation of file loading and uploading, a resourceloader that stores
 * the file names could be useful for serialization of scenes later on allowing for an editor
 * and automatic loading of all files (models, textures and other assets) for scenes.
**/
inline std::uint32_t Upload_Texture2DFromFile(SDL_GPUDevice* device,SDL_GPUCommandBuffer* command_buffer,GraphicsResources& resources, const std::string& filename) {
    if (resources.texture_map.Contains(resources.texture_map.Get_ID(filename))) {
        return resources.texture_map.Get_ID(filename);
    }
    int width, height, channels;
    unsigned char* data = stbi_load(filename.c_str(), &width, &height, &channels, 4);
    if (data == nullptr) {
        SDL_Log(("Unable to load texture: "+filename + "\n").c_str());
        return INVALID_ID;
    }


    SDL_GPUTextureCreateInfo texture_info{};

    texture_info.width = width;
    texture_info.height = height;
    texture_info.layer_count_or_depth = 1;
    texture_info.type = SDL_GPU_TEXTURETYPE_2D;
    texture_info.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER;
    texture_info.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM_SRGB;
    texture_info.num_levels = 1;

    auto* texture = SDL_CreateGPUTexture(device, &texture_info);
    //SDL_GPUTexture* texture = SDL_CreateGPUTexture(device, &texture_info);

    SDL_GPUTransferBufferCreateInfo buffer_info{};
    buffer_info.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    buffer_info.size = static_cast<std::uint32_t>(4* width * height);
    buffer_info.props = 0;




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

    SDL_memcpy(mapped_data, data, (channels*width*height));

    SDL_UnmapGPUTransferBuffer(device, transfer_buffer);


    SDL_UploadToGPUTexture(image_copy_pass,&transfer_info,&texture_region, true);


    SDL_EndGPUCopyPass(image_copy_pass);
    SDL_ReleaseGPUTransferBuffer(device,transfer_buffer);


   return resources.texture_map.Insert_Resource(std::unique_ptr<GPUTexture, ResourceDeleter<GPUTexture>>(new GPUTexture(texture),ResourceDeleter<GPUTexture>(device)),filename);


}

inline std::vector<std::uint32_t> Load_MaterialTextures(SDL_GPUDevice* device,SDL_GPUCommandBuffer* command_buffer,const aiMaterial* material, const aiTextureType type, const std::string& type_name, GraphicsResources& resources,const std::string& directory) {
    std::vector<std::uint32_t> texture_identifiers;

    for (int i = 0; i < aiGetMaterialTextureCount(material, type); i++) {
        aiString str;
        material->GetTexture(type, i, &str);

        Texture2D texture;

        texture.texture_type = type_name;
        texture.file_path = str.C_Str();


        auto texture_id = Upload_Texture2DFromFile(device, command_buffer,resources, directory +"/"+texture.file_path);
        texture_identifiers.push_back(texture_id);
    }
    return texture_identifiers;
}

inline Mesh Process_Mesh(const aiMesh* mesh, const aiScene* scene,GraphicsResources& resources) {



    Mesh loaded_mesh{};
    for (int i = 0; i< mesh->mNumVertices; i++) {

        Vertex vertex{};

        vertex.x = mesh->mVertices[i].x;
        vertex.y= mesh->mVertices[i].y;
        vertex.z = mesh->mVertices[i].z;


        vertex.nx =mesh->mNormals[i].x;
        vertex.ny = mesh->mNormals[i].y;
        vertex.nz = mesh->mNormals[i].z;
        if (mesh->mTextureCoords[0]) {
            vertex.u=mesh->mTextureCoords[0][i].x;
            vertex.v= mesh->mTextureCoords[0][i].y;
        }
        else {
            vertex.u=(float)(mesh->mVertices[i].x*0.5 +0.5);
            vertex.v=(float)(mesh->mVertices[i].y*0.5 +0.5);
        }
        if (mesh->mTangents != nullptr) {
            vertex.tx = mesh->mTangents[i].x;
            vertex.ty = mesh->mTangents[i].y;
            vertex.tz = mesh->mTangents[i].z;
        }
        else {
            vertex.tx=0.0;
            vertex.ty=0.0;
            vertex.tz=0.0;
        }
        if (mesh->mBitangents != nullptr) {
            vertex.btx = mesh->mBitangents[i].x;
            vertex.bty = mesh->mBitangents[i].y;
            vertex.btz = mesh->mBitangents[i].z;
        }
        else {
            vertex.btx=0.0;
            vertex.bty=0.0;
            vertex.btz=0.0;
        }
        loaded_mesh.vertices.push_back(vertex);
    }

    for (int i =0; i < mesh->mNumFaces; i++) {
        const aiFace face = mesh->mFaces[i];
        for (int j = 0; j < face.mNumIndices; j++) {
            loaded_mesh.indices.push_back(face.mIndices[j]);
        }
    }



    return loaded_mesh;

}

inline void Mesh_UploadToGPU(SDL_GPUDevice* device,SDL_GPUCommandBuffer* command_buffer,GraphicsResources& graphics_resources,Mesh& mesh, const std::string& name)
{
    /*
    if (graphics_resources.vertex_buffer_map.Contains(graphics_resources.vertex_buffer_map.Get_ID(name)) || graphics_resources.index_buffer_map.Contains(graphics_resources.index_buffer_map.Get_ID(name))) {
        return;
    }*/

    SDL_GPUBufferCreateInfo buffer_create_info{};
    const std::uint32_t vertex_buffer_size = sizeof(Vertex)*mesh.vertices.size();

    buffer_create_info.size = vertex_buffer_size;
    buffer_create_info.usage = SDL_GPU_BUFFERUSAGE_VERTEX;
    auto vbuffer = SDL_CreateGPUBuffer(device, &buffer_create_info);

    SDL_GPUTransferBufferCreateInfo transfer_buffer_info{};
    transfer_buffer_info.size = vertex_buffer_size;
    transfer_buffer_info.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    SDL_GPUTransferBuffer* transfer_buffer = SDL_CreateGPUTransferBuffer(device, &transfer_buffer_info);

    Vertex* data = (Vertex*)SDL_MapGPUTransferBuffer(device, transfer_buffer, false);


    SDL_GPUCopyPass* copy_pass = SDL_BeginGPUCopyPass(command_buffer);
    SDL_memcpy(data,mesh.vertices.data(),vertex_buffer_size);
    SDL_UnmapGPUTransferBuffer(device, transfer_buffer);
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////



    SDL_GPUTransferBufferLocation transfer_location{};
    transfer_location.transfer_buffer = transfer_buffer;
    transfer_location.offset = 0;

    SDL_GPUBufferRegion buffer_region{};
    buffer_region.buffer = vbuffer;
    buffer_region.size = vertex_buffer_size;
    buffer_region.offset = 0;

    SDL_UploadToGPUBuffer(copy_pass, &transfer_location, &buffer_region, true);

    SDL_EndGPUCopyPass(copy_pass);


    SDL_GPUCopyPass* ib_copy_pass = SDL_BeginGPUCopyPass(command_buffer);

    const std::uint32_t index_buffer_size = sizeof(std::uint16_t)*mesh.indices.size();


    SDL_GPUBufferCreateInfo index_buffer_info{};
    index_buffer_info.usage = SDL_GPU_BUFFERUSAGE_INDEX;
    index_buffer_info.size = index_buffer_size;

    auto i_buffer = SDL_CreateGPUBuffer(device,&index_buffer_info);

    SDL_GPUTransferBufferCreateInfo ib_transfer_buffer_info{};
    ib_transfer_buffer_info.size = index_buffer_size;
    ib_transfer_buffer_info.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;


    SDL_GPUTransferBuffer* ib_transfer_buffer = SDL_CreateGPUTransferBuffer(device, &ib_transfer_buffer_info);

    uint16_t* ib_data = (uint16_t*)SDL_MapGPUTransferBuffer(device, ib_transfer_buffer, false);

    SDL_memcpy(ib_data,mesh.indices.data(),index_buffer_size);
    SDL_UnmapGPUTransferBuffer(device, ib_transfer_buffer);


    SDL_GPUTransferBufferLocation ib_transfer_location{};
    ib_transfer_location.transfer_buffer = ib_transfer_buffer;
    ib_transfer_location.offset = 0;

    SDL_GPUBufferRegion ib_buffer_region{};
    ib_buffer_region.buffer = i_buffer;
    ib_buffer_region.size = index_buffer_size;
    ib_buffer_region.offset = 0;

    SDL_UploadToGPUBuffer(ib_copy_pass, &ib_transfer_location, &ib_buffer_region, true);
    SDL_EndGPUCopyPass(ib_copy_pass);





    mesh.v_buffer_id =graphics_resources.vertex_buffer_map.Insert_Resource(std::unique_ptr<GraphicsBuffer,ResourceDeleter<GraphicsBuffer>>(new GraphicsBuffer(vbuffer),ResourceDeleter<GraphicsBuffer>(device)),name);
    mesh.i_buffer_id = graphics_resources.index_buffer_map.Insert_Resource(std::unique_ptr<GraphicsBuffer,ResourceDeleter<GraphicsBuffer>>( new GraphicsBuffer(i_buffer),ResourceDeleter<GraphicsBuffer>(device)),name);

    SDL_ReleaseGPUTransferBuffer(device, ib_transfer_buffer);
    SDL_ReleaseGPUTransferBuffer(device, transfer_buffer);

}

inline void Process_Node(SDL_GPUDevice* device, SDL_GPUCommandBuffer* command_buffer,const std::string& directory,aiNode* node, const aiScene* scene, std::vector<Mesh>& meshes, GraphicsResources& resources,std::uint32_t& step_count) {
    for (int i = 0; i < node->mNumMeshes; i++) {
        std::uint32_t mesh_index = node->mMeshes[i];
        aiMesh* mesh = scene->mMeshes[mesh_index];





        auto& mesh_to_load = meshes.emplace_back(Process_Mesh(mesh, scene, resources));
        if (mesh->mMaterialIndex>=0) {
            const aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];

            auto base_colors =Load_MaterialTextures(device,command_buffer,material, aiTextureType_BASE_COLOR,"base_color_texture",resources,directory);
            auto normal_textures=Load_MaterialTextures(device,command_buffer,material, aiTextureType_NORMALS,"normal_texture",resources,directory);
            auto roughness_textures =Load_MaterialTextures(device,command_buffer,material, aiTextureType_DIFFUSE_ROUGHNESS,"roughness_texture",resources,directory);
            auto metallic_textures =Load_MaterialTextures(device,command_buffer,material, aiTextureType_METALNESS,"metallic_texture",resources,directory);
            //auto ao_textures = Load_MaterialTextures(device,command_buffer,material, aiTextureType_AMBIENT_OCCLUSION,"ao_texture",resources);

            mesh_to_load.material.albedo=base_colors.front();
            mesh_to_load.material.normal=normal_textures.front();
            mesh_to_load.material.roughness=roughness_textures.front();
            mesh_to_load.material.metallic=metallic_textures.front();
            //mesh_to_load.material.ao=INVALID_ID;

        }

        std::string node_name =node->mName.C_Str();
        Mesh_UploadToGPU(device, command_buffer, resources, mesh_to_load, mesh->mName.C_Str()+node_name+std::to_string(i)+std::to_string(step_count));
    }
    step_count+=1;
    for (int i = 0; i < node->mNumChildren; i++) {
        Process_Node(device, command_buffer,directory,node->mChildren[i], scene, meshes,resources ,step_count);
    }
}

inline Model Load_ModelDataFromFile(SDL_GPUDevice* device,SDL_GPUCommandBuffer* command_buffer,const std::string& filename,GraphicsResources& resources) {
    std::vector<Mesh> loaded_meshes;
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(filename, aiProcess_Triangulate | aiProcess_FlipUVs |aiProcess_CalcTangentSpace| aiProcess_SplitLargeMeshes);


    auto& node = scene->mRootNode;
    std::uint32_t mesh_count = 0;
    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !node){
        SDL_Log(std::string("ERROR::ASIMP::").append(importer.GetErrorString()).c_str());
    }

    std::string directory = filename.substr(0, filename.find_last_of("/"));
    //TODO: Load material data into material structures stored in global material map with identifiers. Give mesh the material indices.
    std::uint32_t step_count=0;
    Process_Node(device,command_buffer,directory,node, scene, loaded_meshes,resources,step_count);

    Model loaded_model{.mesh_storage = loaded_meshes};
    return loaded_model;
    // return indices.size();
}

#endif //GP25_EXERCISES_RENDERUTILS_H