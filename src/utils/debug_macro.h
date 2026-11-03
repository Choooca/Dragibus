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

#define THROW_RUNTIME_ERROR(fmt, ...) \
    do { throw std::runtime_error(std::format("ERROR in {} line {}. " fmt, details::FileName(__FILE__), __LINE__ __VA_OPT__(,) __VA_ARGS__)); } while (0)

#define THROW_INVALID_ARGUMENT(out) \
    do { throw std::invalid_argument(std::format("INVALID ARGUMENT in {} line {}. {}", details::FileName(__FILE__), __LINE__, out)); } while (0)