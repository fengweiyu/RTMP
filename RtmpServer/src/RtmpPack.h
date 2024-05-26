/*****************************************************************************
* Copyright (C) 2023-2028 Hanson Yu  All rights reserved.
------------------------------------------------------------------------------
* File Module       :   RtmpPack.h
* Description       :   RtmpPack operation center
* Created           :   2023.09.21.
* Author            :   Yu Weifeng
* Function List     :   
* Last Modified     :   
* History           :   
******************************************************************************/
#ifndef RTMP_PACK_H
#define RTMP_PACK_H


#include "RtmpCommon.h"
#include "RtmpMediaHandle.h"

#define RTMP_C0_LEN 1
#define RTMP_C1_LEN 1536
#define RTMP_C2_LEN 1536

#define RTMP_S0_LEN RTMP_C0_LEN
#define RTMP_S1_LEN RTMP_C1_LEN
#define RTMP_S2_LEN RTMP_C2_LEN

#define RTMP_S0S1S2_LEN (RTMP_S0_LEN+RTMP_S1_LEN+RTMP_S2_LEN)

#define RTMP_CTL_MSG_LEN 12

#define RTMP_LEVEL_WARNING  "warning"
#define RTMP_LEVEL_STATUS   "status"
#define RTMP_LEVEL_ERROR    "error"
#define RTMP_LEVEL_FINISH   "finish" // ksyun cdn

#define RTMP_FMSVER             "FMS/3,0,1,123"
#define RTMP_CAPABILITIES       31
#define RTMP_OUTPUT_CHUNK_SIZE     (1460-20) //��С128��mtu��Ҫ��ȥ20 chunkͷ����С
#define RTMP_CMD_MAX_LEN        4096
#define RTMP_DATA_MAX_LEN       4096




typedef enum RtmpMessageStreamID
{
    RTMP_MSG_STREAM_ID0 = 0x00,
    RTMP_MSG_STREAM_ID1,//default stream id for response,����ʵ��ץ���������ͨ������Ƶ�����Ϣ����ظ���1
}E_RtmpMessageStreamID;//����ǽ���Ȼ��ظ���Ϣ��id��������Ƿ��ͷ���id

enum
{
    RTMP_BANDWIDTH_LIMIT_HARD = 0,
    RTMP_BANDWIDTH_LIMIT_SOFT = 1,
    RTMP_BANDWIDTH_LIMIT_DYNAMIC = 2,
};
// 7.1.7. User Control Message Events (p27)
typedef enum RtmpMessageEvents
{
    RTMP_MSG_EVENT_STREAM_BEGIN         = 0,
    RTMP_MSG_EVENT_STREAM_EOF           = 1,
    RTMP_MSG_EVENT_STREAM_DRY           = 2,
    RTMP_MSG_EVENT_SET_BUFFER_LENGTH    = 3,
    RTMP_MSG_EVENT_STREAM_IS_RECORD     = 4,

    RTMP_MSG_EVENT_PING                 = 6, // RTMP_EVENT_PING_REQUEST
    RTMP_MSG_EVENT_PONG                 = 7, // RTMP_EVENT_PING_RESPONSE

    // https://www.gnu.org/software/gnash/manual/doxygen/namespacegnash_1_1rtmp.html
    RTMP_MSG_EVENT_REQUEST_VERIFY       = 0x1a,
    RTMP_MSG_EVENT_RESPOND_VERIFY       = 0x1b,
    RTMP_MSG_EVENT_BUFFER_EMPTY         = 0x1f,
    RTMP_MSG_EVENT_BUFFER_READY         = 0x20,
}E_RtmpMessageEvents;

typedef struct RtmpPackCb
{
    long (*GetRandom)();

}T_RtmpPackCb;

typedef struct RtmpCmdConnectReply
{
    double dlTransactionID;
    const char* strFmsVer;
    double dlCapabilities;
    const char* strCode;
    const char* strLevel;
    const char* strDescription;
    double dlEncoding;
}T_RtmpCmdConnectReply;

typedef struct RtmpCmdOnStatus
{
    double dlTransactionID;
    bool blIsSuccess;
    const char* strSuccess;
    const char* strFail;
    const char* strDescription;
}T_RtmpCmdOnStatus;

typedef struct RtmpMediaInfo
{
    E_RTMP_ENC_TYPE eEncType;
    E_RTMP_FRAME_TYPE eFrameType;
    uint64 ddwTimestamp;
    unsigned int dwFrameRate;
    unsigned int dwWidth;
    unsigned int dwHeight;
    double dlDuration;//0.0
    unsigned int dwSampleRate;//44100 dwSamplesPerSecond
    unsigned int dwChannels;
    unsigned int dwBitsPerSample;
    //double dlVideoDatarate;//0.0
    //double dlFileSize;//0.0
    unsigned short wSpsLen;
    unsigned char abSPS[RTMP_SPS_MAX_SIZE];//����nalu type ��ȥ����00 00 00 01��
    unsigned short wPpsLen;
    unsigned char abPPS[RTMP_PPS_MAX_SIZE];
    unsigned short wVpsLen;
    unsigned char abVPS[RTMP_VPS_MAX_SIZE];
}T_RtmpMediaInfo;

typedef struct RtmpChunkPayloadInfo
{
    char * pcChunkPayload;
    int iPayloadLen;
    unsigned int dwOutChunkSize;//�ְ���С
    unsigned int dwStreamID;//������Ϣ����streamID
    int iProcessedLen;// (�ְ���תʹ�ã��ⲿ���踳ֵ)
    unsigned int dwTimestamp; // (�ְ���תʹ�ã��ⲿ���踳ֵ)�ְ�(��)ʱʹ�ã��ֿ��ÿ����Ϣ���ʱ������͵�һ��һ��
}T_RtmpChunkPayloadInfo;


/*****************************************************************************
-Class          : RtmpPack
-Description    : 
* Modify Date     Version        Author           Modification
* -----------------------------------------------
* 2023/09/21      V1.0.0         Yu Weifeng       Created
******************************************************************************/
class RtmpPack
{
public:
    RtmpPack(T_RtmpPackCb *i_ptRtmpPackCb);
    virtual ~RtmpPack();

    int CreateS0S1S2(char *i_pcC1,int i_iC1Len,char *o_pcBuf,int i_iMaxBufLen);
    
    int CreateAcknowledgement(char* o_pcBuf, int i_iBufMaxLen,unsigned int i_iWindowSize);
    int CreateWindowAckSize(char* o_pcBuf, int i_iBufMaxLen,unsigned int i_iWindowSize);
    int CreateSetPeerBandwidth(char* o_pcBuf, int i_iBufMaxLen,unsigned int i_iPeerBandwidth, unsigned char  i_bLimitType);
    int CreateSetChunkSize(char* o_pcBuf, int i_iBufMaxLen,unsigned int i_dwChunkSize);
    
    int CreateCmdConnectReply(T_RtmpCmdConnectReply *i_ptRtmpCmdConnectReply, unsigned char* o_pbBuf, int i_iBufMaxLen);
    int CreateCmdCreateStreamReply(double i_dlTransaction,unsigned char* o_pbBuf, int i_iBufMaxLen);
    int CreateCmdGetStreamLengthReply(double i_dlTransaction,double i_dlStreamDuration,unsigned char* o_pbBuf, int i_iBufMaxLen);
    int CreateCmdOnStatus(T_RtmpCmdOnStatus *i_ptRtmpCmdOnStatus,unsigned char* o_pbBuf, int i_iBufMaxLen);
    int CreateCmdMsg(T_RtmpChunkPayloadInfo *i_ptChunkPayloadInfo,char* o_pcBuf, int i_iBufMaxLen);
    int CreateCmd2Msg(T_RtmpChunkPayloadInfo *i_ptChunkPayloadInfo,char* o_pcBuf, int i_iBufMaxLen);

    
    int CreateEventStreamBegin(char* o_pcBuf, int i_iBufMaxLen);
    int CreateEventStreamIsRecord(char* o_pcBuf, int i_iBufMaxLen);
    
    int CreateDataSampleAccess(unsigned char* o_pcBuf, int i_iBufMaxLen);
    int CreateDataOnMetaData(T_RtmpMediaInfo *i_ptRtmpMediaInfo,unsigned char* o_pbBuf, int i_iBufMaxLen);
    int CreateDataMsg(T_RtmpChunkPayloadInfo *i_ptChunkPayloadInfo,char* o_pcBuf, int i_iBufMaxLen);
    
    int CreateVideoMsg(unsigned int i_dwTimestamp,T_RtmpChunkPayloadInfo *i_ptChunkPayloadInfo,char* o_pcBuf, int i_iBufMaxLen);
    int CreateAudioMsg(unsigned int i_dwTimestamp,T_RtmpChunkPayloadInfo *i_ptChunkPayloadInfo,char* o_pcBuf, int i_iBufMaxLen);



private:

    int CreateEventStream(unsigned short i_wEvent,char* o_pcBuf, int i_iBufMaxLen);
    
    int CreateRtmpPacket(T_RtmpChunkHeader *i_ptRtmpChunkHeader,T_RtmpChunkPayloadInfo *i_ptChunkPayloadInfo,char* o_pcBuf, int i_iBufMaxLen);
    
    int ChunkHeaderCompress(T_RtmpChunkHeader *i_ptRtmpChunkHeader,T_RtmpChunkHeader *o_ptRtmpChunkHeader);
    int SetRtmpChunkBasicHeader(unsigned char* out, unsigned char fmt, unsigned int id);
    int SetRtmpChunkMsgHeader(unsigned char* o_pbBuf, T_RtmpChunkHeader *i_ptRtmpChunkHeader);
    int SetRtmpChunkExtendedTimestamp(unsigned char* out, unsigned int timestamp);
    
    int CreateControlMsgHeader(char* o_pcBuf, int i_iBufMaxLen,unsigned char i_bMsgTypeID,int i_iMsgLen);
    
    int FindLastRtmpChunkHeader(unsigned int i_dwChunkStreamID);
    int GetLastRtmpChunkHeader(unsigned int i_dwChunkStreamID,T_RtmpChunkHeader * o_ptRtmpChunkHeader);
    int SetLastRtmpChunkHeader(unsigned int i_dwChunkStreamID,T_RtmpChunkHeader * i_ptRtmpChunkHeader);

    int RandomGenerate(char* o_pcBuf, int i_iBufLen);





    T_RtmpPackCb m_tRtmpPackCb;
    T_RtmpChunkHeader m_atLastRtmpChunkHeader[RTMP_CHUNK_STREAM_MAX_NUM];
    
};



















#endif

