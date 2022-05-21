#define GLFW_INCLUDE_VULKAN
#include "GLFW/glfw3.h"

#include "app/window.h"

class WindowGLFW : public Window
{
public:
	WindowGLFW(int32_t width, int32_t height);
	~WindowGLFW();

	int32_t get_height() const override;
	int32_t get_width() const override;
	void set_height(int32_t height) override;
	void set_width(int32_t width) override;

private:
	GLFWwindow* glfw_window;
};
