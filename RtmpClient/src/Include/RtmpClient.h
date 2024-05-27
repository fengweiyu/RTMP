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

#include "XStreamParser.h"
#include "RtmpSession.h"
#include "XNet.h"
#include "XNetClient.h"
#include "STDStream.h"
;

/*****************************************************************************
-Class			: RtmpClient
-Description	: 
* Modify Date	  Version		 Author 		  Modification
* -----------------------------------------------
* 2023/09/21	  V1.0.0		 Yu Weifeng 	  Created
******************************************************************************/
class RtmpClient  : public CXNetClient
{
public:
    RtmpClient();
    RtmpClient(int i_iPlayOrPublish,char * i_strURL);
    virtual ~RtmpClient();
    
	
    int Start(int i_iPlayOrPublish,char * i_strURL);
    int Stop(int err);
    int Pushing(FRAME_INFO * i_pFrame);
    
    static int ConnectServer(void *i_pIoHandle,char * i_strIP,unsigned short i_wPort);
    static int SendDatas(void *i_pIoHandle,char * i_acSendBuf,int i_iSendLen);
    static int PlayVideoData(T_RtmpMediaInfo *i_ptRtmpMediaInfo,char * i_acDataBuf,int i_iDataLen,void *i_pIoHandle);
    static int PlayAudioData(T_RtmpMediaInfo *i_ptRtmpMediaInfo,char * i_acDataBuf,int i_iDataLen,void *i_pIoHandle);
    static int PlayScriptData(char *i_strStreamName,unsigned int i_dwTimestamp,char * i_acDataBuf,int i_iDataLen);
    
    static long GetRandom();

    int ConnectToServer(char * i_strIP,unsigned short i_wPort);
    
protected:
    int OnMsg(PXMSG msg);
    int OnConnect(int nResult);
    int OnDisConnect(int nError);
    void OnRecvData(const char* pData, int nDataLen);
    
private:
    
    int SetTimer(int i_iTimeMs);
    static FRAME_INFO* RtmpFrameToFrameInfo(T_RtmpMediaInfo * i_ptRtmpMediaInfo,unsigned char *i_pbFrameData,int i_iDataLen);
    
	RtmpSession *   m_pRtmpSession;
    XHANDLE m_iTimer;
    int m_iStarter;
    int m_iSendResed;
    int m_iSendResTime;
    int m_iConnected;
    int m_iConnectRes;
    int m_iConnectTime;
    int m_iConnectCnt;
};














#endif
