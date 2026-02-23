#include "logger.hpp"

#include "utils.hpp"

namespace vlny{

Logger::Logger(std::string appName, std::string logfile) : appname(appName), logfilename(logfile) { initLogFile(); }
Logger::Logger(std::string appName) : appname(appName), logfilename() {}
Logger::Logger(const Logger& other) : appname(other.appname), logfilename(other.logfilename) { initLogFile(); }

void Logger::setMinSeverity(LogSeverity newMinSev){
    minSev = newMinSev;
}

void Logger::log(LogSeverity sev, std::string msg){
    std::string output = appname + "/" + getSevString(sev) + "\t - " + msg + "\n";
    if(!logfilename.empty()) { writeToLogFile(output); }
    if(sev < minSev){ return; } 
    std::cout << output;
}

Logger& Logger::operator=(const Logger& other){
    if (this != &other) {  // Self-assignment check
        appname = other.appname;
        logfilename = other.logfilename;
    }
    return *this;
}

std::string Logger::getName(){
    return appname;
}
std::string Logger::getLogfile(){
    return logfilename;
}

void Logger::initLogFile(){
    if(logfilename.empty()){ return; }
    logfile = std::ofstream(logfilename, std::ios::app);
    if(!logfile.is_open()){
        std::cerr << "Failed to open log file '" << logfilename << "'" << std::endl;
    }
}

void Logger::writeToLogFile(std::string dat){
    if(logfile.is_open()){
        logfile << dat;
    }
}

std::string Logger::getSevString(LogSeverity s){
    switch(s){
        case VERBOSE: return "[VERBOSE]";
        case INFO: return "[INFO]";
        case WARNING: return "[WARNING]";
        case ERROR: return "[ERROR]";
        default: return "[NONE]";
    }
}

LogSeverity Logger::vulkanDebugSeverityToLogSeverity(VkDebugUtilsMessageSeverityFlagBitsEXT sev){
    switch(sev){
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT: return VERBOSE;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT: return INFO;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT: return WARNING;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT: return ERROR;
        default: return NONE;
    }
}

}