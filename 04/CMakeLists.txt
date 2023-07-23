cmake_minimum_required(VERSION 3.5)

project(vulkan_toturial)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

FILE(GLOB_RECURSE SRC_CPP src/*.cpp)

add_executable(vulkan_app ${SRC_CPP} "src/gpu_resource.cpp")

# 链接 vulkan 文件
target_include_directories(vulkan_app PRIVATE $ENV{VULKAN_ROOT}/Include)
target_link_directories(vulkan_app PRIVATE $ENV{VULKAN_ROOT}/Lib)
target_link_libraries(vulkan_app PRIVATE vulkan-1.lib)

find_package(glm CONFIG REQUIRED)
target_link_libraries(vulkan_app PRIVATE glm::glm)

find_package(SDL2 CONFIG REQUIRED)
target_link_libraries(vulkan_app
    PRIVATE
    $<TARGET_NAME_IF_EXISTS:SDL2::SDL2main>
    $<IF:$<TARGET_EXISTS:SDL2::SDL2>,SDL2::SDL2,SDL2::SDL2-static>)

find_package(fmt CONFIG REQUIRED)
target_link_libraries(vulkan_app PRIVATE fmt::fmt-header-only)


