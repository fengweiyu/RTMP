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
#include <string>
#include <list>
#include <map>

using std::map;
using std::make_pair;
using std::string;


#define RTMP_MSG_MAX_LEN        4*1024*1024 //4m大小，考虑最大的i帧导致的video msg大小
#define RTMP_CHUNK_MAX_LEN        10*1024 //
#define RTMP_PLAY_SRC_MAX_LEN RTMP_STREAM_NAME_MAX_LEN

typedef enum RtmpMediaType
{
    RTMP_MEDIA_TYPE_VIDEO,
    RTMP_MEDIA_TYPE_ADUIO,
    RTMP_MEDIA_TYPE_META,

}E_RtmpMediaType;
typedef enum RtmpState
{
    RTMP_INIT,
    RTMP_HANDSHAKE_START,
    RTMP_HANDSHAKE_S0S1S2,
    RTMP_CONNECT,
    RTMP_CONNECT_RESULT,
    RTMP_CREATE_STREAM,
    RTMP_START_PLAY_OR_PUBLISH,
    RTMP_START_PLAY_OR_PUBLISH_RESULT,
    RTMP_START_PLAY_OR_PUBLISH_RESULT_ERR,
    RTMP_DO_PLAY_OR_PUBLISH,
    RTMP_EXIT,
}E_RtmpState;

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
    int iChunkMaxLen;
}T_RtmpChunkHandle;


typedef struct RtmpSessionConfig
{
    unsigned int dwWindowSize;
    unsigned int dwPeerBandwidth;
    unsigned int dwOutChunkSize;
    unsigned int dwInChunkSize;
    int iPlayOrPublish;//0 play,1 publish
    char strURL[1024];//rtmp://10.10.10.10:10/live_10/Mnx8Y2UEdXTQ%3D%3D.9eaa64fa64a282
}T_RtmpSessionConfig;

typedef struct RtmpCb
{
    T_RtmpPackCb tRtmpPackCb;
    int (*Connect)(void *i_pIoHandle,char * i_strIP,unsigned short i_wPort);
    int (*SendData)(void *i_pIoHandle,char * i_acSendBuf,int i_iSendLen);
    int (*PlayVideoData)(T_RtmpMediaInfo *i_ptRtmpMediaInfo,char * i_acDataBuf,int i_iDataLen,void *i_pIoHandle);//Annex-B格式裸流带00 00 00 01
    int (*PlayAudioData)(T_RtmpMediaInfo *i_ptRtmpMediaInfo,char * i_acDataBuf,int i_iDataLen,void *i_pIoHandle);//aac带7字节头
    int (*PlayScriptData)(char *i_strStreamName,unsigned int i_dwTimestamp,char * i_acDataBuf,int i_iDataLen);
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

    int DoPush(T_RtmpMediaInfo *i_ptRtmpMediaInfo,unsigned char * i_pbFrameData,int i_iFrameLen,char * i_strPlayPath = NULL);
    
    int DoConnect();
    int DoCycle(char *i_pcData,int i_iDataLen);
    int DoStop();
    
    int ClientTestThread(const char *i_strFileName);
    int ClientTest(const char *i_strFileName);
private:
    
    int HandleRtmpMsg(unsigned char i_bMsgTypeID,unsigned int i_dwTimestamp,char * i_pcMsgPayload,int i_iPayloadLen);
    
    int HandleCmdMsg(char * i_pcCmdMsgPayload,int i_iPayloadLen);
    int HandleControlMsg(unsigned char i_bMsgTypeID,char * i_pcMsgPayload,int i_iPayloadLen);
    int HandleVideoMsg(unsigned int i_dwTimestamp,char * i_pcMsgPayload,int i_iPayloadLen);
    int HandleAudioMsg(unsigned int i_dwTimestamp,char * i_pcMsgPayload,int i_iPayloadLen);
    int HandleDataMsg(unsigned int i_dwTimestamp,char * i_pcMsgPayload,int i_iPayloadLen);
    
    int HandCmd_result(double i_dlTransaction,char * i_pcResultBuf,int i_iBufLen);
    int HandCmdOnStatus(double i_dlTransaction,char * i_pcOnStatusBuf,int i_iBufLen);

    int SendAcknowledgement(unsigned int i_dwWindowSize);
    int SendSetChunkSize(unsigned int i_dwChunkSize);
    
    int SendDataOnMetaData(T_RtmpMediaInfo *i_ptRtmpMediaInfo);
    
    int SendCmdConnect();
    int SendCmdCreateStream();
    int SendCmdPlay();
    int SendCmdPublish();
    int SendCmdDeleteStream();
    
    int SendVideo(unsigned int i_dwTimestamp,unsigned char *i_pbVideoData,int i_iVideoDataLen);
    int SendAudio(unsigned int i_dwTimestamp,unsigned char *i_pbAudioData,int i_iAudioDataLen);

    int SendData(char * i_acSendBuf,int i_iSendLen);

    int Handshake(char *i_pcData,int i_iDataLen);
    int HandleRtmpReq(char *i_pcData,int i_iDataLen);
    int HandleRtmpRequest(char *i_pcData,int i_iDataLen);
    int HandleRtmpReqData(char *i_pcData,int i_iDataLen);
    int SimpleHandshake(char *i_pcData,int i_iDataLen);
    int ComplexHandshake(char *i_pcData,int i_iDataLen);
    int ParseRtmpDataToChunk(char *i_pcData,int i_iDataLen,int *o_piProcessedLen);
    

    E_RtmpState m_eState;
    bool m_blPushing;
    bool m_blPushStarted;
    void *m_pIoHandle;
    //TcpServerEpoll * m_pTcpSocket;//后续优化为只需ClientSocketFd
    int  m_iClientSocketFd;
    
    RtmpParse *m_pRtmpParse;
    RtmpPack *m_pRtmpPack;
    RtmpMediaHandle *m_pRtmpMediaHandle;//由于流播放开始需要先发送onmetaData,所以放这里,否则可以放上层
    int m_iChunkStreamID;//唯一标识 一个特定的流通道
    unsigned int m_dwRecvedDataLen;
    T_RtmpSessionConfig m_tRtmpSessionConfig;
    uint64 m_ddwLastTimestamp;
    unsigned int m_dwTimestamp;
    char m_abHandshakeBuf[RTMP_S0S1S2_LEN];
    int m_iHandshakeBufLen;
    T_RtmpMsgBufHandle m_tMsgBufHandle;
    unsigned char *m_pbNaluData;
    unsigned char *m_pbFrameData;
    bool m_blAudioSeqHeaderSended;
    T_RtmpAudioParam m_tPlayAudioParam;
    T_RtmpFrameInfo m_tPlayVideoParam;
    unsigned char *m_pbPlayFrameData;
    int m_iChunkBodyLen;
    int m_iChunkRemainLen;//chunk 被tcp分包
    
    T_RtmpCb m_tRtmpCb;
    T_RtmpChunkHandle m_tRtmpChunkHandle;
    map<unsigned int, CRtmpMsg> m_RtmpMsgHandleMap;
    
	unsigned int m_dwOffset;
    unsigned char *m_pbFileData;
	unsigned int m_dwFileBaseTimestamp = 0;
	unsigned int m_dwFileTimestamp = 0;
	unsigned int m_dwFileDiffCnt = 0;
	
	unsigned int m_dwVideoFrameCntLog = 0;
	unsigned int m_dwAudioFrameCntLog = 0;



    char m_strStreamName[512];
    char m_strAppName[512];
    char m_strTcURL[512];
	unsigned int m_dwConnectFailCnt;
    
    typedef int (RtmpSession::*HandleCmd)(double i_dlTransaction,char * i_pcConnectBuf,int i_iBufLen);
    map<string, HandleCmd>  m_HandleCmdMap;
};











#endif

