//
// Created by S on 2024/9/25.
//
#include <pthread.h>
#include "include/RtspPlayer.h"


void *prepareThread(void *pVoid) {
    RtspPlayer *player = static_cast<RtspPlayer *>(pVoid);
    player->prepare_();
    return 0;
}

// 开始读包
void *startReadPacketThread(void *pVoid) {
    RtspPlayer *player = static_cast<RtspPlayer *>(pVoid);
    player->start_readPacket();
    return 0;
}

// 视频解码线程
void *videoDecoderThread(void *pVoid) {
    RtspPlayer *player = static_cast<RtspPlayer *>(pVoid);
    player->start_video();
    return 0;
}


// 视频播放线程
void *videoPlayThread(void *pVoid) {
    RtspPlayer *player = static_cast<RtspPlayer *>(pVoid);
    player->start_play_video();
    return 0;
}


// 音频解码线程
void *audioDecoderThread(void *pVoid) {
    // RtspPlayer *player = static_cast<RtspPlayer *>(pVoid);
    //player->start_audio();
    return 0;
}


RtspPlayer::RtspPlayer() {

}

RtspPlayer::RtspPlayer(const char *data_source, RtspNativeCallBack *callBack) {
    this->data_source = new char[strlen(data_source) + 1];
    strcpy(this->data_source, data_source);
    this->callBack = callBack;
}

RtspPlayer::~RtspPlayer() {
    if (this->data_source) {
        delete this->data_source;
        this->data_source = nullptr;
    }
}

void RtspPlayer::prepare() {
    pthread_create(&pid_prepare, 0, prepareThread, this);
}

void RtspPlayer::prepare_() {
    // 申请媒体流的上下文对象
    formatContext = avformat_alloc_context();

    avformat_network_init();
    // 
//    AVDictionary *dictionary = 0;
//    av_dict_set(&dictionary, "timeout", "5000000", 0);//单位是微妙

    // 打开流
    int ret = avformat_open_input(&formatContext, data_source, 0, nullptr);
    LOGE("avformat_open_input-> %d", ret);
//    av_dict_free(&dictionary);

    if (ret) {
        if (callBack) {
            callBack->onAction(THREAD_CHILD, FFMPEG_CAN_NOT_OPEN_URL);
            return;
        }
    }
    // 查找流
    ret = avformat_find_stream_info(formatContext, 0);
    if (ret < 0) {
        if (callBack) {
            callBack->onAction(THREAD_CHILD, FFMPEG_CAN_NOT_FIND_STREAMS);
            return;
        }
    }
    // 遍历流 找到对应的音视频流
    for (int i = 0; i < formatContext->nb_streams; ++i) {
        AVStream *stream = formatContext->streams[i];  // 获取流
        AVCodecParameters *codec_par = stream->codecpar; // 获取编码信息参数
        AVCodec *pCodec = avcodec_find_decoder(codec_par->codec_id); // 根据编码参数获取编解码器
        if (!pCodec) {
            callBack->onAction(THREAD_CHILD, FFMPEG_FIND_DECODER_FAIL);
            return;
        }
        AVCodecContext *pContext = avcodec_alloc_context3(pCodec); // 根据编解码器获取上线文
        if (!pContext) {
            callBack->onAction(THREAD_CHILD, FFMPEG_ALLOC_CODEC_CONTEXT_FAIL);
            return;
        }
        ret = avcodec_parameters_to_context(pContext, codec_par); // 给编解码器上下文赋值参数
        if (ret < 0) {
            callBack->onAction(THREAD_CHILD, FFMPEG_CODEC_CONTEXT_PARAMETERS_FAIL);
            return;
        }
        ret = avcodec_open2(pContext, pCodec, 0); // 打开解码器
        if (ret) {
            callBack->onAction(THREAD_CHILD, FFMPEG_OPEN_DECODER_FAIL);
            return;
        }
        AVRational baseTime = stream->time_base;
        // 判断音视频类型
        if (codec_par->codec_type == AVMEDIA_TYPE_VIDEO) {
            // 平均帧率 == 时间基  获取视频帧 fps
            videoContext = pContext;
            video_index = i;
        } else if (codec_par->codec_type == AVMEDIA_TYPE_AUDIO) {
            audioContext = pContext;
            audio_index = i;
        }
        // callBack->onAction(THREAD_CHILD, FFMPEG_OPEN_STATUE_OK);
    }
    LOGE("==================初始化成功======================");
}

void RtspPlayer::start() {
    //定义解码线程
    LOGE("=====开启音视频解码线程==========")
    pthread_create(&pid_start, 0, startReadPacketThread, this);
    pthread_create(&pid_video_decode, 0, videoDecoderThread, this);
    pthread_create(&pid_video_decode, 0, audioDecoderThread, this);
    pthread_create(&pid_video_play, 0, videoPlayThread, this);
}


// 视频解码线程
void RtspPlayer::start_video() {
    AVPacket *packet = 0;
    while (isPlaying) {
        if (isStop) {
            continue;
        }
        int ret = video_packages.pop(packet);
        if (!isPlaying) {
            LOGE(" 停止播放 isPlaying %d", isPlaying);
            break;
        }
        if (!ret) {
            continue;
        }

        // 解码
        ret = avcodec_send_packet(videoContext, packet);
        if (ret) {
            LOGE("avcodec_send_packet   ret %d", ret);
            // release();
            break;//失败了 -1094995529
        }
        av_packet_free(&packet);
        AVFrame *frame = av_frame_alloc();
        ret = avcodec_receive_frame(videoContext, frame);
        if (ret == AVERROR(EAGAIN)) {
            //从新取
            av_frame_free(&frame);
            continue;
        } else if (ret != 0) {
            LOGE("avcodec_receive_frame   ret %d", ret);
            av_frame_free(&frame);
            break;
        }

        LOGE("QUEUE SIZE: %d", video_packages.queueSize())
        video_frames.push(frame);
//        int width = videoContext->width;
//        int height = videoContext->height;
//
//        // 转换为适配 MediaCodec 的格式，例如 YUV420P
//        auto size = frame->linesize[0] * videoContext->height;
//        auto *data = (jbyte *) frame->data[0];
//        callBack->onVideo(THREAD_CHILD, data, size, width, height);
//        av_frame_unref(frame);
    }

}


void RtspPlayer::setWindow(jobject surface) {
    if (nativeWindow) {
        ANativeWindow_release(nativeWindow);
        nativeWindow = 0;
    }


}


// 开始读取包
void RtspPlayer::start_readPacket() {
    while (isPlaying) {
        if (!formatContext) {
            LOGE("formatContext formatContext 已经被释放了");
            return;
        }
        if (isStop) {
            continue;
        }
        AVPacket *packet = av_packet_alloc();

        int ret = av_read_frame(formatContext, packet);

        if (!ret) {
            if (video_index == packet->stream_index) {
                video_packages.push(packet);
            } else if (audio_index == packet->stream_index) {
                // audio_packages.push(packet);
            }
        } else if (ret == AVERROR_EOF) {
            // 读取完成
            LOGE("stream_index 拆包完成 %s", "读取完成了");
            isPlaying = false;
            av_packet_free(&packet);
            break;
        } else {
            LOGE("stream_index 拆包 %s", "读取失败");
            av_packet_free(&packet);
            break;//读取失败
        }
    }
}

// 视频播放线程
void RtspPlayer::start_play_video() {


    // 初始化一个上下文将视频帧 像素格式转换
    SwsContext *sws_ctx = sws_getContext(videoContext->width, videoContext->height,
                                         videoContext->pix_fmt,
                                         videoContext->width, videoContext->height,
                                         AV_PIX_FMT_RGBA,
                                         SWS_BILINEAR, NULL, NULL, NULL);
    uint8_t *dst_data[4];
    int dst_linesize[4];
   // AVFrame *frame = 0;

    // 分配内存存储图片
    int ret = av_image_alloc(dst_data, dst_linesize, videoContext->width, videoContext->height,
                             AV_PIX_FMT_RGBA, 1);
    if (ret < 0) {
        LOGE("========Failed to allocate========")
        return;
    }
    while (isPlaying) {
        if (isStop) {
            continue;
        }
        AVFrame *frame = nullptr;
        ret = video_frames.pop(frame);
        if (!isPlaying) {
            av_frame_free(&frame);
            LOGE(" 停止播放 isPlaying %d", isPlaying);
            break;
        }
        if (!ret) {
            av_frame_free(&frame);
            continue;
        }
        // 将源图像转frame->data 转换为目标格式 RGBA
        sws_scale(sws_ctx, frame->data, frame->linesize, 0, videoContext->height, dst_data,
                  dst_linesize);
        if (videoCallback) {
            videoCallback(dst_data[0], videoContext->width, videoContext->height, dst_linesize[0]);
        }
        av_frame_free(&frame);
    }
    //av_frame_free(&frame);

}

void RtspPlayer::setVideoRenderCallback(VideoRenderCallback videoCallBack) {
    this->videoCallback = videoCallBack;
}

void RtspPlayer::release() {
    isPlaying = false; // 通知线程停止
    pthread_join(pid_start, nullptr);
    pthread_join(pid_video_decode, nullptr);
    pthread_join(pid_video_play, nullptr);
    videoCallback = nullptr;
    if (formatContext) {
        avformat_close_input(&formatContext);
        formatContext = nullptr;
    }
    avformat_network_deinit();

    if (videoContext) {
        avcodec_free_context(&videoContext);
        videoContext = nullptr;
    }
    if (audioContext) {
        avcodec_free_context(&audioContext);
        audioContext = nullptr;
    }

}

