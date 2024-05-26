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
#include <map>
#include <string>

using std::string;
using std::map;
using std::make_pair;


#define RTMP_MSG_MAX_LEN        4*1024*1024 //4m大小，考虑最大的i帧导致的video msg大小

#define RTMP_PLAY_SRC_MAX_LEN RTMP_STREAM_NAME_MAX_LEN

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

typedef struct RtmpMsgBufHandle
{
    T_RtmpChunkHeader tRtmpChunkHeader;
    int iChunkHeaderLen;
    char * pbMsgBuf;
    int iMsgBufLen;
}T_RtmpMsgBufHandle;


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
class RtmpSession
{
public:
    RtmpSession(void *i_pIoHandle,T_RtmpCb *i_ptRtmpCb,T_RtmpSessionConfig *i_ptRtmpSessionConfig = NULL);
    virtual ~RtmpSession();

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
    int HandleRtmpReq(char *i_pcData,int i_iDataLen);
    int HandleRtmpReqData(char *i_pcData,int i_iDataLen);
    int SimpleHandshake(char *i_pcData,int i_iDataLen);
    int ComplexHandshake(char *i_pcData,int i_iDataLen);
    

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
    uint64 m_ddwLastTimestamp;
    unsigned int m_dwTimestamp;
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

