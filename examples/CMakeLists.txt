file(GLOB file_src_list "*.c" "*.cpp")

add_executable(00_hello_world 00_hello_world.c
        ../playground/Rendering/RenderData.h
        ../playground/Rendering/Renderer.h)
add_executable(01_hello_world 01_hello_world.cpp)
add_executable(02_hello_sdl   02_hello_sdl.cpp)
add_executable(testing_sdl   ../playground/testing_sdl.cpp
        ../playground/Rendering/RenderUtils.h
        ../playground/Components/CoreComponents.h)

cmake_minimum_required(VERSION 3.11) # FetchContent is new in version 3.11.

include(FetchContent)

FetchContent_Declare(
        glm
        GIT_REPOSITORY	https://github.com/g-truc/glm.git
        GIT_TAG 	bf71a834948186f4097caa076cd2663c69a10e1e #refs/tags/1.0.1
)

FetchContent_Declare(
        assimp
        GIT_REPOSITORY	https://github.com/assimp/assimp.git
        GIT_TAG 	v6.0.2 #refs/tags/1.0.1
)



FetchContent_MakeAvailable(glm)
FetchContent_MakeAvailable(assimp)

target_link_libraries(testing_sdl PRIVATE assimp::assimp)
target_include_directories(testing_sdl PRIVATE ${assimp_SOURCE_DIR})
target_link_libraries(testing_sdl PRIVATE glm::glm)
target_include_directories(testing_sdl PRIVATE ${glm_SOURCE_DIR})

target_link_libraries(testing_sdl PRIVATE SDL3::SDL3)
target_include_directories(testing_sdl PUBLIC lib/SDL/include)

target_link_libraries(02_hello_sdl PRIVATE SDL3::SDL3)
target_include_directories(02_hello_sdl PUBLIC lib/SDL/include)
