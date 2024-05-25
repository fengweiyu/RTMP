/*****************************************************************************
* Copyright (C) 2023-2028 Hanson Yu  All rights reserved.
------------------------------------------------------------------------------
* File Module		: 	RtmpServer.h
* Description		: 	RtmpServer operation center
                        各个客户端的协议请求命令处理
* Created			: 	2023.09.21.
* Author			: 	Yu Weifeng
* Function List		: 	
* Last Modified 	: 	
* History			: 	
******************************************************************************/
#ifndef RTMP_SERVER_H
#define RTMP_SERVER_H

#include <list>
#include "RtmpSession.h"


using std::list;




/*****************************************************************************
-Class			: RtmpServer
-Description	: 
* Modify Date	  Version		 Author 		  Modification
* -----------------------------------------------
* 2023/09/21	  V1.0.0		 Yu Weifeng 	  Created
******************************************************************************/
class RtmpServer
{
public:
    RtmpServer(void *i_pIoHandle,T_RtmpCb *i_ptRtmpCb,T_RtmpSessionConfig *i_ptRtmpSessionConfig = NULL);
    virtual ~RtmpServer();

    int PushStream(T_RtmpMediaInfo *i_ptRtmpMediaInfo,unsigned char * i_pbFrameData,int i_iFrameLen,char * i_strPlayPath = NULL);//DoPlay
    int HandleRecvData(char *i_pcData,int i_iDataLen);//DoCycle
    
    int SendHandlePlayCmdResult(int i_iResult,char *i_strDescription);//HandCmdPlaySendResult
    int SendHandlePublishCmdResult(int i_iResult,const char *i_strDescription);//HandCmdPublishSendResult

private:
	RtmpSession *   m_pRtmpSession;
};









#if 0
typedef struct RtmpServerConfig
{
    unsigned short wPort;
    T_RtmpSessionConfig tRtmpSessionConfig;
    T_RtmpCb tRtmpCb;
}T_RtmpServerConfig;

/*****************************************************************************
-Class			: RtmpServer
-Description	: 
* Modify Date	  Version		 Author 		  Modification
* -----------------------------------------------
* 2023/09/21	  V1.0.0		 Yu Weifeng 	  Created
******************************************************************************/
class RtmpServer
{
public:
    RtmpServer(T_RtmpServerConfig *i_ptRtmpServerConfig);
    virtual ~RtmpServer();

    int Start();
    int Cycle();
    int Playing(char *i_strPlayPath,E_RTMP_ENC_TYPE i_eEncType,uint64 i_ddwTimestamp,unsigned char * i_pbFrameData,int i_iFrameLen);

private:
    TcpServerEpoll * m_pTcpSocket;
	list<RtmpSession *>   m_RtmpSessionList;

    T_RtmpServerConfig m_tRtmpServerConfig;
};


#endif




















#endif
