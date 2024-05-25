/*****************************************************************************
* Copyright (C) 2023-2028 Hanson Yu  All rights reserved.
------------------------------------------------------------------------------
* File Module       :   RtmpParse.h
* Description       :   RtmpParse operation center
                        RTMP格式协议解析处理
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
    
#include "RtmpParse.h"
#include "RtmpAdapter.h"
#include "AMF/include/amf0.h"


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
RtmpParse::RtmpParse()
{
    memset(&m_atLastRtmpChunkHeader,0,sizeof(m_atLastRtmpChunkHeader));
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
RtmpParse::~RtmpParse()
{

}

/*****************************************************************************
-Fuction        : RtmpParse
-Description    : 
-Input          : 
-Output         : 
-Return         : Version
* Modify Date     Version        Author           Modification
* -----------------------------------------------
* 2023/09/21      V1.0.0         Yu Weifeng       Created
******************************************************************************/
int RtmpParse::GetRtmpVersion(char *i_pcBuf,int i_iBufLen)
{
    int iRtmpVersion = 0;
    if(i_iBufLen > 0)
    {
        iRtmpVersion = (int)i_pcBuf[0];
    }
    return iRtmpVersion;
}

/*****************************************************************************
-Fuction        : RtmpParse
-Description    : ParseRtmpChunk，封装成消息块chunk发送的
    组装成完整消息
-Input          : 
-Output         : 
-Return         : Version
* Modify Date     Version        Author           Modification
* -----------------------------------------------
* 2023/09/21      V1.0.0         Yu Weifeng       Created
******************************************************************************/
int RtmpParse::GetRtmpHeader(char *i_pcBuf,int i_iBufLen,T_RtmpChunkHeader *o_ptRtmpChunkHeader)
{
    int iRet = -1;//0
    int iProcessedLen = 0;
    T_RtmpBasicHeader tBasicHeader;
    T_RtmpMsgHeader tMsgHeader;
    unsigned int dwTimestamp = 0;
    int iExtendedTimestampLen = 0;
    T_RtmpChunkHeader tLastRtmpChunkHeader;
    
    if(NULL == i_pcBuf || NULL == o_ptRtmpChunkHeader)
    {
        RTMP_LOGE("GetRtmpHeader NULL %d \r\n",i_iBufLen);
        return iRet;
    }
    memset(o_ptRtmpChunkHeader,0,sizeof(T_RtmpChunkHeader));
    memset(&tBasicHeader,0,sizeof(T_RtmpBasicHeader));
    iProcessedLen = GetRtmpBasicHeader(i_pcBuf,i_iBufLen,&tBasicHeader);
    if(iProcessedLen <= 0 ||tBasicHeader.bChunkType > RTMP_CHUNK_TYPE_3 ||
    tBasicHeader.dwChunkStreamID < RTMP_CONTROL_CHANNEL_CSID ||tBasicHeader.dwChunkStreamID >= RTMP_MAX_CHANNEL_CSID)
    {
        RTMP_LOGE("GetRtmpBasicHeader err fmt %d,cid %d \r\n",tBasicHeader.bChunkType,tBasicHeader.dwChunkStreamID);
        return iRet;
    }
    memset(&tLastRtmpChunkHeader,0,sizeof(T_RtmpChunkHeader));
    if(GetLastRtmpChunkHeader(tBasicHeader.dwChunkStreamID,&tLastRtmpChunkHeader) < 0)
    {
        if (tBasicHeader.bChunkType != RTMP_CHUNK_TYPE_0)
        {
            RTMP_LOGE("GetLastRtmpChunkHeader err %d \r\n",iProcessedLen);
            return iRet;
        }
    }
    
    memset(&tMsgHeader,0,sizeof(T_RtmpMsgHeader));
    iProcessedLen += GetRtmpMsgHeader(i_pcBuf+iProcessedLen,i_iBufLen-iProcessedLen,tBasicHeader.bChunkType,&tMsgHeader);
    if(0 == tMsgHeader.dwStreamID)
    {//
        tMsgHeader.dwStreamID = tLastRtmpChunkHeader.tMsgHeader.dwStreamID;
    }
    if(0 == tMsgHeader.dwLength)
    {//
        tMsgHeader.dwLength = tLastRtmpChunkHeader.tMsgHeader.dwLength;
    }
    if(0 == tMsgHeader.bTypeID)
    {//
        tMsgHeader.bTypeID = tLastRtmpChunkHeader.tMsgHeader.bTypeID;
    }
    if (tMsgHeader.dwTimestamp == 0xFFFFFF)
    {
        iProcessedLen += GetExtendedTimestamp(i_pcBuf+iProcessedLen,i_iBufLen-iProcessedLen,&tMsgHeader.dwTimestamp);
        //o_ptRtmpChunkHeader->iExtendedTimestamp = 1;
    }
    else//特殊处理TYPE_3带扩展时间戳的问题
    {//协议6.1.3.规定TYPE_3是不能带，但是微信小程序带了
        if(RTMP_CHUNK_TYPE_3 == tBasicHeader.bChunkType)//if (tLastRtmpChunkHeader.iExtendedTimestamp == 1) //(用标记判断则重置条件不明确)
        {//用标记判断需考虑是否符合头部的定义以及使用的地方不依赖整体大小,目前看可以使用
            iExtendedTimestampLen = GetExtendedTimestamp(i_pcBuf+iProcessedLen,i_iBufLen-iProcessedLen,&dwTimestamp);
            if(dwTimestamp == tLastRtmpChunkHeader.tMsgHeader.dwTimestamp && 0 != tLastRtmpChunkHeader.tMsgHeader.dwTimestamp)//暂时使用这个判断,影响最小
            {//&& 0 != tLastRtmpChunkHeader.tMsgHeader.dwTimestamp，如果使用了扩展时间戳，则时间戳不可能为0
                iProcessedLen += iExtendedTimestampLen;//&& 0 != ...解决zlmediakit来取流的时候时间戳为0，分包后面也都是0,就会走到这里
            }//造成body处理过滤掉的问题
        }
        else
        {
            //o_ptRtmpChunkHeader->iExtendedTimestamp = 0;
        }
    }
    if(RTMP_CHUNK_TYPE_0 != tBasicHeader.bChunkType)
    {//delta Timestamp
        tMsgHeader.dwTimestamp = tMsgHeader.dwTimestamp + tLastRtmpChunkHeader.tMsgHeader.dwTimestamp;
    }
    if(tMsgHeader.bTypeID >= RTMP_MSG_TYPE_MAX)
    {//
        RTMP_LOGE("GetRtmpMsgHeader bTypeID err %d \r\n",tMsgHeader.bTypeID);
        return iRet;
    }
    memcpy(&o_ptRtmpChunkHeader->tBasicHeader,&tBasicHeader,sizeof(T_RtmpBasicHeader));
    memcpy(&o_ptRtmpChunkHeader->tMsgHeader,&tMsgHeader,sizeof(T_RtmpMsgHeader));

    memcpy(&tLastRtmpChunkHeader,o_ptRtmpChunkHeader,sizeof(T_RtmpChunkHeader));
    SetLastRtmpChunkHeader(o_ptRtmpChunkHeader->tBasicHeader.dwChunkStreamID,&tLastRtmpChunkHeader);
    //RTMP_LOGD("GetRtmpMsgHeader iProcessedLen %d \r\n",iProcessedLen);
    return iProcessedLen;
}

/*****************************************************************************
-Fuction        : RtmpParse
-Description    : 
-Input          : 
-Output         : 
-Return         : Version
* Modify Date     Version        Author           Modification
* -----------------------------------------------
* 2023/09/21      V1.0.0         Yu Weifeng       Created
******************************************************************************/
int RtmpParse::GetChunkHeaderLen(char *i_pcBuf,int i_iBufLen,unsigned int i_dwLastChunkTimestamp)
{
    int iChunkHeaderLen = 0;
    unsigned char fmt = 0;
    unsigned int cid = 0;
    int iBasicHeaderLen = 0;
    unsigned int dwTimestamp = 0;


    if(NULL == i_pcBuf || i_iBufLen < 3)
    {
        RTMP_LOGE("GetChunkHeaderLen NULL %d \r\n",i_iBufLen);
        return -1;
    }
    
    fmt = i_pcBuf[0] >> 6;
    cid = i_pcBuf[0] & 0x3F;
    if (0 == cid)
    {
        cid = 64 + (unsigned int)i_pcBuf[1];
        iBasicHeaderLen = 2;
    }
    else if (1 == cid)
    {
        cid = 64 + (unsigned int)i_pcBuf[1] + ((unsigned int)i_pcBuf[2] << 8) /* 256 */;
        iBasicHeaderLen = 3;
    }
    else
    {
        iBasicHeaderLen = 1;
    }

    iChunkHeaderLen = iBasicHeaderLen;

    // timestamp / delta
    if (fmt <= RTMP_CHUNK_TYPE_2)
    {
        Read24BE((i_pcBuf + iBasicHeaderLen), &dwTimestamp);
        iChunkHeaderLen += 3;
    }

    // message length + type
    if (fmt <= RTMP_CHUNK_TYPE_1)
    {
        iChunkHeaderLen += 4;
    }

    // message stream id
    if (fmt == RTMP_CHUNK_TYPE_0)
    {
        iChunkHeaderLen += 4;
    }

    if (dwTimestamp == 0xFFFFFF || i_dwLastChunkTimestamp == 0xFFFFFF)
    {
        iChunkHeaderLen += 4;
    }

    return iChunkHeaderLen;
}

/*****************************************************************************
-Fuction        : RtmpParse
-Description    : 
-Input          : 
-Output         : 
-Return         : Version
* Modify Date     Version        Author           Modification
* -----------------------------------------------
* 2023/09/21      V1.0.0         Yu Weifeng       Created
******************************************************************************/
int RtmpParse::ParseCmdMsg(unsigned char *i_pbBuf,int i_iBufLen,char * o_strCommandBuf,int i_iCommandBufMaxLen,double * o_dlTransaction)
{
    int iProcessedLen = 0;
    unsigned char *pbEnd = i_pbBuf + i_iBufLen;
    const unsigned char *pDataOffset = NULL;
    struct amf_object_item_t atTtems[2];

    
    if(NULL == i_pbBuf || NULL == o_strCommandBuf || NULL == o_dlTransaction)
    {
        RTMP_LOGE("ParseCmdMsg err %d \r\n",i_iBufLen);
        return -1;
    }
    AMF_OBJECT_ITEM_VALUE(atTtems[0], AMF_STRING, "command", o_strCommandBuf, i_iCommandBufMaxLen);
    AMF_OBJECT_ITEM_VALUE(atTtems[1], AMF_NUMBER, "transactionId", o_dlTransaction, sizeof(double));
    pDataOffset = amf_read_items(i_pbBuf, pbEnd, atTtems, sizeof(atTtems) / sizeof(struct amf_object_item_t));
    if (!pDataOffset)
        return -1; // invalid data

    iProcessedLen = pDataOffset - i_pbBuf;

    return iProcessedLen;
}

/*****************************************************************************
-Fuction        : RtmpParse
-Description    : 
-Input          : 
-Output         : 
-Return         : Version
* Modify Date     Version        Author           Modification
* -----------------------------------------------
* 2023/09/21      V1.0.0         Yu Weifeng       Created
******************************************************************************/
int RtmpParse::ParseControlMsg(unsigned char *i_pbBuf,int i_iBufLen,unsigned int *o_pdwControlData)
{
    int iProcessedLen = 0;
    
    if(NULL == i_pbBuf || i_iBufLen < 4)
    {
        RTMP_LOGE("ParseControlMsg err %d \r\n",i_iBufLen);
        return -1;
    }
    Read32BE(i_pbBuf,o_pdwControlData);
    return 0;
}

/*****************************************************************************
-Fuction        : RtmpParse
-Description    : 
-Input          : 
-Output         : 
-Return         : Version
* Modify Date     Version        Author           Modification
* -----------------------------------------------
* 2023/09/21      V1.0.0         Yu Weifeng       Created
******************************************************************************/
int RtmpParse::ParseCmdConnect(unsigned char *i_pbBuf,int i_iBufLen,T_RtmpConnectContent * o_ptRtmpConnectContent)
{
    int iRet = -1;
    T_RtmpConnectContent tRtmpConnectContent;
    struct amf_object_item_t atItems[1];
    struct amf_object_item_t atCommands[8];

    
    if(NULL == i_pbBuf || NULL == o_ptRtmpConnectContent )
    {
        RTMP_LOGE("ParseCmdConnect err %d \r\n",i_iBufLen);
        return iRet;
    }

    memset(&tRtmpConnectContent, 0, sizeof(T_RtmpConnectContent));
    tRtmpConnectContent.dlEncoding = (double)RTMP_CMD_ENCODING_AMF_0;
    AMF_OBJECT_ITEM_VALUE(atCommands[0], AMF_STRING, "app", tRtmpConnectContent.strApp, sizeof(tRtmpConnectContent.strApp));
    AMF_OBJECT_ITEM_VALUE(atCommands[1], AMF_STRING, "flashver", tRtmpConnectContent.strFlashVer, sizeof(tRtmpConnectContent.strFlashVer));
    AMF_OBJECT_ITEM_VALUE(atCommands[2], AMF_STRING, "tcUrl", tRtmpConnectContent.strTcUrl, sizeof(tRtmpConnectContent.strTcUrl));
    AMF_OBJECT_ITEM_VALUE(atCommands[3], AMF_BOOLEAN, "fpad", &tRtmpConnectContent.bFpad, 1);
    AMF_OBJECT_ITEM_VALUE(atCommands[4], AMF_NUMBER, "audioCodecs", &tRtmpConnectContent.dlAudioCodecs, 8);
    AMF_OBJECT_ITEM_VALUE(atCommands[5], AMF_NUMBER, "videoCodecs", &tRtmpConnectContent.dlVideoCodecs, 8);
    AMF_OBJECT_ITEM_VALUE(atCommands[6], AMF_NUMBER, "videoFunction", &tRtmpConnectContent.dlVideoFunction, 8);
    AMF_OBJECT_ITEM_VALUE(atCommands[7], AMF_NUMBER, "objectEncoding", &tRtmpConnectContent.dlEncoding, 8);

    AMF_OBJECT_ITEM_VALUE(atItems[0], AMF_OBJECT, "command", atCommands, sizeof(atCommands) / sizeof(struct amf_object_item_t));

    iRet = amf_read_items(i_pbBuf, i_pbBuf + i_iBufLen, atItems, sizeof(atItems) / sizeof(struct amf_object_item_t)) ? 0 : -1;
    
    memcpy(o_ptRtmpConnectContent, &tRtmpConnectContent, sizeof(T_RtmpConnectContent));
    return iRet;
}

/*****************************************************************************
-Fuction        : RtmpParse
-Description    : 
-Input          : 
-Output         : 
-Return         : Version
* Modify Date     Version        Author           Modification
* -----------------------------------------------
* 2023/09/21      V1.0.0         Yu Weifeng       Created
******************************************************************************/
int RtmpParse::ParseCmdCreateStream(unsigned char *i_pbBuf,int i_iBufLen)
{
    int iRet = -1;
    struct amf_object_item_t atItems[1];
    
    if(NULL == i_pbBuf)
    {
        RTMP_LOGE("ParseCmdCreateStream err %d \r\n",i_iBufLen);
        return iRet;
    }

    AMF_OBJECT_ITEM_VALUE(atItems[0], AMF_OBJECT, "command", NULL, 0);

    iRet = amf_read_items(i_pbBuf, i_pbBuf + i_iBufLen, atItems, sizeof(atItems) / sizeof(struct amf_object_item_t)) ? 0 : -1;

    return iRet;
}

/*****************************************************************************
-Fuction        : RtmpParse
-Description    : 
-Input          : 
-Output         : 
-Return         : Version
* Modify Date     Version        Author           Modification
* -----------------------------------------------
* 2023/09/21      V1.0.0         Yu Weifeng       Created
******************************************************************************/
int RtmpParse::ParseCmdGetStreamLength(unsigned char *i_pbBuf,int i_iBufLen,char *o_strStreamName,int i_iNameMaxLen)
{
    int iRet = -1;
    struct amf_object_item_t atItems[3];
    
    if(NULL == i_pbBuf || NULL == o_strStreamName )
    {
        RTMP_LOGE("ParseCmdGetStreamLength err %d \r\n",i_iBufLen);
        return iRet;
    }
    AMF_OBJECT_ITEM_VALUE(atItems[0], AMF_OBJECT, "command", NULL, 0);
    AMF_OBJECT_ITEM_VALUE(atItems[1], AMF_STRING, "playpath", o_strStreamName,i_iNameMaxLen);

    iRet = amf_read_items(i_pbBuf, i_pbBuf + i_iBufLen, atItems, sizeof(atItems) / sizeof(struct amf_object_item_t)) ? 0 : -1;

    return iRet;
}

/*****************************************************************************
-Fuction        : RtmpParse
-Description    : 
-Input          : 
-Output         : 
-Return         : Version
* Modify Date     Version        Author           Modification
* -----------------------------------------------
* 2023/09/21      V1.0.0         Yu Weifeng       Created
******************************************************************************/
int RtmpParse::ParseCmdPlay(unsigned char *i_pbBuf,int i_iBufLen,T_RtmpPlayContent * o_ptRtmpPlayContent)
{
    int iRet = -1;
    T_RtmpPlayContent tRtmpPlayContent;
    struct amf_object_item_t atItems[5];

    
    if(NULL == i_pbBuf || NULL == o_ptRtmpPlayContent )
    {
        RTMP_LOGE("ParseCmdPlay err %d \r\n",i_iBufLen);
        return iRet;
    }
    memset(&tRtmpPlayContent, 0, sizeof(T_RtmpPlayContent));
    tRtmpPlayContent.dlDuration = -1; // duration of playback in seconds, [default] -1-live/record ends, 0-single frame, >0-play duration
    tRtmpPlayContent.dlStart= -2; // the start time in seconds, [default] -2-live/vod, -1-live only, >=0-seek position
    AMF_OBJECT_ITEM_VALUE(atItems[0], AMF_OBJECT, "command", NULL, 0);
    AMF_OBJECT_ITEM_VALUE(atItems[1], AMF_STRING, "stream", tRtmpPlayContent.strStreamName, sizeof(tRtmpPlayContent.strStreamName));
    AMF_OBJECT_ITEM_VALUE(atItems[2], AMF_NUMBER, "start", &tRtmpPlayContent.dlStart, 8);
    AMF_OBJECT_ITEM_VALUE(atItems[3], AMF_NUMBER, "duration", &tRtmpPlayContent.dlDuration, 8);
    AMF_OBJECT_ITEM_VALUE(atItems[4], AMF_BOOLEAN, "reset", &tRtmpPlayContent.bReset, 1);

    iRet = amf_read_items(i_pbBuf, i_pbBuf + i_iBufLen, atItems, sizeof(atItems) / sizeof(struct amf_object_item_t)) ? 0 : -1;
    
    memcpy(o_ptRtmpPlayContent, &tRtmpPlayContent, sizeof(T_RtmpPlayContent));
    return iRet;
}


/*****************************************************************************
-Fuction        : ParseCmdPublish
-Description    : 
-Input          : 
-Output         : 
-Return         : Version
* Modify Date     Version        Author           Modification
* -----------------------------------------------
* 2023/09/21      V1.0.0         Yu Weifeng       Created
******************************************************************************/
int RtmpParse::ParseCmdPublish(unsigned char *i_pbBuf,int i_iBufLen,T_RtmpPublishContent * o_ptRtmpPublishContent)
{
    int iRet = -1;
    T_RtmpPublishContent tRtmpPublishContent;
    struct amf_object_item_t atItems[3];

    
    if(NULL == i_pbBuf || NULL == o_ptRtmpPublishContent)
    {
        RTMP_LOGE("ParseCmdPublish err %d \r\n",i_iBufLen);
        return iRet;
    }
    memset(&tRtmpPublishContent, 0, sizeof(T_RtmpPublishContent));
    AMF_OBJECT_ITEM_VALUE(atItems[0], AMF_OBJECT, "command", NULL, 0);
    AMF_OBJECT_ITEM_VALUE(atItems[1], AMF_STRING, "name", tRtmpPublishContent.strStreamName, sizeof(tRtmpPublishContent.strStreamName));
    AMF_OBJECT_ITEM_VALUE(atItems[2], AMF_STRING, "type", tRtmpPublishContent.strStreamType, sizeof(tRtmpPublishContent.strStreamType));

    iRet = amf_read_items(i_pbBuf, i_pbBuf + i_iBufLen, atItems, sizeof(atItems) / sizeof(struct amf_object_item_t)) ? 0 : -1;
    
    memcpy(o_ptRtmpPublishContent, &tRtmpPublishContent, sizeof(T_RtmpPublishContent));
    return iRet;
}
/*****************************************************************************
-Fuction        : GetRtmpBasicHeader
-Description    : 
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
-Input          : i_pcBuf 完整的rtmp packet
-Output         : 
-Return         : BasicHeaderLen
* Modify Date     Version        Author           Modification
* -----------------------------------------------
* 2023/09/21      V1.0.0         Yu Weifeng       Created
******************************************************************************/
int RtmpParse::GetRtmpBasicHeader(char *i_pcBuf,int i_iBufLen,T_RtmpBasicHeader *o_ptBasicHeader)
{
    int iBasicHeaderLen = 0;

    unsigned char fmt = 0;
    unsigned int cid = 0;
    
    if(NULL == o_ptBasicHeader ||NULL == i_pcBuf)// || 
    {
        RTMP_LOGE("GetRtmpBasicHeader NULL %d \r\n",i_iBufLen);
        return iBasicHeaderLen;
    }
    fmt = (unsigned char)i_pcBuf[0] >> 6;
    cid = i_pcBuf[0] & 0x3F;
    if (0 == cid)
    {
        if(i_iBufLen < 2)// || 
        {
            RTMP_LOGE("i_iBufLen < 2 %d \r\n",i_iBufLen);
            return iBasicHeaderLen;
        }
        cid = 64 + (unsigned int)i_pcBuf[1];
        iBasicHeaderLen = 2;
    }
    else if (1 == cid)
    {
        if(i_iBufLen < 3)// || 
        {
            RTMP_LOGE("i_iBufLen < 3 %d \r\n",i_iBufLen);
            return iBasicHeaderLen;
        }
        cid = 64 + (unsigned int)i_pcBuf[1] + ((unsigned int)i_pcBuf[2] << 8) /* 256 */;
        iBasicHeaderLen = 3;
    }
    else
    {
        iBasicHeaderLen = 1;
    }
    o_ptBasicHeader->bChunkType = fmt;
    o_ptBasicHeader->dwChunkStreamID = cid;
    
    return iBasicHeaderLen;
}

/*****************************************************************************
-Fuction        : GetRtmpMsgHeader
-Description    : 
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
int RtmpParse::GetRtmpMsgHeader(char *i_pcBuf,int i_iBufLen,unsigned char i_bChunkType,T_RtmpMsgHeader *o_ptRtmpMsgHeader)
{
    int offset = 0;

    // timestamp / delta
    if (i_bChunkType <= RTMP_CHUNK_TYPE_2)
    {
        Read24BE((i_pcBuf + offset), &o_ptRtmpMsgHeader->dwTimestamp);
        offset += 3;
    }

    // message length + type
    if (i_bChunkType <= RTMP_CHUNK_TYPE_1)
    {
        Read24BE((i_pcBuf + offset), &o_ptRtmpMsgHeader->dwLength);
        o_ptRtmpMsgHeader->bTypeID= i_pcBuf[offset + 3];
        offset += 4;
    }

    // message stream id
    if (i_bChunkType == RTMP_CHUNK_TYPE_0)
    {
        Read32LE((i_pcBuf + offset), &o_ptRtmpMsgHeader->dwStreamID);
        offset += 4;
    }

    return offset;

}

/*****************************************************************************
-Fuction        : GetRtmpMsgHeader
-Description    : 
// 5.3.1.3. Extended Timestamp (p16)
-Input          : 
-Output         : 
-Return         : BasicHeaderLen
* Modify Date     Version        Author           Modification
* -----------------------------------------------
* 2023/09/21      V1.0.0         Yu Weifeng       Created
******************************************************************************/
int RtmpParse::GetExtendedTimestamp(char *i_pcBuf,int i_iBufLen,unsigned int *o_dwExtendedTimestamp)
{
    int iExtendedTimestampLen = 0;

    if(NULL == o_dwExtendedTimestamp ||NULL == i_pcBuf || i_iBufLen < 4)
    {
        RTMP_LOGW("GetExtendedTimestamp NULL %d \r\n",i_iBufLen);
        return iExtendedTimestampLen;
    }

    // extended timestamp
    Read32BE(i_pcBuf,o_dwExtendedTimestamp);
    iExtendedTimestampLen = 4;

    return iExtendedTimestampLen;
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
int RtmpParse::FindLastRtmpChunkHeader(unsigned int i_dwChunkStreamID)
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
int RtmpParse::GetLastRtmpChunkHeader(unsigned int i_dwChunkStreamID,T_RtmpChunkHeader * o_ptRtmpChunkHeader)
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
int RtmpParse::SetLastRtmpChunkHeader(unsigned int i_dwChunkStreamID,T_RtmpChunkHeader * i_ptRtmpChunkHeader)
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


