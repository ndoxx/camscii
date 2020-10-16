#include "ascii_converter.h"
#include "common.h"

#include <sstream>
#include <cmath>
#include <cassert>
#include <iostream>
#include <kibble/logger/logger.h>

#include <sys/ioctl.h>
#include <unistd.h>

namespace camscii
{

static const std::string s_fillers = "oO8$";

AsciiConverter::AsciiConverter(uint32_t width, uint32_t height):
width_(width),
height_(height),
fixed_(false)
{
	update_console_size();
}


AsciiConverter::AsciiConverter(uint32_t cols, uint32_t rows, uint32_t width, uint32_t height):
cols_(cols),
rows_(rows),
width_(width),
height_(height),
fixed_(true)
{
	block_w_ = width_ / cols_;
	block_h_ = height_ / rows_;
}

void AsciiConverter::display(const Frame& frame, [[maybe_unused]] uint32_t nbytes)
{
	// Check that nbytes == 3*width*height
	assert(nbytes == 3*width_*height_ && "Number of bytes should be 3*width*height");

	std::cout << "\033[2J";

	if(!fixed_)
		update_console_size();

	for(uint32_t row = 0; row < rows_; ++row)
	{
		for(uint32_t col = 0; col < cols_; ++col)
		{
			auto c = get_block_color(frame, col, row);
			size_t filler_idx = size_t(std::round(float(s_fillers.size()-1)*c.luma/255.f));
			char filler = s_fillers[filler_idx];

			std::cout << "\033[1;38;2;" << c.r << ';' << c.g << ';' << c.b << "m" << filler << "\033[0m";
		}
		std::cout << '\n';
	}
}

Color AsciiConverter::get_block_color(const Frame& frame, uint32_t col, uint32_t row)
{
	float r = 0.f;
	float g = 0.f;
	float b = 0.f;

	uint32_t start_x = row * block_h_;
	uint32_t start_y = col * block_w_;

	const unsigned char* buf = static_cast<const unsigned char*>(frame.start);

	for(size_t yy = start_y; yy < start_y+block_h_; ++yy)
	{
		for(size_t xx = start_x; xx < start_x+block_w_; ++xx)
		{
			size_t idx = width_ * xx + yy;
			r += buf[3*idx+0];
			g += buf[3*idx+1];
			b += buf[3*idx+2];
		}
	}

	float blk_size = float(block_h_*block_w_);
	r /= blk_size;
	g /= blk_size;
	b /= blk_size;

	float luma = 0.2126f*r + 0.7152f*g + 0.0722f*b;

	return {uint32_t(std::floor(r)), uint32_t(std::floor(g)), uint32_t(std::floor(b)), luma};
}

void AsciiConverter::update_console_size()
{
	// Get terminal size
	winsize size;
	ioctl(STDOUT_FILENO, TIOCGWINSZ, &size);

	cols_ = uint32_t(size.ws_col);
	rows_ = uint32_t(size.ws_row);
	block_w_ = width_ / cols_;
	block_h_ = height_ / rows_;
}


} // namespace camscii