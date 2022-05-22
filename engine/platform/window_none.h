#pragma once

#include "app/window.h"

class WindowNone : public Window
{
public:
	WindowNone();
	~WindowNone();

	int32_t get_height() const override { return height; };
	int32_t get_width() const override { return width; };
	void set_height(int32_t new_height) override { height = new_height; };
	void set_width(int32_t new_width) override { width = new_width; };

private:
	int32_t width;
	int32_t height;
};