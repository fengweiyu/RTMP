/*****************************************************************************
* Copyright (C) 2023-2028 Hanson Yu  All rights reserved.
------------------------------------------------------------------------------
* File Module       :   RtmpCommon.h
* Description       :   RtmpCommon operation center 
                        协议格式定义
* Created           :   2023.09.21.
* Author            :   Yu Weifeng
* Function List     :   
* Last Modified     :   
* History           :   
******************************************************************************/
#ifndef RTMP_COMMON_H
#define RTMP_COMMON_H

#define RTMP_VERSION 3


#define RTMP_CHUNK_STREAM_MAX_NUM   8 // maximum chunk stream count,rtmp 信道数目种类最多8个

typedef enum RtmpChunkType
{
    RTMP_CHUNK_TYPE_0 = 0, // 11-bytes: timestamp(3) + length(3) + stream type(1) + stream id(4)
    RTMP_CHUNK_TYPE_1 = 1, // 7-bytes: delta(3) + length(3) + stream type(1)
    RTMP_CHUNK_TYPE_2 = 2, // 3-bytes: delta(3)
    RTMP_CHUNK_TYPE_3 = 3, // 0-byte
}E_RtmpChunkType;

typedef enum RtmpChunkStremID
{
    RTMP_CONTROL_CHANNEL_CSID = 0x02,// Protocol Control Messages (1,2,3,5,6) & User Control Messages Event (4)
    RTMP_COMMAND_CHANNEL_CSID,// RTMP_TYPE_INVOKE (20) & RTMP_TYPE_FLEX_MESSAGE (17)
    RTMP_COMMAND_CHANNEL2_CSID,// AMF0/AMF3 command message, invoke method and return the result, over NetConnection,rarely used, e.g. onStatus(NetStream.Play.Reset).
    RTMP_STREAM_CHANNEL_CSID,// The stream message(amf0/amf3), over NetStream.
    RTMP_VIDEO_CHANNEL_CSID,// RTMP_TYPE_VIDEO (9)
    RTMP_AUDIO_CHANNEL_CSID,// RTMP_TYPE_AUDIO (8)
    RTMP_STREAM2_CHANNEL_CSID,//The stream message(amf0/amf3), over NetStream, the midst state(we guess).rarely used, e.g. play("mp4:mystram.f4v")
    RTMP_MAX_CHANNEL_CSID,
}E_RtmpChunkStremID;

typedef enum RtmpMessageTypeID
{
    /* Protocol Control Messages */
    RTMP_MSG_TYPE_SET_CHUNK_SIZE = 1,
    RTMP_MSG_TYPE_ABORT = 2,
    RTMP_MSG_TYPE_ACKNOWLEDGEMENT = 3, // bytes read report
    RTMP_MSG_TYPE_WINDOW_ACK_SIZE = 5, // server bandwidth,WINDOW_ACKNOWLEDGEMENT_SIZE
    RTMP_MSG_TYPE_SET_PEER_BANDWIDTH = 6, // client bandwidth

    /* User Control Messages Event (4) */
    RTMP_MSG_TYPE_EVENT = 4,

    RTMP_MSG_TYPE_AUDIO = 8,
    RTMP_MSG_TYPE_VIDEO = 9,
    
    /* Data Message */
    RTMP_MSG_TYPE_FLEX_STREAM = 15, // AMF3
    RTMP_MSG_TYPE_DATA = 18, // AMF0

    /* Shared Object Message */
    RTMP_MSG_TYPE_FLEX_OBJECT = 16, // AMF3
    RTMP_MSG_TYPE_SHARED_OBJECT = 19, // AMF0

    /* Command Message */
    RTMP_MSG_TYPE_FLEX_MESSAGE = 17, // AMF3
    RTMP_MSG_TYPE_INVOKE = 20, // AMF0

    /* Aggregate Message */
    RTMP_MSG_TYPE_METADATA = 22,

    RTMP_MSG_TYPE_MAX,
}E_RtmpMsgTypeID;



typedef struct RtmpBasicHeader
{
    unsigned char bChunkType; // fmt,RTMP_CHUNK_TYPE_XXX
    unsigned int dwChunkStreamID; //csid, chunk stream id(22-bits)
}T_RtmpBasicHeader;
typedef struct RtmpMsgHeader
{
    unsigned int dwTimestamp; // Timestamp/delta(24-bits) / extended timestamp(32-bits)

    unsigned int dwLength; // message length (24-bits) 不含chunk头
    unsigned char bTypeID; // message type id

    unsigned int dwStreamID; // message stream id
}T_RtmpMsgHeader;

typedef struct RtmpChunkHeader
{
    T_RtmpBasicHeader tBasicHeader; 
    T_RtmpMsgHeader tMsgHeader; // extended timestamp(32-bits)会赋值到里面的timestamp
    //int iExtendedTimestamp;//主要用于LastRtmpChunkHeader,1表示上一包使用了扩展时间戳,0 否
}T_RtmpChunkHeader;



























#endif
