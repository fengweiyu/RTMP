/*****************************************************************************
* Copyright (C) 2023-2028 Hanson Yu  All rights reserved.
------------------------------------------------------------------------------
* File Module		: 	RtmpClient.h
* Description		: 	RtmpClient operation center
* Created			: 	2023.09.21.
* Author			: 	Yu Weifeng
* Function List		: 	
* Last Modified 	: 	
* History			: 	
******************************************************************************/
#ifndef RTMP_CLIENT_H
#define RTMP_CLIENT_H 

#include "RtmpSession.h"
;

/*****************************************************************************
-Class			: RtmpClient
-Description	: 
* Modify Date	  Version		 Author 		  Modification
* -----------------------------------------------
* 2023/09/21	  V1.0.0		 Yu Weifeng 	  Created
******************************************************************************/
class RtmpClient  
{
public:
    RtmpClient();
    RtmpClient(int i_iPlayOrPublish,char * i_strURL,T_RtmpCb *i_ptRtmpCb);
    virtual ~RtmpClient();
    
	
    int Start(int i_iPlayOrPublish,char * i_strURL,T_RtmpCb *i_ptRtmpCb);
    int Stop(int err);
    int Pushing(T_RtmpMediaInfo *i_ptRtmpMediaInfo,unsigned char * i_pbFrameData,int i_iFrameLen,char * i_strPlayPath = NULL);
    
    int DoConnect();
    int DoCycle(char *i_pcData,int i_iDataLen);
    
private:
    
	RtmpSession *   m_pRtmpSession;
    int m_iStarter;
    int m_iSendResed;
    int m_iSendResTime;
    int m_iConnected;
    int m_iConnectRes;
    int m_iConnectTime;
    int m_iConnectCnt;
};














#endif
