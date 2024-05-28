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
RtmpClient::RtmpClient(int i_iPlayOrPublish,char * i_strURL,T_RtmpCb *i_ptRtmpCb)
{
    T_RtmpSessionConfig tRtmpSessionConfig;
    

    memset(&tRtmpSessionConfig,0,sizeof(T_RtmpSessionConfig));
    tRtmpSessionConfig.dwWindowSize= 5000000;//默认值
    tRtmpSessionConfig.dwPeerBandwidth = 5000000;
    tRtmpSessionConfig.dwOutChunkSize = RTMP_OUTPUT_CHUNK_SIZE;
    tRtmpSessionConfig.dwInChunkSize = RTMP_INPUT_CHUNK_MAX_SIZE;
    tRtmpSessionConfig.iPlayOrPublish = i_iPlayOrPublish;
    snprintf(tRtmpSessionConfig.strURL,sizeof(tRtmpSessionConfig.strURL),"%s",i_strURL);
    
    m_pRtmpSession = new RtmpSession(this,i_ptRtmpCb,&tRtmpSessionConfig);

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
int RtmpClient::Start(int i_iPlayOrPublish,char * i_strURL,T_RtmpCb *i_ptRtmpCb)
{
    if(NULL != i_strURL && NULL == m_pRtmpSession)
    {
        T_RtmpSessionConfig tRtmpSessionConfig;
        
        memset(&tRtmpSessionConfig,0,sizeof(T_RtmpSessionConfig));
        tRtmpSessionConfig.dwWindowSize= 5000000;//默认值
        tRtmpSessionConfig.dwPeerBandwidth = 5000000;
        tRtmpSessionConfig.dwOutChunkSize = RTMP_OUTPUT_CHUNK_SIZE;
        tRtmpSessionConfig.dwInChunkSize = RTMP_INPUT_CHUNK_MAX_SIZE;
        tRtmpSessionConfig.iPlayOrPublish = i_iPlayOrPublish;
        snprintf(tRtmpSessionConfig.strURL,sizeof(tRtmpSessionConfig.strURL),"%s",i_strURL);
        
        m_pRtmpSession = new RtmpSession(this,i_ptRtmpCb,&tRtmpSessionConfig);
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
    m_pRtmpSession->DoStop();//主动断开

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
int RtmpClient::Pushing(T_RtmpMediaInfo *i_ptRtmpMediaInfo,unsigned char * i_pbFrameData,int i_iFrameLen,char * i_strPlayPath)
{
    return m_pRtmpSession->DoPush(i_ptRtmpMediaInfo,i_pbFrameData,i_iFrameLen,i_strPlayPath);
}

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
int RtmpClient::DoConnect()
{
    return m_pRtmpSession->DoConnect();
}

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
int RtmpClient::DoCycle(char *i_pcData,int i_iDataLen)
{
    return m_pRtmpSession->DoCycle(i_pcData,i_iDataLen);
}

