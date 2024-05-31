/*****************************************************************************
* Copyright (C) 2020-2025 Hanson Yu  All rights reserved.
------------------------------------------------------------------------------
* File Module           :       RtmpClientManager.c
* Description           : 	    
主要处理播放器相关的逻辑，
比如推流
比如接收流
* Created               :       2023.01.13.
* Author                :       Yu Weifeng
* Function List         : 	
* Last Modified         : 	
* History               : 	
******************************************************************************/
#include "RtmpClientManager.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <iostream>
#include <utility>

#define RTMPC_FRAME_BUF_MAX_LEN	(2*1024*1024) 
#define RTMPC_FILE_BUF_MAX_LEN	(6*1024*1024) 

using std::make_pair;

/*****************************************************************************
-Fuction		: RtmpServerManager
-Description	: RtmpServerManager
-Input			: 
-Output 		: 
-Return 		: 
* Modify Date	  Version		 Author 		  Modification
* -----------------------------------------------
* 2023/10/10	  V1.0.0		 Yu Weifeng 	  Created
******************************************************************************/
RtmpClientManager :: RtmpClientManager()
{
    m_pMediaHandle = new MediaHandle();
    m_iProcFlag = 0;
    m_iMediaProcFlag = 0;
    m_pbFileBuf = new unsigned char [RTMPC_FILE_BUF_MAX_LEN];
    if(NULL == m_pbFileBuf)
    {
        RTMPC_LOGE("NULL == m_pbFileBuf err\r\n");
    } 
    m_pRtmpClientIO = new RtmpClientIO();
    m_pHandlePushMediaHandle = new MediaHandle();
}

/*****************************************************************************
-Fuction		: ~RtmpServerManager
-Description	: ~RtmpServerManager
-Input			: 
-Output 		: 
-Return 		: 
* Modify Date	  Version		 Author 		  Modification
* -----------------------------------------------
* 2023/10/10	  V1.0.0		 Yu Weifeng 	  Created
******************************************************************************/
RtmpClientManager :: ~RtmpClientManager()
{
    if(NULL!= m_pHandlePushMediaHandle)
    {
        delete m_pHandlePushMediaHandle;
        m_pHandlePushMediaHandle = NULL;
    }
    if(NULL!= m_pRtmpClientIO)
    {
        delete m_pRtmpClientIO;
        m_pRtmpClientIO = NULL;
    }
    if(NULL!= m_pMediaHandle)
    {
        delete m_pMediaHandle;
        m_pMediaHandle = NULL;
    }
    if(NULL!= m_pMediaFile)
    {
        fclose(m_pMediaFile);
        m_pMediaFile = NULL;
    }
    if(NULL!= m_pbFileBuf)
    {
        delete [] m_pbFileBuf;
        m_pbFileBuf = NULL;
    }
    if(NULL!= m_pFileName)
    {
        delete m_pFileName;
        m_pFileName = NULL;
    }
}

/*****************************************************************************
-Fuction		: Proc
-Description	: 阻塞
-Input			: 
-Output 		: 
-Return 		: 
* Modify Date	  Version		 Author 		  Modification
* -----------------------------------------------
* 2023/10/10	  V1.0.0		 Yu Weifeng 	  Created
******************************************************************************/
int RtmpClientManager :: Proc(char *i_strURL)
{
    int iRet = -1;
    T_RtmpClientCb tRtmpClientCb;

    int iPlayOrPublish=0;//0 play,1 publish
    string strURL(i_strURL);
    auto dwPush = strURL.find("push/");
    if(string::npos != dwPush)//rtmp://10.10.10.10:10/push_10/Mnx8Y2UEdXTQ%3D%3D.9eaa64fa64a282
    {
        iPlayOrPublish=1;
        m_pFileName = new string(strURL.substr(dwPush+strlen("push/")).c_str());
    }
    auto dwPlay = strURL.find("play/");
    if(string::npos != dwPlay)//rtmp://10.10.10.10:10/push_10/Mnx8Y2UEdXTQ%3D%3D.9eaa64fa64a282
    {
        iPlayOrPublish=0;
        m_pFileName = new string(strURL.substr(dwPlay+strlen("play/")).c_str());
    }
    if(string::npos == dwPush && string::npos == dwPlay)
    {
        iPlayOrPublish=0;
        m_pFileName = new string("tmp");
    }
    if(NULL == m_pFileName)
    {
        RTMPC_LOGW("NULL == m_pFileName err   %s\r\n",i_strURL);
        return -1;
    }
    memset(&tRtmpClientCb,0,sizeof(T_RtmpClientCb));
    tRtmpClientCb.PlayData = RtmpClientManager::HandlePlayData;
    tRtmpClientCb.pIoHandle = this;

    if(0 != iPlayOrPublish)
    {
        m_pMediaHandle->Init((char *)m_pFileName->c_str());//默认取文件流
    }
    m_pRtmpClientIO->Start(i_strURL,tRtmpClientCb);
    m_iProcFlag = 1;
    while(m_iProcFlag)
    {
        if(0 != iPlayOrPublish)
        {
            if(m_pRtmpClientIO->GetPushingFlag()>0)
            {
                iRet=this->MediaProc();//阻塞
                break;
            }
        }
        if(m_pRtmpClientIO->GetProcFlag()<0)  
        {  
            break;
        } 
        else
        {
            SleepMs(100);
        }
    }
    m_iProcFlag = 0;
    return 0;
}
/*****************************************************************************
-Fuction        : Proc
-Description    : //return ResLen,<0 err
-Input          : 
-Output         : 
-Return         : 
* Modify Date     Version             Author           Modification
* -----------------------------------------------
* 2020/01/13      V1.0.0              Yu Weifeng       Created
******************************************************************************/
int RtmpClientManager::MediaProc()
{
    int iRet = -1;
    T_MediaFrameInfo tFileFrameInfo;
	unsigned int dwFileLastTimeStamp=0;
	int iSleepTimeMS=0;

    m_iMediaProcFlag = 1;
    RTMPC_LOGW("RtmpClientManager start MediaProc %s\r\n",m_pFileName->c_str());
    
    memset(&tFileFrameInfo,0,sizeof(T_MediaFrameInfo));
    tFileFrameInfo.pbFrameBuf = new unsigned char [RTMPC_FRAME_BUF_MAX_LEN];
    tFileFrameInfo.iFrameBufMaxLen = RTMPC_FRAME_BUF_MAX_LEN;
    while(m_iMediaProcFlag)
    {
        iRet=m_pMediaHandle->GetFrame(&tFileFrameInfo);//非文件流可直接调用此接口
        if(iRet<0)
        {
            RTMPC_LOGE("MediaProc exit %d[%s]\r\n",iRet,m_pFileName->c_str());
            break;
        }
        if(tFileFrameInfo.dwTimeStamp<dwFileLastTimeStamp)
        {
            RTMPC_LOGE("dwTimeStamp err exit %d,%d\r\n",tFileFrameInfo.dwTimeStamp,dwFileLastTimeStamp);
            break;
        }
        iSleepTimeMS=(int)(tFileFrameInfo.dwTimeStamp-dwFileLastTimeStamp);
        if(iSleepTimeMS > 0)
        {
            SleepMs(iSleepTimeMS);//模拟实时流(直播)，点播和当前的处理机制不匹配，需要后续再开发
        }
        
        iRet=this->Pushing(&tFileFrameInfo);
        if(iRet<0)
        {
            RTMPC_LOGE("Playing err exit %d[%s]\r\n",iRet,m_pFileName->c_str());
            break;
        }
        dwFileLastTimeStamp = tFileFrameInfo.dwTimeStamp;
    }
    if(NULL!= tFileFrameInfo.pbFrameBuf)
    {
        delete[] tFileFrameInfo.pbFrameBuf;
        tFileFrameInfo.pbFrameBuf = NULL;//
    }
    m_iMediaProcFlag = 0;
    return iRet;
}
/*****************************************************************************
-Fuction        : Playing
-Description    : 
-Input          : 
-Output         : 
-Return         : 
* Modify Date     Version        Author           Modification
* -----------------------------------------------
* 2023/09/21      V1.0.0         Yu Weifeng       Created
******************************************************************************/
int RtmpClientManager::Pushing(T_MediaFrameInfo * i_pFrame)
{
    E_RTMP_ENC_TYPE eRtmpEncType = RTMP_UNKNOW_ENC_TYPE;
    T_RtmpMediaInfo tRtmpMediaInfo;
    E_RTMP_FRAME_TYPE eFrameType = RTMP_UNKNOW_FRAME;
    
    if(NULL == i_pFrame)
    {
        RTMP_LOGE("Pushing NULL \r\n");
        return -1;
    }

    switch (i_pFrame->eFrameType)
    {
        case MEDIA_FRAME_TYPE_VIDEO_I_FRAME:
        {
            eFrameType = RTMP_VIDEO_KEY_FRAME;
            break;
        }
        case MEDIA_FRAME_TYPE_VIDEO_P_FRAME:
        case MEDIA_FRAME_TYPE_VIDEO_B_FRAME:
        {
            eFrameType = RTMP_VIDEO_INNER_FRAME;
            break;
        }
        case MEDIA_FRAME_TYPE_AUDIO_FRAME:
        {
            eFrameType = RTMP_AUDIO_FRAME;
            break;
        }
        default:
            break;
    }

    
    switch (i_pFrame->eEncType)
    {
        case MEDIA_ENCODE_TYPE_H264:
        {
            eRtmpEncType = RTMP_ENC_H264;
            break;
        }
        case MEDIA_ENCODE_TYPE_H265:
        {
            eRtmpEncType = RTMP_ENC_H265;
            break;
        }
        case MEDIA_ENCODE_TYPE_AAC:
        {
            eRtmpEncType = RTMP_ENC_AAC;
            break;
        }
        case MEDIA_ENCODE_TYPE_G711A:
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
    tRtmpMediaInfo.ddwTimestamp= i_pFrame->dwTimeStamp;
    tRtmpMediaInfo.dwFrameRate= 25;//内部不使用，暂时填默认值
    tRtmpMediaInfo.dwHeight= i_pFrame->dwHeight;
    tRtmpMediaInfo.dwWidth = i_pFrame->dwWidth;
    tRtmpMediaInfo.dlDuration= 0.0;
    tRtmpMediaInfo.dwSampleRate= i_pFrame->dwSampleRate;
    tRtmpMediaInfo.dwBitsPerSample= i_pFrame->tAudioEncodeParam.dwBitsPerSample;
    tRtmpMediaInfo.dwChannels= i_pFrame->tAudioEncodeParam.dwChannels;
    return m_pRtmpClientIO->Pushing(&tRtmpMediaInfo,i_pFrame->pbFrameStartPos,i_pFrame->iFrameLen,NULL);
}

/*****************************************************************************
-Fuction		: HandlePushURL
-Description	: RtmpServerIO
-Input			: 
-Output 		: 
-Return 		: 
* Modify Date	  Version		 Author 		  Modification
* -----------------------------------------------
* 2017/10/10	  V1.0.0		 Yu Weifeng 	  Created
******************************************************************************/
int RtmpClientManager :: HandlePlayMediaData(T_RtmpMediaInfo *i_ptRtmpMediaInfo,char * i_acDataBuf,int i_iDataLen)
{
    int iRet = -1;
    int iWriteLen = -1;
    int iHeaderLen = -1;
    T_MediaFrameInfo tFileFrameInfo;
    E_MediaEncodeType eEncType;
    E_MediaFrameType eFrameType;
    char strFileName[128]={0,};

    if(NULL == m_pFileName)//"D:\\test\\2023AAC.flv"
    {
        RTMPC_LOGE("HandlePlayMediaData err %s\r\n",m_pFileName->c_str());
        return iRet;
    }
    snprintf(strFileName,sizeof(strFileName),"%s.mp4",m_pFileName->c_str());//固定.h264文件
    if(NULL == m_pMediaFile)
    {
        m_pMediaFile = fopen(strFileName,"wb");//
    } 


    switch (i_ptRtmpMediaInfo->eFrameType)
    {
        case RTMP_VIDEO_KEY_FRAME:
        {
            eFrameType = MEDIA_FRAME_TYPE_VIDEO_I_FRAME;
            break;
        }
        case RTMP_VIDEO_INNER_FRAME:
        {
            eFrameType = MEDIA_FRAME_TYPE_VIDEO_P_FRAME;
            break;
        }
        case RTMP_AUDIO_FRAME:
        {
            eFrameType = MEDIA_FRAME_TYPE_AUDIO_FRAME;
            break;
        }
        default:
            break;
    }

    switch (i_ptRtmpMediaInfo->eEncType)
    {
        case RTMP_ENC_H264:
        {
            eEncType = MEDIA_ENCODE_TYPE_H264;
            break;
        }
        case RTMP_ENC_H265:
        {
            eEncType = MEDIA_ENCODE_TYPE_H265;
            break;
        }
        case RTMP_ENC_AAC:
        {
            eEncType = MEDIA_ENCODE_TYPE_AAC;
            break;
        }
        case RTMP_ENC_G711A:
        {
            eEncType = MEDIA_ENCODE_TYPE_G711A;
            break;
        }
        default:
            break;
    }
    memset(&tFileFrameInfo,0,sizeof(T_MediaFrameInfo));
    tFileFrameInfo.eEncType = eEncType;
    tFileFrameInfo.eFrameType = eFrameType;
    tFileFrameInfo.dwTimeStamp= (unsigned int)i_ptRtmpMediaInfo->ddwTimestamp;
    tFileFrameInfo.dwHeight= i_ptRtmpMediaInfo->dwHeight;
    tFileFrameInfo.dwWidth = i_ptRtmpMediaInfo->dwWidth;
    tFileFrameInfo.dwSampleRate= i_ptRtmpMediaInfo->dwSampleRate;
    tFileFrameInfo.tAudioEncodeParam.dwBitsPerSample= i_ptRtmpMediaInfo->dwBitsPerSample;
    tFileFrameInfo.tAudioEncodeParam.dwChannels= i_ptRtmpMediaInfo->dwChannels;
    
    tFileFrameInfo.tVideoEncodeParam.iSizeOfSPS= i_ptRtmpMediaInfo->wSpsLen;//
    tFileFrameInfo.tVideoEncodeParam.iSizeOfPPS= i_ptRtmpMediaInfo->wPpsLen;//
    tFileFrameInfo.tVideoEncodeParam.iSizeOfVPS= i_ptRtmpMediaInfo->wVpsLen;//
    memcpy(tFileFrameInfo.tVideoEncodeParam.abVPS,i_ptRtmpMediaInfo->abVPS,tFileFrameInfo.tVideoEncodeParam.iSizeOfVPS);//
    memcpy(tFileFrameInfo.tVideoEncodeParam.abSPS,i_ptRtmpMediaInfo->abSPS,tFileFrameInfo.tVideoEncodeParam.iSizeOfSPS);//
    memcpy(tFileFrameInfo.tVideoEncodeParam.abPPS,i_ptRtmpMediaInfo->abPPS,tFileFrameInfo.tVideoEncodeParam.iSizeOfPPS);//
    //tFileFrameInfo.pbFrameStartPos = (unsigned char *)i_acDataBuf;
    //tFileFrameInfo.iFrameLen = i_iDataLen;
    tFileFrameInfo.pbFrameBuf = (unsigned char *)i_acDataBuf;
    tFileFrameInfo.iFrameBufLen = i_iDataLen;
    tFileFrameInfo.iFrameBufMaxLen = i_iDataLen;
    tFileFrameInfo.eStreamType=STREAM_TYPE_MUX_STREAM;
    
    if(NULL == m_pHandlePushMediaHandle)
    {
        RTMPC_LOGE("NULL == m_pHandlePushMediaHandle err iWriteLen %d\r\n",iWriteLen);
        return iRet;
    }
    iRet=m_pHandlePushMediaHandle->GetFrame(&tFileFrameInfo);//annex-b的必须先解析成avcc的格式，所以要先调用这个接口进行分析
    if(iRet < 0)
    {
        RTMPC_LOGE("GetFrame err %d\r\n",iWriteLen);
        return iRet;
    }
    iWriteLen = m_pMediaHandle->FrameToContainer(&tFileFrameInfo,STREAM_TYPE_FMP4_STREAM,m_pbFileBuf,RTMPC_FILE_BUF_MAX_LEN,&iHeaderLen);
    if(iWriteLen < 0)
    {
        RTMPC_LOGE("FrameToContainer err iWriteLen %d\r\n",iWriteLen);
        return iRet;
    }
    if(iWriteLen == 0)
    {
        return iWriteLen;
    }
    iRet = fwrite(m_pbFileBuf, 1,iWriteLen, m_pMediaFile);
    if(iRet != iWriteLen)
    {
        RTMPC_LOGE("fwrite err %d iWriteLen%d\r\n",iRet,iWriteLen);
        return iWriteLen;
    }

    return iWriteLen;
}


/*****************************************************************************
-Fuction        : PushAudioData
-Description    : 
-Input          : 
-Output         : 
-Return         : 
* Modify Date     Version        Author           Modification
* -----------------------------------------------
* 2023/09/21      V1.0.0         Yu Weifeng       Created
******************************************************************************/
int RtmpClientManager::HandlePlayData(T_RtmpMediaInfo *i_ptRtmpMediaInfo,char * i_acDataBuf,int i_iDataLen,void *i_pIoHandle)
{
    int iRet = -1;
    
    if(NULL == i_ptRtmpMediaInfo ||NULL == i_acDataBuf ||NULL == i_pIoHandle)
    {
        RTMP_LOGE("HandlePlayAudioData NULL \r\n");
        return iRet;
    }
    RtmpClientManager *pRtmpIO = (RtmpClientManager *)i_pIoHandle;
    return pRtmpIO->HandlePlayMediaData(i_ptRtmpMediaInfo,i_acDataBuf,i_iDataLen);
}




