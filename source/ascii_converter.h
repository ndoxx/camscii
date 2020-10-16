#pragma once

#include <cstdint>

namespace camscii
{

struct Color
{
	uint32_t r;
	uint32_t g;
	uint32_t b;
	float luma;
};

struct Frame;
class AsciiConverter
{
public:
	AsciiConverter(uint32_t width, uint32_t height);
	AsciiConverter(uint32_t cols, uint32_t rows, uint32_t width, uint32_t height);
    void display(const Frame& frame, uint32_t nbytes);

private:
	Color get_block_color(const Frame& frame, uint32_t col, uint32_t row);
	void update_console_size();

private:
	uint32_t cols_;
	uint32_t rows_;
	uint32_t width_;
	uint32_t height_;

	uint32_t block_w_;
	uint32_t block_h_;

	bool fixed_;
};

} // namespace camscii
