#include <iostream>
#define SDL_MAIN_HANDLED
#include "SDL.h"
#include "HelloTriangleApplication.hpp"

int main(int, char**) {
    HelloTriangleApplication app;
    try {
        app.run();
    }
    catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
