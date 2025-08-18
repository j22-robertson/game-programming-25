//
// Created by James Robertson on 13/08/2025.
//

#ifndef GP25_EXERCISES_RENDERUTILS_H
#define GP25_EXERCISES_RENDERUTILS_H
#include <cstdint>
#include <vector>

#include "SDL3/SDL_gpu.h"
#include "RenderData.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "code/AssetLib/3MF/3MFXmlTags.h"


inline void Load_MaterialTextures(const aiMaterial* material, const aiTextureType type, const std::string& type_name, GraphicsResources& resources) {

    for (int i = 0; i < aiGetMaterialTextureCount(material, type); i++) {
        aiString str;
        material->GetTexture(type, i, &str);

        Texture2D texture;
        /*
        if (.contains(str.C_Str())) {
            return;
        }*/
        texture.texture_type = type_name;
        texture.file_path = str.C_Str();
    }
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
        loaded_mesh.vertices.push_back(vertex);
    }

    for (int i =0; i < mesh->mNumFaces; i++) {
        aiFace face = mesh->mFaces[i];
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

inline void Process_Node(SDL_GPUDevice* device, SDL_GPUCommandBuffer* command_buffer,aiNode* node, const aiScene* scene, std::vector<Mesh>& meshes, GraphicsResources& resources,std::uint32_t& step_count) {
    for (int i = 0; i < node->mNumMeshes; i++) {
        std::uint32_t mesh_index = node->mMeshes[i];
        aiMesh* mesh = scene->mMeshes[mesh_index];

        if (mesh->mMaterialIndex>=0) {
            aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];

            Load_MaterialTextures(material, aiTextureType_BASE_COLOR,"base_color_texture",resources);
            Load_MaterialTextures(material, aiTextureType_NORMALS,"normal_texture",resources);
            Load_MaterialTextures(material, aiTextureType_DIFFUSE_ROUGHNESS,"roughness_texture",resources);
            Load_MaterialTextures(material, aiTextureType_METALNESS,"metallic_texture",resources);
            Load_MaterialTextures(material, aiTextureType_AMBIENT_OCCLUSION,"ao_texture",resources);
        }


        auto& mesh_to_load = meshes.emplace_back(Process_Mesh(mesh, scene, resources));
        std::string node_name =node->mName.C_Str();
        Mesh_UploadToGPU(device, command_buffer, resources, mesh_to_load, mesh->mName.C_Str()+node_name+std::to_string(i)+std::to_string(step_count));
    }
    step_count+=1;
    for (int i = 0; i < node->mNumChildren; i++) {
        Process_Node(device, command_buffer,node->mChildren[i], scene, meshes,resources ,step_count);
    }
}

inline Model Load_ModelDataFromFile(SDL_GPUDevice* device,SDL_GPUCommandBuffer* command_buffer,const std::string& filename,GraphicsResources& resources) {
    std::vector<Mesh> loaded_meshes;
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(filename, aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_SplitLargeMeshes);
    auto& node = scene->mRootNode;
    std::uint32_t mesh_count = 0;
    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !node){
        SDL_Log(std::string("ERROR::ASIMP::").append(importer.GetErrorString()).c_str());
    }


    //TODO: Load material data into material structures stored in global material map with identifiers. Give mesh the material indices.
    std::uint32_t step_count=0;
    Process_Node(device,command_buffer,node, scene, loaded_meshes,resources,step_count);

    Model loaded_model{.mesh_storage = loaded_meshes};
    return loaded_model;
    // return indices.size();
}

#endif //GP25_EXERCISES_RENDERUTILS_H