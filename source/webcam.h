#pragma once

#include <cstdint>
#include <string>
#include <memory>
#include <functional>

namespace camscii
{

struct Frame;
class Webcam
{
public:
	using CaptureCallback = std::function<void(const Frame&, uint32_t)>;

	Webcam();
	~Webcam();

	inline void set_capture_callback(CaptureCallback cb) { capture_callback_ = cb; }
	uint32_t get_image_width();
	uint32_t get_image_height();

	bool connect();
	bool set_format(uint32_t width, uint32_t height);
	bool request_buffers();

    void start_stream();
    void stop_stream();
    void capture();

private:
	struct Pimpl;
	std::shared_ptr<Pimpl> impl_;

	CaptureCallback capture_callback_ = [](const Frame&, uint32_t){};
};

} // namespace camscii