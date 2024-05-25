/*****************************************************************************
* Copyright (C) 2023-2028 Hanson Yu  All rights reserved.
------------------------------------------------------------------------------
* File Module       :   RtmpParse.h
* Description       :   RtmpParse operation center
* Created           :   2023.09.21.
* Author            :   Yu Weifeng
* Function List     :   
* Last Modified     :   
* History           :   
******************************************************************************/
#ifndef RTMP_PARSE_H
#define RTMP_PARSE_H

#include "RtmpCommon.h"


#define AMF_OBJECT_ITEM_VALUE(v, amf_type, amf_name, amf_value, amf_size) { v.type=amf_type; v.name=amf_name; v.value=amf_value; v.size=amf_size; }


#define RTMP_STREAM_NAME_MAX_LEN    (4*1024)
#define RTMP_INPUT_CHUNK_MAX_SIZE   128 //默认块大小为 128 字节

typedef enum RtmpCMDEncodingAmf
{
    RTMP_CMD_ENCODING_AMF_0     = 0,
    RTMP_CMD_ENCODING_AMF_3     = 3,
}E_RtmpCMDEncodingAmf;




typedef struct RtmpConnectContent
{
    char strApp[RTMP_STREAM_NAME_MAX_LEN]; // Server application name, e.g.: testapp 128
    char strFlashVer[32]; // Flash Player version, FMSc/1.0
    char strSwfUrl[256]; // URL of the source SWF file
    char strTcUrl[RTMP_STREAM_NAME_MAX_LEN]; // URL of the Server, rtmp://host:1935/testapp/instance1 256
    unsigned char bFpad; // boolean: True if proxy is being used.
    double dlCapabilities; // double default: 15
    double dlAudioCodecs; // double default: 4071
    double dlVideoCodecs; // double default: 252
    double dlVideoFunction; // double default: 1
    double dlEncoding;
    char strPageUrl[256]; // http://host/sample.html
}T_RtmpConnectContent;

typedef struct RtmpPlayContent
{
    unsigned char bReset; 
    double dlStart; // the start time in seconds, [default] -2-live/vod, -1-live only, >=0-seek position
    double dlDuration; // duration of playback in seconds, [default] -1-live/record ends, 0-single frame, >0-play duration
    char strStreamName[RTMP_STREAM_NAME_MAX_LEN]; 

    double dlTransaction;//Transaction ID 
}T_RtmpPlayContent;

typedef struct RtmpPublishContent
{
    char strStreamName[RTMP_STREAM_NAME_MAX_LEN]; 
    char strStreamType[18]; // Publishing type: live/record/append

    double dlTransaction;//Transaction ID 
}T_RtmpPublishContent;

/*****************************************************************************
-Class          : RtmpParse
-Description    : 
* Modify Date     Version        Author           Modification
* -----------------------------------------------
* 2023/09/21      V1.0.0         Yu Weifeng       Created
******************************************************************************/
class RtmpParse
{
public:
    RtmpParse();
    virtual ~RtmpParse();
    int GetRtmpVersion(char *i_pcBuf,int i_iBufLen);
    int GetRtmpHeader(char *i_pcBuf,int i_iBufLen,T_RtmpChunkHeader *o_ptRtmpChunkHeader);
    int GetChunkHeaderLen(char *i_pcBuf,int i_iBufLen,unsigned int i_dwLastChunkTimestamp);
    int ParseCmdMsg(unsigned char *i_pbBuf,int i_iBufLen,char * o_strCommandBuf,int i_iCommandBufMaxLen,double * o_dlTransaction);
    int ParseControlMsg(unsigned char *i_pbBuf,int i_iBufLen,unsigned int *o_pdwControlData);
    int ParseCmdConnect(unsigned char *i_pbBuf,int i_iBufLen,T_RtmpConnectContent * o_ptRtmpConnectContent);
    int ParseCmdCreateStream(unsigned char *i_pbBuf,int i_iBufLen);
    int ParseCmdGetStreamLength(unsigned char *i_pbBuf,int i_iBufLen,char *o_strStreamName,int i_iNameMaxLen);
    int ParseCmdPlay(unsigned char *i_pbBuf,int i_iBufLen,T_RtmpPlayContent * o_ptRtmpPlayContent);
    int ParseCmdPublish(unsigned char *i_pbBuf,int i_iBufLen,T_RtmpPublishContent * o_ptRtmpPublishContent);

private:
    int GetRtmpBasicHeader(char *i_pcBuf,int i_iBufLen,T_RtmpBasicHeader *o_ptBasicHeader);
    int GetRtmpMsgHeader(char *i_pcBuf,int i_iBufLen,unsigned char i_bChunkType,T_RtmpMsgHeader *o_ptRtmpMsgHeader);
    int GetExtendedTimestamp(char *i_pcBuf,int i_iBufLen,unsigned int *o_dwExtendedTimestamp);

    int FindLastRtmpChunkHeader(unsigned int i_dwChunkStreamID);
    int GetLastRtmpChunkHeader(unsigned int i_dwChunkStreamID,T_RtmpChunkHeader * o_ptRtmpChunkHeader);
    int SetLastRtmpChunkHeader(unsigned int i_dwChunkStreamID,T_RtmpChunkHeader * i_ptRtmpChunkHeader);






    T_RtmpChunkHeader m_atLastRtmpChunkHeader[RTMP_CHUNK_STREAM_MAX_NUM];
};





#endif

