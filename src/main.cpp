#include <iostream>
#include <spdlog/spdlog.h>
#include <core/application.h>


int main(int argc, char* argv[]) {
	
	spdlog::set_level(spdlog::level::debug);

	try {
		std::unique_ptr<Application> app = std::make_unique<Application>();
		app->Loop();
	}
	catch (const std::exception& e) {
		spdlog::error("{}", e.what());
		return EXIT_FAILURE;
	}
	
	return EXIT_SUCCESS;
}