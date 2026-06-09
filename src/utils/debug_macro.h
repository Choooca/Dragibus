#pragma once 
#include <format>
#include <string>

#define THROW_RUNTIME_ERROR(out) std::string file = std::string(__FILE__); throw std::runtime_error(std::format("ERROR in {} line {}. {}",file.substr(file.find_last_of("\\") + 1), __LINE__ ,out));
#define THROW_INVALID_ARGUMENT(out) std::string file = std::string(__FILE__); throw std::invalid_argument(std::format("INVALID ARGUMENT in {} line {}. {}",file.substr(file.find_last_of("\\") + 1), __LINE__ ,out));