/*****************************************************************************
* Copyright (C) 2020-2025 Hanson Yu  All rights reserved.
------------------------------------------------------------------------------
* File Module           :       RtmpClientIO.c
* Description           : 	
* Created               :       2023.01.13.
* Author                :       Yu Weifeng
* Function List         : 	
* Last Modified         : 	
* History               : 	
******************************************************************************/
#include "RtmpClientIO.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
//#include <unistd.h>
#include <thread>

using std::thread;

#define RTMPC_RECV_MAX_LEN (10240)


/*****************************************************************************
-Fuction		: RtmpClientIO
-Description	: 
rtmp://10.10.22.121:9213/push/2024h264aac.flv
rtmp://10.10.22.121:9213/pull/2024h264aac.flv
-Input			: 
-Output 		: 
-Return 		: 
* Modify Date	  Version		 Author 		  Modification
* -----------------------------------------------
* 2017/10/10	  V1.0.0		 Yu Weifeng 	  Created
******************************************************************************/
RtmpClientIO :: RtmpClientIO()
{
    m_iClientSocketFd = 0;
    m_iRtmpClientIOFlag = 0;
    m_pRtmpClientIOProc = NULL;
}

/*****************************************************************************
-Fuction		: RtmpClientIO
-Description	: 
rtmp://10.10.22.121:9213/push/2024h264aac.flv
rtmp://10.10.22.121:9213/pull/2024h264aac.flv
-Input			: 
-Output 		: 
-Return 		: 
* Modify Date	  Version		 Author 		  Modification
* -----------------------------------------------
* 2017/10/10	  V1.0.0		 Yu Weifeng 	  Created
******************************************************************************/
RtmpClientIO :: RtmpClientIO(char *i_strURL,T_RtmpClientCb i_tRtmpClientCb)
{
    T_RtmpCb tRtmpCb;
    int iPlayOrPublish=0;//0 play,1 publish
    string strURL(i_strURL);
    auto dwPush = strURL.find("push");
    if(string::npos != dwPush)//rtmp://10.10.10.10:10/push_10/Mnx8Y2UEdXTQ%3D%3D.9eaa64fa64a282
    {
        iPlayOrPublish=1;
    }

    memset(&tRtmpCb,0,sizeof(T_RtmpCb));
    tRtmpCb.PlayVideoData = i_tRtmpClientCb.PlayVideoData;
    tRtmpCb.PlayAudioData = i_tRtmpClientCb.PlayAudioData;
    tRtmpCb.PlayScriptData = i_tRtmpClientCb.PlayScriptData;
    tRtmpCb.SendData = RtmpClientIO::SendData;
    tRtmpCb.Connect = RtmpClientIO::ConnectServer;
    tRtmpCb.tRtmpPackCb.GetRandom = RtmpClientIO::GetRandom;

    m_RtmpClient.Start(iPlayOrPublish,i_strURL,&tRtmpCb);
    






    m_iClientSocketFd = 0;
    m_iRtmpClientIOFlag = 0;
    m_pRtmpClientIOProc = new thread(&RtmpClientIO::Proc, this);
    //m_pRtmpClientIOProc->detach();//注意线程回收
}

/*****************************************************************************
-Fuction		: ~~RtmpClientIO
-Description	: ~~RtmpClientIO
-Input			: 
-Output 		: 
-Return 		: 
* Modify Date	  Version		 Author 		  Modification
* -----------------------------------------------
* 2017/10/10	  V1.0.0		 Yu Weifeng 	  Created
******************************************************************************/
RtmpClientIO :: ~RtmpClientIO()
{
    if(NULL!= m_pRtmpClientIOProc)
    {
        RTMPC_LOGW("RtmpClientIO start exit\r\n");
        m_iRtmpClientIOFlag = 0;
        //while(0 == m_iExitProcFlag){usleep(10);};
        m_pRtmpClientIOProc->join();//
        delete m_pRtmpClientIOProc;
        m_pRtmpClientIOProc = NULL;
    }
    RTMPC_LOGW("~~RtmpClientIO exit\r\n");
}

/*****************************************************************************
-Fuction		: RtmpClientIO
-Description	: 
rtmp://10.10.22.121:9213/push/2024h264aac.flv
rtmp://10.10.22.121:9213/pull/2024h264aac.flv
-Input			: 
-Output 		: 
-Return 		: 
* Modify Date	  Version		 Author 		  Modification
* -----------------------------------------------
* 2017/10/10	  V1.0.0		 Yu Weifeng 	  Created
******************************************************************************/
int RtmpClientIO :: Start(char *i_strURL,T_RtmpClientCb i_tRtmpClientCb)
{
    if(m_pRtmpClientIOProc != NULL)
        return 0;


    T_RtmpCb tRtmpCb;
    int iPlayOrPublish=0;//0 play,1 publish
    string strURL(i_strURL);
    auto dwPush = strURL.find("push");
    if(string::npos != dwPush)//rtmp://10.10.10.10:10/push_10/Mnx8Y2UEdXTQ%3D%3D.9eaa64fa64a282
    {
        iPlayOrPublish=1;
    }

    memset(&tRtmpCb,0,sizeof(T_RtmpCb));
    tRtmpCb.PlayVideoData = i_tRtmpClientCb.PlayVideoData;
    tRtmpCb.PlayAudioData = i_tRtmpClientCb.PlayAudioData;
    tRtmpCb.PlayScriptData = i_tRtmpClientCb.PlayScriptData;
    tRtmpCb.SendData = RtmpClientIO::SendData;
    tRtmpCb.Connect = RtmpClientIO::ConnectServer;
    tRtmpCb.tRtmpPackCb.GetRandom = RtmpClientIO::GetRandom;

    m_RtmpClient.Start(iPlayOrPublish,i_strURL,&tRtmpCb);
    


    m_iClientSocketFd = 0;
    m_iRtmpClientIOFlag = 0;
    m_pRtmpClientIOProc = new thread(&RtmpClientIO::Proc, this);
    //m_pRtmpClientIOProc->detach();//注意线程回收
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
int RtmpClientIO::Stop(int err)
{
    m_RtmpClient.Stop(err);//主动断开

    return 0;
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
int RtmpClientIO :: Proc()
{
    int iRet=-1;
    char *pcRecvBuf=NULL;
    int iRecvLen=-1;
    int iConnectCnt = 0;
    
    pcRecvBuf = new char[RTMPC_RECV_MAX_LEN];
    if(NULL == pcRecvBuf)
    {
        RTMPC_LOGE("RtmpClientIO NULL == pcRecvBuf err\r\n");
        return -1;
    }

    m_iRtmpClientIOFlag = 1;
    RTMPC_LOGW("RtmpClientIO start Proc\r\n");
    while(m_iRtmpClientIOFlag)
    {
        if(0 == m_iClientSocketFd)
        {
            iRet=m_RtmpClient.DoConnect();
            if(iRet<0)
            {
                iConnectCnt++;
                if(iConnectCnt>3)
                {
                    RTMPC_LOGE("DoConnect err%d\r\n",iConnectCnt);
                    break;
                }
                SleepMs(1000);
                continue;
            }
            m_iClientSocketFd=TcpClient::GetClientSocket();//后续TcpClient接口优化，不传socket则用其内部的
        }
        
        iRecvLen = 0;
        memset(pcRecvBuf,0,RTMPC_RECV_MAX_LEN);
        milliseconds timeMS(10);// 表示30毫秒
        iRet=TcpClient::Recv(pcRecvBuf,&iRecvLen,RTMPC_RECV_MAX_LEN,m_iClientSocketFd,&timeMS);
        if(iRet < 0)
        {
            RTMPC_LOGE("TcpClient::Recv err exit %d\r\n",iRecvLen);
            break;
        }
        if(iRecvLen<=0)
        {
            iRet=m_RtmpClient.DoCycle(NULL,0);
            if(iRet < 0)
            {
                RTMPC_LOGE("m_RtmpClient->DoCycle err exit %d\r\n",iRecvLen);
                Stop(0);
                break;
            }
            continue;
        }
        iRet=m_RtmpClient.DoCycle(pcRecvBuf,iRecvLen);
        if(iRet < 0)
        {
            RTMPC_LOGE("m_RtmpClient->DoCycle err exit %d\r\n",iRecvLen);
            Stop(0);
            break;
        }
    }
    
    if(m_iClientSocketFd>=0)
    {
        TcpClient::Close(m_iClientSocketFd);//主动退出,
        RTMPC_LOGW("m_RtmpClient::Close m_iClientSocketFd Exit%d\r\n",m_iClientSocketFd);
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
int RtmpClientIO :: StopAllProc()
{
    m_iRtmpClientIOFlag = 0;

    return 0;
}

/*****************************************************************************
-Fuction		: GetProcFlag
-Description	: RtmpClientIO
-Input			: 
-Output 		: 
-Return 		: 
* Modify Date	  Version		 Author 		  Modification
* -----------------------------------------------
* 2017/10/10	  V1.0.0		 Yu Weifeng 	  Created
******************************************************************************/
int RtmpClientIO :: GetProcFlag()
{
    if(0 != m_iRtmpClientIOFlag)
    {
        return 0;//多线程竞争注意优化
    }
    return -1;//多线程竞争注意优化
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
int RtmpClientIO::Pushing(T_RtmpMediaInfo *i_ptRtmpMediaInfo,unsigned char * i_pbFrameData,int i_iFrameLen,char * i_strPlayPath)
{
    return m_RtmpClient.Pushing(i_ptRtmpMediaInfo,i_pbFrameData,i_iFrameLen,i_strPlayPath);
}

/*****************************************************************************
-Fuction        : ConnectServer
-Description    : 
-Input          : 
-Output         : 
-Return         : 
* Modify Date     Version        Author           Modification
* -----------------------------------------------
* 2023/09/21      V1.0.0         Yu Weifeng       Created
******************************************************************************/
int RtmpClientIO::ConnectServer(void *i_pIoHandle,char * i_strIP,unsigned short i_wPort)
{
    int iRet = -1;
    RtmpClientIO *pRtmpClientIO = NULL;
    
    if(NULL == i_pIoHandle ||NULL == i_strIP ||i_wPort <= 0)
    {
        RTMP_LOGE("ConnectServer NULL %d \r\n",i_wPort);
        return iRet;
    }

    pRtmpClientIO = (RtmpClientIO *)i_pIoHandle;
    
    return pRtmpClientIO->Init(i_strIP,i_wPort);
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
int RtmpClientIO::SendData(void *i_pIoHandle,char * i_acSendBuf,int i_iSendLen)
{
    int iRet = -1;
    RtmpClientIO *pRtmpClientIO = NULL;
    
    if(NULL == i_pIoHandle ||NULL == i_acSendBuf ||i_iSendLen <= 0)
    {
        RTMP_LOGE("SendData NULL %d \r\n",i_iSendLen);
        return iRet;
    }
    pRtmpClientIO = (RtmpClientIO *)i_pIoHandle;
    pRtmpClientIO->Send(i_acSendBuf,i_iSendLen);

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
long RtmpClientIO::GetRandom()
{
    return rand();
}

