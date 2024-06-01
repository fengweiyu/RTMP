/*****************************************************************************
* Copyright (C) 2020-2025 Hanson Yu  All rights reserved.
------------------------------------------------------------------------------
* File Module           :       RtmpServerIO.c
* Description           : 	
* Created               :       2023.01.13.
* Author                :       Yu Weifeng
* Function List         : 	
* Last Modified         : 	
* History               : 	
******************************************************************************/
#include "RtmpServerIO.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
//#include <unistd.h>
#include <thread>

using std::thread;

#define RTMPS_RECV_MAX_LEN (10240)
#define RTMPS_FRAME_BUF_MAX_LEN	(2*1024*1024) 
#define RTMPS_FILE_BUF_MAX_LEN	(6*1024*1024) 

/*****************************************************************************
-Fuction		: RtmpServerIO
-Description	: RtmpServerIO
-Input			: 
-Output 		: 
-Return 		: 
* Modify Date	  Version		 Author 		  Modification
* -----------------------------------------------
* 2017/10/10	  V1.0.0		 Yu Weifeng 	  Created
******************************************************************************/
RtmpServerIO :: RtmpServerIO(int i_iClientSocketFd)
{
    m_iClientSocketFd=i_iClientSocketFd;


    T_RtmpCb tRtmpCb;
    memset(&tRtmpCb,0,sizeof(T_RtmpCb));
    tRtmpCb.StartPlay = RtmpServerIO::StartPlay;
    tRtmpCb.StopPlay = RtmpServerIO::StopPlay;
    tRtmpCb.SendData = RtmpServerIO::SendData;
    tRtmpCb.StartPushStream = RtmpServerIO::StartHandlePushStream;
    tRtmpCb.StopPushStream = RtmpServerIO::StopHandlePushStream;
    tRtmpCb.PushVideoData = RtmpServerIO::HandlePushVideoData;
    tRtmpCb.PushAudioData = RtmpServerIO::HandlePushAudioData;
    tRtmpCb.PushScriptData = RtmpServerIO::HandlePushScriptData;
    tRtmpCb.tRtmpPackCb.GetRandom = RtmpServerIO::GetRandom;
    m_pRtmpServer = new RtmpServer(this,&tRtmpCb,NULL);

    
    m_iRtmpServerIOFlag = 0;
    m_pRtmpServerIOProc = new thread(&RtmpServerIO::Proc, this);
    //m_pRtmpServerIOProc->detach();//注意线程回收
    m_iMediaProcFlag = 0;
    m_pMediaProc = NULL;
    m_pMediaHandle = new MediaHandle();
    m_pMediaFile=NULL;
    m_pbFileBuf = new unsigned char [RTMPS_FILE_BUF_MAX_LEN];
    if(NULL == m_pbFileBuf)
    {
        RTMPS_LOGE("NULL == m_pbFileBuf err\r\n");
    } 
}

/*****************************************************************************
-Fuction		: ~RtmpServerIO
-Description	: ~RtmpServerIO
-Input			: 
-Output 		: 
-Return 		: 
* Modify Date	  Version		 Author 		  Modification
* -----------------------------------------------
* 2017/10/10	  V1.0.0		 Yu Weifeng 	  Created
******************************************************************************/
RtmpServerIO :: ~RtmpServerIO()
{
    if(NULL!= m_pHandlePushMediaHandle)
    {
        delete m_pHandlePushMediaHandle;
        m_pHandlePushMediaHandle = NULL;
    }
    if(NULL!= m_pMediaHandle)
    {
        delete m_pMediaHandle;
        m_pMediaHandle = NULL;
    }
    if(NULL!= m_pRtmpServer)
    {
        delete m_pRtmpServer;
        m_pRtmpServer = NULL;
    }
    
    if(NULL!= m_pRtmpServerIOProc)
    {
        RTMPS_LOGW("~m_pRtmpServerIOProc start exit\r\n");
        m_iRtmpServerIOFlag = 0;
        //while(0 == m_iExitProcFlag){usleep(10);};
        m_pRtmpServerIOProc->join();//
        delete m_pRtmpServerIOProc;
        m_pRtmpServerIOProc = NULL;
    }
    if(NULL!= m_pMediaProc)
    {
        RTMPS_LOGW("~m_pMediaProc start exit\r\n");
        m_iMediaProcFlag = 0;
        //while(0 == m_iExitProcFlag){usleep(10);};
        m_pMediaProc->join();//
        delete m_pMediaProc;
        m_pMediaProc = NULL;
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
    RTMPS_LOGW("~~RtmpServerIO exit\r\n");
}

/*****************************************************************************
-Fuction		: Proc
-Description	: 阻塞
-Input			: 
-Output 		: 
-Return 		: 
* Modify Date	  Version		 Author 		  Modification
* -----------------------------------------------
* 2017/10/10	  V1.0.0		 Yu Weifeng 	  Created
******************************************************************************/
int RtmpServerIO :: Proc()
{
    int iRet=-1;
    char *pcRecvBuf=NULL;
    int iRecvLen=-1;
    
    if(m_iClientSocketFd < 0)
    {
        RTMPS_LOGE("RtmpServerIO m_iClientSocketFd < 0 err\r\n");
        return -1;
    }
    pcRecvBuf = new char[RTMPS_RECV_MAX_LEN];
    if(NULL == pcRecvBuf)
    {
        RTMPS_LOGE("RtmpServerIO NULL == pcRecvBuf err\r\n");
        return -1;
    }

    m_iRtmpServerIOFlag = 1;
    RTMPS_LOGW("RtmpServerIO start Proc\r\n");
    while(m_iRtmpServerIOFlag)
    {
        iRecvLen = 0;
        memset(pcRecvBuf,0,RTMPS_RECV_MAX_LEN);
        milliseconds timeMS(30);// 表示30毫秒
        iRet=TcpServer::Recv(pcRecvBuf,&iRecvLen,RTMPS_RECV_MAX_LEN,m_iClientSocketFd,&timeMS);
        if(iRet < 0)
        {
            RTMPS_LOGE("TcpServer::Recv err exit %d\r\n",iRecvLen);
            break;
        }
        if(iRecvLen<=0)
        {
            continue;
        }
        iRet=m_pRtmpServer->HandleRecvData(pcRecvBuf,iRecvLen);
        if(iRet < 0)
        {
            RTMPS_LOGE("m_pHlsServer->DoCycle err exit %d\r\n",iRecvLen);
            break;
        }
    }
    
    if(m_iClientSocketFd>=0)
    {
        TcpServer::Close(m_iClientSocketFd);//主动退出,
        RTMPS_LOGW("HlsServerIO::Close m_iClientSocketFd Exit%d\r\n",m_iClientSocketFd);
        m_iClientSocketFd = -1;
    }
    if(NULL != pcRecvBuf)
    {
        delete[] pcRecvBuf;
    }
    
    StopAllProc();
    return 0;
}

/*****************************************************************************
-Fuction		: GetProcFlag
-Description	: RtmpServerIO
-Input			: 
-Output 		: 
-Return 		: 
* Modify Date	  Version		 Author 		  Modification
* -----------------------------------------------
* 2017/10/10	  V1.0.0		 Yu Weifeng 	  Created
******************************************************************************/
int RtmpServerIO :: SendDatas(char * i_acSendBuf,int i_iSendLen)
{
    if(m_iClientSocketFd < 0)
    {
        RTMPS_LOGE("RtmpServerIO SendDatas m_iClientSocketFd < 0 err\r\n");
        return -1;
    }
    return TcpServer::Send(i_acSendBuf,i_iSendLen,m_iClientSocketFd);
}

/*****************************************************************************
-Fuction		: GetProcFlag
-Description	: RtmpServerIO
-Input			: 
-Output 		: 
-Return 		: 
* Modify Date	  Version		 Author 		  Modification
* -----------------------------------------------
* 2017/10/10	  V1.0.0		 Yu Weifeng 	  Created
******************************************************************************/
int RtmpServerIO :: GetProcFlag()
{
    if(0 == m_iMediaProcFlag && 0 == m_iRtmpServerIOFlag)
    {
        return 0;//多线程竞争注意优化
    }
    else
    {
        return -1;//多线程竞争注意优化
    }
}

/*****************************************************************************
-Fuction		: GetProcFlag
-Description	: RtmpServerIO
-Input			: 
-Output 		: 
-Return 		: 
* Modify Date	  Version		 Author 		  Modification
* -----------------------------------------------
* 2017/10/10	  V1.0.0		 Yu Weifeng 	  Created
******************************************************************************/
int RtmpServerIO :: StopAllProc()
{
    m_iRtmpServerIOFlag = 0;
    m_iMediaProcFlag = 0;
    if(NULL!= m_pMediaFile)
    {
        fclose(m_pMediaFile);
        m_pMediaFile = NULL;
    }
    return 0;
}


/*****************************************************************************
-Fuction		: HandlePlayURL
-Description	: RtmpServerIO
-Input			: 
-Output 		: 
-Return 		: 
* Modify Date	  Version		 Author 		  Modification
* -----------------------------------------------
* 2017/10/10	  V1.0.0		 Yu Weifeng 	  Created
******************************************************************************/
int RtmpServerIO :: HandlePlayURL(const char * url)
{
    int iRet = -1;
    string strURL(url);
    auto dwPos = strURL.rfind("/");
    if(string::npos != dwPos)//"D:\\test\\2023AAC.flv"
    {
        if(NULL != m_pFileName)
            delete m_pFileName;
        m_pFileName = new string(strURL.substr(dwPos+strlen("/")).c_str());
        if(string::npos == strURL.rfind(".flv"))//已经带了文件后缀的则不追加
        {
            m_pFileName->append(".flv");//固定.flv文件，ffmpeg url会过滤掉.flv.后面数据所以需要手动加
        }
        iRet = m_pMediaHandle->Init((char *)m_pFileName->c_str());//默认取文件流
        RTMPS_LOGW("m_pMediaHandle->Init %d,%s \r\n",iRet,m_pFileName->c_str());
        m_pRtmpServer->SendHandlePlayCmdResult(iRet,(char *)m_pFileName->c_str());

        m_pMediaProc = new thread(&RtmpServerIO::MediaProc, this);
        //m_pMediaProc->detach();//注意线程回收
        return iRet;
    }
    if(NULL == m_pFileName)
    {
        RTMPS_LOGE("m_pFileName %s err\r\n",url);
        m_pRtmpServer->SendHandlePlayCmdResult(iRet,(char *)url);
        return iRet;
    }
    m_pRtmpServer->SendHandlePlayCmdResult(iRet,(char *)m_pFileName->c_str());
    return -1;
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
int RtmpServerIO :: HandlePushURL(const char * url)
{
    int iRet = -1;
    char strFileName[128]={0,};
    string strURL(url);
    auto dwPos = strURL.rfind("/");
    if(string::npos != dwPos)//"D:\\test\\2023AAC.flv"
    {
        if(NULL != m_pPushFileName)
            delete m_pPushFileName;
        m_pPushFileName = new string(strURL.substr(dwPos+strlen("/")).c_str());
        
        snprintf(strFileName,sizeof(strFileName),"%s.mp4",m_pPushFileName->c_str());//固定.h264文件
        m_pMediaFile = fopen(strFileName,"wb");//
        if(NULL == m_pMediaFile)
        {
            RTMPS_LOGE("fopen %s err\r\n",strFileName);
            m_pRtmpServer->SendHandlePublishCmdResult(iRet,m_pPushFileName->c_str());
            return iRet;
        } 
        m_pHandlePushMediaHandle = new MediaHandle();
        m_pRtmpServer->SendHandlePublishCmdResult(0,NULL);
        return 0;
    }
    if(NULL == m_pPushFileName)
    {
        RTMPS_LOGE("m_pPushFileName %s err\r\n",url);
        m_pRtmpServer->SendHandlePublishCmdResult(iRet,(char *)url);
        return iRet;
    }
    m_pRtmpServer->SendHandlePublishCmdResult(iRet,(char *)m_pFileName->c_str());
    return iRet;
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
int RtmpServerIO :: HandlePushMediaData(T_RtmpMediaInfo *i_ptRtmpMediaInfo,char * i_acDataBuf,int i_iDataLen)
{
    int iRet = -1;
    int iWriteLen = -1;
    int iHeaderLen = -1;
    T_MediaFrameInfo tFileFrameInfo;
    E_MediaEncodeType eEncType=MEDIA_ENCODE_TYPE_UNKNOW;
    E_MediaFrameType eFrameType=MEDIA_FRAME_TYPE_UNKNOW;

    if(NULL == m_pPushFileName)//"D:\\test\\2023AAC.flv"
    {
        RTMPS_LOGE("HandlePushMediaData err %s\r\n",m_pPushFileName->c_str());
        return iRet;
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
        RTMPS_LOGE("NULL == m_pHandlePushMediaHandle err iWriteLen %d\r\n",iWriteLen);
        return iRet;
    }
    iRet=m_pHandlePushMediaHandle->GetFrame(&tFileFrameInfo);//annex-b的必须先解析成avcc的格式，所以要先调用这个接口进行分析
    if(iRet < 0)
    {
        RTMPS_LOGE("GetFrame err %d\r\n",iWriteLen);
        return iRet;
    }
    iWriteLen = m_pMediaHandle->FrameToContainer(&tFileFrameInfo,STREAM_TYPE_FMP4_STREAM,m_pbFileBuf,RTMPS_FILE_BUF_MAX_LEN,&iHeaderLen);
    if(iWriteLen < 0)
    {
        RTMPS_LOGE("FrameToContainer err iWriteLen %d\r\n",iWriteLen);
        return iRet;
    }
    if(iWriteLen == 0)
    {
        return iWriteLen;
    }
    iRet = fwrite(m_pbFileBuf, 1,iWriteLen, m_pMediaFile);
    if(iRet != iWriteLen)
    {
        printf("fwrite err %d iWriteLen%d\r\n",iRet,iWriteLen);
        return iWriteLen;
    }

    return iWriteLen;
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
int RtmpServerIO::MediaProc()
{
    int iRet = -1;
    T_MediaFrameInfo tFileFrameInfo;
	unsigned int dwFileLastTimeStamp=0;
	int iSleepTimeMS=0;

    m_iMediaProcFlag = 1;
    RTMPS_LOGW("RtmpServerIO start MediaProc %s\r\n",m_pFileName->c_str());
    
    memset(&tFileFrameInfo,0,sizeof(T_MediaFrameInfo));
    tFileFrameInfo.pbFrameBuf = new unsigned char [RTMPS_FRAME_BUF_MAX_LEN];
    tFileFrameInfo.iFrameBufMaxLen = RTMPS_FRAME_BUF_MAX_LEN;
    while(m_iMediaProcFlag)
    {
        iRet=m_pMediaHandle->GetFrame(&tFileFrameInfo);//非文件流可直接调用此接口
        if(iRet<0)
        {
            RTMPS_LOGE("TestProc exit %d[%s]\r\n",iRet,m_pFileName->c_str());
            break;
        }
        if(tFileFrameInfo.dwTimeStamp<dwFileLastTimeStamp)
        {
            RTMPS_LOGE("dwTimeStamp err exit %d,%d\r\n",tFileFrameInfo.dwTimeStamp,dwFileLastTimeStamp);
            break;
        }
        iSleepTimeMS=(int)(tFileFrameInfo.dwTimeStamp-dwFileLastTimeStamp);
        if(iSleepTimeMS > 0)
        {
            SleepMs(iSleepTimeMS);//模拟实时流(直播)，点播和当前的处理机制不匹配，需要后续再开发
        }
        
        iRet=this->Playing(&tFileFrameInfo);
        if(iRet<0)
        {
            RTMPS_LOGE("Playing err exit %d[%s]\r\n",iRet,m_pFileName->c_str());
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
int RtmpServerIO::Playing(T_MediaFrameInfo * i_pFrame)
{
    E_RTMP_ENC_TYPE eRtmpEncType = RTMP_UNKNOW_ENC_TYPE;
    T_RtmpMediaInfo tRtmpMediaInfo;
    E_RTMP_FRAME_TYPE eFrameType = RTMP_UNKNOW_FRAME;
    
    if(NULL == i_pFrame)
    {
        RTMP_LOGE("Playing NULL \r\n");
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
    return m_pRtmpServer->PushStream(&tRtmpMediaInfo,i_pFrame->pbFrameStartPos,i_pFrame->iFrameLen);
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
int RtmpServerIO::StartPlay(char *i_strPalySrc,void *i_pIoHandle)
{
    if(NULL == i_strPalySrc ||NULL == i_pIoHandle)
    {
        RTMP_LOGE("StartPlay NULL \r\n");
        return -1;
    }
    //校验url，打开媒体流
    RtmpServerIO *pRtmpServerIO = (RtmpServerIO *)i_pIoHandle;
    
    return pRtmpServerIO->HandlePlayURL(i_strPalySrc);
}

/*****************************************************************************
-Fuction        : StopPlay
-Description    : 该接口暂不使用，由socket断开触发停止
-Input          : 
-Output         : 
-Return         : 
* Modify Date     Version        Author           Modification
* -----------------------------------------------
* 2023/09/21      V1.0.0         Yu Weifeng       Created
******************************************************************************/
int RtmpServerIO::StopPlay(char *i_strPalySrc)//,void *i_pIoHandle
{
    //if(NULL == i_strPalySrc ||NULL == i_pIoHandle)
    {
        RTMP_LOGE("StopPlay NULL \r\n");
        //return -1;
    }
    //RtmpServerIO *pRtmpServerIO = (RtmpServerIO *)i_pIoHandle;
    //return pRtmpServerIO->StopAllProc();
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
int RtmpServerIO::SendData(void *i_pIoHandle,char * i_acSendBuf,int i_iSendLen)
{
    int iRet = -1;
    RtmpServerIO *pMediaSessions = NULL;
    
    if(NULL == i_pIoHandle ||NULL == i_acSendBuf ||i_iSendLen <= 0)
    {
        RTMP_LOGE("SendData NULL %d \r\n",i_iSendLen);
        return iRet;
    }
    pMediaSessions = (RtmpServerIO *)i_pIoHandle;
    return pMediaSessions->SendDatas(i_acSendBuf,i_iSendLen);

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
int RtmpServerIO::StartHandlePushStream(char *i_strStreamSrc,void *i_pIoHandle)
{
    if(NULL == i_strStreamSrc ||NULL == i_pIoHandle)
    {
        RTMP_LOGE("StartPlay NULL \r\n");
        return -1;
    }
    //校验url，打开媒体流
    RtmpServerIO *pRtmpServerIO = (RtmpServerIO *)i_pIoHandle;
    
    return pRtmpServerIO->HandlePushURL(i_strStreamSrc);
}
/*****************************************************************************
-Fuction        : StopHandlePushStream
-Description    : 
-Input          : 
-Output         : 
-Return         : 
* Modify Date     Version        Author           Modification
* -----------------------------------------------
* 2023/09/21      V1.0.0         Yu Weifeng       Created
******************************************************************************/
int RtmpServerIO::StopHandlePushStream(char *i_strStreamSrc,void *i_pIoHandle)
{
    if(NULL == i_strStreamSrc ||NULL == i_pIoHandle)
    {
        RTMP_LOGE("StopHandlePushStream NULL \r\n");
        return -1;
    }
    RtmpServerIO *pRtmpServerIO = (RtmpServerIO *)i_pIoHandle;
    return pRtmpServerIO->StopAllProc();
}
/*****************************************************************************
-Fuction        : PushVideoData
-Description    : 
-Input          : 
-Output         : 
-Return         : 
* Modify Date     Version        Author           Modification
* -----------------------------------------------
* 2023/09/21      V1.0.0         Yu Weifeng       Created
******************************************************************************/
int RtmpServerIO::HandlePushVideoData(T_RtmpMediaInfo *i_ptRtmpMediaInfo,char * i_acDataBuf,int i_iDataLen,void *i_pIoHandle)
{
    int iRet = -1;
    
    if(NULL == i_ptRtmpMediaInfo ||NULL == i_acDataBuf ||NULL == i_pIoHandle)
    {
        RTMP_LOGE("HandlePushVideoData NULL \r\n");
        return iRet;
    }
    RtmpServerIO *pRtmpServerIO = (RtmpServerIO *)i_pIoHandle;
    return pRtmpServerIO->HandlePushMediaData(i_ptRtmpMediaInfo,i_acDataBuf,i_iDataLen);
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
int RtmpServerIO::HandlePushAudioData(T_RtmpMediaInfo *i_ptRtmpMediaInfo,char * i_acDataBuf,int i_iDataLen,void *i_pIoHandle)
{
    int iRet = -1;
    
    if(NULL == i_ptRtmpMediaInfo ||NULL == i_acDataBuf ||NULL == i_pIoHandle)
    {
        RTMP_LOGE("HandlePushAudioData NULL \r\n");
        return iRet;
    }
    RtmpServerIO *pRtmpServerIO = (RtmpServerIO *)i_pIoHandle;
    return pRtmpServerIO->HandlePushMediaData(i_ptRtmpMediaInfo,i_acDataBuf,i_iDataLen);
}

/*****************************************************************************
-Fuction        : HandlePushScriptData
-Description    : 
-Input          : 
-Output         : 
-Return         : 
* Modify Date     Version        Author           Modification
* -----------------------------------------------
* 2023/09/21      V1.0.0         Yu Weifeng       Created
******************************************************************************/
int RtmpServerIO::HandlePushScriptData(char *i_strStreamName,unsigned int i_dwTimestamp,char * i_acDataBuf,int i_iDataLen)
{
    int iRet = -1;

    RTMP_LOGD("PushScriptData %s \r\n",i_strStreamName);

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
long RtmpServerIO::GetRandom()
{
    return rand();
}

