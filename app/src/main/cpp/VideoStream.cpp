//
// Created by S on 2024/9/11.
//
#include "VideoStream.h"
#include <cstring>


VideoStream::VideoStream() : m_frameLen(0),
                             videoCodec(nullptr),
                             pic_in(nullptr),
                             videoCallback(nullptr) {

}



/**
 * 根据配置重置视频编码器
 * @param width  宽度
 * @param height  高度
 * @param fps
 * @param bitrate  比特率
 * @return
 */
int VideoStream::setVideoEncInfo(int width, int height, int fps, int bitrate) {
    // 加锁确保线程安全
    std::lock_guard<std::mutex> l(m_mutex);
    // 计算每帧的像素数
    m_frameLen = width * height;

    // 判断视频编码器是否存在 存在则关闭并释放资源
    if (videoCodec) {
        x264_encoder_close(videoCodec);
        videoCodec = nullptr;
    }
    // 如果输入图片的结构体已经存在，则清理和释放资源
    if (pic_in) {
        x264_picture_clean(pic_in);
        delete pic_in;
        pic_in = nullptr;
    }
    // 设置x264参数
    x264_param_t param;
    // 使用 "ultrafast" 预设和 "zerolatency" 进行低延迟编码
    int ret = x264_param_default_preset(&param, "ultrafast", "zerolatency");
    if (ret < 0) {
        return ret;  // 失败 返回错误码
    }
    param.i_level_idc = 32; // 设置 H.264 的 Profile level 决定了编码视频的复杂度、分辨率、比特率、帧率等方面的限制
    param.i_csp = X264_CSP_I420;  // 设置输入格式为 YUV 4:2:0
    param.i_width = width;  // 设置编码宽度
    param.i_height = height;  // 设置编码高度

    param.i_bframe = 0; // 不使用B帧

    // 设置码率控制为 ABR (Average Bitrate) 模式
    param.rc.i_rc_method = X264_RC_ABR;
    param.rc.i_bitrate = bitrate / 1024;  // 设置目标码率，单位为 kbps
    param.rc.i_vbv_max_bitrate = bitrate / 1024 * 1.2;  // 设置最大码率
    param.rc.i_vbv_buffer_size = bitrate / 1024;  // 设置缓冲区大小，单位为 kbps


    // 设置帧率相关参数
    param.i_fps_num = fps;
    param.i_fps_den = 1;
    param.i_timebase_den = param.i_fps_num;  // 设置时间基准为帧率
    param.i_timebase_num = param.i_fps_den;

    param.b_vfr_input = 0;  // 禁用可变帧率，使用固定帧率
    param.i_keyint_max = fps * 2;  // 设置关键帧间隔（GOP 大小）
    param.b_repeat_headers = 1;  // 每个关键帧都附带 SPS/PPS 头

    param.i_threads = 1;  // 设置编码线程数为 1，适合实时视频编码
    //  param.i_threads = X264_THREADS_AUTO;  // 自动分配线程数以优化编码

    // 应用 "baseline" 配置文件
    ret = x264_param_apply_profile(&param, "baseline");
    if (ret < 0) {
        return ret;  // 如果应用失败，返回错误码
    }

    // 打开解码器
    videoCodec = x264_encoder_open(&param);
    if (!videoCodec) {
        return -1;  // 如果打开失败，返回错误码
    }

    // 分配并初始化输入图片结构
    pic_in = new x264_picture_t();
    x264_picture_alloc(pic_in, X264_CSP_I420, width, height);

    return ret;
}



/**
 *
 * @param sps  序列参数集
 * @param pps  图像集
 * @param sps_len  序列参数集合
 * @param pps_len  图像集合
 *
 * 该方法打包SPS PPS 信息到RTMP视频包中
 *
 */
void VideoStream::sendSpsPps(uint8_t *sps, uint8_t *pps, int sps_len, int pps_len) {
    // 计算RTMP包大小
    // 13字节是SPS固定头部大小 sps_len是 sps的长度
    // 3字节是pps数据的固定头长度 pps_len 是pps的长度
    // SPS 包含了编码序列的全部信息 在解码过程中对于所有图片都适用 如 分辨率，帧率，色彩空间等
    //     在视频开头或者I帧之前发送 确保正确解码
    // PPS 特定图像参数 量化参数和预测模式 每个图像帧之前发送 允许解码器在图像级别调整

    int bodySize = 13 + sps_len + 3 + pps_len;

    // 创建RTMP包 并分配内存大小
    auto *packet = new RTMPPacket();
    RTMPPacket_Alloc(packet, bodySize);

    // 初始化包体索引位置
    int i = 0;

    // 设置包的类型 0x17 表示 H.264 视频关键帧 (I 帧) 0x00 是 AVC 包的类型字段
    packet->m_body[i++] = 0x17;
    packet->m_body[i++] = 0x00;

    // 设置时间戳 sps pps 不需要时间戳
    packet->m_body[i++] = 0x00;
    packet->m_body[i++] = 0x00;
    packet->m_body[i++] = 0x00;

    // 设置AVC版本  0x01-> AVC 解码器版本
    packet->m_body[i++] = 0x01;

    // 设置AVC 编码设置
    packet->m_body[i++] = sps[1];   // profile_idc 指定编码所用的 H.264 配置档案（例如 Baseline, Main, High
    packet->m_body[i++] = sps[2];   // constraint_set_flags 用于表示是否启用特定的 H.264 规范功能
    packet->m_body[i++] = sps[3];  //  level_idc 指定视频流所用的 H.264 级别（例如 Level 3.0, Level 4.0）
    packet->m_body[i++] = 0xFF;   // 0xFF 是 NAL 单元长度的填充值

    // 设置SPS数据
    packet->m_body[i++] = 0xE1;   // 0xE1 表示 SPS NAL 单元

    // 写入SPS长度  满足 H.264 流的格式要求，使得解码器能够正确地解析和处理 SPS 数据。
    packet->m_body[i++] = (sps_len >> 8) & 0xFF;   // sps_len >> 8 提取高字节
    packet->m_body[i++] = sps_len & 0xFF;          //  sps_len & 0xFF 提取低字节
    // 将SPS数据复制到包体积中
    memcpy(&packet->m_body[i], sps, sps_len);
    i += sps_len;

    // 设置PPS 数据
    // 0x01 表示 PPS NAL 单元
    packet->m_body[i++] = 0x01;
    // 写入 PPS 数据的长度
    packet->m_body[i++] = (pps_len >> 8) & 0xFF;  // pps_len >> 8 提取高字节
    packet->m_body[i++] = (pps_len) & 0xFF;   // pps_len & 0xFF 提取低字节

    // 将PPS 复制到包体积中
    memcpy(&packet->m_body[i], pps, pps_len);

    // 设置RTMP包属性
    packet->m_packetType = RTMP_PACKET_TYPE_VIDEO; // 表示包类型是视频
    packet->m_nBodySize = bodySize; // 设置包大小
    packet->m_nChannel = 0x10;        // 视频通道ID

    // sps pps 不需要时间
    packet->m_nTimeStamp = 0;
    packet->m_hasAbsTimestamp = 0;  // 不使用绝对时间
    packet->m_headerType = RTMP_PACKET_SIZE_MEDIUM; //设置包大小为中等

    videoCallback(packet);


}



/**
 * 对视频进行编码
 * @param data
 * @param camera_type
 */
void VideoStream::encodeVideo(int8_t *data, int camera_type) {
    // 使用互斥锁来保证线程安全，防止多个线程同时访问共享资源
    std::lock_guard<std::mutex> l(m_mutex);
    if (!pic_in)
        return;  // 如果 pic_in 为空，返回


    // 根据不同的摄像头类型处理数据
    if (camera_type == 1) {
        // YUV 4:2:0 格式的排列方式，首先拷贝 Y 分量
        memcpy(pic_in->img.plane[0], data, m_frameLen);  // 处理 Y 分量
        // 处理 UV 分量，U 和 V 交替存储，奇数位是 U，偶数位是 V
        for (int i = 0; i < m_frameLen / 4; ++i) {
            *(pic_in->img.plane[1] + i) = *(data + m_frameLen + i * 2 + 1);  // U 分量
            *(pic_in->img.plane[2] + i) = *(data + m_frameLen + i * 2);  // V 分量
        }
    } else if (camera_type == 2) {
        int offset = 0;
        // 直接处理 YUV 4:2:0 数据，拷贝 Y 分量
        memcpy(pic_in->img.plane[0], data, (size_t) m_frameLen);  // Y 分量
        offset += m_frameLen;
        // 拷贝 U 分量
        memcpy(pic_in->img.plane[1], data + offset, (size_t) m_frameLen / 4);  // U 分量
        offset += m_frameLen / 4;
        // 拷贝 V 分量
        memcpy(pic_in->img.plane[2], data + offset, (size_t) m_frameLen / 4);  // V 分量
    } else {
        return;  // 未知摄像头类型，直接返回
    }

    // 开始编码
    x264_nal_t *pp_nal;  // 编码后的 NAL 单元
    int pi_nal;   // NAL 单元数量
    x264_picture_t pic_out;  // 编码后的输出图像

    // 用x264的编码函数 将pic_in 编码成NAL单元
    x264_encoder_encode(videoCodec, &pp_nal, &pi_nal, pic_in, &pic_out);

    // 提起并发送SPS 和 PPS
    int pps_len, sps_len = 0;
    uint8_t sps[100];
    uint8_t pps[100];
    for (int i = 0; i < pi_nal; ++i) {
        x264_nal_t nal = pp_nal[i];
        if (nal.i_type == NAL_SPS) {
            // 提取 SPS 数据并去掉前四个字节的起始码
            sps_len = nal.i_payload - 4;
            memcpy(sps, nal.p_payload + 4, static_cast<size_t>(sps_len));
        } else if (nal.i_type == NAL_PPS) {
            // 提取 PPS 数据并去掉前四个字节的起始码
            pps_len = nal.i_payload - 4;
            memcpy(pps, nal.p_payload + 4, static_cast<size_t>(pps_len));
            sendSpsPps(sps, pps, sps_len, pps_len);  // 发送 SPS 和 PPS 数据
        } else {
            // 发送帧数据
            sendFrame(nal.i_type, nal.p_payload, nal.i_payload);
        }
    }


}

void VideoStream::setVideoCallback(VideoStream::VideoCallback callback) {
    videoCallback = callback;
}

void VideoStream::sendFrame(int type, uint8_t *payload, int i_payload) {

//    if (payload[0] == 0x00 && payload[1] == 0x00 && payload[2] == 0x00 && payload[3] == 0x01) {
//        // 起始码为 0x00000001, 需要跳过 4 个字节
//        i_payload -= 4;
//        payload += 4;
//    } else if (payload[0] == 0x00 && payload[1] == 0x00 && payload[2] == 0x01) {
//        // 起始码为 0x000001, 需要跳过 3 个字节
//        i_payload -= 3;
//        payload += 3;
//    } else {
//        // 如果起始码不匹配，返回，避免继续处理错误数据
//        return;
//    }


    // 根据NAL单元的起始码判断 H.264的NAL起始码有两种0x000001 或 0x00000001
    if (payload[2] == 0x00) {
        // 起始码为 0x00000001, 需要跳过 4 个字节
        i_payload -= 4;
        payload += 4;
    } else {
        // 起始码为 0x000001, 需要跳过 3 个字节
        i_payload -= 3;
        payload += 3;
    }

    // 初始化RTMPacket 并设置大小为9(固定头大小) 加载NAL单元载荷大小
    int i = 0;
    int bodySize = 9 + i_payload;
    auto *packet = new RTMPPacket();
    RTMPPacket_Alloc(packet, bodySize);

    // 设置RTMP包的前缀部分，确定是关键帧(IDR)还是非关键帧
    if (type == NAL_SLICE_IDR) {
        packet->m_body[i++] = 0x17;  // 1 表示关键帧，7 表示 H.264 编码
    } else {
        packet->m_body[i++] = 0x27;  // 2 表示非关键帧，7 表示 H.264 编码
    }
    // 设置 AVC NALU，固定为 0x01 表示 NAL 单元类型
    packet->m_body[i++] = 0x01;

    // 设置时间戳，这里为 0，表示不使用时间戳
    packet->m_body[i++] = 0x00;
    packet->m_body[i++] = 0x00;
    packet->m_body[i++] = 0x00;

    // 设置负载的长度（i_payload），即 NAL 单元的大小
    packet->m_body[i++] = (i_payload >> 24) & 0xFF;
    packet->m_body[i++] = (i_payload >> 16) & 0xFF;
    packet->m_body[i++] = (i_payload >> 8) & 0xFF;
    packet->m_body[i++] = (i_payload) & 0xFF;

    // 将实际的NAL单元数据拷贝到RTMP包中
    memcpy(&packet->m_body[i], payload, static_cast<size_t>(i_payload));
    // 设置 RTMP 包的其他属性
    packet->m_hasAbsTimestamp = 0;  // 使用相对时间戳
    packet->m_nBodySize = bodySize;  // 设置 RTMP 包的总大小
    packet->m_packetType = RTMP_PACKET_TYPE_VIDEO;  // 表示这是视频数据包
    packet->m_nChannel = 0x10;  // 视频的默认 RTMP 通道 ID
    packet->m_headerType = RTMP_PACKET_SIZE_LARGE;  // 大包头（用于关键帧）

    // 通过回调函数将打包好的 RTMP 数据包发送出去
    videoCallback(packet);

}


VideoStream::~VideoStream() {
    if (videoCodec) {
        x264_encoder_close(videoCodec);
        videoCodec = nullptr;
    }

    if (pic_in) {
        x264_picture_clean(pic_in);
        delete pic_in;
        pic_in = nullptr;
    }


}


//void VideoStream::encodeVideo(int8_t *data, int camera_type) {
//    // 使用互斥锁来保证线程安全，防止多个线程同时访问共享资源
//    std::lock_guard<std::mutex> l(m_mutex);
//    if (!pic_in)
//        return;  // 如果 pic_in 为空，返回
//
//    if (camera_type == 1) {
//        // 改进后的块内存操作，减少逐个字节处理
//        memcpy(pic_in->img.plane[0], data, m_frameLen);  // 拷贝 Y 分量
//
//        // 优化 UV 分量的处理，将逐个字节操作改为块拷贝
//        int uv_plane_size = m_frameLen / 4;
//        uint8_t* src_uv = data + m_frameLen;
//        uint8_t* dst_u = pic_in->img.plane[1];
//        uint8_t* dst_v = pic_in->img.plane[2];
//        for (int i = 0; i < uv_plane_size; ++i) {
//            dst_u[i] = src_uv[i * 2 + 1];  // 拷贝 U 分量
//            dst_v[i] = src_uv[i * 2];      // 拷贝 V 分量
//        }
//    } else if (camera_type == 2) {
//        // 改进：减少每次 offset 计算的复杂度
//        memcpy(pic_in->img.plane[0], data, (size_t) m_frameLen);  // Y 分量
//        memcpy(pic_in->img.plane[1], data + m_frameLen, (size_t) m_frameLen / 4);  // U 分量
//        memcpy(pic_in->img.plane[2], data + m_frameLen + m_frameLen / 4, (size_t) m_frameLen / 4);  // V 分量
//    } else {
//        return;  // 未知摄像头类型，直接返回
//    }
//
//    // 编码步骤保持不变
//    x264_nal_t *pp_nal;
//    int pi_nal;
//    x264_picture_t pic_out;
//    x264_encoder_encode(videoCodec, &pp_nal, &pi_nal, pic_in, &pic_out);
//
//    // SPS 和 PPS 数据处理
//    int pps_len, sps_len = 0;
//    uint8_t sps[100];
//    uint8_t pps[100];
//    for (int i = 0; i < pi_nal; ++i) {
//        x264_nal_t nal = pp_nal[i];
//        if (nal.i_type == NAL_SPS) {
//            sps_len = nal.i_payload - 4;
//            memcpy(sps, nal.p_payload + 4, static_cast<size_t>(sps_len));
//        } else if (nal.i_type == NAL_PPS) {
//            pps_len = nal.i_payload - 4;
//            memcpy(pps, nal.p_payload + 4, static_cast<size_t>(pps_len));
//            sendSpsPps(sps, pps, sps_len, pps_len);  // 发送 SPS 和 PPS
//        } else {
//            sendFrame(nal.i_type, nal.p_payload, nal.i_payload);  // 发送帧数据
//        }
//    }
//}


