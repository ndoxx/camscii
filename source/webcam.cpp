#include "webcam.h"
#include "common.h"

#include <kibble/logger/logger.h>

#include <errno.h>
#include <fcntl.h>
#include <libv4l2.h>
#include <linux/videodev2.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <sys/types.h>

namespace camscii
{

#define CLEAR(x) memset(&(x), 0, sizeof(x))

static void xioctl(int fh, unsigned long request, void* arg)
{
    int r;

    do
    {
        r = v4l2_ioctl(fh, request, arg);
    } while(r == -1 && ((errno == EINTR) || (errno == EAGAIN)));

    if(r == -1)
    {
        KLOGE("camscii") << "Error #" << errno << ": " << strerror(errno) << std::endl;
        exit(EXIT_FAILURE);
    }
}

struct Webcam::Pimpl
{
    std::string dev_name_ = "/dev/video0";
    int fd_ = -1;
    unsigned int n_bufs_ = 0;

    v4l2_format fmt_;
    v4l2_requestbuffers req_;
    v4l2_buffer buf_;
    v4l2_buf_type buf_type_;

    Frame* buffers_;
};

Webcam::Webcam() : impl_(new Pimpl) {}

Webcam::~Webcam()
{
    for(unsigned int ii = 0; ii < impl_->n_bufs_; ++ii)
        v4l2_munmap(impl_->buffers_[ii].start, impl_->buffers_[ii].length);
    v4l2_close(impl_->fd_);
}

uint32_t Webcam::get_image_width()
{
	return impl_->fmt_.fmt.pix.width;
}

uint32_t Webcam::get_image_height()
{
	return impl_->fmt_.fmt.pix.height;
}

bool Webcam::connect()
{
    impl_->fd_ = v4l2_open(impl_->dev_name_.c_str(), O_RDWR | O_NONBLOCK, 0);
    return impl_->fd_ > 0;
}

bool Webcam::set_format(uint32_t width, uint32_t height)
{
    CLEAR(impl_->fmt_);
    impl_->fmt_.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    impl_->fmt_.fmt.pix.width = width;
    impl_->fmt_.fmt.pix.height = height;
    impl_->fmt_.fmt.pix.pixelformat = V4L2_PIX_FMT_RGB24;
    impl_->fmt_.fmt.pix.field = V4L2_FIELD_INTERLACED;
    xioctl(impl_->fd_, VIDIOC_S_FMT, &impl_->fmt_);
    if(impl_->fmt_.fmt.pix.pixelformat != V4L2_PIX_FMT_RGB24)
    {
        KLOGE("camscii") << "Libv4l didn't accept RGB24 format. Can't proceed." << std::endl;
        return false;
    }
    if((impl_->fmt_.fmt.pix.width != width) || (impl_->fmt_.fmt.pix.height != height))
    {
        KLOGW("camscii") << "Warning: driver is sending image at " << impl_->fmt_.fmt.pix.width << 'x'
                         << impl_->fmt_.fmt.pix.height << std::endl;
    }
    return true;
}

bool Webcam::request_buffers()
{
    // Set buffers request
    CLEAR(impl_->req_);
    impl_->req_.count = 2;
    impl_->req_.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    impl_->req_.memory = V4L2_MEMORY_MMAP;
    xioctl(impl_->fd_, VIDIOC_REQBUFS, &impl_->req_);

    impl_->buffers_ = static_cast<Frame*>(calloc(impl_->req_.count, sizeof(*impl_->buffers_)));
    
    impl_->n_bufs_ = 0;
    for(impl_->n_bufs_ = 0; impl_->n_bufs_ < impl_->req_.count; ++impl_->n_bufs_)
    {
        CLEAR(impl_->buf_);

        impl_->buf_.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        impl_->buf_.memory = V4L2_MEMORY_MMAP;
        impl_->buf_.index = impl_->n_bufs_;

        xioctl(impl_->fd_, VIDIOC_QUERYBUF, &impl_->buf_);

        impl_->buffers_[impl_->n_bufs_].length = impl_->buf_.length;
        impl_->buffers_[impl_->n_bufs_].start = v4l2_mmap(NULL, impl_->buf_.length, PROT_READ | PROT_WRITE, MAP_SHARED, impl_->fd_, impl_->buf_.m.offset);

        if(MAP_FAILED == impl_->buffers_[impl_->n_bufs_].start)
        {
        	KLOGE("camscii") << "mmap error." << std::endl;
            return false;
        }
    }

    for(unsigned int ii = 0; ii < impl_->n_bufs_; ++ii)
    {
        CLEAR(impl_->buf_);
        impl_->buf_.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        impl_->buf_.memory = V4L2_MEMORY_MMAP;
        impl_->buf_.index = ii;
        xioctl(impl_->fd_, VIDIOC_QBUF, &impl_->buf_);
    }

    return true;
}

void Webcam::start_stream()
{
    impl_->buf_type_ = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    xioctl(impl_->fd_, VIDIOC_STREAMON, &impl_->buf_type_);
}

void Webcam::stop_stream()
{
    impl_->buf_type_ = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    xioctl(impl_->fd_, VIDIOC_STREAMOFF, &impl_->buf_type_);
}

void Webcam::capture()
{
    int r = -1;
    fd_set fds;
    timeval tv;

    do
    {
        FD_ZERO(&fds);
        FD_SET(impl_->fd_, &fds);

        /* Timeout. */
        tv.tv_sec = 2;
        tv.tv_usec = 0;

        r = select(impl_->fd_ + 1, &fds, NULL, NULL, &tv);
    } while((r == -1 && (errno = EINTR)));
    if(r == -1)
    {
        KLOGE("camscii") << "select error." << std::endl;
        exit(EXIT_FAILURE);
    }

    CLEAR(impl_->buf_);
    impl_->buf_.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    impl_->buf_.memory = V4L2_MEMORY_MMAP;

    xioctl(impl_->fd_, VIDIOC_DQBUF, &impl_->buf_);
    capture_callback_(impl_->buffers_[impl_->buf_.index], impl_->buf_.bytesused);
    xioctl(impl_->fd_, VIDIOC_QBUF, &impl_->buf_);
}


} // namespace camscii