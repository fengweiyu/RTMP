/*****************************************************************************
* Copyright (C) 2023-2028 Hanson Yu  All rights reserved.
------------------------------------------------------------------------------
* File Module		: 	RtmpServer.h
* Description		: 	RtmpServer operation center
                        ���տͻ������󣬲�new RtmpSession
                        ͬʱѭ������RtmpSessionList�е�cycle����
                        ����io�Ĵ���ʽ��һ�������ļ���ʱ�ò���,
                        �����Ż�
* Created			: 	2023.09.21.
* Author			: 	Yu Weifeng
* Function List		: 	
* Last Modified 	: 	
* History			: 	
******************************************************************************/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
        
#include "RtmpServer.h"



/*****************************************************************************
-Fuction        : RtmpServer
-Description    : 
-Input          : 
-Output         : 
-Return         : 
* Modify Date     Version        Author           Modification
* -----------------------------------------------
* 2023/09/21      V1.0.0         Yu Weifeng       Created
******************************************************************************/
RtmpServer::RtmpServer(void *i_pIoHandle,T_RtmpCb *i_ptRtmpCb,T_RtmpSessionConfig *i_ptRtmpSessionConfig)
{
    m_pRtmpSession = new RtmpSession(i_pIoHandle,i_ptRtmpCb);
}

/*****************************************************************************
-Fuction        : ~RtmpServer
-Description    : 
-Input          : 
-Output         : 
-Return         : 
* Modify Date     Version        Author           Modification
* -----------------------------------------------
* 2023/09/21      V1.0.0         Yu Weifeng       Created
******************************************************************************/
RtmpServer::~RtmpServer()
{
    if(NULL != m_pRtmpSession)
    {
        delete m_pRtmpSession;
        m_pRtmpSession = NULL;
    }
}

/*****************************************************************************
-Fuction		: PushStream
-Description	: ������
-Input			: 
-Output 		: 
-Return 		: 
* Modify Date	  Version		 Author 		  Modification
* -----------------------------------------------
* 2023/09/21	  V1.0.0		 Yu Weifeng 	  Created
******************************************************************************/
int RtmpServer::PushStream(T_RtmpMediaInfo *i_ptRtmpMediaInfo,unsigned char * i_pbFrameData,int i_iFrameLen,char * i_strPlayPath)
{
    return m_pRtmpSession->DoPlay(i_ptRtmpMediaInfo,i_pbFrameData,i_iFrameLen,i_strPlayPath);
}

/*****************************************************************************
-Fuction		: HandleRecvData
-Description	: ������
-Input			: 
-Output 		: 
-Return 		: 
* Modify Date	  Version		 Author 		  Modification
* -----------------------------------------------
* 2023/09/21	  V1.0.0		 Yu Weifeng 	  Created
******************************************************************************/
int RtmpServer::HandleRecvData(char *i_pcData,int i_iDataLen)
{
    return m_pRtmpSession->DoCycle(i_pcData,i_iDataLen);
}

/*****************************************************************************
-Fuction		: SendHandlePlayCmdResult
-Description	: ������
-Input			: 
-Output 		: 
-Return 		: 
* Modify Date	  Version		 Author 		  Modification
* -----------------------------------------------
* 2023/09/21	  V1.0.0		 Yu Weifeng 	  Created
******************************************************************************/
int RtmpServer::SendHandlePlayCmdResult(int i_iResult,const char *i_strDescription)
{
    return m_pRtmpSession->HandCmdPlaySendResult(i_iResult,(char *)i_strDescription);
}

/*****************************************************************************
-Fuction		: SendHandlePublishCmdResult
-Description	: ������
-Input			: 
-Output 		: 
-Return 		: 
* Modify Date	  Version		 Author 		  Modification
* -----------------------------------------------
* 2023/09/21	  V1.0.0		 Yu Weifeng 	  Created
******************************************************************************/
int RtmpServer::SendHandlePublishCmdResult(int i_iResult,const char *i_strDescription)
{
    return m_pRtmpSession->HandCmdPublishSendResult(i_iResult,i_strDescription);
}




#if 0
/*****************************************************************************
-Fuction        : RtmpServer
-Description    : 
-Input          : 
-Output         : 
-Return         : 
* Modify Date     Version        Author           Modification
* -----------------------------------------------
* 2023/09/21      V1.0.0         Yu Weifeng       Created
******************************************************************************/
RtmpServer::RtmpServer(T_RtmpServerConfig *i_ptRtmpServerConfig)
{
    m_RtmpSessionList.clear();
    
    memset(&m_tRtmpServerConfig,0,sizeof(T_RtmpServerConfig));
    m_tRtmpServerConfig.wPort = 15251;//Ĭ��ֵ
    if(NULL != i_ptRtmpServerConfig)
        memcpy(&m_tRtmpServerConfig,i_ptRtmpServerConfig,sizeof(T_RtmpServerConfig));
    else
        RTMP_LOGE("RtmpServer m_tRtmpServerConfig NULL\r\n");

    m_pTcpSocket = new TcpServerEpoll();
    if(NULL == m_pTcpSocket)
    {
        RTMP_LOGE("RtmpServer m_pTcpSocket err NULL\r\n");
    }
}

/*****************************************************************************
-Fuction        : ~RtmpServer
-Description    : 
-Input          : 
-Output         : 
-Return         : 
* Modify Date     Version        Author           Modification
* -----------------------------------------------
* 2023/09/21      V1.0.0         Yu Weifeng       Created
******************************************************************************/
RtmpServer::~RtmpServer()
{
    if(NULL != m_pTcpSocket)
    {
        delete m_pTcpSocket;
        m_pTcpSocket = NULL;
    }
}

/*****************************************************************************
-Fuction		: Start
-Description	: ������
-Input			: 
-Output 		: 
-Return 		: 
* Modify Date	  Version		 Author 		  Modification
* -----------------------------------------------
* 2023/09/21	  V1.0.0		 Yu Weifeng 	  Created
******************************************************************************/
int RtmpServer::Start()
{
    m_pTcpSocket->Init(m_tRtmpServerConfig.wPort);

    return 0;
}

/*****************************************************************************
-Fuction		: Cycle
-Description	: ������
-Input			: 
-Output 		: 
-Return 		: 
* Modify Date	  Version		 Author 		  Modification
* -----------------------------------------------
* 2023/09/21	  V1.0.0		 Yu Weifeng 	  Created
******************************************************************************/
int RtmpServer::Cycle()
{
    int iClientSocketFd =-1;
    RtmpSession *ptRtmpSession = NULL;
    T_RtmpCb tRtmpCb;
    list<RtmpSession *>::iterator Iter;

    
    iClientSocketFd =m_pTcpSocket->Accept();
    if(iClientSocketFd >= 0)
    {
        memset(&tRtmpCb,0,sizeof(T_RtmpCb));
        memcpy(&tRtmpCb,&m_tRtmpServerConfig.tRtmpCb,sizeof(T_RtmpCb));
        ptRtmpSession = new RtmpSession(iClientSocketFd,m_pTcpSocket,&tRtmpCb);//T_RtmpSessionConfig use default
        if(NULL == ptRtmpSession)
        {
            RTMP_LOGE("RtmpVersion new err NULL\r\n");
        }
        else
        {
            m_RtmpSessionList.push_back(ptRtmpSession);
        }
    }
    if(true == m_RtmpSessionList.empty())
    {
        return 0;
    }
    
    for(Iter=m_RtmpSessionList.begin();Iter!=m_RtmpSessionList.end();Iter++)
    {
        if((*Iter)->DoCycle() < 0)
        {
            delete (*Iter);
            m_RtmpSessionList.erase(Iter);
            RTMP_LOGE("m_RtmpSessionList DoCycle err \r\n");
            break;
        }
    }
    return 0;
}

/*****************************************************************************
-Fuction		: Play
-Description	: �������Ż�Ϊ
��RtmpSession��handle������Դ���а󶨣�Ȼ��ַ��������Ͳ��ô���i_strPlayPath��
���߿��Դ�RtmpSession��get ��strPlayPath,Ȼ������ȶԣ���Ҳ������������flv��
-Input			: 
-Output 		: 
-Return 		: 
* Modify Date	  Version		 Author 		  Modification
* -----------------------------------------------
* 2023/09/21	  V1.0.0		 Yu Weifeng 	  Created
******************************************************************************/
int RtmpServer::Playing(char *i_strPlayPath,E_RTMP_ENC_TYPE i_eEncType,uint64 i_ddwTimestamp,unsigned char * i_pbFrameData,int i_iFrameLen)
{
    list<RtmpSession *>::iterator Iter;

    for(Iter=m_RtmpSessionList.begin();Iter!=m_RtmpSessionList.end();Iter++)
    {
        if((*Iter)->DoPlay(NULL,i_pbFrameData,i_iFrameLen) == 0)
        {
            //break;//���ܴ�����ͬý������Դ�����ǻỰ��ͬ�����
        }
    }

    return 0;
}
#endif
