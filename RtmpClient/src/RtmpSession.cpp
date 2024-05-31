/*****************************************************************************
* Copyright (C) 2023-2028 Hanson Yu  All rights reserved.
------------------------------------------------------------------------------
* File Module       :   RtmpSession.h
* Description       :   RtmpSession operation center
                        各RTMP会话维护(保存RTMP协议交互过程信息)
                        通过主函数cycle
                        处理协议中的各种命令消息,不阻塞
* Created           :   2023.09.21.
* Author            :   Yu Weifeng
* Function List     :   
* Last Modified     :   
* History           :   
******************************************************************************/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <string>

#include "RtmpMediaHandle.h"
#include "RtmpSession.h"
#include "RtmpCommon.h"
#include "RtmpAdapter.h"

using std::string;

/*****************************************************************************
-Fuction        : RtmpSession
-Description    : 
-Input          : 
-Output         : 
-Return         : 
* Modify Date     Version        Author           Modification
* -----------------------------------------------
* 2023/09/21      V1.0.0         Yu Weifeng       Created
******************************************************************************/
RtmpSession::RtmpSession(void *i_pIoHandle,T_RtmpCb *i_ptRtmpCb,T_RtmpSessionConfig *i_ptRtmpSessionConfig)
{
    m_eState = RTMP_INIT;
    m_pIoHandle = i_pIoHandle;
    m_dwConnectFailCnt = 0;
    memset(m_strStreamName,0,sizeof(m_strStreamName));


    m_dwRecvedDataLen = 0;
    m_blPushing = false;
    m_blPushStarted = false;
    m_HandleCmdMap.insert(make_pair("_result", &RtmpSession::HandCmd_result)); //WirelessNetManage::指明所属类,所以头文件里定义声明时也要指明
    m_HandleCmdMap.insert(make_pair("onStatus", &RtmpSession::HandCmdOnStatus)); //类的成员函数不是指针，所以要加 &


    //m_iClientSocketFd = i_iClientFd;
    //m_pTcpSocket = i_pTcpServer;
    m_dwOffset = 0;
    m_iChunkBodyLen = 0;
    m_iChunkRemainLen = 0;
    memset(&m_tPlayAudioParam, 0, sizeof(T_RtmpAudioParam));
    memset(&m_tPlayVideoParam, 0, sizeof(T_RtmpFrameInfo));

    
    memset(&m_tRtmpCb,0,sizeof(T_RtmpCb));
    memcpy(&m_tRtmpCb,i_ptRtmpCb,sizeof(T_RtmpCb));
    
    memset(&m_tRtmpSessionConfig,0,sizeof(T_RtmpSessionConfig));
    m_tRtmpSessionConfig.dwWindowSize= 5000000;//默认值
    m_tRtmpSessionConfig.dwPeerBandwidth = 5000000;
    m_tRtmpSessionConfig.dwOutChunkSize = RTMP_OUTPUT_CHUNK_SIZE;
    m_tRtmpSessionConfig.dwInChunkSize = RTMP_INPUT_CHUNK_MAX_SIZE;
    if(NULL != i_ptRtmpSessionConfig)
        memcpy(&m_tRtmpSessionConfig,i_ptRtmpSessionConfig,sizeof(T_RtmpSessionConfig));
    else
        RTMP_LOGW("RtmpVersion m_tRtmpSessionConfig default NULL\r\n");
    m_iHandshakeBufLen = 0;
    memset(m_abHandshakeBuf,0,sizeof(m_abHandshakeBuf));
    memset(&m_tMsgBufHandle,0,sizeof(T_RtmpMsgBufHandle));
    m_tMsgBufHandle.pbMsgBuf = new char[RTMP_MSG_MAX_LEN];
    if(NULL == m_tMsgBufHandle.pbMsgBuf)
    {
        RTMP_LOGE("new m_pbMsgBuf err %d\r\n",RTMP_MSG_MAX_LEN);
    }
    memset(m_tMsgBufHandle.pbMsgBuf,0,RTMP_MSG_MAX_LEN);
    
    memset(&m_tRtmpChunkHandle,0,sizeof(T_RtmpChunkHandle));
    m_tRtmpChunkHandle.pbChunkBuf = new char[RTMP_CHUNK_MAX_LEN];
    if(NULL == m_tRtmpChunkHandle.pbChunkBuf)
    {
        RTMP_LOGE("new pbChunkBuf err %d\r\n",RTMP_CHUNK_MAX_LEN);
    }
    else
    {
        m_tRtmpChunkHandle.iChunkMaxLen = RTMP_CHUNK_MAX_LEN;
        memset(m_tRtmpChunkHandle.pbChunkBuf,0,RTMP_CHUNK_MAX_LEN);
    }
    m_RtmpMsgHandleMap.clear();
    
    m_pRtmpParse = new RtmpParse();
    if(NULL == m_pRtmpParse)
    {
        RTMP_LOGE("RtmpVersion m_pRtmpParse err NULL\r\n");
    }
    m_pRtmpPack = new RtmpPack(&m_tRtmpCb.tRtmpPackCb);
    if(NULL == m_pRtmpPack)
    {
        RTMP_LOGE("RtmpVersion RtmpPack err NULL\r\n");
    }
    m_pRtmpMediaHandle = new RtmpMediaHandle();
    if(NULL == m_pRtmpMediaHandle)
    {
        RTMP_LOGE("RtmpVersion RtmpMediaHandle err NULL\r\n");
    }

    m_pbNaluData = new unsigned char[RTMP_MSG_MAX_LEN];
    if(NULL == m_pbNaluData)
    {
        RTMP_LOGE("new unsigned char NULL %d \r\n",RTMP_MSG_MAX_LEN);
    }
    m_pbFrameData = new unsigned char[RTMP_MSG_MAX_LEN];
    if(NULL == m_pbFrameData)
    {
        RTMP_LOGE("new unsigned char NULL %d \r\n",RTMP_MSG_MAX_LEN);
    }


    
    m_pbPlayFrameData = new unsigned char[RTMP_MSG_MAX_LEN];
    if(NULL == m_pbPlayFrameData)
    {
        RTMP_LOGE("new unsigned char NULL %d \r\n",RTMP_MSG_MAX_LEN);
    }
    m_pbFileData = new unsigned char[1024*1024];
    if(NULL == m_pbFileData)
    {
        RTMP_LOGE("new unsigned char NULL %d \r\n",1024*1024);
    }


    
}

/*****************************************************************************
-Fuction        : ~RtmpSession
-Description    : 
-Input          : 
-Output         : 
-Return         : 
* Modify Date     Version        Author           Modification
* -----------------------------------------------
* 2023/09/21      V1.0.0         Yu Weifeng       Created
******************************************************************************/
RtmpSession::~RtmpSession()
{
    if(NULL != m_pRtmpParse)
    {
        delete m_pRtmpParse;
        m_pRtmpParse = NULL;
    }
    if(NULL != m_pRtmpPack)
    {
        delete m_pRtmpPack;
        m_pRtmpPack = NULL;
    }
    if(NULL != m_pRtmpMediaHandle)
    {
        delete m_pRtmpMediaHandle;
        m_pRtmpMediaHandle = NULL;
    }
    if(NULL != m_tMsgBufHandle.pbMsgBuf)
    {
        delete [] m_tMsgBufHandle.pbMsgBuf;
        m_tMsgBufHandle.pbMsgBuf = NULL;
    }
    if(NULL != m_tRtmpChunkHandle.pbChunkBuf)
    {
        delete [] m_tRtmpChunkHandle.pbChunkBuf;
        memset(&m_tRtmpChunkHandle,0,sizeof(T_RtmpChunkHandle));
    }
    if(NULL != m_pbNaluData)
    {
        delete [] m_pbNaluData;
        m_pbNaluData = NULL;
    }
    if(NULL != m_pbFrameData)
    {
        delete [] m_pbFrameData;
        m_pbFrameData = NULL;
    }
    if(NULL != m_pbPlayFrameData)
    {
        delete [] m_pbPlayFrameData;
        m_pbPlayFrameData = NULL;
    }
    if(NULL != m_pbFileData)
    {
        delete [] m_pbFileData;
        m_pbFileData = NULL;
    }

    RTMP_LOGW("RTMP_VIDEO_FRAME end ,cnt %d \r\n",m_dwVideoFrameCntLog);
    RTMP_LOGW("RTMP_AUDIO_FRAME end ,cnt %d\r\n",m_dwAudioFrameCntLog);
}

/*****************************************************************************
-Fuction        : SendDataSampleAccess
-Description    : netstream_rtmp sampleaccess
-Input          : 
-Output         : 
-Return         : 
* Modify Date     Version        Author           Modification
* -----------------------------------------------
* 2023/09/21      V1.0.0         Yu Weifeng       Created
******************************************************************************/
int RtmpSession::SendDataOnMetaData(T_RtmpMediaInfo *i_ptRtmpMediaInfo)
{
    int iRet = -1;
    unsigned char abBuf[RTMP_DATA_MAX_LEN];
    int iLen = 0;
    char abSendBuf[RTMP_OUTPUT_CHUNK_SIZE+1024];
    T_RtmpChunkPayloadInfo tChunkPayloadInfo;
    
    memset(abBuf,0,sizeof(abBuf));
    iLen = m_pRtmpPack->CreateDataOnMetaData(i_ptRtmpMediaInfo,abBuf,sizeof(abBuf));
    if(iLen <= 0)
    {
        RTMP_LOGE("SendDataOnMetaData err %d\r\n",iLen);
        return iRet;
    }
    
    memset(&tChunkPayloadInfo,0,sizeof(T_RtmpChunkPayloadInfo));
    tChunkPayloadInfo.pcChunkPayload = (char *)abBuf;
    tChunkPayloadInfo.iPayloadLen = iLen;
    tChunkPayloadInfo.dwOutChunkSize = m_tRtmpSessionConfig.dwOutChunkSize;
    tChunkPayloadInfo.dwStreamID = RTMP_MSG_STREAM_ID1;
    do
    {
        memset(abSendBuf,0,sizeof(abSendBuf));
        iLen = m_pRtmpPack->CreateDataMsg(&tChunkPayloadInfo,abSendBuf,sizeof(abSendBuf));
        if(iLen < 0)
        {
            RTMP_LOGE("CreateDataMsg err %d\r\n",iLen);
        }
        else if(0 == iLen)
        {
            RTMP_LOGD("CreateDataMsg over %d\r\n",iLen);
        }
        else
        {
            iRet = SendData(abSendBuf,iLen);
            if(iRet < 0)
            {
                RTMP_LOGE("SendData err %d\r\n",iLen);
                return iRet;
            }
        }
    }while(iLen > 0);
    
    return iRet;
}

/*****************************************************************************
-Fuction        : RecvData
-Description    : 
-Input          : 
-Output         : 
-Return         : len
* Modify Date     Version        Author           Modification
* -----------------------------------------------
* 2023/09/21      V1.0.0         Yu Weifeng       Created
******************************************************************************/
int RtmpSession::SendData(char * i_acSendBuf,int i_iSendLen)
{
    int iRet = -1;

    if(NULL == i_acSendBuf ||i_iSendLen <= 0)
    {
        RTMP_LOGE("SendData NULL %d \r\n",i_iSendLen);
        return iRet;
    }
    if(NULL == m_tRtmpCb.SendData ||NULL == m_pIoHandle)
    {
        RTMP_LOGE("m_tRtmpCb.SendData NULL %d \r\n",i_iSendLen);
        return iRet;
    }
    iRet = m_tRtmpCb.SendData(m_pIoHandle,i_acSendBuf,i_iSendLen);
    //iRet = m_pTcpSocket->Send(i_acSendBuf,i_iSendLen,m_iClientSocketFd);
    if(iRet < 0)
    {
        RTMP_LOGE("RtmpSession::SendData err");
        iRet = -1;
    }
    else
    {
        iRet = 0;
    }
    return iRet;
}



/*****************************************************************************
-Fuction        : DoPush
-Description    : 
-Input          : 
-Output         : 
-Return         : 
* Modify Date     Version        Author           Modification
* -----------------------------------------------
* 2023/09/21      V1.0.0         Yu Weifeng       Created
******************************************************************************/
int RtmpSession::DoPush(T_RtmpMediaInfo *i_ptRtmpMediaInfo,unsigned char * i_pbFrameData,int i_iFrameLen,char *i_strPlayPath)
{
    int iRet = -1;
    T_RtmpFrameInfo tFrameInfo;
    int iVideoDataLen = 0;
    int iAudioDataLen = 0;
    T_RtmpAudioInfo tRtmpAudioInfo;

    if (NULL == i_ptRtmpMediaInfo || NULL == i_pbFrameData) 
    {
        RTMP_LOGE("DoPlay NULL %d \r\n",i_iFrameLen);
        return iRet;
    }
    
    if (false == m_blPushing) 
    {
        return iRet;
    }
    if (false == m_blPushStarted) //for metadata
    {
        if(RTMP_VIDEO_KEY_FRAME != i_ptRtmpMediaInfo->eFrameType)
        {
            RTMP_LOGE("i_ptRtmpMediaInfo->eFrameType err %d \r\n",i_ptRtmpMediaInfo->eFrameType);
            //return iRet;//不做i帧开始判断，因为有的只有音频数据
        }//出流慢是播放器缓存原因，播放器设置好可以1s出流ffplay -probesize 32 -analyzeduration 0 -i "rtmp://ip:port/(TcUrl)/StreamName"
        //SendDataOnMetaData(i_ptRtmpMediaInfo);//不发也可以正常播放,video msg中也带这些解码信息
        m_ddwLastTimestamp = i_ptRtmpMediaInfo->ddwTimestamp;//ms
        m_dwTimestamp = 0;
        m_blAudioSeqHeaderSended = false;
        m_blPushStarted = true;
        RTMP_LOGW("RTMP_VIDEO_KEY_FRAME start enc %d,frameType %d ,chan %d ,frameRate %d ,w %d h %d ,time %lld \r\n",i_ptRtmpMediaInfo->eEncType,i_ptRtmpMediaInfo->eFrameType,
        i_ptRtmpMediaInfo->dwChannels,i_ptRtmpMediaInfo->dwFrameRate,i_ptRtmpMediaInfo->dwWidth,i_ptRtmpMediaInfo->dwHeight,i_ptRtmpMediaInfo->ddwTimestamp);
    }
    if(RTMP_VIDEO_KEY_FRAME == i_ptRtmpMediaInfo->eFrameType ||RTMP_VIDEO_INNER_FRAME == i_ptRtmpMediaInfo->eFrameType)
    {
        switch (i_ptRtmpMediaInfo->eEncType)
        {
            case RTMP_ENC_H264:
            case RTMP_ENC_H265:
            {//实测，音视频时钟不同源,因此会导致时间戳不均匀
                m_dwTimestamp += (unsigned int)(i_ptRtmpMediaInfo->ddwTimestamp - m_ddwLastTimestamp);//以视频的为准
                m_ddwLastTimestamp = i_ptRtmpMediaInfo->ddwTimestamp;//ms
            
                memset(&tFrameInfo,0,sizeof(T_RtmpFrameInfo));
                tFrameInfo.pbNaluData = m_pbNaluData;
                tFrameInfo.dwNaluDataMaxLen = RTMP_MSG_MAX_LEN;
                tFrameInfo.eFrameType = i_ptRtmpMediaInfo->eFrameType;
                iRet = m_pRtmpMediaHandle->ParseNaluFromFrame(i_ptRtmpMediaInfo->eEncType, i_pbFrameData,i_iFrameLen,&tFrameInfo);
                if(iRet < 0)
                {
                    RTMP_LOGE("ParseNaluFromFrame err %d \r\n",i_ptRtmpMediaInfo->eEncType);
                    return iRet;
                }
                if(RTMP_VIDEO_KEY_FRAME == i_ptRtmpMediaInfo->eFrameType)
                {
                    iVideoDataLen = m_pRtmpMediaHandle->GenerateVideoData(&tFrameInfo, 1,m_pbFrameData,RTMP_MSG_MAX_LEN);
                    if(iVideoDataLen <= 0)
                    {
                        RTMP_LOGE("GenerateVideoData err %d \r\n",iVideoDataLen);
                        return -1;
                    }
                    iRet=SendVideo(m_dwTimestamp,m_pbFrameData,iVideoDataLen);//必须分开发送，否则播放无法解析
                }
                iRet=iVideoDataLen = m_pRtmpMediaHandle->GenerateVideoData(&tFrameInfo, 0,m_pbFrameData,RTMP_MSG_MAX_LEN);
                SendVideo(m_dwTimestamp,m_pbFrameData,iVideoDataLen);

                m_dwVideoFrameCntLog++;
                if(m_dwVideoFrameCntLog < 10)
                {
                    RTMP_LOGW("RTMP_VIDEO_FRAME %d enc %d,frameType %d ,chan %d ,frameRate %d ,w %d h %d ,time %lld \r\n",m_dwVideoFrameCntLog,i_ptRtmpMediaInfo->eEncType,i_ptRtmpMediaInfo->eFrameType,
                    i_ptRtmpMediaInfo->dwChannels,i_ptRtmpMediaInfo->dwFrameRate,i_ptRtmpMediaInfo->dwWidth,i_ptRtmpMediaInfo->dwHeight,i_ptRtmpMediaInfo->ddwTimestamp);
                }
                break;
            }
            default:
                break;
        }
    }
    else if(RTMP_AUDIO_FRAME == i_ptRtmpMediaInfo->eFrameType)
    {
        memset(&tRtmpAudioInfo,0,sizeof(T_RtmpAudioInfo));
        tRtmpAudioInfo.tParam.eEncType= i_ptRtmpMediaInfo->eEncType;
        tRtmpAudioInfo.tParam.dwBitsPerSample = i_ptRtmpMediaInfo->dwBitsPerSample;
        tRtmpAudioInfo.tParam.dwChannels = i_ptRtmpMediaInfo->dwChannels;
        tRtmpAudioInfo.tParam.dwSamplesPerSecond = i_ptRtmpMediaInfo->dwSampleRate;
        tRtmpAudioInfo.pbAudioData= i_pbFrameData;
        tRtmpAudioInfo.dwAudioDataLen= i_iFrameLen;
        switch (i_ptRtmpMediaInfo->eEncType)
        {
            case RTMP_ENC_AAC:
            {
                if(false == m_blAudioSeqHeaderSended)
                {
                    iAudioDataLen = m_pRtmpMediaHandle->GenerateAudioData(&tRtmpAudioInfo, 1,m_pbFrameData,RTMP_MSG_MAX_LEN);
                    iRet=SendAudio(m_dwTimestamp,m_pbFrameData,iAudioDataLen);
                    m_blAudioSeqHeaderSended = true;
                    RTMP_LOGW("RTMP_ENC_AAC start enc %d,frameType %d ,chan %d ,SampleRate %d ,BitsPerSample %d,time %lld \r\n",i_ptRtmpMediaInfo->eEncType,i_ptRtmpMediaInfo->eFrameType,
                    i_ptRtmpMediaInfo->dwChannels,i_ptRtmpMediaInfo->dwSampleRate,i_ptRtmpMediaInfo->dwBitsPerSample,i_ptRtmpMediaInfo->ddwTimestamp);
                }
                else
                {
                    iAudioDataLen = m_pRtmpMediaHandle->GenerateAudioData(&tRtmpAudioInfo, 0,m_pbFrameData,RTMP_MSG_MAX_LEN);
                    iRet=SendAudio(m_dwTimestamp,m_pbFrameData,iAudioDataLen);
                }
                break;
            }
            case RTMP_ENC_G711A:
            {
                if(false == m_blAudioSeqHeaderSended)
                {
                    m_blAudioSeqHeaderSended = true;
                    RTMP_LOGW("RTMP_ENC_G711A start enc%d,frameType%d ,chan%d ,SampleRate%d ,BitsPerSample%d,time%lld \r\n",i_ptRtmpMediaInfo->eEncType,i_ptRtmpMediaInfo->eFrameType,
                    i_ptRtmpMediaInfo->dwChannels,i_ptRtmpMediaInfo->dwSampleRate,i_ptRtmpMediaInfo->dwBitsPerSample,i_ptRtmpMediaInfo->ddwTimestamp);
                }
                iAudioDataLen = m_pRtmpMediaHandle->GenerateAudioData(&tRtmpAudioInfo, 0,m_pbFrameData,RTMP_MSG_MAX_LEN);
                iRet=SendAudio(m_dwTimestamp,m_pbFrameData,iAudioDataLen);
                break;
            }
            default:
            {
                RTMP_LOGE("i_ptRtmpMediaInfo->eEncType err %d \r\n",i_ptRtmpMediaInfo->eEncType);
                break;
            }
        }
        m_dwAudioFrameCntLog++;
        if(m_dwAudioFrameCntLog < 10)
        {
            RTMP_LOGW("RTMP_AUDIO_FRAME %d enc %d,frameType %d ,chan %d ,SampleRate %d ,BitsPerSample %d,time %lld \r\n",m_dwAudioFrameCntLog,i_ptRtmpMediaInfo->eEncType,i_ptRtmpMediaInfo->eFrameType,
            i_ptRtmpMediaInfo->dwChannels,i_ptRtmpMediaInfo->dwSampleRate,i_ptRtmpMediaInfo->dwBitsPerSample,i_ptRtmpMediaInfo->ddwTimestamp);
        }
    }
    else
    {

    }

    return iRet;
}

/*****************************************************************************
-Fuction        : HandleRtmpMsg
-Description    : 非阻塞
-Input          : 
-Output         : 
-Return         : 
* Modify Date     Version        Author           Modification
* -----------------------------------------------
* 2023/09/21      V1.0.0         Yu Weifeng       Created
******************************************************************************/
int RtmpSession::HandleRtmpMsg(unsigned char i_bMsgTypeID,unsigned int i_dwTimestamp,char * i_pcMsgPayload,int i_iPayloadLen)
{
    int iRet = -1;
    int iHandledDataLen = 0;
    
    if(NULL == i_pcMsgPayload || i_iPayloadLen <= 0)
    {
        RTMP_LOGE("HandleRtmpMsg NULL %d \r\n",i_iPayloadLen);
        return iRet;
    }
    switch(i_bMsgTypeID)
    {
        case RTMP_MSG_TYPE_FLEX_MESSAGE :
        {
            iRet = HandleCmdMsg(i_pcMsgPayload+1,i_iPayloadLen-1);// filter AMF3 0x00
            break;
        }
        case RTMP_MSG_TYPE_INVOKE :
        {
            iRet = HandleCmdMsg(i_pcMsgPayload,i_iPayloadLen);
            break;
        }
        case RTMP_MSG_TYPE_SET_CHUNK_SIZE:
        case RTMP_MSG_TYPE_ACKNOWLEDGEMENT:
        case RTMP_MSG_TYPE_WINDOW_ACK_SIZE:
        case RTMP_MSG_TYPE_SET_PEER_BANDWIDTH:
        {
            iRet = HandleControlMsg(i_bMsgTypeID,i_pcMsgPayload,i_iPayloadLen);
            break;
        }
        case RTMP_MSG_TYPE_VIDEO :
        {
            iRet = HandleVideoMsg(i_dwTimestamp,i_pcMsgPayload,i_iPayloadLen);
            break;
        }
        case RTMP_MSG_TYPE_AUDIO :
        {
            iRet = HandleAudioMsg(i_dwTimestamp,i_pcMsgPayload,i_iPayloadLen);
            break;
        }
        case RTMP_MSG_TYPE_DATA :
        {
            iRet = HandleDataMsg(i_dwTimestamp,i_pcMsgPayload,i_iPayloadLen);
            break;
        }
        case RTMP_MSG_TYPE_EVENT :
        {
            RTMP_LOGW("RTMP_MSG_TYPE_EVENT egnore %d \r\n",i_pcMsgPayload[1]);
            iRet = 0;//stream begin /stream is record
            break;
        }
        default :
        {
            break;
        }
    }

    return iRet;
}
/*****************************************************************************
-Fuction        : HandleCmdMsg
-Description    : 非阻塞
-Input          : 
-Output         : 
-Return         : 
* Modify Date     Version        Author           Modification
* -----------------------------------------------
* 2023/09/21      V1.0.0         Yu Weifeng       Created
******************************************************************************/
int RtmpSession::HandleCmdMsg(char * i_pcCmdMsgPayload,int i_iPayloadLen)
{
    int iRet = -1;
    char strCommand[64] = { 0 };
    double dlTransaction = -1;
    int i = 0;
    map<string, HandleCmd> ::iterator HandleCmdMapIter;
    int iProcessedLen = 0;

    if(NULL == i_pcCmdMsgPayload || i_iPayloadLen <= 0)
    {
        RTMP_LOGE("HandleRtmpMsg NULL %d \r\n",i_iPayloadLen);
        return iRet;
    }
    iProcessedLen = m_pRtmpParse->ParseCmdMsg((unsigned char *)i_pcCmdMsgPayload,i_iPayloadLen,strCommand,sizeof(strCommand),&dlTransaction);
    if(iProcessedLen <= 0)
    {
        RTMP_LOGE("m_pRtmpParse->ParseCmdMsg err %d \r\n",i_iPayloadLen);
        return iRet;
    }
    if (-1.0 == dlTransaction)
    {
        RTMP_LOGW("ParseCmdMsg -1.0 == dlTransaction %d \r\n",i_iPayloadLen);
        return 0; // fix: no transactionId onFCPublish
    }

    HandleCmdMapIter = m_HandleCmdMap.find(strCommand);
    if(HandleCmdMapIter == m_HandleCmdMap.end())
    {
        RTMP_LOGW("Can not find cmd %s \r\n",strCommand);
        iRet=0;
    }
    else
    {
        iRet = (this->*(HandleCmdMapIter->second))(dlTransaction,i_pcCmdMsgPayload+iProcessedLen,i_iPayloadLen-iProcessedLen);
    }
    if(iRet < 0)
    {
        RTMP_LOGE("HandCmd err %s \r\n",strCommand);
    }
    else
    {
        RTMP_LOGW("HandCmd success %s \r\n",strCommand);
    }
    return iRet; 
}

/*****************************************************************************
-Fuction        : HandleControlMsg
-Description    : 非阻塞
-Input          : 
-Output         : 
-Return         : 
* Modify Date     Version        Author           Modification
* -----------------------------------------------
* 2023/09/21      V1.0.0         Yu Weifeng       Created
******************************************************************************/
int RtmpSession::HandleControlMsg(unsigned char i_bMsgTypeID,char * i_pcMsgPayload,int i_iPayloadLen)
{
    int iRet = -1;
    unsigned int dwControlData = 0;
    
    if(NULL == i_pcMsgPayload || i_iPayloadLen <= 0)
    {
        RTMP_LOGE("HandleControlMsg NULL %d \r\n",i_iPayloadLen);
        return iRet;
    }
    switch(i_bMsgTypeID)
    {
        case RTMP_MSG_TYPE_SET_CHUNK_SIZE:
        {
            iRet = m_pRtmpParse->ParseControlMsg((unsigned char *)i_pcMsgPayload,i_iPayloadLen,&dwControlData);
            if(iRet < 0)
            {
                RTMP_LOGE("m_pRtmpParse->ParseControlMsg err %d \r\n",i_iPayloadLen);
            }
            else
            {
                //if(dwControlData > 4096)//一般是4096
                {
                    //RTMP_LOGE("RTMP_MSG_TYPE_SET_CHUNK_SIZE err %d \r\n",dwControlData);
                }
                //else
                {
                    m_tRtmpSessionConfig.dwInChunkSize = dwControlData;
                }
            }
            break;
        }
        case RTMP_MSG_TYPE_ACKNOWLEDGEMENT:
        {
            iRet = m_pRtmpParse->ParseControlMsg((unsigned char *)i_pcMsgPayload,i_iPayloadLen,&dwControlData);
            if(iRet < 0)
            {
                RTMP_LOGE("m_pRtmpParse->ParseControlMsg err %d \r\n",i_iPayloadLen);
            }
            else
            {
                RTMP_LOGD("RTMP_MSG_TYPE_ACKNOWLEDGEMENT %d \r\n",dwControlData);
            }
            break;
        }
        default:
        {
            RTMP_LOGD("HandleControlMsg default egnore %d \r\n",i_bMsgTypeID);
            iRet = 0;
            break;
        }
    }
    return iRet; 
}

/*****************************************************************************
-Fuction        : HandleVideoMsg
-Description    : 非阻塞
-Input          : 
-Output         : 
-Return         : 
* Modify Date     Version        Author           Modification
* -----------------------------------------------
* 2023/09/21      V1.0.0         Yu Weifeng       Created
******************************************************************************/
int RtmpSession::HandleVideoMsg(unsigned int i_dwTimestamp,char * i_pcMsgPayload,int i_iPayloadLen)
{
    int iRet = -1;
    T_RtmpMediaInfo tRtmpMediaInfo;
    int iVideoDataLen = 0;

    
    if(NULL == i_pcMsgPayload || i_iPayloadLen <= 0)
    {
        RTMP_LOGE("HandleVideoMsg NULL %d \r\n",i_iPayloadLen);
        return iRet;
    }
    
    if(NULL != m_tRtmpCb.PlayVideoData)
    {
        //demux
        iVideoDataLen = m_pRtmpMediaHandle->GetVideoData((unsigned char *)i_pcMsgPayload,i_iPayloadLen,&m_tPlayVideoParam,m_pbPlayFrameData,RTMP_MSG_MAX_LEN);
        if(iVideoDataLen < 0)
        {
            RTMP_LOGE("GetVideoData err %d \r\n",iVideoDataLen);
        }
        else if(iVideoDataLen == 0)
        {
            RTMP_LOGD("GetVideoData need more data %d \r\n",iVideoDataLen);
            iRet = 0;
        }
        else
        {
            memset(&tRtmpMediaInfo,0,sizeof(T_RtmpMediaInfo));
            tRtmpMediaInfo.eEncType = m_tPlayVideoParam.eEncType;
            tRtmpMediaInfo.eFrameType= m_tPlayVideoParam.eFrameType;
            tRtmpMediaInfo.ddwTimestamp = i_dwTimestamp;
            tRtmpMediaInfo.dwWidth= m_tPlayVideoParam.dwWidth;
            tRtmpMediaInfo.dwHeight = m_tPlayVideoParam.dwHeight;
            
            tRtmpMediaInfo.dwSampleRate = m_tPlayAudioParam.dwSamplesPerSecond;
            tRtmpMediaInfo.dwChannels = m_tPlayAudioParam.dwChannels;
            tRtmpMediaInfo.dwBitsPerSample = m_tPlayAudioParam.dwBitsPerSample;
            
            tRtmpMediaInfo.wSpsLen=m_tPlayVideoParam.wSpsLen;
            tRtmpMediaInfo.wPpsLen=m_tPlayVideoParam.wPpsLen;
            tRtmpMediaInfo.wVpsLen=m_tPlayVideoParam.wVpsLen;
            memcpy(tRtmpMediaInfo.abVPS,m_tPlayVideoParam.abVPS,tRtmpMediaInfo.wVpsLen);
            memcpy(tRtmpMediaInfo.abSPS,m_tPlayVideoParam.abSPS,tRtmpMediaInfo.wSpsLen);
            memcpy(tRtmpMediaInfo.abPPS,m_tPlayVideoParam.abPPS,tRtmpMediaInfo.wPpsLen);
            
            iRet = m_tRtmpCb.PlayVideoData(&tRtmpMediaInfo,(char *)m_pbPlayFrameData,iVideoDataLen,m_pIoHandle);
            
        }
    }
    else
    {
        RTMP_LOGW("m_tRtmpCb.PushVideoData NULL %s \r\n",m_strStreamName);
    }
    return iRet; 
}

/*****************************************************************************
-Fuction        : HandleAudioMsg
-Description    : 非阻塞
-Input          : 
-Output         : 
-Return         : 
* Modify Date     Version        Author           Modification
* -----------------------------------------------
* 2023/09/21      V1.0.0         Yu Weifeng       Created
******************************************************************************/
int RtmpSession::HandleAudioMsg(unsigned int i_dwTimestamp,char * i_pcMsgPayload,int i_iPayloadLen)
{
    int iRet = -1;
    int iAudioDataLen = 0;
    unsigned char abPublishAudioData[RTMP_PLAY_SRC_MAX_LEN];
    T_RtmpMediaInfo tRtmpMediaInfo;
    
    if(NULL == i_pcMsgPayload || i_iPayloadLen <= 0)
    {
        RTMP_LOGE("HandleAudioMsg NULL %d \r\n",i_iPayloadLen);
        return iRet;
    }

    if(NULL != m_tRtmpCb.PlayAudioData)
    {
        //demux
        iAudioDataLen = m_pRtmpMediaHandle->GetAudioData((unsigned char *)i_pcMsgPayload,i_iPayloadLen,&m_tPlayAudioParam,abPublishAudioData,RTMP_PLAY_SRC_MAX_LEN);
        if(iAudioDataLen < 0)
        {
            RTMP_LOGE("GetAudioData err %d \r\n",iAudioDataLen);
        }
        else if(iAudioDataLen == 0)
        {
            RTMP_LOGD("GetAudioData need more data %d \r\n",iAudioDataLen);
            iRet = 0;
        }
        else
        {
            memset(&tRtmpMediaInfo,0,sizeof(T_RtmpMediaInfo));
            tRtmpMediaInfo.eEncType = m_tPlayAudioParam.eEncType;
            tRtmpMediaInfo.eFrameType= RTMP_AUDIO_FRAME;
            tRtmpMediaInfo.ddwTimestamp = i_dwTimestamp;
            tRtmpMediaInfo.dwSampleRate = m_tPlayAudioParam.dwSamplesPerSecond;
            tRtmpMediaInfo.dwChannels = m_tPlayAudioParam.dwChannels;
            tRtmpMediaInfo.dwBitsPerSample = m_tPlayAudioParam.dwBitsPerSample;
            
            iRet = m_tRtmpCb.PlayAudioData(&tRtmpMediaInfo,(char *)abPublishAudioData,iAudioDataLen,m_pIoHandle);
        }
    }
    else
    {
        RTMP_LOGW("m_tRtmpCb.PushAudioData NULL %s \r\n",m_strStreamName);
    }
    return iRet; 
}

/*****************************************************************************
-Fuction        : HandleDataMsg
-Description    : 非阻塞
-Input          : 
-Output         : 
-Return         : 
* Modify Date     Version        Author           Modification
* -----------------------------------------------
* 2023/09/21      V1.0.0         Yu Weifeng       Created
******************************************************************************/
int RtmpSession::HandleDataMsg(unsigned int i_dwTimestamp,char * i_pcMsgPayload,int i_iPayloadLen)
{
    int iRet = -1;
    // filter @setDataFrame
    const static unsigned char s_abSetFrameData[] = { 0x02, 0x00, 0x0d, 0x40, 0x73, 0x65, 0x74, 0x44, 0x61, 0x74, 0x61, 0x46, 0x72, 0x61, 0x6d, 0x65 };

    if(NULL == i_pcMsgPayload || i_iPayloadLen <= 0)
    {
        RTMP_LOGE("HandleControlMsg NULL %d \r\n",i_iPayloadLen);
        return iRet;
    }
    if (i_iPayloadLen > (int)sizeof(s_abSetFrameData) && 0 == memcmp(s_abSetFrameData, i_pcMsgPayload, sizeof(s_abSetFrameData)))
    {
        if(NULL != m_tRtmpCb.PlayScriptData)
        {
            iRet = m_tRtmpCb.PlayScriptData(m_strStreamName,i_dwTimestamp,i_pcMsgPayload+sizeof(s_abSetFrameData),i_iPayloadLen-sizeof(s_abSetFrameData));
        }
        else
        {
            RTMP_LOGW("m_tRtmpCb.PushScriptData NULL %s \r\n",m_strStreamName);
        }
    }
    else
    {
        if(NULL != m_tRtmpCb.PlayScriptData)
        {
            iRet = m_tRtmpCb.PlayScriptData(m_strStreamName,i_dwTimestamp,i_pcMsgPayload,i_iPayloadLen);
        }
        else
        {
            RTMP_LOGW("m_tRtmpCb.PushScriptData NULL %s \r\n",m_strStreamName);
        }
    }
    return iRet; 
}

/*****************************************************************************
-Fuction        : HandCmd_result
-Description    : _result 响应命令处理
-Input          : 
-Output         : 
-Return         : 
* Modify Date     Version        Author           Modification
* -----------------------------------------------
* 2023/09/21      V1.0.0         Yu Weifeng       Created
******************************************************************************/
int RtmpSession::HandCmd_result(double i_dlTransaction,char * i_pcResultBuf,int i_iBufLen)
{
    int iRet = -1;
    T_RtmpResultContent tRtmpResultContent;
    
    if(NULL == i_pcResultBuf || i_iBufLen <= 0)
    {
        RTMP_LOGE("HandCmd_result NULL %d \r\n",i_iBufLen);
        return iRet;
    }
    if(RTMP_CONNECT_RESULT == m_eState)
    {
        memset(&tRtmpResultContent,0,sizeof(T_RtmpResultContent));
        iRet = m_pRtmpParse->ParseCmd_result((unsigned char *)i_pcResultBuf,i_iBufLen,&tRtmpResultContent);
        if(iRet < 0)
        {
            RTMP_LOGE("HandCmd_result err %d \r\n",i_iBufLen);
            return iRet;
        }
        
        if(0 == strcmp("NetConnection.Connect.Success",tRtmpResultContent.strCode))
        {
            RTMP_LOGW("RTMP_CONNECT_RESULT Success %s \r\n",tRtmpResultContent.strCode);
            m_eState = RTMP_CREATE_STREAM;
        }
        else
        {
            RTMP_LOGE("RTMP_CONNECT_RESULT err %s \r\n",tRtmpResultContent.strCode);
            m_eState = RTMP_EXIT;
        }
    }
    else
    {
        //create stream 回应直接返回成功，
        iRet = 0;
        RTMP_LOGW("HandCmd_result m_eState %d\r\n",m_eState);
    }
    return iRet;
}

/*****************************************************************************
-Fuction        : HandCmd_result
-Description    : _result 响应命令处理
-Input          : 
-Output         : 
-Return         : 
* Modify Date     Version        Author           Modification
* -----------------------------------------------
* 2023/09/21      V1.0.0         Yu Weifeng       Created
******************************************************************************/
int RtmpSession::HandCmdOnStatus(double i_dlTransaction,char * i_pcOnStatusBuf,int i_iBufLen)
{
    int iRet = -1;
    T_RtmpOnStatusContent tRtmpOnStatusContent;
    
    if(NULL == i_pcOnStatusBuf || i_iBufLen <= 0)
    {
        RTMP_LOGE("HandCmdOnStatus NULL %d \r\n",i_iBufLen);
        return iRet;
    }
    memset(&tRtmpOnStatusContent,0,sizeof(T_RtmpOnStatusContent));
    iRet = m_pRtmpParse->ParseCmdOnStatus((unsigned char *)i_pcOnStatusBuf,i_iBufLen,&tRtmpOnStatusContent);
    if(iRet < 0)
    {
        RTMP_LOGE("HandCmdOnStatus err %d \r\n",i_iBufLen);
        return iRet;
    }
    if(RTMP_START_PLAY_OR_PUBLISH_RESULT == m_eState)
    {
        string strStart(tRtmpOnStatusContent.strCode);//使用strstr找到了还会返回null
        if(string::npos == strStart.find(".Start"))//NetStream.Publish.Start
        {
            RTMP_LOGE("RTMP_START_PLAY_OR_PUBLISH_RESULT err %s \r\n",tRtmpOnStatusContent.strCode);
            m_eState = RTMP_START_PLAY_OR_PUBLISH_RESULT_ERR;
        }
        else
        {
            RTMP_LOGW("RTMP_START_PLAY_OR_PUBLISH_RESULT Success %s \r\n",tRtmpOnStatusContent.strCode);
            if(m_tRtmpSessionConfig.iPlayOrPublish != 0)
            {
                m_blPushing = true;
            }
            m_eState = RTMP_DO_PLAY_OR_PUBLISH;
        }
    }
    else
    {
        RTMP_LOGW("HandCmdOnStatus m_eState %d,strCode %s \r\n",m_eState,tRtmpOnStatusContent.strCode);
    }
    return iRet;
}

/*****************************************************************************
-Fuction        : SendSetChunkSize
-Description    : 
-Input          : 
-Output         : 
-Return         : 
* Modify Date     Version        Author           Modification
* -----------------------------------------------
* 2023/09/21      V1.0.0         Yu Weifeng       Created
******************************************************************************/
int RtmpSession::SendSetChunkSize(unsigned int i_dwChunkSize)
{
    int iRet = -1;
    char abBuf[RTMP_CTL_MSG_LEN+sizeof(unsigned int)];
    int iLen = 0;
    
    memset(abBuf,0,sizeof(abBuf));
    iLen = m_pRtmpPack->CreateSetChunkSize(abBuf,sizeof(abBuf),i_dwChunkSize);
    if(iLen < 0)
    {
        RTMP_LOGE("SendSetChunkSize err %d\r\n",iLen);
        return iRet;
    }
    iRet = SendData(abBuf,iLen);
    
    return 0;
}

/*****************************************************************************
-Fuction        : SendAcknowledgement
-Description    : 
-Input          : 
-Output         : 
-Return         : 
* Modify Date     Version        Author           Modification
* -----------------------------------------------
* 2023/09/21      V1.0.0         Yu Weifeng       Created
******************************************************************************/
int RtmpSession::SendAcknowledgement(unsigned int i_dwWindowSize)
{
    int iRet = -1;
    char abBuf[RTMP_CTL_MSG_LEN+sizeof(unsigned int)];
    int iLen = 0;

    if(m_tRtmpSessionConfig.dwWindowSize > 0 && m_dwRecvedDataLen  > i_dwWindowSize)
    {
        memset(abBuf,0,sizeof(abBuf));
        iLen = m_pRtmpPack->CreateAcknowledgement(abBuf,sizeof(abBuf),i_dwWindowSize);
        if(iLen < 0)
        {
            RTMP_LOGE("CreateAcknowledgement err %d\r\n",iLen);
            return iRet;
        }
        iRet = SendData(abBuf,iLen);

        m_dwRecvedDataLen = 0;
    }
    return iRet;
}

/*****************************************************************************
-Fuction        : SendCmdConnect
-Description    : 
-Input          : 
-Output         : 
-Return         : 
* Modify Date     Version        Author           Modification
* -----------------------------------------------
* 2023/09/21      V1.0.0         Yu Weifeng       Created
******************************************************************************/
int RtmpSession::SendCmdConnect()
{
    int iRet = -1;
    unsigned char abBuf[RTMP_CMD_MAX_LEN];
    int iLen = 0;
    char abSendBuf[RTMP_OUTPUT_CHUNK_SIZE+1024];
    T_RtmpChunkPayloadInfo tChunkPayloadInfo;

    memset(abBuf,0,sizeof(abBuf));
    iLen = m_pRtmpPack->CreateCmdConnect(m_strAppName,m_strTcURL,abBuf,sizeof(abBuf));
    if(iLen <= 0)
    {
        RTMP_LOGE("CreateCmdConnect err %d\r\n",iLen);
        return iRet;
    }
    
    memset(&tChunkPayloadInfo,0,sizeof(T_RtmpChunkPayloadInfo));
    tChunkPayloadInfo.pcChunkPayload = (char *)abBuf;
    tChunkPayloadInfo.iPayloadLen = iLen;
    tChunkPayloadInfo.dwOutChunkSize = m_tRtmpSessionConfig.dwOutChunkSize;
    tChunkPayloadInfo.dwStreamID = RTMP_MSG_STREAM_ID0;//for play
    do
    {
        memset(abSendBuf,0,sizeof(abSendBuf));
        iLen = m_pRtmpPack->CreateCmdMsg(&tChunkPayloadInfo,abSendBuf,sizeof(abSendBuf));
        if(iLen < 0)
        {
            RTMP_LOGE("CreateCmdMsg err %d\r\n",iLen);
        }
        else if(0 == iLen)
        {
            RTMP_LOGD("CreateCmdMsg over %d\r\n",iLen);
        }
        else
        {
            iRet = SendData(abSendBuf,iLen);
            if(iRet < 0)
            {
                RTMP_LOGE("SendData err %d\r\n",iLen);
                return iRet;
            }
        }
    }while(iLen > 0);
    
    return iRet;
}

/*****************************************************************************
-Fuction        : SendCmdConnect
-Description    : 
-Input          : 
-Output         : 
-Return         : 
* Modify Date     Version        Author           Modification
* -----------------------------------------------
* 2023/09/21      V1.0.0         Yu Weifeng       Created
******************************************************************************/
int RtmpSession::SendCmdCreateStream()
{
    int iRet = -1;
    unsigned char abBuf[RTMP_CMD_MAX_LEN];
    int iLen = 0;
    char abSendBuf[RTMP_OUTPUT_CHUNK_SIZE+1024];
    T_RtmpChunkPayloadInfo tChunkPayloadInfo;


    memset(abBuf,0,sizeof(abBuf));
    iLen = m_pRtmpPack->CreateCmdCreateStream(abBuf,sizeof(abBuf));
    if(iLen <= 0)
    {
        RTMP_LOGE("SendCmdCreateStream err %d\r\n",iLen);
        return iRet;
    }
    
    memset(&tChunkPayloadInfo,0,sizeof(T_RtmpChunkPayloadInfo));
    tChunkPayloadInfo.pcChunkPayload = (char *)abBuf;
    tChunkPayloadInfo.iPayloadLen = iLen;
    tChunkPayloadInfo.dwOutChunkSize = m_tRtmpSessionConfig.dwOutChunkSize;
    tChunkPayloadInfo.dwStreamID = RTMP_MSG_STREAM_ID0;//for play
    do
    {
        memset(abSendBuf,0,sizeof(abSendBuf));
        iLen = m_pRtmpPack->CreateCmdMsg(&tChunkPayloadInfo,abSendBuf,sizeof(abSendBuf));
        if(iLen < 0)
        {
            RTMP_LOGE("CreateCmdMsg err %d\r\n",iLen);
        }
        else if(0 == iLen)
        {
            RTMP_LOGD("CreateCmdMsg over %d\r\n",iLen);
        }
        else
        {
            iRet = SendData(abSendBuf,iLen);
            if(iRet < 0)
            {
                RTMP_LOGE("SendData err %d\r\n",iLen);
                return iRet;
            }
        }
    }while(iLen > 0);
    
    return iRet;
}

/*****************************************************************************
-Fuction        : SendCmdConnect
-Description    : 
-Input          : 
-Output         : 
-Return         : 
* Modify Date     Version        Author           Modification
* -----------------------------------------------
* 2023/09/21      V1.0.0         Yu Weifeng       Created
******************************************************************************/
int RtmpSession::SendCmdPlay()
{
    int iRet = -1;
    unsigned char abBuf[RTMP_CMD_MAX_LEN];
    int iLen = 0;
    char abSendBuf[RTMP_OUTPUT_CHUNK_SIZE+1024];
    T_RtmpChunkPayloadInfo tChunkPayloadInfo;


    if(0 == strlen(m_strStreamName))//rtmp://10.10.10.10:10/live_10/Mnx8Y2UEdXTQ%3D%3D.9eaa64fa64a282
    {
        RTMP_LOGE("SendCmdPlay err m_strStreamName %s\r\n",m_tRtmpSessionConfig.strURL);
        return iRet;
    }

    memset(abBuf,0,sizeof(abBuf));
    iLen = m_pRtmpPack->CreateCmdPlay(m_strStreamName,abBuf,sizeof(abBuf));
    if(iLen <= 0)
    {
        RTMP_LOGE("SendCmdPlay err %d\r\n",iLen);
        return iRet;
    }
    
    memset(&tChunkPayloadInfo,0,sizeof(T_RtmpChunkPayloadInfo));
    tChunkPayloadInfo.pcChunkPayload = (char *)abBuf;
    tChunkPayloadInfo.iPayloadLen = iLen;
    tChunkPayloadInfo.dwOutChunkSize = m_tRtmpSessionConfig.dwOutChunkSize;
    tChunkPayloadInfo.dwStreamID = RTMP_MSG_STREAM_ID1;//for play
    do
    {
        memset(abSendBuf,0,sizeof(abSendBuf));
        iLen = m_pRtmpPack->CreateCmdMsg(&tChunkPayloadInfo,abSendBuf,sizeof(abSendBuf));
        if(iLen < 0)
        {
            RTMP_LOGE("CreateCmdMsg err %d\r\n",iLen);
        }
        else if(0 == iLen)
        {
            RTMP_LOGD("CreateCmdMsg over %d\r\n",iLen);
        }
        else
        {
            iRet = SendData(abSendBuf,iLen);
            if(iRet < 0)
            {
                RTMP_LOGE("SendData err %d\r\n",iLen);
                return iRet;
            }
        }
    }while(iLen > 0);
    
    return iRet;
}

/*****************************************************************************
-Fuction        : SendCmdConnect
-Description    : 
-Input          : 
-Output         : 
-Return         : 
* Modify Date     Version        Author           Modification
* -----------------------------------------------
* 2023/09/21      V1.0.0         Yu Weifeng       Created
******************************************************************************/
int RtmpSession::SendCmdPublish()
{
    int iRet = -1;
    unsigned char abBuf[RTMP_CMD_MAX_LEN];
    int iLen = 0;
    char abSendBuf[RTMP_OUTPUT_CHUNK_SIZE+1024];
    T_RtmpChunkPayloadInfo tChunkPayloadInfo;


    if(0 == strlen(m_strStreamName))//rtmp://10.10.10.10:10/live_10/Mnx8Y2UEdXTQ%3D%3D.9eaa64fa64a282
    {
        RTMP_LOGE("SendCmdPublish err m_strStreamName %s\r\n",m_tRtmpSessionConfig.strURL);
        return iRet;
    }

    memset(abBuf,0,sizeof(abBuf));
    iLen = m_pRtmpPack->CreateCmdPublish(m_strStreamName,abBuf,sizeof(abBuf));
    if(iLen <= 0)
    {
        RTMP_LOGE("SendCmdPublish err %d\r\n",iLen);
        return iRet;
    }
    
    memset(&tChunkPayloadInfo,0,sizeof(T_RtmpChunkPayloadInfo));
    tChunkPayloadInfo.pcChunkPayload = (char *)abBuf;
    tChunkPayloadInfo.iPayloadLen = iLen;
    tChunkPayloadInfo.dwOutChunkSize = m_tRtmpSessionConfig.dwOutChunkSize;
    tChunkPayloadInfo.dwStreamID = RTMP_MSG_STREAM_ID1;//for play
    do
    {
        memset(abSendBuf,0,sizeof(abSendBuf));
        iLen = m_pRtmpPack->CreateCmdMsg(&tChunkPayloadInfo,abSendBuf,sizeof(abSendBuf));
        if(iLen < 0)
        {
            RTMP_LOGE("CreateCmdMsg err %d\r\n",iLen);
        }
        else if(0 == iLen)
        {
            RTMP_LOGD("CreateCmdMsg over %d\r\n",iLen);
        }
        else
        {
            iRet = SendData(abSendBuf,iLen);
            if(iRet < 0)
            {
                RTMP_LOGE("SendData err %d\r\n",iLen);
                return iRet;
            }
        }
    }while(iLen > 0);
    
    return iRet;
}

/*****************************************************************************
-Fuction        : SendCmdDeleteStream
-Description    : 
-Input          : 
-Output         : 
-Return         : 
* Modify Date     Version        Author           Modification
* -----------------------------------------------
* 2023/09/21      V1.0.0         Yu Weifeng       Created
******************************************************************************/
int RtmpSession::SendCmdDeleteStream()
{
    int iRet = -1;
    unsigned char abBuf[RTMP_CMD_MAX_LEN];
    int iLen = 0;
    char abSendBuf[RTMP_OUTPUT_CHUNK_SIZE+1024];
    T_RtmpChunkPayloadInfo tChunkPayloadInfo;

    memset(abBuf,0,sizeof(abBuf));
    iLen = m_pRtmpPack->CreateCmdDeleteStream(abBuf,sizeof(abBuf));
    if(iLen <= 0)
    {
        RTMP_LOGE("CreateCmdConnect err %d\r\n",iLen);
        return iRet;
    }
    
    memset(&tChunkPayloadInfo,0,sizeof(T_RtmpChunkPayloadInfo));
    tChunkPayloadInfo.pcChunkPayload = (char *)abBuf;
    tChunkPayloadInfo.iPayloadLen = iLen;
    tChunkPayloadInfo.dwOutChunkSize = m_tRtmpSessionConfig.dwOutChunkSize;
    tChunkPayloadInfo.dwStreamID = RTMP_MSG_STREAM_ID0;
    do
    {
        memset(abSendBuf,0,sizeof(abSendBuf));
        iLen = m_pRtmpPack->CreateCmdMsg(&tChunkPayloadInfo,abSendBuf,sizeof(abSendBuf));
        if(iLen < 0)
        {
            RTMP_LOGE("CreateCmdMsg err %d\r\n",iLen);
        }
        else if(0 == iLen)
        {
            RTMP_LOGD("CreateCmdMsg over %d\r\n",iLen);
        }
        else
        {
            iRet = SendData(abSendBuf,iLen);
            if(iRet < 0)
            {
                RTMP_LOGE("SendData err %d\r\n",iLen);
                return iRet;
            }
        }
    }while(iLen > 0);
    
    return iRet;
}

/*****************************************************************************
-Fuction        : SendDataSampleAccess
-Description    : netstream_rtmp sampleaccess
-Input          : 
-Output         : 
-Return         : 
* Modify Date     Version        Author           Modification
* -----------------------------------------------
* 2023/09/21      V1.0.0         Yu Weifeng       Created
******************************************************************************/
int RtmpSession::SendVideo(unsigned int i_dwTimestamp,unsigned char *i_pbVideoData,int i_iVideoDataLen)
{
    int iRet = -1;
    int iLen = 0;
    char abSendBuf[RTMP_OUTPUT_CHUNK_SIZE+1024];
    T_RtmpChunkPayloadInfo tChunkPayloadInfo;
    
    if(NULL == i_pbVideoData || i_iVideoDataLen <= 0)
    {
        RTMP_LOGE("SendVideo NULL %d \r\n",i_iVideoDataLen);
        return iRet;
    }
    
    memset(&tChunkPayloadInfo,0,sizeof(T_RtmpChunkPayloadInfo));
    tChunkPayloadInfo.pcChunkPayload = (char *)i_pbVideoData;
    tChunkPayloadInfo.iPayloadLen = i_iVideoDataLen;
    tChunkPayloadInfo.dwOutChunkSize = m_tRtmpSessionConfig.dwOutChunkSize;
    tChunkPayloadInfo.dwStreamID = RTMP_MSG_STREAM_ID1;
    do
    {
        memset(abSendBuf,0,sizeof(abSendBuf));
        iLen = m_pRtmpPack->CreateVideoMsg(i_dwTimestamp,&tChunkPayloadInfo,abSendBuf,sizeof(abSendBuf));
        if(iLen < 0)
        {
            RTMP_LOGE("CreateVideoMsg err %d\r\n",iLen);
        }
        else if(0 == iLen)
        {
            //RTMP_LOGD("CreateVideoMsg over %d\r\n",iLen);
        }
        else
        {
            iRet = SendData(abSendBuf,iLen);
            if(iRet < 0)
            {
                RTMP_LOGE("SendData err %d\r\n",iLen);
                return iRet;
            }
        }
    }while(iLen > 0);
    
    return iRet;
}

/*****************************************************************************
-Fuction        : SendAudio
-Description    : netstream_rtmp sampleaccess
-Input          : 
-Output         : 
-Return         : 
* Modify Date     Version        Author           Modification
* -----------------------------------------------
* 2023/09/21      V1.0.0         Yu Weifeng       Created
******************************************************************************/
int RtmpSession::SendAudio(unsigned int i_dwTimestamp,unsigned char *i_pbAudioData,int i_iAudioDataLen)
{
    int iRet = -1;
    int iLen = 0;
    char abSendBuf[RTMP_OUTPUT_CHUNK_SIZE+1024];
    T_RtmpChunkPayloadInfo tChunkPayloadInfo;
    
    if(NULL == i_pbAudioData || i_iAudioDataLen <= 0)
    {
        RTMP_LOGE("SendAudio NULL %d \r\n",i_iAudioDataLen);
        return iRet;
    }
    
    memset(&tChunkPayloadInfo,0,sizeof(T_RtmpChunkPayloadInfo));
    tChunkPayloadInfo.pcChunkPayload = (char *)i_pbAudioData;
    tChunkPayloadInfo.iPayloadLen = i_iAudioDataLen;
    tChunkPayloadInfo.dwOutChunkSize = m_tRtmpSessionConfig.dwOutChunkSize;
    tChunkPayloadInfo.dwStreamID = RTMP_MSG_STREAM_ID1;
    do
    {
        memset(abSendBuf,0,sizeof(abSendBuf));
        iLen = m_pRtmpPack->CreateAudioMsg(i_dwTimestamp,&tChunkPayloadInfo,abSendBuf,sizeof(abSendBuf));
        if(iLen < 0)
        {
            RTMP_LOGE("CreateAudioMsg err %d\r\n",iLen);
        }
        else if(0 == iLen)
        {
            //RTMP_LOGD("CreateAudioMsg over %d\r\n",iLen);
        }
        else
        {
            iRet = SendData(abSendBuf,iLen);
            if(iRet < 0)
            {
                RTMP_LOGE("SendData err %d\r\n",iLen);
                return iRet;
            }
        }
    }while(iLen > 0);
    
    return iRet;
}


/****以下新增的重载都是为了适配IO由外部传入的形式***/


/*****************************************************************************
-Fuction        : DoConnect
-Description    : 
-Input          : 
-Output         : 
-Return         : 
* Modify Date     Version        Author           Modification
* -----------------------------------------------
* 2023/09/21      V1.0.0         Yu Weifeng       Created
******************************************************************************/
int RtmpSession::DoConnect()
{
    int iRet = -1;
    
    if(NULL == m_tRtmpCb.Connect ||NULL == m_pIoHandle)
    {
        RTMP_LOGE("m_tRtmpCb.SendData NULL %d \r\n",iRet);
        return iRet;
    }
    string strURL(m_tRtmpSessionConfig.strURL);
    string strIp;
    auto dwIpOrDomainStartPos = strURL.find("rtmp://");
    if(string::npos == dwIpOrDomainStartPos)//rtmp://10.10.10.10:10/live_10/Mnx8Y2UEdXTQ%3D%3D.9eaa64fa64a282
    {
        RTMP_LOGE("dwIpOrDomainStartPos err %s\r\n",strURL.c_str());
        return iRet;
    }
    auto dwIpOrDomainEndPos = strURL.find(":",dwIpOrDomainStartPos+strlen("rtmp://"));
    if(string::npos == dwIpOrDomainEndPos)
    {
        RTMP_LOGE("dwIpOrDomainEndPos err %s\r\n",strURL.c_str());
        return iRet;
    }
    strIp.assign(strURL,dwIpOrDomainStartPos+strlen("rtmp://"),dwIpOrDomainEndPos-(dwIpOrDomainStartPos+strlen("rtmp://")));
    auto dwPortEndPos = strURL.find("/",dwIpOrDomainEndPos);
    if(string::npos == dwPortEndPos)
    {
        RTMP_LOGE("dwPortEndPos err %s\r\n",strURL.c_str());
        return iRet;
    }
    unsigned short wPort = atoi(strURL.substr(dwIpOrDomainEndPos+1,dwPortEndPos-dwIpOrDomainEndPos-1).c_str());
    auto dwAppNameEndPos = strURL.find("/",dwPortEndPos+1);
    if(string::npos == dwAppNameEndPos)
    {
        RTMP_LOGE("dwAppNameEndPos err %s\r\n",strURL.c_str());
        return -1;
    }
    snprintf(m_strStreamName,sizeof(m_strStreamName),"%s",strURL.substr(dwAppNameEndPos+1).c_str());
    snprintf(m_strAppName,sizeof(m_strAppName),"%s",strURL.substr(dwPortEndPos+1,dwAppNameEndPos-dwPortEndPos-1).c_str());
    snprintf(m_strTcURL,sizeof(m_strTcURL),"%s",strURL.substr(0,dwAppNameEndPos).c_str());
    
    RTMP_LOGW("SendCmdConnect IpOrDomain%s,port %d\r\n",strIp.c_str(),wPort);
    iRet = m_tRtmpCb.Connect((void *)m_pIoHandle,(char *)strIp.c_str(),wPort);
    if(iRet < 0)
    {
        RTMP_LOGE("RtmpSession::Connect err");
        return -1;
    }
    
    return iRet;
}
/*****************************************************************************
-Fuction        : DoStop
-Description    : 
-Input          : 
-Output         : 
-Return         : 
* Modify Date     Version        Author           Modification
* -----------------------------------------------
* 2023/09/21      V1.0.0         Yu Weifeng       Created
******************************************************************************/
int RtmpSession::DoStop()
{
    return this->SendCmdDeleteStream();
}

/*****************************************************************************
-Fuction        : DoCycle
-Description    : 非阻塞,可优化为先接收到数据再调用这个
-Input          : 
-Output         : 
-Return         : 
* Modify Date     Version        Author           Modification
* -----------------------------------------------
* 2023/09/21      V1.0.0         Yu Weifeng       Created
******************************************************************************/
int RtmpSession::DoCycle(char *i_pcData,int i_iDataLen)
{
    int iHandledDataLen = 0;
    int iRet = 0;

    switch(m_eState)
    {
        case RTMP_INIT:
        {
            /*if(this->DoConnect()<0)//异步接口由外面调用
            {
                RTMP_LOGE("DoConnect err %d\r\n",iRet);
                return iRet;
            }*/
            m_eState = RTMP_HANDSHAKE_START;
            //break;
        }
        case RTMP_HANDSHAKE_START:
        {
            char abC0C1[RTMP_C0_LEN+RTMP_C1_LEN];
            int iC0C1Len;
            memset(abC0C1,0,sizeof(abC0C1));
            iC0C1Len = m_pRtmpPack->CreateC0C1(abC0C1,sizeof(abC0C1));
            if(iC0C1Len < 0)
            {
                RTMP_LOGE("CreateC0C1 err %d\r\n",iC0C1Len);
                return iRet;
            }
            iRet = SendData(abC0C1,iC0C1Len);
            if(iRet < 0)
            {
                RTMP_LOGE("SendData err %d\r\n",iRet);
                return iRet;
            }

            m_eState = RTMP_HANDSHAKE_S0S1S2;
            break;
        }
        case RTMP_HANDSHAKE_S0S1S2:
        {
            char *pcData = NULL;
            int iDataLen = 0;
            char abC2[RTMP_C2_LEN];
            int iC2Len;
            
            if(i_iDataLen+m_iHandshakeBufLen < RTMP_S0S1S2_LEN)
            {
                memcpy(m_abHandshakeBuf+m_iHandshakeBufLen,i_pcData,i_iDataLen);
                m_iHandshakeBufLen+=i_iDataLen;
                RTMP_LOGE("SimpleHandshake need more data %d\r\n",i_iDataLen);
                return 0;
            }
            if(0 != m_iHandshakeBufLen)
            {
                memcpy(m_abHandshakeBuf+m_iHandshakeBufLen,i_pcData,RTMP_S0S1S2_LEN-m_iHandshakeBufLen);
                i_iDataLen -= RTMP_S0S1S2_LEN-m_iHandshakeBufLen;
                i_pcData+= RTMP_S0S1S2_LEN-m_iHandshakeBufLen;
                m_iHandshakeBufLen=RTMP_S0S1S2_LEN;
                pcData = m_abHandshakeBuf;
                iDataLen = m_iHandshakeBufLen;
            }
            else
            {
                pcData = i_pcData;
                iDataLen = i_iDataLen;
                i_pcData+=RTMP_S0S1S2_LEN;
                i_iDataLen-=RTMP_S0S1S2_LEN;
            }
            memset(abC2,0,sizeof(abC2));
            iC2Len = m_pRtmpPack->CreateC2(pcData+RTMP_S0_LEN+RTMP_S1_LEN,iDataLen-RTMP_S0_LEN-RTMP_S1_LEN,abC2,sizeof(abC2));
            if(iC2Len < 0)
            {
                RTMP_LOGE("CreateC2 err %d\r\n",iC2Len);
                return iRet;
            }
            iRet = SendData(abC2,iC2Len);
            if(iRet < 0)
            {
                RTMP_LOGE("SendData err %d\r\n",iRet);
                return iRet;
            }
            
            m_eState = RTMP_CONNECT;
            memset(m_abHandshakeBuf,0,sizeof(m_abHandshakeBuf));
            m_iHandshakeBufLen = 0;
            //break;
        }
        case RTMP_CONNECT:
        {
            SendSetChunkSize(m_tRtmpSessionConfig.dwOutChunkSize);
            if(SendCmdConnect() < 0)
            {
                RTMP_LOGE("RTMP_CONNECT err   %d %d\r\n",i_iDataLen,m_dwConnectFailCnt);
                m_dwConnectFailCnt++;
                if(m_dwConnectFailCnt > 3)
                {
                    m_eState = RTMP_EXIT;
                    return -1;
                }
                return 0;
            }
            m_eState = RTMP_CONNECT_RESULT;
            break;
        }
        case RTMP_CREATE_STREAM:
        {
            if(this->SendCmdCreateStream() < 0)
            {
                RTMP_LOGE("RTMP_CREATE_STREAM err   %d %d\r\n",i_iDataLen,m_dwConnectFailCnt);
                m_eState = RTMP_EXIT;
                return -1;
            }
            m_eState = RTMP_START_PLAY_OR_PUBLISH;
            break;
        }
        case RTMP_START_PLAY_OR_PUBLISH:
        {
            if(0 == m_tRtmpSessionConfig.iPlayOrPublish)
            {
                iRet = this->SendCmdPlay();
            }
            else
            {
                iRet = this->SendCmdPublish();
            }
            if(iRet < 0)
            {
                RTMP_LOGE("RTMP_CREATE_STREAM err   %d %d\r\n",i_iDataLen,m_dwConnectFailCnt);
                m_eState = RTMP_EXIT;
                return -1;
            }
            m_eState = RTMP_START_PLAY_OR_PUBLISH_RESULT;
            break;
        }
        case RTMP_DO_PLAY_OR_PUBLISH:
        {
            iRet = 1;
            break;
        }
        case RTMP_START_PLAY_OR_PUBLISH_RESULT_ERR:
        {
            return -2;
        }
        case RTMP_EXIT:
        {
            return -1;
        }
        default:
        {
            break;
        }

    }
    if(NULL != i_pcData && i_iDataLen > 0)
    {
        iRet = HandleRtmpReq(i_pcData,i_iDataLen);
        m_dwRecvedDataLen += i_iDataLen ;
    }
    
    return iRet;
}

/*****************************************************************************
-Fuction        : Handshake
-Description    : 
-Input          : 
-Output         : 
-Return         : 
* Modify Date     Version        Author           Modification
* -----------------------------------------------
* 2023/09/21      V1.0.0         Yu Weifeng       Created
******************************************************************************/
int RtmpSession::Handshake(char *i_pcData,int i_iDataLen)
{
    int iRet = -1;
    
    //if (ComplexHandshake(i_pcData,i_iDataLen) < 0) //to do
    {
        iRet = SimpleHandshake(i_pcData,i_iDataLen);
        if (iRet < 0) 
        {
            RTMP_LOGE("Handshake err %d\r\n",iRet);
            return iRet;
        } 
    }
    //RTMP_LOGE("SimpleHandshake %d i_iDataLen %d\r\n",iRet,i_iDataLen);
    //if(RTMP_HANDSHAKE_DONE != m_eHandshakeState)//先发也无用，对方还是按照128分包
        //iRet = SendSetChunkSize(m_tRtmpSessionConfig.dwOutChunkSize);
    
    m_dwRecvedDataLen += i_iDataLen ;
    return iRet;
}

/*****************************************************************************
-Fuction        : HandleMsg
-Description    : 先缓存一个完整的chunk包，再解析chunk并根据csid分配重组消息，最后再处理msg
1.i_pcData中包含多个chunk 以及不带chunk头的数据，其中部分chunk组成1个msg
2.i_pcData中包含多个chunk 以及不带chunk头的数据，但只组成1个msg
3.i_pcData中包含多个chunk 以及不带chunk头的数据，不能组成1个msg
4.i_pcData中包含1个chunk ，组成1个msg
5.i_pcData中包含1个chunk ，不能组成1个msg
6.i_pcData中只有不带chunk头的数据，也自然不能组成1个msg
7.i_pcData中包含多个不同种类的chunk,分别组成不同的msg
-Input          : 
-Output         : 
-Return         : 
* Modify Date     Version        Author           Modification
* -----------------------------------------------
* 2023/09/21      V1.0.0         Yu Weifeng       Created
******************************************************************************/
int RtmpSession::HandleRtmpRequest(char *i_pcData,int i_iDataLen)
{
    int iRet = -1;
    T_RtmpChunkHeader tRtmpChunkHeader;
    int iProcessedLen = 0;
    char *pRtmpPacket = NULL;
    int iPacketLen = 0;
    char *pRtmpMsg = NULL;
    int iChunkBodyLen = 0;
    int iDiffLen = 0;
    int iChunkHeaderLen = 0;

    
    if(NULL == i_pcData ||i_iDataLen <= 0)
    {
        RTMP_LOGE("HandleRtmpRequest NULL %d \r\n",i_iDataLen);
        return iRet;
    }
    if(m_tRtmpChunkHandle.iChunkMaxLen < (int)m_tRtmpSessionConfig.dwInChunkSize)
    {
        RTMP_LOGE("m_tRtmpChunkHandle.iChunkMaxLen %d< (int)m_tRtmpSessionConfig.dwInChunkSize %d err \r\n",m_tRtmpChunkHandle.iChunkMaxLen,(int)m_tRtmpSessionConfig.dwInChunkSize);
        return iRet;
    }
    m_dwRecvedDataLen += i_iDataLen;
    (void)SendAcknowledgement(m_tRtmpSessionConfig.dwWindowSize);

    pRtmpPacket = i_pcData;
    iPacketLen = i_iDataLen;
    while(iPacketLen > 0)//很可能pRtmpPacket包含多个msg
    {
        //先缓存一个完整的chunk包
        iProcessedLen=0;
        iRet = ParseRtmpDataToChunk(pRtmpPacket,iPacketLen,&iProcessedLen);
        if(iRet < 0)//不是分包也不符合协议格式
        {
            RTMP_LOGE("ParseRtmpDataToChunk err exit %#x iChunkCurLen %d ,iPacketLen %d\r\n",(unsigned char)m_tRtmpChunkHandle.pbChunkBuf[0],m_tRtmpChunkHandle.iChunkCurLen,iPacketLen);
            return -1;//数据错误最好要结束当前会话
        }
        pRtmpPacket += iProcessedLen;//1.iRet>0取出符合dwInChunkSize大小的chunk
        iPacketLen -= iProcessedLen;
        if(0 == iRet)
        {
            if(iPacketLen > 0)
            {
                continue;
            }
            //chunk body有可能小于dwInChunkSize，比如消息的最后一块
            //(要区分是msg的某个chunk被tcp分包,还是msg的最后一个chunk或者是msg size<chunk size的情况)
            RTMP_LOGD("0==iPacketLen dwInChunkSize %d,pbChunkBuf %#x iChunkCurLen %d ,iChunkHeaderLen %d\r\n",m_tRtmpSessionConfig.dwInChunkSize,
            (unsigned char)m_tRtmpChunkHandle.pbChunkBuf[0],m_tRtmpChunkHandle.iChunkCurLen,m_tRtmpChunkHandle.iChunkHeaderLen);
            if(m_tRtmpChunkHandle.iChunkHeaderLen <= 0)
            {
                memset(&tRtmpChunkHeader,0,sizeof(T_RtmpChunkHeader));
                iChunkHeaderLen = m_pRtmpParse->GetRtmpHeader(m_tRtmpChunkHandle.pbChunkBuf,m_tRtmpChunkHandle.iChunkCurLen,&tRtmpChunkHeader);
                if(iChunkHeaderLen < 0)//不是分包也不符合协议格式
                {
                    RTMP_LOGE("m_pRtmpParse GetRtmpHeader err exit %#x iPacketLen %d ,iChunkHeaderLen %d\r\n",m_tRtmpChunkHandle.pbChunkBuf[0],m_tRtmpChunkHandle.iChunkCurLen,iChunkHeaderLen);
                    return -1;//数据错误最好要结束当前会话
                }
                memcpy(&m_tRtmpChunkHandle.tRtmpChunkHeader,&tRtmpChunkHeader,sizeof(T_RtmpChunkHeader));
                m_tRtmpChunkHandle.iChunkHeaderLen = iChunkHeaderLen;
            }
        }
        
        //一个(多个)完整的chunk包重组出一个完整的msg报文
        RTMP_LOGD("HandleChunk %#x iChunkHeaderLen %d,iChunkCurLen %d iPacketLen %d,dwLength %d\r\n",(unsigned char)m_tRtmpChunkHandle.pbChunkBuf[0],
        m_tRtmpChunkHandle.iChunkHeaderLen,m_tRtmpChunkHandle.iChunkCurLen,iPacketLen,m_tRtmpChunkHandle.tRtmpChunkHeader.tMsgHeader.dwLength);
        if(m_tRtmpChunkHandle.tRtmpChunkHeader.tMsgHeader.dwLength > RTMP_MSG_MAX_LEN)//
        {
            RTMP_LOGE("tMsgHeader.dwLength%d > RTMP_MSG_MAX_LEN%d err\r\n",m_tRtmpChunkHandle.tRtmpChunkHeader.tMsgHeader.dwLength, RTMP_MSG_MAX_LEN);
            return -1;//数据错误最好要结束当前会话
        }
        iChunkBodyLen = m_tRtmpChunkHandle.iChunkCurLen-m_tRtmpChunkHandle.iChunkHeaderLen;
        do
        {
            if(iChunkBodyLen >= (int)m_tRtmpChunkHandle.tRtmpChunkHeader.tMsgHeader.dwLength)//msg<chunk size
            {
                pRtmpMsg = m_tRtmpChunkHandle.pbChunkBuf+m_tRtmpChunkHandle.iChunkHeaderLen;
                memcpy(&tRtmpChunkHeader,&m_tRtmpChunkHandle.tRtmpChunkHeader,sizeof(T_RtmpChunkHeader));
                m_tRtmpChunkHandle.iChunkCurLen -= m_tRtmpChunkHandle.tRtmpChunkHeader.tMsgHeader.dwLength+m_tRtmpChunkHandle.iChunkHeaderLen;
                m_tRtmpChunkHandle.iChunkHeaderLen=0;
                break;
            }
            //msg由多个chunk组成
            auto iter = m_RtmpMsgHandleMap.find(m_tRtmpChunkHandle.tRtmpChunkHeader.tBasicHeader.dwChunkStreamID);//用map的目的是同一msg chunk可能不按顺序达到的情况
            if (iter == m_RtmpMsgHandleMap.end())
            {//没找到则缓存当前的
                CRtmpMsg cRtmpMsgBuf;
                memcpy(&cRtmpMsgBuf.tRtmpChunkHeader,&m_tRtmpChunkHandle.tRtmpChunkHeader,sizeof(T_RtmpChunkHeader));
                cRtmpMsgBuf.iChunkHeaderLen = m_tRtmpChunkHandle.iChunkHeaderLen;
                memcpy(cRtmpMsgBuf.pbMsgBuf+cRtmpMsgBuf.iMsgBufLen,m_tRtmpChunkHandle.pbChunkBuf+m_tRtmpChunkHandle.iChunkHeaderLen,iChunkBodyLen);
                cRtmpMsgBuf.iMsgBufLen+=iChunkBodyLen;
                m_RtmpMsgHandleMap.insert(make_pair(m_tRtmpChunkHandle.tRtmpChunkHeader.tBasicHeader.dwChunkStreamID,cRtmpMsgBuf));
                m_tRtmpChunkHandle.iChunkCurLen -= iChunkBodyLen+m_tRtmpChunkHandle.iChunkHeaderLen;
                m_tRtmpChunkHandle.iChunkHeaderLen=0;
                break;
            }
            iDiffLen = (int)m_tRtmpChunkHandle.tRtmpChunkHeader.tMsgHeader.dwLength-iter->second.iMsgBufLen;
            if(iChunkBodyLen < iDiffLen)//msg还没缓存完整
            {
                memcpy(iter->second.pbMsgBuf+iter->second.iMsgBufLen,m_tRtmpChunkHandle.pbChunkBuf+m_tRtmpChunkHandle.iChunkHeaderLen,iChunkBodyLen);
                iter->second.iMsgBufLen+=iChunkBodyLen;
                m_tRtmpChunkHandle.iChunkCurLen -= iChunkBodyLen+m_tRtmpChunkHandle.iChunkHeaderLen;
                m_tRtmpChunkHandle.iChunkHeaderLen=0;
                break;
            }
            pRtmpMsg = iter->second.pbMsgBuf;//用完释放
            memcpy(pRtmpMsg+iter->second.iMsgBufLen,m_tRtmpChunkHandle.pbChunkBuf+m_tRtmpChunkHandle.iChunkHeaderLen,iDiffLen);
            iter->second.iMsgBufLen+=iDiffLen;
            memcpy(&tRtmpChunkHeader,&m_tRtmpChunkHandle.tRtmpChunkHeader,sizeof(T_RtmpChunkHeader));
            m_tRtmpChunkHandle.iChunkCurLen -= iDiffLen+m_tRtmpChunkHandle.iChunkHeaderLen;
            m_tRtmpChunkHandle.iChunkHeaderLen=0;
            
            memset(&iter->second.tRtmpChunkHeader,0,sizeof(T_RtmpChunkHeader));
            iter->second.iMsgBufLen=0;
        }while(0);
        if(pRtmpMsg == NULL)
        {
            iRet = 0;
            continue;
        }
        iRet = HandleRtmpMsg(tRtmpChunkHeader.tMsgHeader.bTypeID,tRtmpChunkHeader.tMsgHeader.dwTimestamp,pRtmpMsg,tRtmpChunkHeader.tMsgHeader.dwLength);
        if(iRet < 0)
        {
            RTMP_LOGE("HandleRtmpMsg err %d ,dwLength %d ,iPacketLen %d,iChunkCurLen %d\r\n",tRtmpChunkHeader.tMsgHeader.bTypeID,tRtmpChunkHeader.tMsgHeader.dwLength,iPacketLen,m_tRtmpChunkHandle.iChunkCurLen);
        }
        else
        {
            RTMP_LOGI("HandleRtmpMsg success %d ,dwLength %d ,iPacketLen %d,iChunkCurLen %d\r\n",tRtmpChunkHeader.tMsgHeader.bTypeID,tRtmpChunkHeader.tMsgHeader.dwLength,iPacketLen,m_tRtmpChunkHandle.iChunkCurLen);
        }
        
        pRtmpMsg = NULL;
        if(m_tRtmpChunkHandle.iChunkCurLen > 0)
        {
            pRtmpPacket -= m_tRtmpChunkHandle.iChunkCurLen;
            iPacketLen += m_tRtmpChunkHandle.iChunkCurLen;
            m_tRtmpChunkHandle.iChunkCurLen = 0;
        }
    }

    return iRet;
}

/*****************************************************************************
-Fuction        : ParseRtmpDataToChunk
-Description    : 
-Input          : 
-Output         : 
-Return         : 
* Modify Date     Version        Author           Modification
* -----------------------------------------------
* 2023/09/21      V1.0.0         Yu Weifeng       Created
******************************************************************************/
int RtmpSession::ParseRtmpDataToChunk(char *i_pcData,int i_iDataLen,int *o_piProcessedLen)
{
    int iRet = -1;
    int iProcessedLen = 0;
    char *pRtmpPacket = NULL;
    int iPacketLen = 0;
    int iChunkHeaderLen = 0;
    T_RtmpChunkHeader tRtmpChunkHeader;

    pRtmpPacket = i_pcData;
    iPacketLen = i_iDataLen;

    if(0 == m_tRtmpChunkHandle.iChunkHeaderLen)
    {
        if(m_tRtmpChunkHandle.iChunkCurLen < (int)m_tRtmpSessionConfig.dwInChunkSize)//已有的还没缓存到一个chunk
        {//m_tRtmpSessionConfig.dwInChunkSize不包含Header
            if(iPacketLen < (int)m_tRtmpSessionConfig.dwInChunkSize-m_tRtmpChunkHandle.iChunkCurLen)//现在也还没缓存到一个chunk
            {
                iProcessedLen = iPacketLen;
            }
            else
            {
                iProcessedLen = (int)m_tRtmpSessionConfig.dwInChunkSize-m_tRtmpChunkHandle.iChunkCurLen;
            }
            memcpy(m_tRtmpChunkHandle.pbChunkBuf+m_tRtmpChunkHandle.iChunkCurLen,pRtmpPacket,iProcessedLen);
            m_tRtmpChunkHandle.iChunkCurLen += iProcessedLen;
            pRtmpPacket += iProcessedLen;
            iPacketLen -= iProcessedLen;
            if(iPacketLen <= 0)
            {
                *o_piProcessedLen = iProcessedLen;
                return 0;
            }
        }
        memset(&tRtmpChunkHeader,0,sizeof(T_RtmpChunkHeader));
        iChunkHeaderLen = m_pRtmpParse->GetRtmpHeader(m_tRtmpChunkHandle.pbChunkBuf,m_tRtmpChunkHandle.iChunkCurLen,&tRtmpChunkHeader);
        if(iChunkHeaderLen < 0)//不是分包也不符合协议格式
        {
            RTMP_LOGE("GetRtmpHeader %d err ,exit %#x iChunkCurLen %d ,iPacketLen %d\r\n",iChunkHeaderLen,(unsigned char)m_tRtmpChunkHandle.pbChunkBuf[0],m_tRtmpChunkHandle.iChunkCurLen,iPacketLen);
            return -1;//数据错误最好要结束当前会话
        }
        memcpy(&m_tRtmpChunkHandle.tRtmpChunkHeader,&tRtmpChunkHeader,sizeof(T_RtmpChunkHeader));
        m_tRtmpChunkHandle.iChunkHeaderLen = iChunkHeaderLen;
        
        *o_piProcessedLen = iProcessedLen;
        return 0;
    }

    //已经解析过头部
    if(m_tRtmpChunkHandle.iChunkCurLen < (int)m_tRtmpSessionConfig.dwInChunkSize+m_tRtmpChunkHandle.iChunkHeaderLen)
    {
        if(iPacketLen < (int)m_tRtmpSessionConfig.dwInChunkSize+m_tRtmpChunkHandle.iChunkHeaderLen-m_tRtmpChunkHandle.iChunkCurLen)//现在也还没缓存到一个chunk
        {
            iProcessedLen = iPacketLen;
        }
        else
        {
            iProcessedLen = (int)m_tRtmpSessionConfig.dwInChunkSize+m_tRtmpChunkHandle.iChunkHeaderLen-m_tRtmpChunkHandle.iChunkCurLen;
        }
        memcpy(m_tRtmpChunkHandle.pbChunkBuf+m_tRtmpChunkHandle.iChunkCurLen,pRtmpPacket,iProcessedLen);
        m_tRtmpChunkHandle.iChunkCurLen += iProcessedLen;
        pRtmpPacket += iProcessedLen;
        iPacketLen -= iProcessedLen;
        if(iPacketLen <= 0)
        {
            *o_piProcessedLen = iProcessedLen;
            return 0;
        }
    }
    
    *o_piProcessedLen = iProcessedLen;
    return iProcessedLen;
}

/*****************************************************************************
-Fuction        : HandleMsg
-Description    : 
1.i_pcData中包含多个chunk 以及不带chunk头的数据，其中部分chunk组成1个msg
2.i_pcData中包含多个chunk 以及不带chunk头的数据，但只组成1个msg
3.i_pcData中包含多个chunk 以及不带chunk头的数据，不能组成1个msg
4.i_pcData中包含1个chunk ，组成1个msg
5.i_pcData中包含1个chunk ，不能组成1个msg
6.i_pcData中只有不带chunk头的数据，也自然不能组成1个msg
7.i_pcData中包含多个不同种类的chunk,分别组成不同的msg
-Input          : 
-Output         : 
-Return         : 
* Modify Date     Version        Author           Modification
* -----------------------------------------------
* 2023/09/21      V1.0.0         Yu Weifeng       Created
******************************************************************************/
int RtmpSession::HandleRtmpReq(char *i_pcData,int i_iDataLen)
{
    int iRet = -1;
    T_RtmpChunkHeader tRtmpChunkHeader;
    int iProcessedLen = 0;
    char *pRtmpPacket = NULL;
    int iPacketLen = 0;
    int iRtmpBodyLen = 0;//整个网络流出去解析出头的长度

    
    if(NULL == i_pcData ||i_iDataLen <= 0)
    {
        RTMP_LOGE("HandleRtmpReq NULL %d \r\n",i_iDataLen);
        return iRet;
    }
    m_dwRecvedDataLen += i_iDataLen;
    (void)SendAcknowledgement(m_tRtmpSessionConfig.dwWindowSize);

    pRtmpPacket = i_pcData;
    iPacketLen = i_iDataLen;
    //char cqw[4096];
    //memcpy(cqw,i_pcData,i_iDataLen);
    while(iPacketLen > 0)//很可能pRtmpPacket包含多个msg
    {
        memset(&tRtmpChunkHeader,0,sizeof(T_RtmpChunkHeader));
        
        if(0 != m_tMsgBufHandle.iMsgBufLen % m_tRtmpSessionConfig.dwInChunkSize)//(新增逻辑),
        {//header 已处理了,后面的body由于tcp分包再次进来也会被处理,
            RTMP_LOGW("iMsgBufLen %d iPacketLen %d ,dwInChunkSize %d\r\n", m_tMsgBufHandle.iMsgBufLen,iPacketLen, m_tRtmpSessionConfig.dwInChunkSize);
            iProcessedLen = 0;//如果body数据恰巧和header数据吻合,会造成body数据当做header数据过滤处理掉,
        }//body数据就不正常,因此增加逻辑只有发生rtmp分包时才把数据当header处理,
        else//其余时刻都当做body数据不做header 解析处理。
        {//rtmp协议规定,分包后,分包的开头必须要带rtmp header。
            if(0 != m_tMsgBufHandle.iChunkHeaderLen && 0 == m_iChunkBodyLen)//特殊情况,
            {//如果已经接收到分包的header数据了,
                RTMP_LOGW("0 == m_iChunkBodyLen %#x iPacketLen %d ,iProcessedLen %d\r\n",pRtmpPacket[0],iPacketLen,iProcessedLen);
                iProcessedLen = 0;//但是body数据一个还没接收到,
            }//此时收到的是body数据，则不能当做header处理,
            else//否则会把有效的body数据过滤排除掉。
            {
                iProcessedLen = m_pRtmpParse->GetRtmpHeader(pRtmpPacket,iPacketLen,&tRtmpChunkHeader);
            }
        }//

        if(iProcessedLen < 0)//不是分包也不符合协议格式(iProcessedLen <= 0)
        {
            RTMP_LOGE("GetRtmpHeader err exit %#x iPacketLen %d ,iProcessedLen %d\r\n",pRtmpPacket[0],iPacketLen,iProcessedLen);
            return -1;//数据错误最好要结束当前会话
        }
        //if(iProcessedLen <= 0){break;}//忽略返回,因为有不带协议头的分包
        iRtmpBodyLen = iPacketLen-iProcessedLen;
        if(0 == m_tMsgBufHandle.iChunkHeaderLen)
        {//if(0 == m_tMsgBufHandle.iMsgBufLen)
            if(iRtmpBodyLen > (int)m_tRtmpSessionConfig.dwInChunkSize)//也可以考虑把判断提到if(0 == m_tMsgBufHandle.iMsgBufLen)外面
            {
                RTMP_LOGW("many chunk packet! iProcessedLen %d,iRtmpBodyLen %d,InChunkSize %d\r\n",iProcessedLen,iRtmpBodyLen,m_tRtmpSessionConfig.dwInChunkSize);
                m_iChunkBodyLen = m_tRtmpSessionConfig.dwInChunkSize;//dwInChunkSize不包含Header
            }
            else
            {
                m_iChunkBodyLen = iRtmpBodyLen;
            }
            if(m_iChunkBodyLen < (int)tRtmpChunkHeader.tMsgHeader.dwLength)//也可能多个chunk(pRtmpPacket)组成一个msg,第一个分包
            {
                RTMP_LOGW("m_iChunkBodyLen < tMsgHeader.dwLength %d<%d ,iPacketLen %d-iProcessedLen %d\r\n",
                m_iChunkBodyLen,tRtmpChunkHeader.tMsgHeader.dwLength,iPacketLen,iProcessedLen);
                memcpy(m_tMsgBufHandle.pbMsgBuf,pRtmpPacket+iProcessedLen,m_iChunkBodyLen);
                m_tMsgBufHandle.iMsgBufLen = m_iChunkBodyLen;
                memcpy(&m_tMsgBufHandle.tRtmpChunkHeader,&tRtmpChunkHeader,sizeof(T_RtmpChunkHeader));
                m_tMsgBufHandle.iChunkHeaderLen = sizeof(T_RtmpChunkHeader);
                pRtmpPacket = pRtmpPacket+iProcessedLen+m_iChunkBodyLen;
                iPacketLen = iPacketLen - iProcessedLen - m_iChunkBodyLen;
                if(0 != m_iChunkRemainLen)
                {
                    RTMP_LOGE("0 != m_iChunkRemainLen err %d\r\n",m_iChunkRemainLen);
                }
                if(m_iChunkBodyLen<(int)m_tRtmpSessionConfig.dwInChunkSize)
                {
                    RTMP_LOGD("first m_iChunkBodyLen %d< dwInChunkSize %d ,iPacketLen %d\r\n",m_iChunkBodyLen,m_tRtmpSessionConfig.dwInChunkSize,iPacketLen);
                    m_iChunkRemainLen = (int)m_tRtmpSessionConfig.dwInChunkSize-m_iChunkBodyLen;
                }
                continue;
            }
        }
        else
        {
            if(m_iChunkRemainLen > 0)
            {//Chunk被tcp分包，则应该先把剩下的chunkdata拷贝，这样后续的chunkheader才能被处理
                if(iRtmpBodyLen > m_iChunkRemainLen)
                {
                    m_iChunkBodyLen = m_iChunkRemainLen;
                }
                else
                {
                    m_iChunkBodyLen = iRtmpBodyLen;
                }
            }
            else
            {
                if(iRtmpBodyLen > (int)m_tRtmpSessionConfig.dwInChunkSize)
                {
                    RTMP_LOGI("many chunk packet2! iProcessedLen %d,iRtmpBodyLen %d,InChunkSize %d ,iPacketLen %d\r\n",iProcessedLen,iRtmpBodyLen,m_tRtmpSessionConfig.dwInChunkSize,iPacketLen);
                    m_iChunkBodyLen = m_tRtmpSessionConfig.dwInChunkSize;
                }
                else
                {
                    m_iChunkBodyLen = iRtmpBodyLen;
                }
            }
            if(iProcessedLen != 0 && tRtmpChunkHeader.tBasicHeader.dwChunkStreamID != m_tMsgBufHandle.tRtmpChunkHeader.tBasicHeader.dwChunkStreamID)
            {//同一个chunk流结果不按顺序到达，中间插入了其他chunk数据
                if(m_iChunkBodyLen>=(int)tRtmpChunkHeader.tMsgHeader.dwLength)//不等则寻找缓存中相应的chunk (返回指针处理)
                {//相等则用上一次的chunk，
                    iRet = HandleRtmpMsg(tRtmpChunkHeader.tMsgHeader.bTypeID,tRtmpChunkHeader.tMsgHeader.dwTimestamp,pRtmpPacket + iProcessedLen,tRtmpChunkHeader.tMsgHeader.dwLength);
                    pRtmpPacket = pRtmpPacket+iProcessedLen+tRtmpChunkHeader.tMsgHeader.dwLength;
                    iPacketLen = iPacketLen - iProcessedLen - tRtmpChunkHeader.tMsgHeader.dwLength;
                }
                else
                {
                    RTMP_LOGE("err m_iChunkBodyLen %d  , dwInChunkSize %d ,iPacketLen %d\r\n",m_iChunkBodyLen,m_tRtmpSessionConfig.dwInChunkSize,iPacketLen);
                    if(m_iChunkBodyLen<(int)m_tRtmpSessionConfig.dwInChunkSize)
                    {//需要记录当前的tRtmpChunkHeader才能偏移，暂时不处理
                    }//音频不分包，目前无影响
                    pRtmpPacket = pRtmpPacket+iProcessedLen+m_iChunkBodyLen;
                    iPacketLen = iPacketLen - iProcessedLen - m_iChunkBodyLen;
                }
                RTMP_LOGW("tRtmpChunkHeader.tBasicHeader.dwChunkStreamID old csid %d,new csid %d,bTypeID %d,dwLength %d,iPacketLen %d,iRet %d\r\n",m_tMsgBufHandle.tRtmpChunkHeader.tBasicHeader.dwChunkStreamID,
                tRtmpChunkHeader.tBasicHeader.dwChunkStreamID,tRtmpChunkHeader.tMsgHeader.bTypeID,tRtmpChunkHeader.tMsgHeader.dwLength,iPacketLen,iRet);
                continue;
            }
            if(m_iChunkBodyLen+m_tMsgBufHandle.iMsgBufLen < (int)m_tMsgBufHandle.tRtmpChunkHeader.tMsgHeader.dwLength)//也可能多个chunk组成一个msg,中间包
            {
                RTMP_LOGI("m_iChunkBodyLen+iMsgBufLen < dwLength!%d+%d<%d,iProcessedLen %d ,iPacketLen %d\r\n",m_iChunkBodyLen,m_tMsgBufHandle.iMsgBufLen,
                m_tMsgBufHandle.tRtmpChunkHeader.tMsgHeader.dwLength,iProcessedLen,iPacketLen);
                memcpy(m_tMsgBufHandle.pbMsgBuf+m_tMsgBufHandle.iMsgBufLen,pRtmpPacket+iProcessedLen,m_iChunkBodyLen);
                m_tMsgBufHandle.iMsgBufLen += m_iChunkBodyLen;
                pRtmpPacket = pRtmpPacket+iProcessedLen+m_iChunkBodyLen;
                iPacketLen = iPacketLen - iProcessedLen - m_iChunkBodyLen;
                if(0==m_iChunkRemainLen)
                {
                    if(m_iChunkBodyLen<(int)m_tRtmpSessionConfig.dwInChunkSize)
                    {
                        RTMP_LOGD("m_iChunkBodyLen %d< dwInChunkSize %d ,iPacketLen %d\r\n",m_iChunkBodyLen,m_tRtmpSessionConfig.dwInChunkSize,iPacketLen);
                        m_iChunkRemainLen = (int)m_tRtmpSessionConfig.dwInChunkSize-m_iChunkBodyLen;
                    }
                }
                else
                {
                    m_iChunkRemainLen -= m_iChunkBodyLen;
                }
                continue;
            }
            memcpy(m_tMsgBufHandle.pbMsgBuf+m_tMsgBufHandle.iMsgBufLen,pRtmpPacket+iProcessedLen,m_iChunkBodyLen);//最后包
            m_tMsgBufHandle.iMsgBufLen += m_iChunkBodyLen;
            pRtmpPacket = m_tMsgBufHandle.pbMsgBuf;
            memcpy(&tRtmpChunkHeader,&m_tMsgBufHandle.tRtmpChunkHeader,sizeof(T_RtmpChunkHeader));
            iPacketLen = iPacketLen - iProcessedLen - m_iChunkBodyLen;
            iProcessedLen = 0;
            m_iChunkRemainLen = 0;
            RTMP_LOGW("pbMsgBuf end bTypeID %d ,iMsgBufLen %d ,dwLength %d\r\n",tRtmpChunkHeader.tMsgHeader.bTypeID,m_tMsgBufHandle.iMsgBufLen,tRtmpChunkHeader.tMsgHeader.dwLength);
        }
        iRet = HandleRtmpMsg(tRtmpChunkHeader.tMsgHeader.bTypeID,tRtmpChunkHeader.tMsgHeader.dwTimestamp,pRtmpPacket + iProcessedLen,tRtmpChunkHeader.tMsgHeader.dwLength);
        if(iRet < 0)
        {
            RTMP_LOGE("HandleRtmpMsg err %d ,dwLength %d ,iPacketLen %d\r\n",tRtmpChunkHeader.tMsgHeader.bTypeID,tRtmpChunkHeader.tMsgHeader.dwLength,iPacketLen);
        }
        else
        {
            RTMP_LOGD("HandleRtmpMsg success %d ,dwLength %d ,iPacketLen %d\r\n",tRtmpChunkHeader.tMsgHeader.bTypeID,tRtmpChunkHeader.tMsgHeader.dwLength,iPacketLen);
        }
        if(0 != m_tMsgBufHandle.iChunkHeaderLen)//i_pcData多个msg情况
        {//if(0!=m_tMsgBufHandle.iMsgBufLen)
            if(m_tMsgBufHandle.iMsgBufLen > (int)tRtmpChunkHeader.tMsgHeader.dwLength)
                iPacketLen+=m_tMsgBufHandle.iMsgBufLen - tRtmpChunkHeader.tMsgHeader.dwLength;
            iProcessedLen = i_iDataLen - iPacketLen;
            pRtmpPacket = i_pcData+iProcessedLen;
            m_tMsgBufHandle.iMsgBufLen = 0;
            m_tMsgBufHandle.iChunkHeaderLen = 0;
        }
        else
        {
            iProcessedLen += tRtmpChunkHeader.tMsgHeader.dwLength;
            pRtmpPacket += iProcessedLen;
            iPacketLen -= iProcessedLen;
        }
        iRet = 0;
    }

    return i_iDataLen;
}
/*****************************************************************************
-Fuction        : HandleRtmpReqData
-Description    : 该接口考虑把所有的分包先缓存起来，然后再处理，逻辑应当更简单
1.i_pcData中包含多个chunk 以及不带chunk头的数据，其中部分chunk组成1个msg
2.i_pcData中包含多个chunk 以及不带chunk头的数据，但只组成1个msg
3.i_pcData中包含多个chunk 以及不带chunk头的数据，不能组成1个msg
4.i_pcData中包含1个chunk ，组成1个msg
5.i_pcData中包含1个chunk ，不能组成1个msg
6.i_pcData中只有不带chunk头的数据，也自然不能组成1个msg
7.i_pcData中包含多个不同种类的chunk,分别组成不同的msg
-Input          : 
-Output         : 
-Return         : 
* Modify Date     Version        Author           Modification
* -----------------------------------------------
* 2023/09/21      V1.0.0         Yu Weifeng       Created
******************************************************************************/
int RtmpSession::HandleRtmpReqData(char *i_pcData,int i_iDataLen)
{
    int iRet = -1;
    T_RtmpChunkHeader tRtmpChunkHeader;
    int iProcessedLen = 0;
    char *pRtmpPacket = NULL;
    int iPacketLen = 0;
    int iRtmpBodyLen = 0;//整个网络流出去解析出头的长度

    
    if(NULL == i_pcData ||i_iDataLen <= 0)
    {
        RTMP_LOGE("HandleRtmpReq NULL %d \r\n",i_iDataLen);
        return iRet;
    }
    m_dwRecvedDataLen += i_iDataLen;
    (void)SendAcknowledgement(m_tRtmpSessionConfig.dwWindowSize);


    memcpy(&m_tMsgBufHandle.pbMsgBuf[m_tMsgBufHandle.iMsgBufLen],i_pcData,i_iDataLen);
    m_tMsgBufHandle.iMsgBufLen += i_iDataLen;

    while(m_tMsgBufHandle.iMsgBufLen > (int)m_tRtmpSessionConfig.dwInChunkSize)//很可能pRtmpPacket包含多个msg
    {
        memset(&tRtmpChunkHeader,0,sizeof(T_RtmpChunkHeader));
        
    }

    return i_iDataLen;
}

/*****************************************************************************
-Fuction        : SimpleHandshake
-Description    : 
-Input          : 
-Output         : 
-Return         : 0 ok,-1 err -2 need more data ,RTMP_HANDSHAKE_DONE processedLen
* Modify Date     Version        Author           Modification
* -----------------------------------------------
* 2023/09/21      V1.0.0         Yu Weifeng       Created
******************************************************************************/
int RtmpSession::SimpleHandshake(char *i_pcData,int i_iDataLen)
{
    int iRet = -1;
    int iVersion = 0;
    char abS0S1S2[RTMP_S0S1S2_LEN];
    int iS0S1S2Len = 0;
    char *pcData = NULL;
    int iDataLen = 0;

    
    if(NULL == i_pcData ||i_iDataLen <= 0)
    {
        RTMP_LOGE("SimpleHandshake NULL %d \r\n",i_iDataLen);
        return iRet;
    }
#if 0
    switch(m_eHandshakeState)
    {
        case RTMP_HANDSHAKE_INIT:
        {
            if(i_iDataLen+m_iHandshakeBufLen < RTMP_C0_LEN+RTMP_C1_LEN)
            {
                memcpy(m_abHandshakeBuf+m_iHandshakeBufLen,i_pcData,i_iDataLen);
                m_iHandshakeBufLen+=i_iDataLen;
                RTMP_LOGE("SimpleHandshake need more data %d\r\n",i_iDataLen);
                return -2;
            }
            if(0 != m_iHandshakeBufLen)
            {
                memcpy(m_abHandshakeBuf+m_iHandshakeBufLen,i_pcData,i_iDataLen);
                m_iHandshakeBufLen+=i_iDataLen;
                pcData = m_abHandshakeBuf;
                iDataLen = m_iHandshakeBufLen;
            }
            else
            {
                pcData = i_pcData;
                iDataLen = i_iDataLen;
            }
            iVersion = m_pRtmpParse->GetRtmpVersion(pcData,iDataLen);
            if(iVersion > RTMP_VERSION)
            {
                RTMP_LOGE("RtmpVersion err %d\r\n",iVersion);
                return iRet;
            }
            memset(abS0S1S2,0,sizeof(abS0S1S2));
            iS0S1S2Len = m_pRtmpPack->CreateS0S1S2(pcData+RTMP_C0_LEN,iDataLen-RTMP_C0_LEN,abS0S1S2,sizeof(abS0S1S2));
            if(iS0S1S2Len < 0)
            {
                RTMP_LOGE("CreateS0S1S2 err %d\r\n",iS0S1S2Len);
                return iRet;
            }
            iRet = SendData(abS0S1S2,iS0S1S2Len);
            if(iRet < 0)
            {
                RTMP_LOGE("SendData err %d\r\n",iRet);
                return iRet;
            }
            m_eHandshakeState = RTMP_HANDSHAKE_C0C1;
            memset(m_abHandshakeBuf,0,sizeof(m_abHandshakeBuf));
            m_iHandshakeBufLen = 0;
            break;
        }
        case RTMP_HANDSHAKE_C0C1:
        {
            if(i_iDataLen+m_iHandshakeBufLen < RTMP_C2_LEN)
            {
                m_iHandshakeBufLen+=i_iDataLen;
                RTMP_LOGW("SimpleHandshake need more data %d %d\r\n",i_iDataLen,m_iHandshakeBufLen);
                return 0;
            }
            iRet = RTMP_C2_LEN-m_iHandshakeBufLen;
            m_iHandshakeBufLen = 0;
            m_eHandshakeState = RTMP_HANDSHAKE_DONE;
            break;
        }
        default:
        {
            break;
        }
    }
    
#endif
    return iRet;
}

/*****************************************************************************
-Fuction        : Handshake
-Description    : 参考SrsComplexHandshake::handshake_with_client
-Input          : 
-Output         : 
-Return         : 
* Modify Date     Version        Author           Modification
* -----------------------------------------------
* 2023/09/21      V1.0.0         Yu Weifeng       Created
******************************************************************************/
int RtmpSession::ComplexHandshake(char *i_pcData,int i_iDataLen)
{
    int iRet = -1;
    char abC0C1[RTMP_C0_LEN+RTMP_C1_LEN];
    int iLen = 0;
    int iVersion = 0;

    memset(abC0C1,0,sizeof(abC0C1));
    //iLen = RecvData(abC0C1,sizeof(abC0C1));
    if(iLen <= 0)
    {
        return iRet;
    }
    iVersion = m_pRtmpParse->GetRtmpVersion(abC0C1,iLen);
    if(iVersion > RTMP_VERSION)
    {
        RTMP_LOGE("RtmpVersion err %d\r\n",iVersion);
        return iRet;
    }
    //后续参考SrsComplexHandshake::handshake_with_client 或者media server的rtmp_server_input
    return 0;
}

/*****************************************************************************
-Fuction        : SeverTestThread
-Description    : 需要每1ms得到处理,这样音视频包才能及时发走
-Input          : 
-Output         : 
-Return         :  must > 0
* Modify Date     Version        Author           Modification
* -----------------------------------------------
* 2023/11/21      V1.0.0         Yu Weifeng       Created
******************************************************************************/
int RtmpSession::ClientTestThread(const char *i_strFileName)
{
	T_FlvTagHeader tFlvTagHeader;
    unsigned int dwPreviousTagSize;
    size_t count = 0;
    int iRet = -1;
    
    if (false == m_blPushing) 
    {
        return 0;
    }
    m_dwFileDiffCnt++;
    if (m_dwFileDiffCnt < m_dwFileTimestamp) 
    {
        return 0;
    }
    FILE *p = fopen(i_strFileName, "rb");
    if(p == NULL)
    { 
        RTMP_LOGE("SeverTest NULL %s \r\n", i_strFileName);
        return -1;
    }
    do
    {
        if(0 == m_dwOffset)
        {
            count = fread(m_pbFileData, sizeof(char),FLV_HEADER_LEN+4, p);
            if(count != FLV_HEADER_LEN+4)
            { 
                RTMP_LOGE("SeverTest FLV_HEADER_LEN err %d \r\n", count);
                break;
            }
            m_dwOffset+=FLV_HEADER_LEN+4;
        }
        else
        {
            fseek(p, m_dwOffset, SEEK_CUR);
        }
        count = fread(m_pbFileData, sizeof(char),FLV_TAG_HEADER_LEN, p);
        if(count != FLV_TAG_HEADER_LEN)
        {
            if(feof(p))
            {
                m_dwOffset = 0;//一直循环
                m_dwFileBaseTimestamp = m_dwFileTimestamp;
                iRet = 0;
                break;
            }
            RTMP_LOGE("SeverTest FLV_TAG_HEADER_LEN err %d \r\n", count);
            break;
        }
        m_pRtmpMediaHandle->ParseFlvTagHeader(m_pbFileData,FLV_TAG_HEADER_LEN,&tFlvTagHeader);
        if(tFlvTagHeader.dwSize > 1024*1024)
        {
            RTMP_LOGE("SeverTest size err%d\r\n", tFlvTagHeader.dwSize);
            break;
        }
        count = fread(m_pbFileData, sizeof(char),tFlvTagHeader.dwSize, p);
        if(count != tFlvTagHeader.dwSize)
        { 
            RTMP_LOGE("SeverTest fread dwSize err %d %d \r\n",count, tFlvTagHeader.dwSize);
            break;
        }
        if(tFlvTagHeader.dwTimestamp+m_dwFileBaseTimestamp>m_dwFileDiffCnt)//依赖文件开始时间戳是0
        {
            m_dwFileTimestamp = tFlvTagHeader.dwTimestamp+m_dwFileBaseTimestamp;
            iRet = 1;
            break;
        }
        m_dwOffset+=FLV_TAG_HEADER_LEN+tFlvTagHeader.dwSize+4;
        iRet = 0;
    }while(0);
    fclose(p);
    if(iRet < 0)
    {
        return -1;
    }
    if(0 == m_dwOffset ||1 == iRet)
    {
        return 0;
    }
    
    int iVideoDataLen = 0;
    T_RtmpMediaInfo tRtmpMediaInfo;
    int iAudioDataLen = 0;
    unsigned char abPublishAudioData[RTMP_PLAY_SRC_MAX_LEN];
    m_dwFileTimestamp = tFlvTagHeader.dwTimestamp+m_dwFileBaseTimestamp;
    switch(tFlvTagHeader.bType)
    {
        case FLV_TAG_VIDEO_TYPE :
        {
            //demux
            iVideoDataLen = m_pRtmpMediaHandle->GetVideoData(m_pbFileData,tFlvTagHeader.dwSize,&m_tPlayVideoParam,m_pbPlayFrameData,RTMP_MSG_MAX_LEN);
            if(iVideoDataLen < 0)
            {
                RTMP_LOGE("GetVideoData err %d \r\n",iVideoDataLen);
            }
            else if(iVideoDataLen == 0)
            {
                RTMP_LOGD("GetVideoData need more data %d \r\n",iVideoDataLen);
            }
            else
            {
                memset(&tRtmpMediaInfo,0,sizeof(T_RtmpMediaInfo));
                tRtmpMediaInfo.eEncType = m_tPlayVideoParam.eEncType;
                tRtmpMediaInfo.eFrameType= m_tPlayVideoParam.eFrameType;
                tRtmpMediaInfo.ddwTimestamp = m_dwFileTimestamp;
                
                tRtmpMediaInfo.dwSampleRate = m_tPlayAudioParam.dwSamplesPerSecond;
                tRtmpMediaInfo.dwChannels = m_tPlayAudioParam.dwChannels;
                tRtmpMediaInfo.dwBitsPerSample = m_tPlayAudioParam.dwBitsPerSample;
                
                //WriteFile("d:\\test\\2023AAC.h264", m_pbPlayFrameData,iVideoDataLen);
                DoPush(&tRtmpMediaInfo,m_pbPlayFrameData, iVideoDataLen,NULL);
            }
            break;
        }
        case FLV_TAG_AUDIO_TYPE :
        {
            //demux
            iAudioDataLen = m_pRtmpMediaHandle->GetAudioData(m_pbFileData,tFlvTagHeader.dwSize,&m_tPlayAudioParam,abPublishAudioData,RTMP_PLAY_SRC_MAX_LEN);
            if(iAudioDataLen < 0)
            {
                RTMP_LOGE("GetAudioData err %d \r\n",iAudioDataLen);
            }
            else if(iAudioDataLen == 0)
            {
                RTMP_LOGD("GetAudioData need more data %d \r\n",iAudioDataLen);
            }
            else
            {
                memset(&tRtmpMediaInfo,0,sizeof(T_RtmpMediaInfo));
                tRtmpMediaInfo.eEncType = m_tPlayAudioParam.eEncType;
                tRtmpMediaInfo.eFrameType= RTMP_AUDIO_FRAME;
                tRtmpMediaInfo.ddwTimestamp = m_dwFileTimestamp;
                tRtmpMediaInfo.dwSampleRate = m_tPlayAudioParam.dwSamplesPerSecond;
                tRtmpMediaInfo.dwChannels = m_tPlayAudioParam.dwChannels;
                tRtmpMediaInfo.dwBitsPerSample = m_tPlayAudioParam.dwBitsPerSample;
                
                DoPush(&tRtmpMediaInfo,abPublishAudioData,iAudioDataLen,NULL);
            }
            break;
        }
        case FLV_TAG_SCRIPT_TYPE :
        {
            break;
        }
        default :
        {
            break;
        }
    }
    
	return 0;
}

/*****************************************************************************
-Fuction        : SeverTest
-Description    : SeverTest
-Input          : 
-Output         : 
-Return         :  must > 0
* Modify Date     Version        Author           Modification
* -----------------------------------------------
* 2023/11/21      V1.0.0         Yu Weifeng       Created
******************************************************************************/
int RtmpSession::ClientTest(const char *i_strFileName)
{
	T_FlvTagHeader tFlvTagHeader;
    unsigned int dwPreviousTagSize;
    size_t count = 0;
    int iRet = -1;
    int iVideoDataLen = 0;
    T_RtmpMediaInfo tRtmpMediaInfo;
    int iAudioDataLen = 0;
    unsigned char abPublishAudioData[RTMP_PLAY_SRC_MAX_LEN];
    int iProcesssed = 0;
    int iExit = 0;

    if (false == m_blPushing) 
    {
        return 0;
    }
    memset(&tFlvTagHeader,0,sizeof(T_FlvTagHeader));
    while(0 == iExit)
    {
        FILE *p = fopen(i_strFileName, "rb");
        if(p == NULL)
        { 
            RTMP_LOGE("SeverTest NULL %s \r\n", i_strFileName);
            return -1;
        }
        do
        {
            if(0 == m_dwOffset)
            {
                count = fread(m_pbFileData, sizeof(char),FLV_HEADER_LEN+4, p);
                if(count != FLV_HEADER_LEN+4)
                { 
                    RTMP_LOGE("SeverTest FLV_HEADER_LEN err %d \r\n", count);
                    break;
                }
                m_dwOffset+=FLV_HEADER_LEN+4;
            }
            else
            {
                fseek(p, m_dwOffset, SEEK_CUR);
            }
            count = fread(m_pbFileData, sizeof(char),FLV_TAG_HEADER_LEN, p);
            if(count != FLV_TAG_HEADER_LEN)
            {
                if(feof(p))
                {
                    m_dwOffset = 0;//一直循环
                    m_dwFileBaseTimestamp = m_dwFileTimestamp;
                    iRet = 0;
                    break;
                }
                RTMP_LOGE("SeverTest FLV_TAG_HEADER_LEN err %d \r\n", count);
                break;
            }
            m_pRtmpMediaHandle->ParseFlvTagHeader(m_pbFileData,FLV_TAG_HEADER_LEN,&tFlvTagHeader);
            if(tFlvTagHeader.dwSize > 1024*1024)
            {
                RTMP_LOGE("SeverTest size err%d\r\n", tFlvTagHeader.dwSize);
                break;
            }
            count = fread(m_pbFileData, sizeof(char),tFlvTagHeader.dwSize, p);
            if(count != tFlvTagHeader.dwSize)
            { 
                RTMP_LOGE("SeverTest fread dwSize err %d %d \r\n",count, tFlvTagHeader.dwSize);
                break;
            }
            if(FLV_TAG_VIDEO_TYPE == tFlvTagHeader.bType && tFlvTagHeader.dwTimestamp+m_dwFileBaseTimestamp-m_dwFileTimestamp>0)
            {//以视频时间戳为基准
                if(0 == iProcesssed)
                {
                    iProcesssed=1;//第一次正常处理
                    m_dwOffset+=FLV_TAG_HEADER_LEN+tFlvTagHeader.dwSize+4;
                }
                else
                {
                    iExit = 1;//第二次延迟发送,让时间戳均匀
                }
            }
            else
            {
                if(FLV_TAG_VIDEO_TYPE == tFlvTagHeader.bType && 0 == iProcesssed)
                {
                    iProcesssed=1;//第一次正常处理
                }
                m_dwOffset+=FLV_TAG_HEADER_LEN+tFlvTagHeader.dwSize+4;
            }
            iRet = 0;
        }while(0);
        fclose(p);
        if(0 != iRet)
        {
            return -1;
        }
        if(0 == m_dwOffset ||0 != iExit)
        {
            return 0;
        }
        
        switch(tFlvTagHeader.bType)
        {
            case FLV_TAG_VIDEO_TYPE :
            {
                m_dwFileTimestamp = tFlvTagHeader.dwTimestamp+m_dwFileBaseTimestamp;//以视频时间戳为基准
                //demux
                iVideoDataLen = m_pRtmpMediaHandle->GetVideoData(m_pbFileData,tFlvTagHeader.dwSize,&m_tPlayVideoParam,m_pbPlayFrameData,RTMP_MSG_MAX_LEN);
                if(iVideoDataLen < 0)
                {
                    RTMP_LOGE("GetVideoData err %d \r\n",iVideoDataLen);
                }
                else if(iVideoDataLen == 0)
                {
                    RTMP_LOGD("GetVideoData need more data %d \r\n",iVideoDataLen);
                }
                else
                {
                    memset(&tRtmpMediaInfo,0,sizeof(T_RtmpMediaInfo));
                    tRtmpMediaInfo.eEncType = m_tPlayVideoParam.eEncType;
                    tRtmpMediaInfo.eFrameType= m_tPlayVideoParam.eFrameType;
                    tRtmpMediaInfo.ddwTimestamp = m_dwFileTimestamp;
                    
                    tRtmpMediaInfo.dwSampleRate = m_tPlayAudioParam.dwSamplesPerSecond;
                    tRtmpMediaInfo.dwChannels = m_tPlayAudioParam.dwChannels;
                    tRtmpMediaInfo.dwBitsPerSample = m_tPlayAudioParam.dwBitsPerSample;
                    
                    //WriteFile("d:\\test\\2023AAC.h264", m_pbPlayFrameData,iVideoDataLen);
                    DoPush(&tRtmpMediaInfo,m_pbPlayFrameData, iVideoDataLen,NULL);
                }
                break;
            }
            case FLV_TAG_AUDIO_TYPE :
            {
                //demux
                iAudioDataLen = m_pRtmpMediaHandle->GetAudioData(m_pbFileData,tFlvTagHeader.dwSize,&m_tPlayAudioParam,abPublishAudioData,RTMP_PLAY_SRC_MAX_LEN);
                if(iAudioDataLen < 0)
                {
                    RTMP_LOGE("GetAudioData err %d \r\n",iAudioDataLen);
                }
                else if(iAudioDataLen == 0)
                {
                    RTMP_LOGD("GetAudioData need more data %d \r\n",iAudioDataLen);
                }
                else
                {
                    memset(&tRtmpMediaInfo,0,sizeof(T_RtmpMediaInfo));
                    tRtmpMediaInfo.eEncType = m_tPlayAudioParam.eEncType;
                    tRtmpMediaInfo.eFrameType= RTMP_AUDIO_FRAME;
                    tRtmpMediaInfo.ddwTimestamp = m_dwFileTimestamp;
                    tRtmpMediaInfo.dwSampleRate = m_tPlayAudioParam.dwSamplesPerSecond;
                    tRtmpMediaInfo.dwChannels = m_tPlayAudioParam.dwChannels;
                    tRtmpMediaInfo.dwBitsPerSample = m_tPlayAudioParam.dwBitsPerSample;
                    
                    DoPush(&tRtmpMediaInfo,abPublishAudioData,iAudioDataLen,NULL);
                }
                break;
            }
            case FLV_TAG_SCRIPT_TYPE :
            {
                break;
            }
            default :
            {
                break;
            }
        }
    }

	return 0;
}

