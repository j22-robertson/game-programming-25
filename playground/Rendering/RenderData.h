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
};

struct Model {
    std::uint32_t model_identifier;
    std::vector<Mesh> mesh_storage;
    std::string directory;
};

struct GraphicsBuffer {

    explicit GraphicsBuffer(SDL_GPUBuffer* buffer): gpu_buffer(buffer) {}

    [[nodiscard]] SDL_GPUBuffer* Get_GPUBuffer() const {
        return gpu_buffer;
    }
private:
    SDL_GPUBuffer* gpu_buffer = nullptr;
};


struct GPUTexture {
    explicit GPUTexture(SDL_GPUTexture* texture) : gpu_texture(texture){};

    [[nodiscard]] SDL_GPUTexture* Get_GPUTexture() const {
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
struct GPUResourceMap /// A storage for automatic management of GPU resources, each resource can be accessed via identifiers once uploaded to the GPU and stored within the map.
{

    std::map<std::uint32_t, std::unique_ptr<T,ResourceDeleter<T>>> resources = std::map<std::uint32_t, std::unique_ptr<T,ResourceDeleter<T>>>();


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

    std::uint32_t Insert_Resource(std::unique_ptr<T, ResourceDeleter<T>> resource) /// Insert GPU resource gives ownership to the map
    {
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

    bool Remove_Resource(SDL_GPUDevice* device,const std::int32_t& resource_id) {
        if (!resources.contains(resource_id)) {
            SDL_Log(("No resource exists in this slot \n"));
            return false;
        }

        if (resources.contains(resource_id)) {
            try {
                if constexpr (std::is_same_v<T, GraphicsBuffer>) {
                    SDL_ReleaseGPUBuffer(device, resources[resource_id]->gpu_buffer);
                    available_id.push(resource_id);
                    resource_count -=1;
                    resources.erase(resource_id);
                    return true;
                }
                else if constexpr (std::is_same_v<T, GPUTexture>) {
                    SDL_ReleaseGPUTexture(device, resources[resource_id]->gpu_texture);
                    available_id.push(resource_id);
                    resource_count -=1;
                    resources.erase(resource_id);
                    return true;

                }
                else {
                    std::cerr << "Error: Unknown resource type." << std::endl;
                }
            }
            catch ( const std::runtime_error& e) {
                std::cerr << "Incorrect type, cannot remove resource " << std::endl;
            }
        }
        return true;
    }
};


struct GraphicsResources {
    GPUResourceMap<GraphicsBuffer> vertex_buffer_map;
    GPUResourceMap<GraphicsBuffer> index_buffer_map;
    GPUResourceMap<GPUTexture> texture_map;
};


#endif //GP25_EXERCISES_RENDERDATA_H