//
// Created by James Robertson on 13/08/2025.
//
#pragma once
#include <map>
#include <SDL3/SDL_gpu.h>
#include <concepts>
#include <queue>


#ifndef GP25_EXERCISES_RENDERDATA_H
#define GP25_EXERCISES_RENDERDATA_H


constexpr std::uint32_t INVALID_ID = std::numeric_limits<std::uint32_t>::max();

struct Vertex {
    float x,y,z;
    float nx,ny,nz;
    float u,v;
};

using Vertices = std::vector<Vertex>;
using Indices = std::vector<std::uint16_t>;



struct Mesh {
    std::uint32_t v_buffer_id = INVALID_ID;
    std::uint32_t i_buffer_id = INVALID_ID;
    Vertices vertices;
    Indices indices;
    //std::vector<std::uint32_t> material_id
};

struct Model {
    std::vector<Mesh> mesh_storage;
    std::string directory;

    ////void processNode(aiNode *node, const aiScene *scene)

};

struct GraphicsBuffer {

    explicit GraphicsBuffer(SDL_GPUBuffer* buffer): gpu_buffer(buffer) {}

    SDL_GPUBuffer* Get_GPUBuffer() const {
        return gpu_buffer;
    }
private:
    SDL_GPUBuffer* gpu_buffer = nullptr;
};


struct GPUTexture {
    SDL_GPUTexture* Get_GPUTexture() const {
        return gpu_texture;
    }
private:
    SDL_GPUTexture* gpu_texture = nullptr;
};



template <typename T>
concept IsGPUResource = std::is_same_v<T, GraphicsBuffer> || std::is_same_v<T, GPUTexture>;
template <typename T>
struct ResourceDeleter {
    SDL_GPUDevice* device;

    explicit ResourceDeleter(SDL_GPUDevice* _device) : device(_device){}

    void operator()(T* ptr) const {
        if (!ptr) return;

        if constexpr(std::is_same_v<T, GraphicsBuffer>) {
            SDL_ReleaseGPUBuffer(device, ptr->Get_GPUBuffer());
        }
        else if constexpr(std::is_same_v<T, GPUTexture>) {
            SDL_ReleaseGPUTexture(device, ptr->Get_GPUTexture());
        }
        else {
            std::cerr << "Error: Unknown resource type. Not handled by deleter." << std::endl;
        }
    }
};

template <typename T>
    requires IsGPUResource<T>
struct GPUResourceMap {

    std::map<std::uint32_t, std::unique_ptr<T,ResourceDeleter<T>>> resources = std::map<std::uint32_t, std::unique_ptr<T,ResourceDeleter<T>>>();

    std::map<std::string, std::uint32_t> name_to_id = std::map<std::string, std::uint32_t>(); // Converted name to internal identifier
    std::map<std::uint32_t, std::string> id_to_name = std::map<uint32_t, std::string>(); // Converts internal identifier to name

    std::queue<std::uint32_t> available_id = std::queue<std::uint32_t>(); // Allows for reuse of identifiers once a resource is removed.

    std::uint32_t resource_count = 0; // Shows the number of actively utilized resources
    std::uint32_t identifier_count = 0; // Shows the number of active identifiers in circulation (Within queue or used)

    T* Get_Resource(const std::uint32_t &key) // Get the allocated graphics resource by direct identifier, returns null if non-existent
    {
        if (!resources.contains(key)) {
            return nullptr;
        }
        return resources.at(key).get();
    }

    T* Get_Resource(const std::string &key) // Get the allocated graphics resource by name, returns null if non-existent
    {
        if (!name_to_id.contains(key)) {
            return nullptr;
        }
        if (!resources.contains(name_to_id[key])) {
            return nullptr;
        }
        return resources.at(name_to_id[key]).get();
    }

    std::uint32_t Get_ResourceID(const std::string &key) // Get the ID of the resource, returns INVALID_ID (max lim of std::uint32_t if non-existent).
    {
        if (name_to_id.contains(key)) {
            return INVALID_ID;
        }
        if (!resources.contains(name_to_id[key])) {
            return INVALID_ID;
        }
        return name_to_id[key];
    }

    std::uint32_t Insert_Resource(std::unique_ptr<T, ResourceDeleter<T>> resource) {
        if (available_id.empty()) {
            const std::uint32_t id = identifier_count;
            resources.insert(std::pair(id, std::move(resource)));
            identifier_count+=1;
            resource_count+=1;
            return id;
        }
        std::uint32_t id = available_id.front();
        available_id.pop();
        resources.insert(std::pair(id, std::move(resource)));
        resource_count+=1;
        return id;
    }
    bool Remove_Resource(SDL_GPUDevice* device,const std::string& resource_name) {
        if (!name_to_id.contains(resource_name)) {
            SDL_Log(("No resource exists named: " + resource_name).c_str());
            return false;
        }
        const std::uint32_t resource_id = name_to_id[resource_name];

        if (resources.contains(resource_id)) {
            try {
                if constexpr (std::is_same_v<T, GraphicsBuffer>) {
                    SDL_ReleaseGPUBuffer(device, resources[resource_id]->gpu_buffer);
                    available_id.push(resource_id);
                    resource_count -=1;
                    resources.erase(resource_id);
                    std::string name_key = id_to_name[resource_id];
                    id_to_name.erase(resource_id);
                    name_to_id.erase(resource_name);
                }
                else if constexpr (std::is_same_v<T, GPUTexture>) {
                    SDL_ReleaseGPUTexture(device, resources[resource_id]->gpu_texture);
                    available_id.push(resource_id);
                    resource_count -=1;
                    resources.erase(resource_id);
                    id_to_name.erase(resource_id);
                    name_to_id.erase(resource_name);
                }
                else {
                    std::cerr << "Error: Unknown resource type." << std::endl;
                }
            }
            catch ( const std::runtime_error& e) {
                std::cerr << "Incorrect type, cannot remove resource " << resource_name <<std::endl;
            }
        }
        return true;
    }
};


struct GraphicsResources {
    GPUResourceMap<GraphicsBuffer> vertex_buffer_map;
    GPUResourceMap<GraphicsBuffer> index_buffer_map;
};


#endif //GP25_EXERCISES_RENDERDATA_H