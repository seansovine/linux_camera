/**
 * Receives raw video from H264 directly using Linux v4l2 API.
 *
 * Adapted from: https://gist.github.com/mik30s/6dd4eb42b2ec906e064d
 */

#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <linux/v4l2-common.h>
#include <linux/v4l2-controls.h>
#include <linux/videodev2.h>

#include <sys/ioctl.h>
#include <sys/mman.h>

#include <cassert>
#include <cstdint>
#include <iomanip>
#include <iostream>

int main() {
    int fd;
    fd = open("/dev/video4", O_RDWR);
    if (fd < 0) {
        perror("Failed to open device, OPEN");
        return 1;
    }

    v4l2_capability capability;
    if (ioctl(fd, VIDIOC_QUERYCAP, &capability) < 0) {
        // something went wrong... exit
        perror("Failed to get device capabilities, VIDIOC_QUERYCAP");
        return 1;
    }

    v4l2_format imageFormat;
    imageFormat.type                = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    imageFormat.fmt.pix.width       = 1920;
    imageFormat.fmt.pix.height      = 1080;
    imageFormat.fmt.pix.pixelformat = V4L2_PIX_FMT_H264;
    imageFormat.fmt.pix.field       = V4L2_FIELD_NONE;
    if (ioctl(fd, VIDIOC_S_FMT, &imageFormat) < 0) {
        perror("Device could not set format, VIDIOC_S_FMT");
        return 1;
    }

    v4l2_requestbuffers requestBuffer = {0};
    requestBuffer.count               = 1;
    requestBuffer.type                = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    requestBuffer.memory              = V4L2_MEMORY_MMAP;

    if (ioctl(fd, VIDIOC_REQBUFS, &requestBuffer) < 0) {
        perror("Could not request buffer from device, VIDIOC_REQBUFS");
        return 1;
    }

    v4l2_buffer queryBuffer = {0};
    queryBuffer.type        = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    queryBuffer.memory      = V4L2_MEMORY_MMAP;
    queryBuffer.index       = 0;

    if (ioctl(fd, VIDIOC_QUERYBUF, &queryBuffer) < 0) {
        perror("Device did not return the buffer information, VIDIOC_QUERYBUF");
        return 1;
    }
    std::cout << "Buffer length: " << queryBuffer.length << "." << std::endl;

    char *buffer = (char *)mmap(NULL, queryBuffer.length, PROT_READ | PROT_WRITE, MAP_SHARED, //
                                fd, queryBuffer.m.offset);
    memset(buffer, 0, queryBuffer.length);

    v4l2_buffer bufferInfo;
    memset(&bufferInfo, 0, sizeof(bufferInfo));
    bufferInfo.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    bufferInfo.memory = V4L2_MEMORY_MMAP;
    bufferInfo.index  = 0;

    uint32_t type = bufferInfo.type;
    if (ioctl(fd, VIDIOC_STREAMON, &type) < 0) {
        perror("Could not start streaming, VIDIOC_STREAMON");
        return 1;
    }
    std::cout << "Starting streaming...\n" << std::endl;

    // START STREAM HANDLING>

    static constexpr uint32_t NUM_FRAMES = 5;

    for (uint32_t frame_i = 0; frame_i < NUM_FRAMES; ++frame_i) {
        std::cout << "Starting to receive frame " << frame_i << std::endl;

        // Enqueueu the buffer; video driver  will fill it.
        if (ioctl(fd, VIDIOC_QBUF, &bufferInfo) < 0) {
            perror("Could not queue buffer, VIDIOC_QBUF");
            return 1;
        }

        // Will return when buffer is full with a frame.
        if (ioctl(fd, VIDIOC_DQBUF, &bufferInfo) < 0) {
            perror("Could not dequeue the buffer, VIDIOC_DQBUF");
            return 1;
        }

        std::cout << "Buffer flags: " << std::hex << std::setw(8) << std::setfill('0')
                  << bufferInfo.flags << std::dec << std::endl;
        assert(~(bufferInfo.flags & V4L2_BUF_FLAG_ERROR));

        std::cout << "Buffer frame sequence #: " << bufferInfo.sequence << std::endl;
        std::cout << "Bytes written into buffer: " << bufferInfo.bytesused << std::endl;
        std::cout << "Read full frame successfully.\n" << std::endl;
    }

    // TODO: Copy data from buffer pointer, of length bufferinfo.bytesused,
    //       for use by libavcodec for conversion to image format.

    // END STREAM HANDLING>

    if (ioctl(fd, VIDIOC_STREAMOFF, &type) < 0) {
        perror("Could not end streaming, VIDIOC_STREAMOFF");
        return 1;
    }
    std::cout << "Succes! Now exiting." << std::endl;

    munmap(buffer, queryBuffer.length);
    close(fd);
    return 0;
}
