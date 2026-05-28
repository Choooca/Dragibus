#pragma once
#include <memory>

class Window;

class Application {

public:

	Application();
	~Application();

	void Loop();

private:

	std::unique_ptr<Window> m_window;
	
};