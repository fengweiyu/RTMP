/*****************************************************************************
* Copyright (C) 2023-2028 Hanson Yu  All rights reserved.
------------------------------------------------------------------------------
* File Module       :   RtmpPack.h
* Description       :   RtmpPack operation center
                        RTMP协议格式打包处理
                        //private 属性的内部函数暂不做容错判断,后续再做
* Created           :   2023.09.21.
* Author            :   Yu Weifeng
* Function List     :   
* Last Modified     :   
* History           :   
******************************************************************************/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
        
#include "RtmpPack.h"
#include "RtmpAdapter.h"
#include "AMF/include/amf0.h"

/*****************************************************************************
-Fuction        : RtmpPack
-Description    : 
-Input          : 
-Output         : 
-Return         : 
* Modify Date     Version        Author           Modification
* -----------------------------------------------
* 2023/09/21      V1.0.0         Yu Weifeng       Created
******************************************************************************/
RtmpPack::RtmpPack(T_RtmpPackCb *i_ptRtmpPackCb)
{
    memset(&m_atLastRtmpChunkHeader,0,sizeof(m_atLastRtmpChunkHeader));

    memset(&m_tRtmpPackCb,0,sizeof(T_RtmpPackCb));
    if(NULL != i_ptRtmpPackCb)
        memcpy(&m_tRtmpPackCb,i_ptRtmpPackCb,sizeof(T_RtmpPackCb));
    else
        RTMP_LOGE("RtmpPack m_tRtmpPackCb err NULL\r\n");

}

/*****************************************************************************
-Fuction        : RtmpParse
-Description    : 
-Input          : 
-Output         : 
-Return         : 
* Modify Date     Version        Author           Modification
* -----------------------------------------------
* 2023/09/21      V1.0.0         Yu Weifeng       Created
******************************************************************************/
RtmpPack::~RtmpPack()
{

}


/*****************************************************************************
-Fuction        : RtmpPack
-Description    : 
-Input          : 
-Output         : 
-Return         : -1 err >0 len
* Modify Date     Version        Author           Modification
* -----------------------------------------------
* 2023/09/21      V1.0.0         Yu Weifeng       Created
******************************************************************************/
int RtmpPack::CreateS0S1S2(char *i_pcC1,int i_iC1Len,char *o_pcBuf,int i_iMaxBufLen)
{
    int iRet = -1;
    int iDataLen = 0;
    char* pcBuf = NULL;

    if(i_iMaxBufLen < RTMP_S0S1S2_LEN || NULL == i_pcC1 ||i_iC1Len != RTMP_C1_LEN)
    {
        RTMP_LOGE("CreateS0S1S2 err %d %d\r\n",i_iMaxBufLen,i_iC1Len);
        return iRet;
    }
    o_pcBuf[iDataLen] = RTMP_VERSION;
    iDataLen++;
    pcBuf = &o_pcBuf[iDataLen];
    Write32BE(pcBuf,(unsigned int)time(NULL));
    iDataLen+=4;
    memset(&o_pcBuf[iDataLen],0,4);//memcpy(&o_pcBuf[iDataLen],i_pcC1,4);//SRS// s1 time2 copy from c1
    iDataLen+=4;
    iRet = RandomGenerate(&o_pcBuf[iDataLen],RTMP_S1_LEN - 8);
    if(iRet < 0)
    {
        RTMP_LOGE("RandomGenerate err %d %d\r\n",iDataLen,i_iC1Len);
        return iRet;
    }
    iDataLen += RTMP_S1_LEN - 8;
    
    memcpy(&o_pcBuf[iDataLen], i_pcC1, RTMP_C1_LEN);// if c1 specified, copy c1 to s2.
    iDataLen+=RTMP_C1_LEN;

    iRet = iDataLen;
    return iRet;
}

/*****************************************************************************
-Fuction        : CreateAcknowledgement
-Description    : // 5.4.3. Acknowledgement (3) (p20)
-Input          : 
-Output         : 
-Return         : -1 err >0 len
* Modify Date     Version        Author           Modification
* -----------------------------------------------
* 2023/09/21      V1.0.0         Yu Weifeng       Created
******************************************************************************/
int RtmpPack::CreateAcknowledgement(char* o_pcBuf, int i_iBufMaxLen,unsigned int i_iWindowSize)
{
    int iRet = -1;
    int iLen = 0;
    unsigned int iWindowSize = i_iWindowSize;
    char* pcBuf = NULL;
    
    if(i_iBufMaxLen < (int)(RTMP_CTL_MSG_LEN + sizeof(iWindowSize)) || NULL == o_pcBuf)
    {
        RTMP_LOGE("CreateAcknowledgement err %d %d\r\n",i_iBufMaxLen,i_iWindowSize);
        return iRet;
    }
    iLen = CreateControlMsgHeader(o_pcBuf,i_iBufMaxLen,RTMP_MSG_TYPE_ACKNOWLEDGEMENT,sizeof(iWindowSize));
    if(iLen <= 0)
    {
        RTMP_LOGE("CreateControlMsgHeader err %d %d\r\n",i_iBufMaxLen,i_iWindowSize);
        return iRet;
    }
    pcBuf = o_pcBuf + iLen;
    Write32BE(pcBuf,iWindowSize);
    iLen += sizeof(iWindowSize);
    
    return iLen;
}

/*****************************************************************************
-Fuction        : CreateWindowAckSize
-Description    : // 5.4.4. Window Acknowledgement Size (5) (p20)
-Input          : 
-Output         : 
-Return         : -1 err >0 len
* Modify Date     Version        Author           Modification
* -----------------------------------------------
* 2023/09/21      V1.0.0         Yu Weifeng       Created
******************************************************************************/
int RtmpPack::CreateWindowAckSize(char* o_pcBuf, int i_iBufMaxLen,unsigned int i_iWindowSize)
{
    int iRet = -1;
    int iLen = 0;
    unsigned int iWindowSize = i_iWindowSize;
    char* pcBuf = NULL;
    
    if(i_iBufMaxLen < (int)(RTMP_CTL_MSG_LEN + sizeof(iWindowSize)) || NULL == o_pcBuf)
    {
        RTMP_LOGE("CreateWindowAckSize err %d %d\r\n",i_iBufMaxLen,i_iWindowSize);
        return iRet;
    }
    iLen = CreateControlMsgHeader(o_pcBuf,i_iBufMaxLen,RTMP_MSG_TYPE_WINDOW_ACK_SIZE,sizeof(iWindowSize));
    if(iLen <= 0)
    {
        RTMP_LOGE("CreateControlMsgHeader err %d %d\r\n",i_iBufMaxLen,i_iWindowSize);
        return iRet;
    }
    pcBuf = o_pcBuf + iLen;
    Write32BE(pcBuf,iWindowSize);
    iLen += sizeof(iWindowSize);
    
    return iLen;
}

/*****************************************************************************
-Fuction        : CreateSetPeerBandwidth
-Description    : // 5.4.5. Set Peer Bandwidth (6) (p21)
-Input          : 
-Output         : 
-Return         : -1 err >0 len
* Modify Date     Version        Author           Modification
* -----------------------------------------------
* 2023/09/21      V1.0.0         Yu Weifeng       Created
******************************************************************************/
int RtmpPack::CreateSetPeerBandwidth(char* o_pcBuf, int i_iBufMaxLen,unsigned int i_iPeerBandwidth, unsigned char  i_bLimitType)
{
    int iRet = -1;
    int iLen = 0;
    unsigned int iWindowSize = i_iPeerBandwidth;
    unsigned char bLimitType = i_bLimitType;
    char* pcBuf = NULL;

    if(i_iBufMaxLen < (int)(RTMP_CTL_MSG_LEN + sizeof(iWindowSize)+sizeof(bLimitType)) || NULL == o_pcBuf)
    {
        RTMP_LOGE("CreateSetPeerBandwidth err %d %d\r\n",i_iBufMaxLen,i_iPeerBandwidth);
        return iRet;
    }
    iLen = CreateControlMsgHeader(o_pcBuf,i_iBufMaxLen,RTMP_MSG_TYPE_SET_PEER_BANDWIDTH,sizeof(iWindowSize)+sizeof(bLimitType));
    if(iLen <= 0)
    {
        RTMP_LOGE("CreateControlMsgHeader err %d %d\r\n",i_iBufMaxLen,i_iPeerBandwidth);
        return iRet;
    }
    pcBuf = o_pcBuf + iLen;
    Write32BE(pcBuf,iWindowSize);
    iLen += sizeof(iWindowSize);
    o_pcBuf[iLen] = bLimitType;
    iLen++;
    return iLen;
}

/*****************************************************************************
-Fuction        : CreateSetChunkSize
-Description    : // 5.4.1. Set Chunk Size (1) (p19)
-Input          : 
-Output         : 
-Return         : -1 err >0 len
* Modify Date     Version        Author           Modification
* -----------------------------------------------
* 2023/09/21      V1.0.0         Yu Weifeng       Created
******************************************************************************/
int RtmpPack::CreateSetChunkSize(char* o_pcBuf, int i_iBufMaxLen,unsigned int i_dwChunkSize)
{
    int iRet = -1;
    int iLen = 0;
    unsigned int dwChunkSize = i_dwChunkSize;
    char* pcBuf = NULL;

    if(i_iBufMaxLen < (int)(RTMP_CTL_MSG_LEN + sizeof(dwChunkSize)) || NULL == o_pcBuf)
    {
        RTMP_LOGE("CreateSetChunkSize err %d %d\r\n",i_iBufMaxLen,dwChunkSize);
        return iRet;
    }
    iLen = CreateControlMsgHeader(o_pcBuf,i_iBufMaxLen,RTMP_MSG_TYPE_SET_CHUNK_SIZE,sizeof(dwChunkSize));
    if(iLen <= 0)
    {
        RTMP_LOGE("CreateControlMsgHeader err %d %d\r\n",i_iBufMaxLen,dwChunkSize);
        return iRet;
    }
    pcBuf = o_pcBuf + iLen;
    Write32BE(pcBuf,(dwChunkSize & 0x7FFFFFFF)); // first bit MUST be zero
    iLen += sizeof(dwChunkSize);
    
    return iLen;
}

/*****************************************************************************
-Fuction        : CreateCmdConnectReply
-Description    : // netconnection_connect_reply
-Input          : 
-Output         : 
-Return         : -1 err >0 len
* Modify Date     Version        Author           Modification
* -----------------------------------------------
* 2023/09/21      V1.0.0         Yu Weifeng       Created
******************************************************************************/
int RtmpPack::CreateCmdConnectReply(T_RtmpCmdConnectReply *i_ptRtmpCmdConnectReply, unsigned char * o_pbBuf, int i_iBufMaxLen)
{
    int iLen = 0;
    unsigned char * pbEnd = o_pbBuf + i_iBufMaxLen;
    const char* strCommand = "_result";
    unsigned char * pbOffset = o_pbBuf;
    
    if(NULL == i_ptRtmpCmdConnectReply || NULL == o_pbBuf)
    {
        RTMP_LOGE("CreateCmdConnectReply err %d \r\n",i_iBufMaxLen);
        return -1;
    }

    pbOffset = AMFWriteString(pbOffset, pbEnd, strCommand, strlen(strCommand));
    pbOffset = AMFWriteDouble(pbOffset, pbEnd, i_ptRtmpCmdConnectReply->dlTransactionID);

    pbOffset = AMFWriteObject(pbOffset, pbEnd);
    if(NULL != i_ptRtmpCmdConnectReply->strFmsVer)
        pbOffset = AMFWriteNamedString(pbOffset, pbEnd, "fmsVer", 6, i_ptRtmpCmdConnectReply->strFmsVer, strlen(i_ptRtmpCmdConnectReply->strFmsVer));
    pbOffset = AMFWriteNamedDouble(pbOffset, pbEnd, "capabilities", 12, i_ptRtmpCmdConnectReply->dlCapabilities);
    pbOffset = AMFWriteObjectEnd(pbOffset, pbEnd);

    pbOffset = AMFWriteObject(pbOffset, pbEnd);
    if(NULL != i_ptRtmpCmdConnectReply->strLevel)
        pbOffset = AMFWriteNamedString(pbOffset, pbEnd, "level", 5, i_ptRtmpCmdConnectReply->strLevel, strlen(i_ptRtmpCmdConnectReply->strLevel));
    if(NULL != i_ptRtmpCmdConnectReply->strCode)
        pbOffset = AMFWriteNamedString(pbOffset, pbEnd, "code", 4, i_ptRtmpCmdConnectReply->strCode, strlen(i_ptRtmpCmdConnectReply->strCode));
    if(NULL != i_ptRtmpCmdConnectReply->strDescription)
        pbOffset = AMFWriteNamedString(pbOffset, pbEnd, "description", 11, i_ptRtmpCmdConnectReply->strDescription, strlen(i_ptRtmpCmdConnectReply->strDescription));
    pbOffset = AMFWriteNamedDouble(pbOffset, pbEnd, "objectEncoding", 14, i_ptRtmpCmdConnectReply->dlEncoding);
    pbOffset = AMFWriteObjectEnd(pbOffset, pbEnd);
    iLen = pbOffset - o_pbBuf;
    
    return iLen;
}

/*****************************************************************************
-Fuction        : CreateCmdCreateStreamReply
-Description    : // netconnection_create_stream
-Input          : 
-Output         : 
-Return         : -1 err >0 len
* Modify Date     Version        Author           Modification
* -----------------------------------------------
* 2023/09/21      V1.0.0         Yu Weifeng       Created
******************************************************************************/
int RtmpPack::CreateCmdCreateStreamReply(double i_dlTransaction,unsigned char * o_pbBuf, int i_iBufMaxLen)
{
    int iLen = 0;
    unsigned char * pbEnd = o_pbBuf + i_iBufMaxLen;
    const char* strCommand = "_result";
    unsigned char * pbOffset = o_pbBuf;
    double dlStreamID = 1;
    
    if(NULL == o_pbBuf)
    {
        RTMP_LOGE("CreateCmdCreateStreamReply err %d \r\n",i_iBufMaxLen);
        return -1;
    }

    pbOffset = AMFWriteString(pbOffset, pbEnd, strCommand, strlen(strCommand));
    pbOffset = AMFWriteDouble(pbOffset, pbEnd, i_dlTransaction);
    pbOffset = AMFWriteNull(pbOffset, pbEnd);
    pbOffset = AMFWriteDouble(pbOffset, pbEnd, dlStreamID);
    iLen = pbOffset - o_pbBuf;
    
    return iLen;
}

/*****************************************************************************
-Fuction        : CreateCmdGetStreamLengthReply
-Description    : // netconnection_create_stream
-Input          : 
-Output         : 
-Return         : -1 err >0 len
* Modify Date     Version        Author           Modification
* -----------------------------------------------
* 2023/09/21      V1.0.0         Yu Weifeng       Created
******************************************************************************/
int RtmpPack::CreateCmdGetStreamLengthReply(double i_dlTransaction,double i_dlStreamDuration,unsigned char* o_pbBuf, int i_iBufMaxLen)
{
    int iLen = 0;
    unsigned char * pbEnd = o_pbBuf + i_iBufMaxLen;
    unsigned char * pbOffset = o_pbBuf;
    const char* strCommand = "_result";
    
    if(NULL == o_pbBuf)
    {
        RTMP_LOGE("CreateCmdCreateStreamReply err %d \r\n",i_iBufMaxLen);
        return -1;
    }

    pbOffset = AMFWriteString(pbOffset, pbEnd, strCommand, strlen(strCommand));
    pbOffset = AMFWriteDouble(pbOffset, pbEnd, i_dlTransaction);
    pbOffset = AMFWriteNull(pbOffset, pbEnd);
    pbOffset = AMFWriteDouble(pbOffset, pbEnd, i_dlStreamDuration);
    iLen = pbOffset - o_pbBuf;
    
    return iLen;

}

/*****************************************************************************
-Fuction        : CreateCmdGetStreamLengthReply
-Description    : // netconnection_create_stream
-Input          : 
-Output         : 
-Return         : -1 err >0 len
* Modify Date     Version        Author           Modification
* -----------------------------------------------
* 2023/09/21      V1.0.0         Yu Weifeng       Created
******************************************************************************/
int RtmpPack::CreateCmdOnStatus(T_RtmpCmdOnStatus *i_ptRtmpCmdOnStatus,unsigned char* o_pbBuf, int i_iBufMaxLen)
{
    int iLen = 0;
    unsigned char * pbEnd = o_pbBuf + i_iBufMaxLen;
    unsigned char * pbOffset = o_pbBuf;
    const char* strCommand = "onStatus";
    const char *strStatusLevel = NULL;
    const char * strCode = NULL;

    
    if(NULL == i_ptRtmpCmdOnStatus || NULL == o_pbBuf)
    {
        RTMP_LOGE("CreateCmdOnStatus err %d \r\n",i_iBufMaxLen);
        return -1;
    }
    if(false == i_ptRtmpCmdOnStatus->blIsSuccess)
    {
        strStatusLevel = RTMP_LEVEL_ERROR;
        strCode = i_ptRtmpCmdOnStatus->strFail;
    }
    else
    {
        strStatusLevel = RTMP_LEVEL_STATUS;
        strCode = i_ptRtmpCmdOnStatus->strSuccess;
    }
    pbOffset = AMFWriteString(pbOffset, pbEnd, strCommand, strlen(strCommand));// Command Name
    pbOffset = AMFWriteDouble(pbOffset, pbEnd, i_ptRtmpCmdOnStatus->dlTransactionID);// Transaction ID
    pbOffset = AMFWriteNull(pbOffset, pbEnd);// command object
    
    pbOffset = AMFWriteObject(pbOffset, pbEnd);
    pbOffset = AMFWriteNamedString(pbOffset, pbEnd, "level", 5, strStatusLevel, strlen(strStatusLevel));
    pbOffset = AMFWriteNamedString(pbOffset, pbEnd, "code", 4, strCode, strlen(strCode));
    pbOffset = AMFWriteNamedString(pbOffset, pbEnd, "description", 11, i_ptRtmpCmdOnStatus->strDescription, strlen(i_ptRtmpCmdOnStatus->strDescription));
    pbOffset = AMFWriteObjectEnd(pbOffset, pbEnd);
    iLen = pbOffset - o_pbBuf;
    
    return iLen;

}

/*****************************************************************************
-Fuction        : CreateCmdMsg
-Description    : rtmp packet,rtmp_chunk header and body
-Input          : i_iProcessedLen 已经处理了多少长度
-Output         : 
-Return         : -1 err >0 len
* Modify Date     Version        Author           Modification
* -----------------------------------------------
* 2023/09/21      V1.0.0         Yu Weifeng       Created
******************************************************************************/
int RtmpPack::CreateCmdMsg(T_RtmpChunkPayloadInfo *i_ptChunkPayloadInfo,char* o_pcBuf, int i_iBufMaxLen)
{
    int iLen = 0;
    T_RtmpChunkHeader tRtmpChunkHeader;

    
    if(NULL == i_ptChunkPayloadInfo || NULL == o_pcBuf)
    {
        RTMP_LOGE("CreateCmdReply err %d \r\n",i_iBufMaxLen);
        return -1;
    }

    memset(&tRtmpChunkHeader,0,sizeof(T_RtmpChunkHeader));
    tRtmpChunkHeader.tBasicHeader.bChunkType = RTMP_CHUNK_TYPE_0; // 预设,也作标记传给下面决定是否压缩,disable compact header
    tRtmpChunkHeader.tBasicHeader.dwChunkStreamID = RTMP_COMMAND_CHANNEL_CSID;
    tRtmpChunkHeader.tMsgHeader.dwTimestamp = 0;
    tRtmpChunkHeader.tMsgHeader.dwLength = i_ptChunkPayloadInfo->iPayloadLen;
    tRtmpChunkHeader.tMsgHeader.bTypeID= RTMP_MSG_TYPE_INVOKE;
    tRtmpChunkHeader.tMsgHeader.dwStreamID = i_ptChunkPayloadInfo->dwStreamID;/* default 0 */
    
    return CreateRtmpPacket(&tRtmpChunkHeader,i_ptChunkPayloadInfo, o_pcBuf, i_iBufMaxLen);

}

/*****************************************************************************
-Fuction        : CreateCmdMsg
-Description    : rtmp packet,rtmp_chunk header and body
-Input          : i_iProcessedLen 已经处理了多少长度
-Output         : 
-Return         : -1 err >0 len
* Modify Date     Version        Author           Modification
* -----------------------------------------------
* 2023/09/21      V1.0.0         Yu Weifeng       Created
******************************************************************************/
int RtmpPack::CreateCmd2Msg(T_RtmpChunkPayloadInfo *i_ptChunkPayloadInfo,char* o_pcBuf, int i_iBufMaxLen)
{
    int iLen = 0;
    T_RtmpChunkHeader tRtmpChunkHeader;

    
    if(NULL == i_ptChunkPayloadInfo || NULL == o_pcBuf)
    {
        RTMP_LOGE("CreateCmdReply err %d \r\n",i_iBufMaxLen);
        return -1;
    }

    memset(&tRtmpChunkHeader,0,sizeof(T_RtmpChunkHeader));
    tRtmpChunkHeader.tBasicHeader.bChunkType = RTMP_CHUNK_TYPE_0; // 预设,也作标记传给下面决定是否压缩,disable compact header
    tRtmpChunkHeader.tBasicHeader.dwChunkStreamID = RTMP_STREAM_CHANNEL_CSID;
    tRtmpChunkHeader.tMsgHeader.dwTimestamp = 0;
    tRtmpChunkHeader.tMsgHeader.dwLength = i_ptChunkPayloadInfo->iPayloadLen;
    tRtmpChunkHeader.tMsgHeader.bTypeID= RTMP_MSG_TYPE_INVOKE;
    tRtmpChunkHeader.tMsgHeader.dwStreamID = i_ptChunkPayloadInfo->dwStreamID;/* default 0 */
    
    return CreateRtmpPacket(&tRtmpChunkHeader,i_ptChunkPayloadInfo, o_pcBuf, i_iBufMaxLen);

}

/*****************************************************************************
-Fuction        : CreateEventStreamBegin
-Description    : // 5.4.1. Set Chunk Size (1) (p19)
-Input          : 
-Output         : 
-Return         : -1 err >0 len
* Modify Date     Version        Author           Modification
* -----------------------------------------------
* 2023/09/21      V1.0.0         Yu Weifeng       Created
******************************************************************************/
int RtmpPack::CreateEventStreamBegin(char* o_pcBuf, int i_iBufMaxLen)
{
    return CreateEventStream(RTMP_MSG_EVENT_STREAM_BEGIN,o_pcBuf,i_iBufMaxLen);
}

/*****************************************************************************
-Fuction        : CreateEventStreamBegin
-Description    : // 5.4.1. Set Chunk Size (1) (p19)
-Input          : 
-Output         : 
-Return         : -1 err >0 len
* Modify Date     Version        Author           Modification
* -----------------------------------------------
* 2023/09/21      V1.0.0         Yu Weifeng       Created
******************************************************************************/
int RtmpPack::CreateEventStreamIsRecord(char* o_pcBuf, int i_iBufMaxLen)
{
    return CreateEventStream(RTMP_MSG_EVENT_STREAM_IS_RECORD,o_pcBuf,i_iBufMaxLen);
}

/*****************************************************************************
-Fuction        : CreateEventStreamBegin
-Description    : // 5.4.1. Set Chunk Size (1) (p19)
-Input          : 
-Output         : 
-Return         : -1 err >0 len
* Modify Date     Version        Author           Modification
* -----------------------------------------------
* 2023/09/21      V1.0.0         Yu Weifeng       Created
******************************************************************************/
int RtmpPack::CreateEventStream(unsigned short i_wEvent,char* o_pcBuf, int i_iBufMaxLen)
{
    int iRet = -1;
    int iLen = 0;
    unsigned short wEvent = i_wEvent;
    unsigned int dwStreamId = RTMP_MSG_STREAM_ID1;//事件数据是表示开始起作用的流的ID
    char* pcBuf = NULL;

    
    if(i_iBufMaxLen < (int)(RTMP_CTL_MSG_LEN + sizeof(wEvent)+ sizeof(dwStreamId)) || NULL == o_pcBuf)
    {
        RTMP_LOGE("CreateEventStreamBegin err %d %d\r\n",i_iBufMaxLen,dwStreamId);
        return iRet;
    }
    iLen = CreateControlMsgHeader(o_pcBuf,i_iBufMaxLen,RTMP_MSG_TYPE_EVENT,sizeof(wEvent)+sizeof(dwStreamId));
    if(iLen <= 0)
    {
        RTMP_LOGE("CreateControlMsgHeader err %d %d\r\n",i_iBufMaxLen,dwStreamId);
        return iRet;
    }
    pcBuf = o_pcBuf+iLen;
    Write16BE(pcBuf,wEvent ); 
    iLen += sizeof(wEvent);
    pcBuf = o_pcBuf+iLen;
    Write32BE(pcBuf,dwStreamId); 
    iLen += sizeof(dwStreamId);
    
    return iLen;
}

/*****************************************************************************
-Fuction        : CreateCmdGetStreamLengthReply
-Description    : // netconnection_create_stream
-Input          : 
-Output         : 
-Return         : -1 err >0 len
* Modify Date     Version        Author           Modification
* -----------------------------------------------
* 2023/09/21      V1.0.0         Yu Weifeng       Created
******************************************************************************/
int RtmpPack::CreateDataSampleAccess(unsigned char* o_pbBuf, int i_iBufMaxLen)
{
    int iLen = 0;
    unsigned char * pbEnd = o_pbBuf + i_iBufMaxLen;
    unsigned char * pbOffset = o_pbBuf;
    const char* strCommand = "|RtmpSampleAccess";
    
    if(NULL == o_pbBuf)
    {
        RTMP_LOGE("CreateDataSampleAccess err %d \r\n",i_iBufMaxLen);
        return -1;
    }
    pbOffset = AMFWriteString(pbOffset, pbEnd, strCommand, strlen(strCommand));
    pbOffset = AMFWriteBoolean(pbOffset, pbEnd, 1);//使用true,true允许音频和视频访问
    pbOffset = AMFWriteBoolean(pbOffset, pbEnd, 1);
    iLen = pbOffset - o_pbBuf;
    
    return iLen;

}

/*****************************************************************************
-Fuction        : CreateCmdGetStreamLengthReply
-Description    : // netconnection_create_stream
-Input          : 
-Output         : 
-Return         : -1 err >0 len
* Modify Date     Version        Author           Modification
* -----------------------------------------------
* 2023/09/21      V1.0.0         Yu Weifeng       Created
******************************************************************************/
int RtmpPack::CreateDataOnMetaData(T_RtmpMediaInfo *i_ptRtmpMediaInfo,unsigned char* o_pbBuf, int i_iBufMaxLen)
{
    int iLen = 0;
    unsigned char * pbEnd = o_pbBuf + i_iBufMaxLen;
    unsigned char * pbOffset = o_pbBuf;
    const char* strCommand = "onMetaData";
    
    if(NULL == o_pbBuf)
    {
        RTMP_LOGE("CreateDataOnMetaData err %d \r\n",i_iBufMaxLen);
        return -1;
    }
    pbOffset = AMFWriteString(pbOffset, pbEnd, strCommand, strlen(strCommand));

    pbOffset = AMFWriteObject(pbOffset, pbEnd);
    pbOffset = AMFWriteNamedDouble(pbOffset, pbEnd, "width", strlen("width"), i_ptRtmpMediaInfo->dwWidth);
    pbOffset = AMFWriteNamedDouble(pbOffset, pbEnd, "height", strlen("height"), i_ptRtmpMediaInfo->dwHeight);
    pbOffset = AMFWriteNamedDouble(pbOffset, pbEnd, "displayWidth", strlen("displayWidth"), i_ptRtmpMediaInfo->dwWidth);
    pbOffset = AMFWriteNamedDouble(pbOffset, pbEnd, "displayHeight", strlen("displayHeight"), i_ptRtmpMediaInfo->dwHeight);
    pbOffset = AMFWriteNamedDouble(pbOffset, pbEnd, "duration", strlen("duration"), i_ptRtmpMediaInfo->dlDuration);
    
    if (i_ptRtmpMediaInfo->eEncType == RTMP_ENC_H265)
    {
        pbOffset = AMFWriteNamedDouble(pbOffset, pbEnd, "videocodecid", strlen("videocodecid"), 12);
    }
    else
    {
        pbOffset = AMFWriteNamedDouble(pbOffset, pbEnd, "videocodecid", strlen("videocodecid"), 7);
    }//
    pbOffset = AMFWriteNamedDouble(pbOffset, pbEnd, "audiocodecid", strlen("audiocodecid"), 10);//aac 10
    pbOffset = AMFWriteNamedDouble(pbOffset, pbEnd, "audiosamplerate", strlen("audiosamplerate"),44100); //i_ptRtmpMediaInfo->dwSampleRate);
    pbOffset = AMFWriteObjectEnd(pbOffset, pbEnd);
    
    iLen = pbOffset - o_pbBuf;
    
    return iLen;

}
/*****************************************************************************
-Fuction        : CreateDataMsg
-Description    : rtmp packet,rtmp_chunk header and body
-Input          : i_iProcessedLen 已经处理了多少长度
-Output         : 
-Return         : -1 err >0 len
* Modify Date     Version        Author           Modification
* -----------------------------------------------
* 2023/09/21      V1.0.0         Yu Weifeng       Created
******************************************************************************/
int RtmpPack::CreateDataMsg(T_RtmpChunkPayloadInfo *i_ptChunkPayloadInfo,char* o_pcBuf, int i_iBufMaxLen)
{
    int iLen = 0;
    T_RtmpChunkHeader tRtmpChunkHeader;

    
    if(NULL == i_ptChunkPayloadInfo || NULL == o_pcBuf)
    {
        RTMP_LOGE("CreateCmdReply err %d \r\n",i_iBufMaxLen);
        return -1;
    }

    memset(&tRtmpChunkHeader,0,sizeof(T_RtmpChunkHeader));
    tRtmpChunkHeader.tBasicHeader.bChunkType = RTMP_CHUNK_TYPE_0; // 预设,也作标记传给下面决定是否压缩,disable compact header
    tRtmpChunkHeader.tBasicHeader.dwChunkStreamID = RTMP_STREAM_CHANNEL_CSID;
    tRtmpChunkHeader.tMsgHeader.dwTimestamp = 0;
    tRtmpChunkHeader.tMsgHeader.dwLength = i_ptChunkPayloadInfo->iPayloadLen;
    tRtmpChunkHeader.tMsgHeader.bTypeID= RTMP_MSG_TYPE_DATA;
    tRtmpChunkHeader.tMsgHeader.dwStreamID = i_ptChunkPayloadInfo->dwStreamID;/* default 0 */
    
    return CreateRtmpPacket(&tRtmpChunkHeader,i_ptChunkPayloadInfo, o_pcBuf, i_iBufMaxLen);

}

/*****************************************************************************
-Fuction        : CreateVideoMsg
-Description    : rtmp packet,rtmp_chunk header and body
-Input          : i_iProcessedLen 已经处理了多少长度
-Output         : 
-Return         : -1 err >0 len
* Modify Date     Version        Author           Modification
* -----------------------------------------------
* 2023/09/21      V1.0.0         Yu Weifeng       Created
******************************************************************************/
int RtmpPack::CreateVideoMsg(unsigned int i_dwTimestamp,T_RtmpChunkPayloadInfo *i_ptChunkPayloadInfo,char* o_pcBuf, int i_iBufMaxLen)
{
    int iLen = 0;
    T_RtmpChunkHeader tRtmpChunkHeader;

    
    if(NULL == i_ptChunkPayloadInfo || NULL == o_pcBuf)
    {
        RTMP_LOGE("CreateCmdReply err %d \r\n",i_iBufMaxLen);
        return -1;
    }

    memset(&tRtmpChunkHeader,0,sizeof(T_RtmpChunkHeader));
    tRtmpChunkHeader.tBasicHeader.bChunkType = RTMP_CHUNK_TYPE_1; // 预设,也作标记传给下面决定是否压缩,enable compact header
    tRtmpChunkHeader.tBasicHeader.dwChunkStreamID = RTMP_VIDEO_CHANNEL_CSID;
    tRtmpChunkHeader.tMsgHeader.dwTimestamp = i_dwTimestamp;
    tRtmpChunkHeader.tMsgHeader.dwLength = i_ptChunkPayloadInfo->iPayloadLen;
    tRtmpChunkHeader.tMsgHeader.bTypeID= RTMP_MSG_TYPE_VIDEO;
    tRtmpChunkHeader.tMsgHeader.dwStreamID = i_ptChunkPayloadInfo->dwStreamID;/* default 0 */
    
    return CreateRtmpPacket(&tRtmpChunkHeader,i_ptChunkPayloadInfo, o_pcBuf, i_iBufMaxLen);

}

/*****************************************************************************
-Fuction        : CreateAudioMsg
-Description    : rtmp packet,rtmp_chunk header and body
-Input          : i_iProcessedLen 已经处理了多少长度
-Output         : 
-Return         : -1 err >0 len
* Modify Date     Version        Author           Modification
* -----------------------------------------------
* 2023/09/21      V1.0.0         Yu Weifeng       Created
******************************************************************************/
int RtmpPack::CreateAudioMsg(unsigned int i_dwTimestamp,T_RtmpChunkPayloadInfo *i_ptChunkPayloadInfo,char* o_pcBuf, int i_iBufMaxLen)
{
    int iLen = 0;
    T_RtmpChunkHeader tRtmpChunkHeader;

    
    if(NULL == i_ptChunkPayloadInfo || NULL == o_pcBuf)
    {
        RTMP_LOGE("CreateCmdReply err %d \r\n",i_iBufMaxLen);
        return -1;
    }

    memset(&tRtmpChunkHeader,0,sizeof(T_RtmpChunkHeader));
    tRtmpChunkHeader.tBasicHeader.bChunkType = RTMP_CHUNK_TYPE_1; // 预设,也作标记传给下面决定是否压缩,enable compact header
    tRtmpChunkHeader.tBasicHeader.dwChunkStreamID = RTMP_AUDIO_CHANNEL_CSID;
    tRtmpChunkHeader.tMsgHeader.dwTimestamp = i_dwTimestamp;
    tRtmpChunkHeader.tMsgHeader.dwLength = i_ptChunkPayloadInfo->iPayloadLen;
    tRtmpChunkHeader.tMsgHeader.bTypeID= RTMP_MSG_TYPE_AUDIO;
    tRtmpChunkHeader.tMsgHeader.dwStreamID = i_ptChunkPayloadInfo->dwStreamID;/* default 0 */
    
    return CreateRtmpPacket(&tRtmpChunkHeader,i_ptChunkPayloadInfo, o_pcBuf, i_iBufMaxLen);

}

/*****************************************************************************
-Fuction        : CreateRtmpPacket
-Description    : rtmp packet,rtmp_chunk header and body
-Input          : i_iProcessedLen 已经处理了多少长度
-Output         : 
-Return         : -1 err >0 len
* Modify Date     Version        Author           Modification
* -----------------------------------------------
* 2023/09/21      V1.0.0         Yu Weifeng       Created
******************************************************************************/
int RtmpPack::CreateRtmpPacket(T_RtmpChunkHeader *i_ptRtmpChunkHeader,T_RtmpChunkPayloadInfo *i_ptChunkPayloadInfo,char* o_pcBuf, int i_iBufMaxLen)
{
    int iLen = 0;
    T_RtmpChunkHeader tRtmpChunkHeader;
    int iChunkPayloadLen = 0;//不含chunk头
    int iRemainLen = 0;
    
    if(NULL == i_ptRtmpChunkHeader || NULL == i_ptChunkPayloadInfo || NULL == i_ptChunkPayloadInfo->pcChunkPayload
    || NULL == o_pcBuf || i_iBufMaxLen < (int)sizeof(T_RtmpChunkHeader)+RTMP_OUTPUT_CHUNK_SIZE)
    {
        RTMP_LOGE("CreateRtmpPacket NULL %d \r\n",i_iBufMaxLen);
        return -1;
    }
    iRemainLen = i_ptChunkPayloadInfo->iPayloadLen - i_ptChunkPayloadInfo->iProcessedLen;
    if(iRemainLen <= 0)
    {
        //RTMP_LOGD("CreateRtmpPacket over %d \r\n",iRemainLen);
        SetLastRtmpChunkHeader(i_ptRtmpChunkHeader->tBasicHeader.dwChunkStreamID,i_ptRtmpChunkHeader);
        return iLen;
    }
    if(0 == i_ptChunkPayloadInfo->iProcessedLen)
    {
        memset(&tRtmpChunkHeader,0,sizeof(T_RtmpChunkHeader));
        if(ChunkHeaderCompress(i_ptRtmpChunkHeader,&tRtmpChunkHeader) < 0)
        {
            memcpy(&tRtmpChunkHeader,i_ptRtmpChunkHeader,sizeof(T_RtmpChunkHeader));
        }
        if(tRtmpChunkHeader.tMsgHeader.dwLength >= 0xFFFFFF)
        {
            RTMP_LOGE("CreateRtmpPacket err %d \r\n",tRtmpChunkHeader.tMsgHeader.dwLength);
            return -1;// invalid length
        }
        iLen = SetRtmpChunkBasicHeader((unsigned char*)o_pcBuf,tRtmpChunkHeader.tBasicHeader.bChunkType,tRtmpChunkHeader.tBasicHeader.dwChunkStreamID);
        iLen += SetRtmpChunkMsgHeader((unsigned char*)&o_pcBuf[iLen],&tRtmpChunkHeader);
        if(tRtmpChunkHeader.tMsgHeader.dwTimestamp >= 0xFFFFFF)
            iLen += SetRtmpChunkExtendedTimestamp((unsigned char*)&o_pcBuf[iLen], tRtmpChunkHeader.tMsgHeader.dwTimestamp);
        i_ptChunkPayloadInfo->dwTimestamp = tRtmpChunkHeader.tMsgHeader.dwTimestamp;
    }
    else
    {
        iLen = SetRtmpChunkBasicHeader((unsigned char*)o_pcBuf, RTMP_CHUNK_TYPE_3, i_ptRtmpChunkHeader->tBasicHeader.dwChunkStreamID);
        if (i_ptChunkPayloadInfo->dwTimestamp >= 0xFFFFFF)
            iLen += SetRtmpChunkExtendedTimestamp((unsigned char*)&o_pcBuf[iLen], i_ptChunkPayloadInfo->dwTimestamp);
    }
    iChunkPayloadLen = iRemainLen < (int)i_ptChunkPayloadInfo->dwOutChunkSize ? iRemainLen : i_ptChunkPayloadInfo->dwOutChunkSize;
    memcpy(&o_pcBuf[iLen],i_ptChunkPayloadInfo->pcChunkPayload+i_ptChunkPayloadInfo->iProcessedLen,iChunkPayloadLen);
    iLen += iChunkPayloadLen;

    i_ptChunkPayloadInfo->iProcessedLen += iChunkPayloadLen;
    return iLen;
}


/*****************************************************************************
-Fuction        : ChunkHeaderCompress
-Description    : 
-Input          : 
-Output         : 
-Return         : 
* Modify Date     Version        Author           Modification
* -----------------------------------------------
* 2023/09/21      V1.0.0         Yu Weifeng       Created
******************************************************************************/
int RtmpPack::ChunkHeaderCompress(T_RtmpChunkHeader *i_ptRtmpChunkHeader,T_RtmpChunkHeader *o_ptRtmpChunkHeader)
{
    int iRet = -1;
    T_RtmpChunkHeader tRtmpChunkHeader;
    T_RtmpChunkHeader tLastRtmpChunkHeader;

    if(NULL == i_ptRtmpChunkHeader || NULL == o_ptRtmpChunkHeader)
    {
        RTMP_LOGE("ChunkHeaderCompress NULL \r\n");
        return -1;
    }
    if(0 == i_ptRtmpChunkHeader->tBasicHeader.dwChunkStreamID || 1 == i_ptRtmpChunkHeader->tBasicHeader.dwChunkStreamID)
    {
        RTMP_LOGE("ChunkHeaderCompress dwChunkStreamID err \r\n");
        return -1;
    }

    
    memcpy(&tRtmpChunkHeader,i_ptRtmpChunkHeader,sizeof(T_RtmpChunkHeader));
    // find previous chunk header
    memset(&tLastRtmpChunkHeader,0,sizeof(T_RtmpChunkHeader));
    iRet = GetLastRtmpChunkHeader(i_ptRtmpChunkHeader->tBasicHeader.dwChunkStreamID,&tLastRtmpChunkHeader);
    if (iRet < 0)
    {
        RTMP_LOGW("ChunkHeaderCompress GetLastRtmpChunkHeader no find \r\n");
        i_ptRtmpChunkHeader->tBasicHeader.bChunkType = RTMP_CHUNK_TYPE_0;//第一次不压缩则修改预设
        return iRet; // can't find chunk stream id 
    }
    tRtmpChunkHeader.tBasicHeader.bChunkType= RTMP_CHUNK_TYPE_0;
    if (RTMP_CHUNK_TYPE_0 != i_ptRtmpChunkHeader->tBasicHeader.bChunkType/* enable compress */
        && i_ptRtmpChunkHeader->tBasicHeader.dwChunkStreamID== tLastRtmpChunkHeader.tBasicHeader.dwChunkStreamID /* not the first packet */
        && i_ptRtmpChunkHeader->tMsgHeader.dwTimestamp >= tLastRtmpChunkHeader.tMsgHeader.dwTimestamp /* timestamp wrap */
        && i_ptRtmpChunkHeader->tMsgHeader.dwTimestamp - tLastRtmpChunkHeader.tMsgHeader.dwTimestamp < 0xFFFFFF /* timestamp delta < 1 << 24 */
        && i_ptRtmpChunkHeader->tMsgHeader.dwStreamID == tLastRtmpChunkHeader.tMsgHeader.dwStreamID/* message stream id */)
    {
        tRtmpChunkHeader.tBasicHeader.bChunkType = RTMP_CHUNK_TYPE_1;
        tRtmpChunkHeader.tMsgHeader.dwTimestamp -= tLastRtmpChunkHeader.tMsgHeader.dwTimestamp; // timestamp delta,>0xFFFFFF其实也可以,后续优化
        if (i_ptRtmpChunkHeader->tMsgHeader.bTypeID == tLastRtmpChunkHeader.tMsgHeader.bTypeID && 
        i_ptRtmpChunkHeader->tMsgHeader.dwLength == tLastRtmpChunkHeader.tMsgHeader.dwLength)
        {
            tRtmpChunkHeader.tBasicHeader.bChunkType = RTMP_CHUNK_TYPE_2;
            if (i_ptRtmpChunkHeader->tMsgHeader.dwTimestamp == 0)
                tRtmpChunkHeader.tBasicHeader.bChunkType = RTMP_CHUNK_TYPE_3;
        }
    }

    memcpy(o_ptRtmpChunkHeader,&tRtmpChunkHeader,sizeof(T_RtmpChunkHeader));

    return 0;
}

/*****************************************************************************
-Fuction        : 
-Description    : rtmp_chunk_basic_header
// 5.3.1.1. Chunk Basic Header (p12)
 0 1 2 3 4 5 6 7
+-+-+-+-+-+-+-+-+
|fmt|   cs id   |
+-+-+-+-+-+-+-+-+

 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|fmt|     0     |   cs id - 64  |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|fmt|     1     |          cs id - 64           |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
-Input          : 
-Output         : 
-Return         : BasicHeaderLen
* Modify Date     Version        Author           Modification
* -----------------------------------------------
* 2023/09/21      V1.0.0         Yu Weifeng       Created
******************************************************************************/
int RtmpPack::SetRtmpChunkBasicHeader(unsigned char* out, unsigned char fmt, unsigned int id)
{
    if (id >= 64 + 256)
    {
        *out++ = (fmt << 6) | 1;
        *out++ = (unsigned char)((id - 64) & 0xFF);
        *out++ = (unsigned char)(((id - 64) >> 8) & 0xFF);
        return 3;
    }
    else if (id >= 64)
    {
        *out++ = (fmt << 6) | 0;
        *out++ = (unsigned char)(id - 64);
        return 2;
    }
    else
    {
        *out++ = (fmt << 6) | (unsigned char)id;
        return 1;
    }
}

/*****************************************************************************
-Fuction        : 
-Description    : rtmp_chunk_message_header
// 5.3.1.2. Chunk Message Header (p13)
 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                   timestamp                   |message length |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
| message length (cont)         |message type id| msg stream id |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|           message stream id (cont)            |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
-Input          : 
-Output         : 
-Return         : BasicHeaderLen
* Modify Date     Version        Author           Modification
* -----------------------------------------------
* 2023/09/21      V1.0.0         Yu Weifeng       Created
******************************************************************************/
int RtmpPack::SetRtmpChunkMsgHeader(unsigned char* o_pbBuf, T_RtmpChunkHeader *i_ptRtmpChunkHeader)
{
    int iLen = 0;
    unsigned char* pbBuf = NULL;

    // timestamp / delta
    if (i_ptRtmpChunkHeader->tBasicHeader.bChunkType <= RTMP_CHUNK_TYPE_2)
    {
        pbBuf = &o_pbBuf[iLen];
        Write24BE(pbBuf, i_ptRtmpChunkHeader->tMsgHeader.dwTimestamp >= 0xFFFFFF ? 0xFFFFFF : i_ptRtmpChunkHeader->tMsgHeader.dwTimestamp);
        iLen += 3;
    }

    // message length + type
    if (i_ptRtmpChunkHeader->tBasicHeader.bChunkType <= RTMP_CHUNK_TYPE_1)
    {
        pbBuf = &o_pbBuf[iLen];
        Write24BE(pbBuf, i_ptRtmpChunkHeader->tMsgHeader.dwLength);
        iLen += 3;
        o_pbBuf[iLen] = i_ptRtmpChunkHeader->tMsgHeader.bTypeID;
        iLen ++;
    }

    // message stream id
    if (i_ptRtmpChunkHeader->tBasicHeader.bChunkType == RTMP_CHUNK_TYPE_0)
    {
        pbBuf = &o_pbBuf[iLen];
        Write32LE(pbBuf, i_ptRtmpChunkHeader->tMsgHeader.dwStreamID);
        iLen += 4;
    }

    return iLen;
}

/*****************************************************************************
-Fuction        : SetRtmpChunkExtendedTimestamp
-Description    : rtmp_chunk_extended_timestamp
// 5.3.1.3. Extended Timestamp (p16)
-Input          : 
-Output         : 
-Return         : BasicHeaderLen
* Modify Date     Version        Author           Modification
* -----------------------------------------------
* 2023/09/21      V1.0.0         Yu Weifeng       Created
******************************************************************************/
int RtmpPack::SetRtmpChunkExtendedTimestamp(unsigned char* out, unsigned int timestamp)
{
    // extended timestamp
    Write32BE(out, timestamp);
    
    return 4;
}


/*****************************************************************************
-Fuction        : RandomGenerate
-Description    : 
-Input          : 
-Output         : 
-Return         : -1 err >0 len
* Modify Date     Version        Author           Modification
* -----------------------------------------------
* 2023/09/21      V1.0.0         Yu Weifeng       Created
******************************************************************************/
int RtmpPack::CreateControlMsgHeader(char* o_pcBuf, int i_iBufMaxLen,unsigned char i_bMsgTypeID,int i_iMsgLen)
{
    // 5.4. Protocol Control Messages (p18)
    // These protocol control messages MUST have message stream ID 0 (known
    // as the control stream) and be sent in chunk stream ID 2.
    int iRet = -1;
    int iLen = RTMP_CTL_MSG_LEN;
    
    if(NULL == o_pcBuf ||i_iBufMaxLen < iLen)
    {
        RTMP_LOGE("CreateControlMsgHeader err %d %d\r\n",i_iBufMaxLen,i_bMsgTypeID);
        return iRet;
    }
    o_pcBuf[0] = (0x00 << 6) /*fmt*/ | RTMP_CONTROL_CHANNEL_CSID /*cs id*/;

    /* timestamp */
    o_pcBuf[1] = 0x00;
    o_pcBuf[2] = 0x00;
    o_pcBuf[3] = 0x00;

    /* message length */
    o_pcBuf[4] = (uint8_t)(i_iMsgLen >> 16);
    o_pcBuf[5] = (uint8_t)(i_iMsgLen >> 8);
    o_pcBuf[6] = (uint8_t)i_iMsgLen;

    /* message type id */
    o_pcBuf[7] = i_bMsgTypeID;

    /* message stream id */
    o_pcBuf[8] = RTMP_MSG_STREAM_ID0;
    o_pcBuf[9] = RTMP_MSG_STREAM_ID0;
    o_pcBuf[10] = RTMP_MSG_STREAM_ID0;
    o_pcBuf[11] = RTMP_MSG_STREAM_ID0;

    return iLen;
}

/*****************************************************************************
-Fuction        : GetLastRtmpChunkHeader
-Description    : 
-Input          : 
-Output         : 
-Return         : Version
* Modify Date     Version        Author           Modification
* -----------------------------------------------
* 2023/09/21      V1.0.0         Yu Weifeng       Created
******************************************************************************/
int RtmpPack::FindLastRtmpChunkHeader(unsigned int i_dwChunkStreamID)
{
    int iRet = -1;
    int i = 0;

    for(i = 0;i < (int)(sizeof(m_atLastRtmpChunkHeader)/sizeof(T_RtmpChunkHeader));i++)
    {
        if(m_atLastRtmpChunkHeader[i].tBasicHeader.dwChunkStreamID == i_dwChunkStreamID)
        {
            iRet = 0;
            break;
        }
    }
    if(0 == iRet)
    {
        iRet = i;
    }
    return iRet;
}

/*****************************************************************************
-Fuction        : GetLastRtmpChunkHeader
-Description    : 
-Input          : 
-Output         : 
-Return         : Version
* Modify Date     Version        Author           Modification
* -----------------------------------------------
* 2023/09/21      V1.0.0         Yu Weifeng       Created
******************************************************************************/
int RtmpPack::GetLastRtmpChunkHeader(unsigned int i_dwChunkStreamID,T_RtmpChunkHeader * o_ptRtmpChunkHeader)
{
    int iRet = -1;
    
    if(NULL == o_ptRtmpChunkHeader)
    {
        RTMP_LOGE("GetLastRtmpChunkHeader NULL %d \r\n",i_dwChunkStreamID);
        return iRet;
    }
    iRet = FindLastRtmpChunkHeader(i_dwChunkStreamID);
    if(iRet < 0)
    {
        RTMP_LOGW("GetLastRtmpChunkHeader   no find %d \r\n",i_dwChunkStreamID);
        return iRet;
    }

    memcpy(o_ptRtmpChunkHeader,&m_atLastRtmpChunkHeader[iRet],sizeof(T_RtmpChunkHeader));
    return iRet;
}

/*****************************************************************************
-Fuction        : GetLastRtmpChunkHeader
-Description    : 
-Input          : 
-Output         : 
-Return         : Version
* Modify Date     Version        Author           Modification
* -----------------------------------------------
* 2023/09/21      V1.0.0         Yu Weifeng       Created
******************************************************************************/
int RtmpPack::SetLastRtmpChunkHeader(unsigned int i_dwChunkStreamID,T_RtmpChunkHeader * i_ptRtmpChunkHeader)
{
    int iRet = -1;
    int i = 0;
    
    if(NULL == i_ptRtmpChunkHeader)
    {
        RTMP_LOGE("SetLastRtmpChunkHeader NULL %d \r\n",i_dwChunkStreamID);
        return iRet;
    }
    iRet = FindLastRtmpChunkHeader(i_dwChunkStreamID);
    if(iRet < 0)
    {
        for(i = 0;i < (int)(sizeof(m_atLastRtmpChunkHeader)/sizeof(T_RtmpChunkHeader));i++)
        {
            if(m_atLastRtmpChunkHeader[i].tBasicHeader.dwChunkStreamID == 0)
            {
                iRet = i;
                break;
            }
        }
        if(iRet < 0)
        {
            RTMP_LOGE("SetLastRtmpChunkHeader err %d \r\n",i_dwChunkStreamID);
            return iRet;
        }
    }
    memcpy(&m_atLastRtmpChunkHeader[iRet],i_ptRtmpChunkHeader,sizeof(T_RtmpChunkHeader));
    return iRet;
}


/*****************************************************************************
-Fuction        : RandomGenerate
-Description    : 
-Input          : 
-Output         : 
-Return         : -1 err >0 len
* Modify Date     Version        Author           Modification
* -----------------------------------------------
* 2023/09/21      V1.0.0         Yu Weifeng       Created
******************************************************************************/
int RtmpPack::RandomGenerate(char* o_pcBuf, int i_iBufLen)
{
    if(NULL == m_tRtmpPackCb.GetRandom || NULL == o_pcBuf)
    {
        RTMP_LOGE("m_tRtmpPackCb.GetRandom NULL \r\n");
        return -1;
    }
    for (int i = 0; i < i_iBufLen; i++) 
    {
        // the common value in [0x0f, 0xf0]
        o_pcBuf[i] = 0x0f + (m_tRtmpPackCb.GetRandom() % (256 - 0x0f - 0x0f));
    }
    return 0;
}


