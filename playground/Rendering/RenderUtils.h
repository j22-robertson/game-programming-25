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

/*
inline void Mesh_UploadToGPU(SDL_GPUDevice* device,std::vector<SDL_GPUBuffer*>& vertex_buffer_storage,std::vector<SDL_GPUBuffer*>& index_buffer_storage,Mesh& mesh)
{
    //indices_length_storage.push_back(_indices.size());


    mesh.v_buffer_id = vertex_buffer_storage.size();
    mesh.i_buffer_id = index_buffer_storage.size();


    SDL_GPUBufferCreateInfo buffer_create_info{};
    const std::uint32_t vertex_buffer_size = sizeof(Vertex)*mesh.vertices.size();

    buffer_create_info.size = vertex_buffer_size;
    buffer_create_info.usage = SDL_GPU_BUFFERUSAGE_VERTEX;
    auto& vbuffer = vertex_buffer_storage.emplace_back(SDL_CreateGPUBuffer(device, &buffer_create_info));

    SDL_GPUTransferBufferCreateInfo transfer_buffer_info{};
    transfer_buffer_info.size = vertex_buffer_size;
    transfer_buffer_info.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    SDL_GPUTransferBuffer* transfer_buffer = SDL_CreateGPUTransferBuffer(device, &transfer_buffer_info);

    Vertex* data = (Vertex*)SDL_MapGPUTransferBuffer(device, transfer_buffer, false);

    SDL_GPUCommandBuffer* command_buffer = SDL_AcquireGPUCommandBuffer(device);
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

    auto& i_buffer = index_buffer_storage.emplace_back(SDL_CreateGPUBuffer(device,&index_buffer_info));

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

    SDL_SubmitGPUCommandBuffer(command_buffer);

    SDL_ReleaseGPUTransferBuffer(device, ib_transfer_buffer);
    SDL_ReleaseGPUTransferBuffer(device, transfer_buffer);



    //TODO: Refactor to proper identifier system, I do not approve of this below
    //return vertex_buffer_storage.size()-1;
}*/
//TODO: Figure out a good way to get access to num verts and num indices

inline Mesh Process_Mesh(const aiMesh* mesh) {
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

inline void Mesh_UploadToGPU(SDL_GPUDevice* device,SDL_GPUCommandBuffer* command_buffer,GraphicsResources& graphics_resources,Mesh& mesh)
{

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





    mesh.v_buffer_id =graphics_resources.vertex_buffer_map.Insert_Resource(std::unique_ptr<GraphicsBuffer,ResourceDeleter<GraphicsBuffer>>(new GraphicsBuffer(vbuffer),ResourceDeleter<GraphicsBuffer>(device)));
    mesh.i_buffer_id = graphics_resources.index_buffer_map.Insert_Resource(std::unique_ptr<GraphicsBuffer,ResourceDeleter<GraphicsBuffer>>( new GraphicsBuffer(i_buffer),ResourceDeleter<GraphicsBuffer>(device)));

    SDL_ReleaseGPUTransferBuffer(device, ib_transfer_buffer);
    SDL_ReleaseGPUTransferBuffer(device, transfer_buffer);

}

inline void Process_Node(aiNode* node, const aiScene* scene, std::vector<Mesh>& meshes) {
    for (int i = 0; i < node->mNumMeshes; i++) {
        std::uint32_t mesh_index = node->mMeshes[i];
        aiMesh* mesh = scene->mMeshes[mesh_index];
        meshes.emplace_back(Process_Mesh(mesh));
    }

    for (int i = 0; i < node->mNumChildren; i++) {
        Process_Node(node->mChildren[i], scene, meshes);
    }
}

inline Model Load_ModelDataFromFile(SDL_GPUDevice* device, const std::string& filename ) {



    std::vector<Mesh> loaded_meshes;


    Assimp::Importer importer;

    const aiScene* scene = importer.ReadFile(filename, aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_SplitLargeMeshes);
    auto& node = scene->mRootNode;


    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !node){
        SDL_Log(std::string("ERROR::ASIMP::").append(importer.GetErrorString()).c_str());
    }

    Process_Node(node, scene, loaded_meshes);

    Model loaded_model{.mesh_storage = loaded_meshes};
    return loaded_model;
    // return indices.size();
}

#endif //GP25_EXERCISES_RENDERUTILS_H