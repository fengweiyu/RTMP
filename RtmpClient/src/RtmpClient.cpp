/*****************************************************************************
* Copyright (C) 2023-2028 Hanson Yu  All rights reserved.
------------------------------------------------------------------------------
* File Module       :   RtmpClient.h
* Description       :   RtmpClient operation center
                        RTMP服务适配外部环境(输入输出，主要采用注册机制)
* Created           :   2023.09.21.
* Author            :   Yu Weifeng
* Function List     :   
* Last Modified     :   
* History           :   
******************************************************************************/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "Include/RtmpClient.h"
#include "RtmpMediaHandle.h"
#include "RtmpAdapter.h"

#include "libFoxNetObject.h"
#include "LibsMsgIdDefine.h"
#include "XNetObject.h"

#define RTMP_CLIENT_TIMER_MSG_ID 5332
#define RTMP_CLIENT_START_MSG_ID 5333
#define RTMP_CLIENT_STOP_MSG_ID 5334
#define RTMP_CLIENT_PUSHING_MSG_ID 5335



/*****************************************************************************
-Fuction        : RtmpClient
-Description    : //0 play,1 publish
-Input          : 
-Output         : 
-Return         : 
* Modify Date     Version        Author           Modification
* -----------------------------------------------
* 2023/09/21      V1.0.0         Yu Weifeng       Created
******************************************************************************/
RtmpClient::RtmpClient()
{
    m_pRtmpSession = NULL;
    m_iTimer = -1;
    m_iStarter = -1;
    m_iSendResed = 0;
    m_iSendResTime = 0;
    m_iConnected = 0;
    m_iConnectRes = -1;
    m_iConnectTime = 0;
    m_iConnectCnt = 0;
}

/*****************************************************************************
-Fuction        : RtmpClient
-Description    : //0 play,1 publish
-Input          : 
-Output         : 
-Return         : 
* Modify Date     Version        Author           Modification
* -----------------------------------------------
* 2023/09/21      V1.0.0         Yu Weifeng       Created
******************************************************************************/
RtmpClient::RtmpClient(int i_iPlayOrPublish,char * i_strURL)
{
    T_RtmpCb tRtmpCb;
    T_RtmpSessionConfig tRtmpSessionConfig;
    
    memset(&tRtmpCb,0,sizeof(T_RtmpCb));

    tRtmpCb.SendData = RtmpClient::SendDatas;
    tRtmpCb.PlayVideoData = RtmpClient::PlayVideoData;
    tRtmpCb.PlayAudioData = RtmpClient::PlayAudioData;
    tRtmpCb.PlayScriptData = RtmpClient::PlayScriptData;
    tRtmpCb.Connect = RtmpClient::ConnectServer;
    tRtmpCb.tRtmpPackCb.GetRandom = RtmpClient::GetRandom;

    memset(&tRtmpSessionConfig,0,sizeof(T_RtmpSessionConfig));
    tRtmpSessionConfig.dwWindowSize= 5000000;//默认值
    tRtmpSessionConfig.dwPeerBandwidth = 5000000;
    tRtmpSessionConfig.dwOutChunkSize = RTMP_OUTPUT_CHUNK_SIZE;
    tRtmpSessionConfig.dwInChunkSize = RTMP_INPUT_CHUNK_MAX_SIZE;
    tRtmpSessionConfig.iPlayOrPublish = i_iPlayOrPublish;
    snprintf(tRtmpSessionConfig.strURL,sizeof(tRtmpSessionConfig.strURL),"%s",i_strURL);
    
    m_pRtmpSession = new RtmpSession(this,&tRtmpCb,&tRtmpSessionConfig);

    if(m_iTimer < 0)
    {
        SetTimer(500);
    }
}

/*****************************************************************************
-Fuction        : RtmpClient
-Description    : 
-Input          : 
-Output         : 
-Return         : 
* Modify Date     Version        Author           Modification
* -----------------------------------------------
* 2023/09/21      V1.0.0         Yu Weifeng       Created
******************************************************************************/
RtmpClient::~RtmpClient()
{
    if(NULL!= m_pRtmpSession)
    {
        delete m_pRtmpSession;
    }
    if(m_iTimer >= 0)
    {
        KillXTimer(m_iTimer);
        m_iTimer = -1;
    }
}


/*****************************************************************************
-Fuction        : Start
-Description    : 
-Input          : 
-Output         : 
-Return         : 
* Modify Date     Version        Author           Modification
* -----------------------------------------------
* 2023/09/21      V1.0.0         Yu Weifeng       Created
******************************************************************************/
int RtmpClient::Start(int i_iPlayOrPublish,char * i_strURL)
{
    if(NULL != i_strURL && NULL == m_pRtmpSession)
    {
        T_RtmpCb tRtmpCb;
        T_RtmpSessionConfig tRtmpSessionConfig;
        
        memset(&tRtmpCb,0,sizeof(T_RtmpCb));
        
        tRtmpCb.SendData = RtmpClient::SendDatas;
        tRtmpCb.PlayVideoData = RtmpClient::PlayVideoData;
        tRtmpCb.PlayAudioData = RtmpClient::PlayAudioData;
        tRtmpCb.PlayScriptData = RtmpClient::PlayScriptData;
        tRtmpCb.Connect = RtmpClient::ConnectServer;
        tRtmpCb.tRtmpPackCb.GetRandom = RtmpClient::GetRandom;
        
        memset(&tRtmpSessionConfig,0,sizeof(T_RtmpSessionConfig));
        tRtmpSessionConfig.dwWindowSize= 5000000;//默认值
        tRtmpSessionConfig.dwPeerBandwidth = 5000000;
        tRtmpSessionConfig.dwOutChunkSize = RTMP_OUTPUT_CHUNK_SIZE;
        tRtmpSessionConfig.dwInChunkSize = RTMP_INPUT_CHUNK_MAX_SIZE;
        tRtmpSessionConfig.iPlayOrPublish = i_iPlayOrPublish;
        snprintf(tRtmpSessionConfig.strURL,sizeof(tRtmpSessionConfig.strURL),"%s",i_strURL);
        
        m_pRtmpSession = new RtmpSession(this,&tRtmpCb,&tRtmpSessionConfig);
    }

    if(m_iTimer < 0)
    {
        SetTimer(500);
    }

    return 0;
}

/*****************************************************************************
-Fuction        : Stop
-Description    : 
-Input          : 
-Output         : 
-Return         : 
* Modify Date     Version        Author           Modification
* -----------------------------------------------
* 2023/09/21      V1.0.0         Yu Weifeng       Created
******************************************************************************/
int RtmpClient::Stop(int err)
{
    if(m_iTimer >= 0)
    {
        KillXTimer(m_iTimer);
        m_iTimer = -1;
    }
    this->OnDisConnect(err);

    return 0;
}

/*****************************************************************************
-Fuction        : Pushing
-Description    : 非阻塞
-Input          : 
-Output         : 
-Return         : 
* Modify Date     Version        Author           Modification
* -----------------------------------------------
* 2023/09/21      V1.0.0         Yu Weifeng       Created
******************************************************************************/
int RtmpClient::Pushing(FRAME_INFO * i_pFrame)
{
    E_RTMP_ENC_TYPE eRtmpEncType = RTMP_UNKNOW_ENC_TYPE;
    T_RtmpMediaInfo tRtmpMediaInfo;
    E_RTMP_FRAME_TYPE eFrameType = RTMP_UNKNOW_FRAME;
    
    if(NULL == i_pFrame)
    {
        RTMP_LOGE("Playing NULL \r\n");
        return -1;
    }

    if (i_pFrame->nType == FRAME_TYPE_VIDEO)
    {
        switch (i_pFrame->nSubType)
        {
            case FRAME_TYPE_VIDEO_I_FRAME:
            {
                eFrameType = RTMP_VIDEO_KEY_FRAME;
                break;
            }
            case FRAME_TYPE_VIDEO_P_FRAME:
            case FRAME_TYPE_VIDEO_B_FRAME:
            case FRAME_TYPE_VIDEO_S_FRAME:
            {
                eFrameType = RTMP_VIDEO_INNER_FRAME;
                break;
            }
            default:
                break;
        }
    }
    else if (i_pFrame->nType == FRAME_TYPE_AUDIO)
    {
        eFrameType = RTMP_AUDIO_FRAME;
    }
    else
    {
    }
    
    switch (i_pFrame->nEncodeType)
    {
        case ENCODE_VIDEO_H264:
        {
            eRtmpEncType = RTMP_ENC_H264;
            break;
        }
        case ENCODE_VIDEO_H265:
        {
            eRtmpEncType = RTMP_ENC_H265;
            break;
        }
        case ENCODE_AUDIO_AAC:
        {
            eRtmpEncType = RTMP_ENC_AAC;
            break;
        }
        case ENCODE_AUDIO_G711A:
        {
            eRtmpEncType = RTMP_ENC_G711A;
            break;
        }
        default:
            break;
    }
    
    memset(&tRtmpMediaInfo,0,sizeof(T_RtmpMediaInfo));
    tRtmpMediaInfo.eEncType = eRtmpEncType;
    tRtmpMediaInfo.eFrameType = eFrameType;
    tRtmpMediaInfo.ddwTimestamp= i_pFrame->nTimeStamp;
    tRtmpMediaInfo.dwFrameRate= i_pFrame->nFrameRate;
    tRtmpMediaInfo.dwHeight= i_pFrame->nHeight;
    tRtmpMediaInfo.dwWidth = i_pFrame->nWidth;
    tRtmpMediaInfo.dlDuration= 0.0;
    tRtmpMediaInfo.dwSampleRate= i_pFrame->nSamplesPerSecond;
    tRtmpMediaInfo.dwBitsPerSample= i_pFrame->nBitsPerSample;
    tRtmpMediaInfo.dwChannels= i_pFrame->nChannels;
    return m_pRtmpSession->DoPush(&tRtmpMediaInfo,i_pFrame->pContent,i_pFrame->nFrameLength);
}

/*****************************************************************************
-Fuction        : OnMsg
-Description    : 非阻塞
-Input          : 
-Output         : 
-Return         : 
* Modify Date     Version        Author           Modification
* -----------------------------------------------
* 2023/09/21      V1.0.0         Yu Weifeng       Created
******************************************************************************/
int RtmpClient::OnMsg(PXMSG msg)
{
    int iRet = -1;
    
    switch (msg->id)
    {
        case RTMP_CLIENT_START_MSG_ID:
        {
            RTMP_LOGW("RTMP_CLIENT_START_MSG_ID %d,%d\r\n",m_iConnectRes,m_iConnected);
            this->Start(msg->param1,(char *)msg->Str());//0 play,1 publish,default 1,url
            m_iStarter = msg->sender.Id();
            break;
        }
        case RTMP_CLIENT_STOP_MSG_ID:
        {
            RTMP_LOGW("RTMP_CLIENT_STOP_MSG_ID %d,%d\r\n",m_iConnectRes,m_iConnected);
            m_pRtmpSession->DoStop();//主动断开
            m_iStarter = -1;
            this->Stop(-1);//url
            break;
        }
        case RTMP_CLIENT_PUSHING_MSG_ID:
        {
            iRet = this->Pushing(dynamic_cast<FRAME_INFO*>(msg->pParam));//
            CMSGObject::PushMsg(msg->sender, XMSG::Obtain(msg->sender, EIMSG_MEDIA_ON_STATE, iRet));            
            break;
        }
        case RTMP_CLIENT_TIMER_MSG_ID:
        {
            if(0 == m_iConnected)
            {
                iRet = m_pRtmpSession->DoConnect();
                if(iRet >= 0)
                {
                    m_iConnected = 1;
                    break;
                }
            }
            if(0 != m_iConnectRes ||0 == m_iConnected )
            {
                RTMP_LOGE("DoConnect err %d,%d\r\n",m_iConnectRes,m_iConnected);
                m_iConnectTime++;
                if(m_iConnectTime > 10)//5s超时
                {
                    RTMP_LOGE("DoConnect timeout %d,%d\r\n",m_iConnectTime,m_iConnectCnt);
                    m_iConnected = 0;//重连
                    m_iConnectTime = 0;
                    m_iConnectCnt++;
                    if(m_iConnectCnt > 3)
                    {
                        RTMP_LOGE("DoConnect stop %d,%d\r\n",m_iConnectTime,m_iConnectCnt);
                        this->Stop(-1);//url
                    }
                }
                break;
            }
            iRet = m_pRtmpSession->DoCycle(NULL,0);
            if(iRet > 0)
            {
                if(m_iSendResed == 0)
                {
                    CMSGObject::PushMsg(m_iStarter, XMSG::Obtain(m_iStarter, EIMSG_MEDIA_ON_STATE, 0));
                    m_iSendResed = 1;
                }
            }
            else
            {
                m_iSendResTime++;
                if(m_iSendResTime > 10)//5s超时
                {
                    this->Stop(iRet);//url
                }
            }
            break;
        }
        default:
            break;
    }
    return CXNetClient::OnMsg(msg);
}

/*****************************************************************************
-Fuction        : PlayVideoData
-Description    : 
-Input          : 
-Output         : 
-Return         : 
* Modify Date     Version        Author           Modification
* -----------------------------------------------
* 2023/09/21      V1.0.0         Yu Weifeng       Created
******************************************************************************/
int RtmpClient::PlayVideoData(T_RtmpMediaInfo *i_ptRtmpMediaInfo,char * i_acDataBuf,int i_iDataLen,void *i_pIoHandle)
{
    int iRet = -1;
    FRAME_INFO* ptFrameInfo = NULL;
    
    ptFrameInfo = RtmpFrameToFrameInfo(i_ptRtmpMediaInfo,(unsigned char *)i_acDataBuf,i_iDataLen);
    if(NULL == ptFrameInfo)
    {
        RTMP_LOGE("PushVideoData err \r\n");
        return -1;
    }

    return 0;
}

/*****************************************************************************
-Fuction        : PlayAudioData
-Description    : 
-Input          : 
-Output         : 
-Return         : 
* Modify Date     Version        Author           Modification
* -----------------------------------------------
* 2023/09/21      V1.0.0         Yu Weifeng       Created
******************************************************************************/
int RtmpClient::PlayAudioData(T_RtmpMediaInfo *i_ptRtmpMediaInfo,char * i_acDataBuf,int i_iDataLen,void *i_pIoHandle)
{
    int iRet = -1;
    FRAME_INFO* ptFrameInfo = NULL;
    
    ptFrameInfo = RtmpFrameToFrameInfo(i_ptRtmpMediaInfo,(unsigned char *)i_acDataBuf,i_iDataLen);
    if(NULL == ptFrameInfo)
    {
        RTMP_LOGE("PushAudioData err \r\n");
        return -1;
    }
    
    return 0;
}

/*****************************************************************************
-Fuction        : StartPlay
-Description    : 
-Input          : 
-Output         : 
-Return         : 
* Modify Date     Version        Author           Modification
* -----------------------------------------------
* 2023/09/21      V1.0.0         Yu Weifeng       Created
******************************************************************************/
int RtmpClient::PlayScriptData(char *i_strStreamName,unsigned int i_dwTimestamp,char * i_acDataBuf,int i_iDataLen)
{
    int iRet = -1;

    RTMP_LOGD("PushScriptData %s \r\n",i_strStreamName);

    return 0;
}

/*****************************************************************************
-Fuction        : SendData
-Description    : 
-Input          : 
-Output         : 
-Return         : 
* Modify Date     Version        Author           Modification
* -----------------------------------------------
* 2023/09/21      V1.0.0         Yu Weifeng       Created
******************************************************************************/
int RtmpClient::ConnectToServer(char * i_strIP,unsigned short i_wPort)
{
    int iRet = -1;
    
    if(NULL == i_strIP ||i_wPort <= 0)
    {
        RTMP_LOGE("ConnectToServer NULL %d \r\n",i_wPort);
        return iRet;
    }
	//InitNetWorker(EXNetClientType_TCP_Def, false);
	//CXNetClient::_pWorker->SetIP((const char *)i_strIP);
	//CXNetClient::_pWorker->SetPort((int)i_wPort);
    
    CXNetClient::Connect((const char *)i_strIP,(int)i_wPort);//依赖CXNetClient::OnMsg(msg);得到调用才会去connect
    return 0;
}

/*****************************************************************************
-Fuction        : SendData
-Description    : 
-Input          : 
-Output         : 
-Return         : 
* Modify Date     Version        Author           Modification
* -----------------------------------------------
* 2023/09/21      V1.0.0         Yu Weifeng       Created
******************************************************************************/
int RtmpClient::ConnectServer(void *i_pIoHandle,char * i_strIP,unsigned short i_wPort)
{
    int iRet = -1;
    RtmpClient *pRtmpClient = NULL;
    
    if(NULL == i_pIoHandle ||NULL == i_strIP ||i_wPort <= 0)
    {
        RTMP_LOGE("ConnectServer NULL %d \r\n",i_wPort);
        return iRet;
    }

    pRtmpClient = (RtmpClient *)i_pIoHandle;
    
    return pRtmpClient->ConnectToServer(i_strIP,i_wPort);
}

/*****************************************************************************
-Fuction        : SendDatas
-Description    : 
-Input          : 
-Output         : 
-Return         : 
* Modify Date     Version        Author           Modification
* -----------------------------------------------
* 2023/09/21      V1.0.0         Yu Weifeng       Created
******************************************************************************/
int RtmpClient::SendDatas(void *i_pIoHandle,char * i_acSendBuf,int i_iSendLen)
{
    int iRet = -1;
    RtmpClient *pRtmpClient = NULL;
    
    if(NULL == i_pIoHandle ||NULL == i_acSendBuf ||i_iSendLen <= 0)
    {
        RTMP_LOGE("SendData NULL %d \r\n",i_iSendLen);
        return iRet;
    }
    pRtmpClient = (RtmpClient *)i_pIoHandle;
    pRtmpClient->SendData(i_acSendBuf,i_iSendLen);

    return 0;
}
/*****************************************************************************
-Fuction        : ~CRtmpMediaSource
-Description    : 
-Input          : 
-Output         : 
-Return         : 
* Modify Date     Version        Author           Modification
* -----------------------------------------------
* 2023/09/21      V1.0.0         Yu Weifeng       Created
******************************************************************************/
void RtmpClient::OnRecvData(const char* pData, int nDataLen)
{
    if(m_pRtmpSession->DoCycle((char *)pData,nDataLen)<0)
    {
        RTMP_LOGE("OnRecvData err ,will del self!!![%d]",nDataLen);
        this->OnDisConnect(-1);
    }
}

/*****************************************************************************
-Fuction        : OnDisConnect
-Description    : 
-Input          : 
-Output         : 
-Return         : 
* Modify Date     Version        Author           Modification
* -----------------------------------------------
* 2023/09/21      V1.0.0         Yu Weifeng       Created
******************************************************************************/
int RtmpClient::OnConnect(int nResult)
{
    if(0 != nResult )
    {
        m_iConnectRes = -1;
        RTMP_LOGE("OnConnect err \r\n");
        return -1;
    }
    m_iConnectRes = 0;
    return 0;
}

/*****************************************************************************
-Fuction        : OnDisConnect
-Description    : 
-Input          : 
-Output         : 
-Return         : 
* Modify Date     Version        Author           Modification
* -----------------------------------------------
* 2023/09/21      V1.0.0         Yu Weifeng       Created
******************************************************************************/
int RtmpClient::OnDisConnect(int nError)
{
    int iRet = -1;
    if(m_iStarter > 0)
        CMSGObject::PushMsg(m_iStarter, XMSG::Obtain(m_iStarter, EIMSG_MEDIA_ON_STATE, nError));
    this->DeleteSelf();
    return 0;
}

/*****************************************************************************
-Fuction        : SetTimer
-Description    : 非阻塞
-Input          : 
-Output         : 
-Return         : 
* Modify Date     Version        Author           Modification
* -----------------------------------------------
* 2023/09/21      V1.0.0         Yu Weifeng       Created
******************************************************************************/
int RtmpClient::SetTimer(int i_iTimeMs)
{
    m_iTimer = SetXTimer(_handle, i_iTimeMs, XMSG::Obtain(RTMP_CLIENT_TIMER_MSG_ID));
    return 0;
}


/*****************************************************************************
-Fuction        : GetRandom
-Description    : 非阻塞
-Input          : 
-Output         : 
-Return         : 
* Modify Date     Version        Author           Modification
* -----------------------------------------------
* 2023/09/21      V1.0.0         Yu Weifeng       Created
******************************************************************************/
long RtmpClient::GetRandom()
{
    return rand();
}

/*****************************************************************************
-Fuction        : RtmpFrameToFrameInfo
-Description    : 非阻塞
-Input          : 
-Output         : 
-Return         : 
* Modify Date     Version        Author           Modification
* -----------------------------------------------
* 2023/09/21      V1.0.0         Yu Weifeng       Created
******************************************************************************/
FRAME_INFO* RtmpClient::RtmpFrameToFrameInfo(T_RtmpMediaInfo * i_ptRtmpMediaInfo,unsigned char *i_pbFrameData,int i_iDataLen) 
{
	FRAME_INFO* ptFrameInfo = new FRAME_INFO();

    switch (i_ptRtmpMediaInfo->eFrameType)
    {
        case RTMP_VIDEO_KEY_FRAME:
        {
            ptFrameInfo->nType = FRAME_TYPE_VIDEO;
            ptFrameInfo->nSubType = FRAME_TYPE_VIDEO_I_FRAME;
            break;
        }
        case RTMP_VIDEO_INNER_FRAME:
        {
            ptFrameInfo->nType = FRAME_TYPE_VIDEO;
            ptFrameInfo->nSubType = FRAME_TYPE_VIDEO_P_FRAME;
            break;
        }
        case RTMP_AUDIO_FRAME:
        {
            ptFrameInfo->nType = FRAME_TYPE_AUDIO;
            break;
        }
        default:
            break;
    }
    switch (i_ptRtmpMediaInfo->eEncType)
    {
        case RTMP_ENC_H264:
        {
            ptFrameInfo->nEncodeType = ENCODE_VIDEO_H264;
            break;
        }
        case RTMP_ENC_H265:
        {
            ptFrameInfo->nEncodeType = ENCODE_VIDEO_H265;
            break;
        }
        case RTMP_ENC_AAC:
        {
            ptFrameInfo->nEncodeType = ENCODE_AUDIO_AAC;
            break;
        }
        case RTMP_ENC_G711A:
        {
            ptFrameInfo->nEncodeType = ENCODE_AUDIO_G711A;
            break;
        }
        case RTMP_ENC_G711U:
        {
            ptFrameInfo->nEncodeType = ENCODE_AUDIO_G711U;
            break;
        }
        case RTMP_ENC_ADPCM:
        {
            ptFrameInfo->nEncodeType = ENCODE_AUDIO_ADPCM;
            break;
        }
        default:
        {
            RTMP_LOGE("i_ptRtmpMediaInfo->eEncType err%d\r\n",i_ptRtmpMediaInfo->eEncType);
            delete ptFrameInfo;
            return NULL;
        }
    }
	ptFrameInfo->nChannels = i_ptRtmpMediaInfo->dwChannels;
	ptFrameInfo->nSamplesPerSecond = i_ptRtmpMediaInfo->dwSampleRate;
	ptFrameInfo->nBitsPerSample = i_ptRtmpMediaInfo->dwBitsPerSample;
	ptFrameInfo->nWidth = i_ptRtmpMediaInfo->dwWidth;
	ptFrameInfo->nHeight = i_ptRtmpMediaInfo->dwHeight;
	ptFrameInfo->nTimeStamp = i_ptRtmpMediaInfo->ddwTimestamp;
	ptFrameInfo->nFrameRate = i_ptRtmpMediaInfo->dwFrameRate;
	ptFrameInfo->nFrameLength = i_iDataLen;

#if 0	
	ptFrameInfo->pContent = new unsigned char[i_iDataLen]{ 0 };

	memcpy(ptFrameInfo->pContent,i_pbFrameData,i_iDataLen);
	PXData pXD = new XData(ptFrameInfo->pContent, ptFrameInfo->nFrameLength, false);
	ptFrameInfo->pHeader = ptFrameInfo->pContent;
	ptFrameInfo->SetData(pXD);
#endif	

    FRAME_INFO* pFrame = CSTDStream::NewFrame(ptFrameInfo, (const char*)i_pbFrameData, i_iDataLen);

    delete ptFrameInfo;
	return pFrame;
}

