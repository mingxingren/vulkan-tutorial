cmake_minimum_required(VERSION 3.0.0)
project(hellovulkan VERSION 0.1.0)

# 设置 CPP 版本
set(CMAKE_CXX_STANDARD 17)

include(CTest)
enable_testing()

include_directories($ENV{SDL_INCLUDE_DIR})

file(GLOB SRC_FILE_CPP ./*.cpp)
file(GLOB SRC_FILE_HPP ./*.hpp)

add_executable(hellovulkan ${SRC_FILE_CPP} ${SRC_FILE_HPP})

# 链接 vulkan
target_include_directories(hellovulkan PRIVATE $ENV{VULKAN_SDK}/include)
target_link_directories(hellovulkan PRIVATE $ENV{VULKAN_SDK}/Lib)
target_link_libraries(hellovulkan vulkan-1.lib)

target_link_directories(hellovulkan PRIVATE $ENV{SDL_LIB_DIR})
target_link_libraries(hellovulkan SDL2.lib)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
