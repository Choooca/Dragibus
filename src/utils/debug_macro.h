#pragma once 
#include <format>
#include <string>
#include <stdexcept>
#include <string_view>

namespace details {
    inline std::string_view FileName(std::string_view path) {
        return path.substr(path.find_last_of("\\/") + 1);
    }
}

#define THROW_RUNTIME_ERROR(...) \
    throw std::runtime_error(std::format("ERROR in {} line {}. {}", details::FileName(__FILE__), __LINE__ , __VA_ARGS__));

#define PRINT_RUNTIME_ERROR(...) \
    throw std::runtime_error(std::format("ERROR in {} line {}. {}", details::FileName(__FILE__), __LINE__ , __VA_ARGS__));

#define THROW_INVALID_ARGUMENT(out) std::string file = std::string(__FILE__); throw std::invalid_argument(std::format("INVALID ARGUMENT in {} line {}. {}",file.substr(file.find_last_of("\\") + 1), __LINE__ ,out));
