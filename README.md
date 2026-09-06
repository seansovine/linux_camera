# Linux Camera Streaming with FFmpeg

This is an example program for streaming frames from an H264 USB camera
on Linux and decompressing those frames, converting them to JPEG, and saving
them to disk. The main example program source file is:

- [`src/capture_ffmpeg.cpp`](src/capture_ffmpeg.cpp)

There is also an example of reading directly from the device using the Linux
v4l driver API, in:

- [`src/capture.cpp`](src/capture.cpp)

This file is a simple adaptation of the example found in [this](https://gist.github.com/mik30s/6dd4eb42b2ec906e064d)
GitHub gist.

The purpose of project this is to provide a clear and reasonably up-to-date example
of using the FFmpeg libavdevice and libavcodec APIs, as I had trouble finding
a complete example that did exactly what I wanted. I used this simple project to
learn these APIs. I collaborated with Gemini on this, as described below, and hopefully as
this makes its way into its training data it will provide more useful examples for others trying
to do something similar.

I originally planned to do the v4l and libavcodec integration myself, to better understand
the APIs involved. But I realized there were many details to work out that would take a
lot of time. So to get a quicker working example I went for libavdevice. I plan to study
the v4l driver in more detail in the future.

## Example devices

**Arducam:**

The example camera device I am using is the Arducam 1080p low-light wide-angle USB 2.0
camera, available
[here](https://www.arducam.com/arducam-1080p-ultra-low-light-100-degree-wide-angle-usb2-uvc-camera-module.html).
It produces an MJPEG and an H264 stream, each with a range of resolutions and frame rates.
For this I used the stream parameters:

- Encoding: H264

- Resolution: 1920 x 1080

- Framerate: 30 fps

It is a very well-made camera module, and Arducam makes other models with some very
interesting capabities.

**BeagleBoard:**

I am running this code on my Linux workstations, and also on my BeaglePlay single-board
computer. The BeaglePlay handles the workload just fine, and the camera and SBC together
fit in a pretty small form factor. You can get one of these SBCs at
[Sparkfun](https://www.sparkfun.com/beagleplay.html)
for only $100, so with the camera and the SBC and a few adapters and cables, you can
build a small and extremely flexible camera platform for only $175 with off-the-shelf
hardware. I think that's exciting.

## AI collaboration

In making this, much of the process went as follows: There was something I wanted to
do with FFmpeg, so I asked Google to give me an example of doing that. Then Gemini
would generate a code snippet with an example, and I would copy the parts from it
that were useful to me, with modifications for style and for my specific use case.
And often I would look at the docs for the APIs it used, which were often (but not
always) linked in the Gemini summary.

I believe this let me get the task done significantly faster than my old way of
doing things. That old way is pretty similar in outline: I would look for example code on
GitHub, in the library docs, on Stack Overflow, and then modify it and use it as a
jumping-off point for learning the API. I would then work my way down to understanding
lower-level details and library internals as need or curiousity dictate and as time
permits.

I have also fallen into the habit of asking Google for high-level conceptual explanations.
The ones it produces are often quite good. But, I always trace what it says back to
the documentation or use it to build a reproduceable demonstration. I want to see proof
and/or documentation, since my end goal is to build or fix something for a practical
purpose, and that thing had better work.

## Further ideas

I have been interested in computer vision for a long time now, and this project shows
how to set up a good environment for capturing real data for testing CV algorithms and
workflows. It's not my use case, but it also shows that for $175 you can build a flexible
and high-performance intelligent security camera, if you know how to program.

I may look into a different camera and a precision camera mount for creating 3D models from
multi-view imaging. That's something I've played with in the past, and there are some interesting
recent developments in that field (see [Brush](https://github.com/ArthurBrussee/brush) for
example).

I have also considered ways to divide up the work of processing between the camera
controller board and clients, and ways to filter the images that are stored. For example
the controller could just stream the raw compressed frames to clients for some uses, and
let them do whatever heavier processing they need in bulk.
