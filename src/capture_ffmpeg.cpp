/**
 * Following example to stream video from H264 device using ffmpeg
 * libavdevice to read from a camera using underlying Linux v4l
 * driver, and converting from H264 to JPEG using libavcodec.
 *
 * We assume the following video stream parameters:
 *
 *  FormatInfo desired_format = {
 *       .width  = 1920,
 *       .height = 1080,
 *       .fps    = 30,
 *   };
 *
 * though these could also be set dynamically from device info.
 */

#include <cassert>
#include <cstdint>
#include <format>
#include <fstream>
#include <iostream>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavcodec/packet.h>
#include <libavdevice/avdevice.h>
#include <libavutil/frame.h>
}

struct H264ToJPEGInfo {
    const AVCodec *decoder         = nullptr;
    AVCodecContext *decode_context = nullptr;

    const AVCodec *encoder         = nullptr;
    AVCodecContext *encode_context = nullptr;
};

struct FormatInfo {
    uint32_t width;
    uint32_t height;
    uint32_t fps;
};

struct TranscodeData {
    AVFrame *h264_frame   = nullptr;
    AVPacket *h264_packet = nullptr;
    AVPacket *jpeg_packet = nullptr;
};

int setup_transcoder(H264ToJPEGInfo &transcoder_info, const FormatInfo &format_info) {
    // --------------
    // Setup decoder.

    const AVCodec *decoder = avcodec_find_decoder(AV_CODEC_ID_H264);
    if (!decoder) {
        std::cerr << "Error: H.264 decoder not found." << std::endl;
        return -1;
    }

    AVCodecContext *decode_context = avcodec_alloc_context3(decoder);
    if (!decode_context) {
        std::cerr << "Error: Failed to allocate decoder context." << std::endl;
        return -1;
    }

    if (avcodec_open2(decode_context, decoder, NULL) < 0) {
        std::cerr << "Error: Failed to open H.264 decoder." << std::endl;
        avcodec_free_context(&decode_context);
        return -1;
    }

    // --------------
    // Setup encoder.

    const AVCodec *encoder = avcodec_find_encoder(AV_CODEC_ID_MJPEG);
    if (!encoder) {
        std::cerr << "Error: MJPEG encoder not found." << std::endl;
        avcodec_free_context(&decode_context);
        return -1;
    }

    AVCodecContext *encode_context = avcodec_alloc_context3(encoder);
    if (!encode_context) {
        std::cerr << "Error: Failed to allocate encoder context." << std::endl;
        return -1;
    }

    encode_context->width     = format_info.width;
    encode_context->height    = format_info.height;
    encode_context->time_base = (AVRational){1, (int)format_info.fps};

    // We choose the first supported format.
    encode_context->pix_fmt = encoder->pix_fmts[0];

    if (avcodec_open2(encode_context, encoder, NULL) < 0) {
        std::cerr << "Error: Failed to open MJPEG encoder." << std::endl;
        avcodec_free_context(&encode_context);
        avcodec_free_context(&decode_context);
        return -1;
    }

    // --------
    // Success.

    transcoder_info.decoder        = decoder;
    transcoder_info.decode_context = decode_context;

    transcoder_info.encoder        = encoder;
    transcoder_info.encode_context = encode_context;

    return 0;
}

int check_format(AVFormatContext *format_context, uint32_t stream_index) {
    assert(stream_index < format_context->nb_streams);
    AVStream *stream                = format_context->streams[stream_index];
    AVCodecParameters *codec_params = stream->codecpar;
    enum AVCodecID codec_id         = codec_params->codec_id;
    enum AVPixelFormat pixel_format = (enum AVPixelFormat)codec_params->format;

    std::cout << " - Stream format (expect H.264): " << avcodec_get_name(codec_id) << " ("
              << static_cast<unsigned>(codec_id) << ")." << std::endl;
    assert(codec_id == AV_CODEC_ID_H264);

    std::cout << " - Verifying pixel format (expect 0 = YUV420P): " << pixel_format << std::endl;
    // assert(pixel_format == AV_PIX_FMT_YUV420P);
    // This may not actually matter as the decoder is flexible.

    return 0;
}

int do_transcode(H264ToJPEGInfo &transcoder_info, TranscodeData &transcode_data) {
    // ---------------------------------
    // Convert raw H264 packet to frame.

    AVPacket *h264_packet = transcode_data.h264_packet;
    AVFrame *h264_frame   = transcode_data.h264_frame;
    AVPacket *jpeg_packet = transcode_data.jpeg_packet;

    avcodec_send_packet(transcoder_info.decode_context, h264_packet);

    int ret = avcodec_receive_frame(transcoder_info.decode_context, h264_frame);
    if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
        std::cerr << "Error: Not enough data or EOF during decode." << std::endl;
        return -1;
    } else if (ret < 0) {
        std::cerr << "Error: Failed to decode h264 packet." << std::endl;
        return -1;
    }

    // ----------------------------------
    // Convert H264 frame to JPEG packet.

    ret = avcodec_send_frame(transcoder_info.encode_context, h264_frame);
    if (ret < 0) {
        std::cerr << "Error: Error sending frame to encoder." << std::endl;
        return -1;
    }

    static int frame_num = 0;
    std::string filename = std::format("scratch/output_{}.jpg", frame_num);

    ret = avcodec_receive_packet(transcoder_info.encode_context, jpeg_packet);
    if (ret < 0) {
        std::cerr << "Error: Failed to decode H264 packet to JPEG." << std::endl;
        return -1;
    }

    // --------------------
    // Write image to file.

    std::ofstream outFile(filename, std::ios::binary);
    assert(outFile);
    outFile.write(reinterpret_cast<const char *>(jpeg_packet->data), jpeg_packet->size);

    std::cout << std::format("Successfully encoded output and saved to  {} ({} bytes)\n", filename,
                             jpeg_packet->size)
              << std::endl;
    frame_num++;

    return 0;
}

int main() {
    avdevice_register_all();

    const char *device_driver = "v4l2";
    const char *device_name   = "/dev/video4";

    const AVInputFormat *input_format = av_find_input_format(device_driver);
    if (!input_format) {
        std::cerr << "Error: Could not find device driver." << std::endl;
        return -1;
    }

    // -------------------
    // Initialize decoder.

    FormatInfo desired_format = {
        .width  = 1920,
        .height = 1080,
        .fps    = 30,
    };

    H264ToJPEGInfo decoder_info;
    if (int result = setup_transcoder(decoder_info, desired_format)) {
        return result;
    }

    TranscodeData transcode_data = {.h264_frame  = av_frame_alloc(),
                                    .h264_packet = av_packet_alloc(),
                                    .jpeg_packet = av_packet_alloc()};

    if (!transcode_data.h264_frame || !transcode_data.h264_packet || !transcode_data.jpeg_packet) {
        std::cerr << "Error: Failed to allocate transcode packet and frame data." << std::endl;
        return -1;
    }

    // ------------------------------------
    // Setup device for reading and verify.

    AVDictionary *options = nullptr;
    av_dict_set(&options, "video_size", "1920x1080", 0);
    av_dict_set(&options, "framerate", "30", 0);

    AVFormatContext *format_context = nullptr;
    std::cout << "Opening device..." << std::endl;

    if (avformat_open_input(&format_context, device_name, input_format, &options) < 0) {
        std::cerr << "Error: Could not open input device." << std::endl;
        av_dict_free(&options);
        return -1;
    }
    av_dict_free(&options);

    if (avformat_find_stream_info(format_context, nullptr) < 0) {
        std::cerr << "Error: Could not find stream information." << std::endl;
        avformat_close_input(&format_context);
        return -1;
    }

    // Our device has one video stream, so we only read its packets.
    static constexpr int STREAM_INDEX = 0;

    std::cout << "Verifying stream format..." << std::endl;
    check_format(format_context, STREAM_INDEX);
    std::cout << "Dumping format info..." << std::endl;
    av_dump_format(format_context, STREAM_INDEX, device_name, 0);

    // -------------------
    // Start receive data.

    static constexpr int FRAMES_TO_CAPTURE = 10;
    int frames_remaining                   = FRAMES_TO_CAPTURE;

    std::cout << "\nStarting packet capture..." << std::endl;

    AVPacket *packet = transcode_data.h264_packet;
    while (frames_remaining > 0) {
        if (av_read_frame(format_context, packet) >= 0) {
            std::cout << "Captured Packet: " << std::endl;
            std::cout << " - Size: " << packet->size << " bytes" << std::endl;

            // Verify assumption.
            if (packet->stream_index != 0) {
                std::cerr << "Warning: Received packet from steam other than 0; dropping packet."
                          << std::endl;
                continue;
            }

            // TODO: Send packet data to decoder to get image to write or display.
            do_transcode(decoder_info, transcode_data);
            std::cout << "> Decode successful." << std::endl;

            av_packet_unref(packet);
            frames_remaining--;
        } else {
            std::cerr << "Warning: Failed to read frame or end of stream reached." << std::endl;
            break;
        }
    }

    // -----------------
    // End receive data.

    std::cout << "\nCleaning up..." << std::endl;

    avformat_close_input(&format_context);

    av_frame_free(&transcode_data.h264_frame);
    av_packet_free(&transcode_data.h264_packet);
    av_packet_free(&transcode_data.jpeg_packet);

    std::cout << "Done!" << std::endl;

    return 0;
}
