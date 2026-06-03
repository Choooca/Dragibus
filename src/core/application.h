#pragma once

class Window;

class Application {

public:
	Application();
	~Application();

	void Loop();

private:

	Window *m_window;
};