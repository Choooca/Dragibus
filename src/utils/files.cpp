#pragma once
#include "files.h"
#include <fstream>
#include <utils/debug_macro.h>

std::vector<char> ReadFile(const std::string &file_path) {
	std::ifstream file(file_path, std::ios::binary | std::ios::ate);

	if (!file.is_open()) {
		THROW_RUNTIME_ERROR("Failed to open : " + file_path);
	}

	size_t file_size = static_cast<size_t>(file.tellg());
	std::vector<char> buffer(file_size);

	file.seekg(0);
	file.read(buffer.data(), file_size);

	file.close();

	return buffer;
}