#ifndef VILLAINY_LOGGER_HPP
#define VILLAINY_LOGGER_HPP

#include <iostream>
#include <string>
#include <fstream>

#include <vulkan/vulkan.h>

#ifdef VILLAINY_NO_VERBOSE
#define VILLAINY_VERBOSE_LOG(logger,msg) ;
#else
#define VILLAINY_VERBOSE_LOG(logger,msg) logger.log(VERBOSE, msg);
#endif

namespace vlny{

enum LogSeverity{
    VERBOSE,
    INFO,
    WARNING,
    ERROR,
    NONE
};

class Logger{
public:
    Logger(std::string appName, std::string logfile);
    Logger(std::string appName);
    Logger(const Logger& other);

    void setMinSeverity(LogSeverity newMinSev);

    void log(LogSeverity sev, std::string msg);

    Logger& operator=(const Logger& other);

    std::string getName();
    std::string getLogfile();

    static VKAPI_ATTR VkBool32 VKAPI_CALL validationLayerCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT messegeSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageType,
        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void* pUserData
    ){
        if(messegeSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT){
            std::string sev = messegeSeverity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT ? "[ERROR]" : 
                messegeSeverity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT ? "[WARNING]" : "[INFO]";
            std::cerr << sev << " validation layer: " + std::string(pCallbackData->pMessage) << std::endl;
        }
        return VK_FALSE;
    }
private:
    std::string appname;
    std::string logfilename;
    std::ofstream logfile;
    LogSeverity minSev = INFO;

    void initLogFile();

    void writeToLogFile(std::string dat);

    std::string getSevString(LogSeverity s);

    LogSeverity vulkanDebugSeverityToLogSeverity(VkDebugUtilsMessageSeverityFlagBitsEXT sev);
};

}
#endif