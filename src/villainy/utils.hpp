#ifndef VILLAINY_UTILS_HPP
#define VILLAINY_UTILS_HPP

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>
#include <vector>
#include <fstream>

namespace vlny{

// IO utils

std::vector<char> readFile(const std::string& filename);

void saveToFile(std::string filename, std::string data);

// C++ utils

template<typename T>
uint32_t scast_ui32(T in){
    return static_cast<uint32_t>(in);
}

// Vulkan utils
VkImageView createImageView(VkImage image, VkFormat format);

}
#endif