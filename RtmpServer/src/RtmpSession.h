/*****************************************************************************
* Copyright (C) 2023-2028 Hanson Yu  All rights reserved.
------------------------------------------------------------------------------
* File Module       :   RtmpSession.h
* Description       :   RtmpSession operation center
* Created           :   2023.09.21.
* Author            :   Yu Weifeng
* Function List     :   
* Last Modified     :   
* History           :   
******************************************************************************/
#ifndef RTMP_SESSION_H
#define RTMP_SESSION_H

//#include "TcpSocket.h"
#include "RtmpPack.h"
#include "RtmpParse.h"
#include "RtmpMediaHandle.h"
#include <string.h>
#include <map>
#include <string>

using std::string;
using std::map;
using std::make_pair;


#define RTMP_MSG_MAX_LEN        4*1024*1024 //4m大小，考虑最大的i帧导致的video msg大小
#define RTMP_CHUNK_MAX_LEN        10*1024 //
#define RTMP_PLAY_SRC_MAX_LEN RTMP_STREAM_NAME_MAX_LEN
#define RTMP_CHUNK_MIN_LEN       12 //服务器拉流时必须处理最小12的报文(connect报文当ip为localhost时),chunk header最大的大小18字节，有的msg只有5字节,待长期验证

typedef enum RtmpMediaType
{
    RTMP_MEDIA_TYPE_VIDEO,
    RTMP_MEDIA_TYPE_ADUIO,
    RTMP_MEDIA_TYPE_META,

}E_RtmpMediaType;
typedef enum RtmpHandshakeState
{
    RTMP_HANDSHAKE_INIT,
    RTMP_HANDSHAKE_C0C1,
    RTMP_HANDSHAKE_C2,
    RTMP_HANDSHAKE_DONE,
}E_RtmpHandshakeState;
typedef enum RtmpChunkHandleState
{
    RTMP_CHUNK_HANDLE_INIT=0,
    RTMP_CHUNK_HANDLE_BASIC_HEADER,
    RTMP_CHUNK_HANDLE_MSG_HEADER,
    RTMP_CHUNK_HANDLE_EX_TIMESTAMP,
    RTMP_CHUNK_HANDLE_CHUNK_HEADER,
    RTMP_CHUNK_HANDLE_CHUNK_BODY,
}E_RtmpChunkHandleState;

typedef struct RtmpMsgBufHandle
{
    T_RtmpChunkHeader tRtmpChunkHeader;
    int iChunkHeaderLen;
    char * pbMsgBuf;//RTMP_MSG_MAX_LEN
    int iMsgBufLen;
}T_RtmpMsgBufHandle;

typedef struct RtmpChunkHandle
{
    T_RtmpChunkHeader tRtmpChunkHeader;
    int iChunkHeaderLen;
    char * pbChunkBuf;//pbChunkBuf包含Header
    int iChunkCurLen;//iChunkCurLen包含Header
    int iChunkProcessedLen;//已处理的数据长度
    int iChunkMaxLen;
    E_RtmpChunkHandleState eState;
    int iChunkBasicHeaderLen;
    int iChunkMsgHeaderLen;
    int iChunkRemainLen;//chunk 被tcp分包，如果外层一直读完tcp数据，不让tcp发生分包，则此变量相关的逻辑可删除
}T_RtmpChunkHandle;


typedef struct RtmpSessionConfig
{
    unsigned int dwWindowSize;
    unsigned int dwPeerBandwidth;
    unsigned int dwOutChunkSize;
    unsigned int dwInChunkSize;
}T_RtmpSessionConfig;

typedef struct RtmpCb
{
    T_RtmpPackCb tRtmpPackCb;
    double (*GetStreamDuration)(char *i_strStreamName);//(seconds)
    int (*StartPlay)(char *i_strPalyPath,void *i_pIoHandle);
    int (*StopPlay)(char *i_strPalyPath);
    int (*StartPushStream)(char *i_strStreamPath,void *i_pIoHandle);
    int (*StopPushStream)(char *i_strStreamPath,void *i_pIoHandle);
    int (*SendData)(void *i_pIoHandle,char * i_acSendBuf,int i_iSendLen);
    int (*PushVideoData)(T_RtmpMediaInfo *i_ptRtmpMediaInfo,char * i_acDataBuf,int i_iDataLen,void *i_pIoHandle);//Annex-B格式裸流带00 00 00 01
    int (*PushAudioData)(T_RtmpMediaInfo *i_ptRtmpMediaInfo,char * i_acDataBuf,int i_iDataLen,void *i_pIoHandle);//aac带7字节头
    int (*PushScriptData)(char *i_strStreamName,unsigned int i_dwTimestamp,char * i_acDataBuf,int i_iDataLen);
}T_RtmpCb;
/*****************************************************************************
-Class          : RtmpSession
-Description    : 
* Modify Date     Version        Author           Modification
* -----------------------------------------------
* 2023/09/21      V1.0.0         Yu Weifeng       Created
******************************************************************************/
class CRtmpMsg
{
public:
    CRtmpMsg()
    {
        pbMsgBuf = new char [RTMP_MSG_MAX_LEN];
        memset(&tRtmpChunkHeader,0,sizeof(T_RtmpChunkHeader));
        iChunkHeaderLen = 0;
        iMsgBufLen = 0;
    };
    CRtmpMsg(const CRtmpMsg& other)
    {
        pbMsgBuf = new char [RTMP_MSG_MAX_LEN];
        memcpy(pbMsgBuf,other.pbMsgBuf,other.iMsgBufLen);
        memcpy(&tRtmpChunkHeader,&other.tRtmpChunkHeader,sizeof(T_RtmpChunkHeader));
        iChunkHeaderLen = other.iChunkHeaderLen;
        iMsgBufLen = other.iMsgBufLen;
    }
    virtual ~CRtmpMsg()
    {
        if(NULL != pbMsgBuf)
            delete [] pbMsgBuf;
    };
    
    T_RtmpChunkHeader tRtmpChunkHeader;
    int iChunkHeaderLen;
    char * pbMsgBuf;//map默认调用拷贝构造，如果没有定义则使用默认的浅拷贝，只拷贝指针不会拷贝内容
    int iMsgBufLen;//即两个指针共用一块内存
};
/*****************************************************************************
-Class          : RtmpSession
-Description    : 
* Modify Date     Version        Author           Modification
* -----------------------------------------------
* 2023/09/21      V1.0.0         Yu Weifeng       Created
******************************************************************************/
class RtmpSession
{
public:
    RtmpSession(void *i_pIoHandle,T_RtmpCb *i_ptRtmpCb,T_RtmpSessionConfig *i_ptRtmpSessionConfig = NULL);
    virtual ~RtmpSession();
    int SetEnhancedFlag(int i_iEnhancedFlag);
    
    int DoCycle();
    int DoPlay(T_RtmpMediaInfo *i_ptRtmpMediaInfo,unsigned char * i_pbFrameData,int i_iFrameLen,char * i_strPlayPath = NULL);

    int DoCycle(char *i_pcData,int i_iDataLen);
    int GetStopPlaySrc(char *o_pcData,int i_iMaxDataLen);
    
    int HandCmdPlaySendResult(int i_iResult,char *i_strDescription);
    int HandCmdPublishSendResult(int i_iResult,const char *i_strDescription);
    int SeverTestThread(const char *i_strFileName);
    int SeverTest(const char *i_strFileName);
private:
    int Handshake();
    int HandleRtmpReq();
    
    int HandleRtmpMsg(unsigned char i_bMsgTypeID,unsigned int i_dwTimestamp,char * i_pcMsgPayload,int i_iPayloadLen);
    
    int HandleCmdMsg(char * i_pcCmdMsgPayload,int i_iPayloadLen);
    int HandleControlMsg(unsigned char i_bMsgTypeID,char * i_pcMsgPayload,int i_iPayloadLen);
    int HandleVideoMsg(unsigned int i_dwTimestamp,char * i_pcMsgPayload,int i_iPayloadLen);
    int HandleAudioMsg(unsigned int i_dwTimestamp,char * i_pcMsgPayload,int i_iPayloadLen);
    int HandleDataMsg(unsigned int i_dwTimestamp,char * i_pcMsgPayload,int i_iPayloadLen);
    
    int HandCmdConnect(double i_dlTransaction,char * i_pcConnectBuf,int i_iBufLen);
    int HandCmdCreateStream(double i_dlTransaction,char * i_pcCreateStreamBuf,int i_iBufLen);
    int HandCmdGetStreamLength(double i_dlTransaction,char * i_pcBuf,int i_iBufLen);
    int HandCmdDeleteStream(double i_dlTransaction,char * i_pcBuf,int i_iBufLen);
    int HandCmdPlay(double i_dlTransaction,char * i_pcBuf,int i_iBufLen);
    int HandCmdPublish(double i_dlTransaction,char * i_pcPublishBuf,int i_iBufLen);

    int SendAcknowledgement(unsigned int i_dwWindowSize);
    int SendWindowAckSize(unsigned int i_dwWindowSize);
    int SendSetPeerBandwidth(unsigned int i_dwPeerBandwidth);
    int SendSetChunkSize(unsigned int i_dwChunkSize);

    int SendCmdConnectReply(double i_dlEncoding);
    int SendCmdCreateStreamReply(double i_dlTransaction);
    int SendCmdGetStreamLengthReply(double i_dlTransaction,double i_dlStreamDuration);
    int SendCmdOnStatus(T_RtmpCmdOnStatus *i_ptRtmpCmdOnStatus);

    int SendEventStreamBegin();
    int SendEventStreamIsRecord();

    int SendDataSampleAccess();
    int SendDataOnMetaData(T_RtmpMediaInfo *i_ptRtmpMediaInfo);
    
    int SendVideo(unsigned int i_dwTimestamp,unsigned char *i_pbVideoData,int i_iVideoDataLen);
    int SendAudio(unsigned int i_dwTimestamp,unsigned char *i_pbAudioData,int i_iAudioDataLen);

    int SimpleHandshake();
    int ComplexHandshake();

    int RecvData(char *o_acRecvBuf,int i_iRecvBufMaxLen);
    int SendData(char * i_acSendBuf,int i_iSendLen);

    int Handshake(char *i_pcData,int i_iDataLen);
    int HandleRtmpReqPacket(char *i_pcData,int i_iDataLen);
    int HandleRtmpRequest(char *i_pcData,int i_iDataLen);
    int HandleRtmpReq(char *i_pcData,int i_iDataLen);
    int HandleRtmpReqData(char *i_pcData,int i_iDataLen);
    int SimpleHandshake(char *i_pcData,int i_iDataLen);
    int ComplexHandshake(char *i_pcData,int i_iDataLen);
    int ParseRtmpDataToChunk(char *i_pcData,int i_iDataLen,int *o_piProcessedLen);
    int HandleRtmpDataToChunk(char *i_pcData,int i_iDataLen,int *o_piProcessedLen);
    

    E_RtmpHandshakeState m_eHandshakeState;
    bool m_blPlaying;
    bool m_blPlayStarted;
    void *m_pIoHandle;
    //TcpServerEpoll * m_pTcpSocket;//后续优化为只需ClientSocketFd
    int  m_iClientSocketFd;
    
    RtmpParse *m_pRtmpParse;
    RtmpPack *m_pRtmpPack;
    RtmpMediaHandle *m_pRtmpMediaHandle;//由于流播放开始需要先发送onmetaData,所以放这里,否则可以放上层
    int m_iChunkStreamID;//唯一标识 一个特定的流通道
    unsigned int m_dwRecvedDataLen;
    T_RtmpSessionConfig m_tRtmpSessionConfig;
    T_RtmpPlayContent m_tRtmpPlayContent;//strStreamName是完整url
    char m_strTcURL[RTMP_PLAY_SRC_MAX_LEN];//
    uint64 m_ddwLastVideoTimestamp;
    uint64 m_ddwLastAudioTimeStamp;//ms
    unsigned int m_dwVideoTimestamp;
    unsigned int m_dwAudioTimestamp;
    char m_abHandshakeBuf[RTMP_C0_LEN+RTMP_C1_LEN];
    int m_iHandshakeBufLen;
    T_RtmpMsgBufHandle m_tMsgBufHandle;
    unsigned char *m_pbNaluData;
    unsigned char *m_pbFrameData;
    bool m_blAudioSeqHeaderSended;
    T_RtmpPublishContent m_tRtmpPublishContent;//strStreamName是完整url
    T_RtmpAudioParam m_tPublishAudioParam;
    T_RtmpFrameInfo m_tPublishVideoParam;
    unsigned char *m_pbPublishFrameData;
    int m_iChunkBodyLen;
    int m_iChunkRemainLen;//chunk 被tcp分包
    
    T_RtmpCb m_tRtmpCb;
	unsigned int m_dwOffset;
    T_RtmpChunkHandle m_tRtmpChunkHandle;
    map<unsigned int, CRtmpMsg> m_RtmpMsgHandleMap;
    int m_iChunkTcpRemainLen;//chunk 被tcp分包，如果外层一直读完tcp数据，不让tcp发生分包，则此变量相关的逻辑可删除

    unsigned char *m_pbFileData;
	unsigned int m_dwFileBaseTimestamp = 0;
	unsigned int m_dwFileTimestamp = 0;
	unsigned int m_dwFileDiffCnt = 0;
	
	unsigned int m_dwVideoFrameCntLog = 0;
	unsigned int m_dwAudioFrameCntLog = 0;
	
    typedef int (RtmpSession::*HandleCmd)(double i_dlTransaction,char * i_pcConnectBuf,int i_iBufLen);//
    map<string, HandleCmd>  m_HandleCmdMap;
};



















#endif

