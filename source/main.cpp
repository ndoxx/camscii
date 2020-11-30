#include <iostream>

#include <kibble/logger/logger.h>
#include <kibble/logger/sink.h>
#include <kibble/logger/dispatcher.h>

#include "webcam.h"
#include "common.h"
#include "ascii_converter.h"

using namespace kb;

void init_logger()
{
    KLOGGER_START();
    KLOGGER(create_channel("camscii", 3));
    KLOGGER(attach_all("console_sink", std::make_unique<klog::ConsoleSink>()));
    KLOGGER(set_backtrace_on_error(false));
}

int main(int argc, char** argv)
{
    (void)argc;
    (void)argv;

    init_logger();

    camscii::Webcam cam;

    KLOGN("camscii") << "Connecting to webcam." << std::endl;
    if(!cam.connect())
    {
    	KLOGE("camscii") << "Cannot open device." << std::endl;
    	exit(EXIT_FAILURE);
    }

    KLOGN("camscii") << "Configuring device." << std::endl;
    if(!cam.set_format(640, 480))
    {
    	KLOGE("camscii") << "Configuration error, exiting." << std::endl;
    	exit(EXIT_FAILURE);
    }

    KLOGN("camscii") << "Requesting buffers." << std::endl;
    if(!cam.request_buffers())
    {
    	KLOGE("camscii") << "Buffer request error, exiting." << std::endl;
    	exit(EXIT_FAILURE);
    }

    uint32_t width = cam.get_image_width();
    uint32_t height = cam.get_image_height();

    /*cam.set_capture_callback([width,height](const camscii::Frame& frame, uint32_t nbytes)
    {
    	FILE* fout;
        fout = fopen("out.ppm", "w");
        if(!fout)
        {
    		KLOGE("camscii") << "Cannot open image, exiting." << std::endl;
            exit(EXIT_FAILURE);
        }
        fprintf(fout, "P6\n%d %d 255\n", width, height);
        fwrite(frame.start, nbytes, 1, fout);
        fclose(fout);
    });*/

    camscii::AsciiConverter cv(width, height);
	cam.set_capture_callback([&cv](const camscii::Frame& frame, uint32_t nbytes)
    {
    	cv.display(frame, nbytes);
    });

    cam.start_stream();

    for(int ii=0; ii<100; ++ii)
    	cam.capture();

    cam.stop_stream();

    return 0;
}
