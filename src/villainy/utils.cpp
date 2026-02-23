#include "utils.hpp"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

namespace vlny{

std::vector<char> readFile(const std::string& filename){
    std::ifstream file(filename, std::ios::ate | std::ios::binary);

    if(!file.is_open()){
        throw std::runtime_error("Failed to open file!");
    }

    size_t filesize = (size_t) file.tellg();
    std::vector<char> buffer(filesize);

    file.seekg(0);
    file.read(buffer.data(), filesize);

    file.close();
    return buffer;
}

void saveToFile(std::string filename, std::string data){
    std::ofstream f(filename);

    if(f.is_open()){
        f << data;
        f.close();
    }
}

}