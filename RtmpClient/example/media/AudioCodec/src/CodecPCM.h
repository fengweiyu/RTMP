/*****************************************************************************
* Copyright (C) 2023-2028 Hanson Yu  All rights reserved.
------------------------------------------------------------------------------
* File Module		: 	CodecPCM.h
* Description		: 	CodecPCM operation center
* Created			: 	2017.09.21.
* Author			: 	Yu Weifeng
* Function List		: 	
* Last Modified 	: 	
* History			: 	
******************************************************************************/
#ifndef CODEC_PCM_H
#define CODEC_PCM_H

#include <stdlib.h>
#include <stdio.h>
#include <string>
#include "AudioCodecAdapter.h"

using std::string;




/*****************************************************************************
-Class			: CodecPCM
-Description	: 
* Modify Date	  Version		 Author 		  Modification
* -----------------------------------------------
* 2023/09/21	  V1.0.0		 Yu Weifeng 	  Created
******************************************************************************/
class CodecPCM
{
public:
    CodecPCM();
    virtual ~CodecPCM();
    int Init(T_AudioCodecParam i_tSrcCodecParam,T_AudioCodecParam i_tDstCodecParam);
    int TransSampleRate(unsigned char * i_abBuf,int i_iBufLen,int i_iBufMaxLen);

    
private:
    int UpSampleRate(unsigned char * i_abSrcBuf,int i_iSrcBufLen,unsigned char * o_abDstBuf,int i_iDstBufMaxLen);
    int DownSampleRate(unsigned char * i_abSrcBuf,int i_iSrcBufLen,unsigned char * o_abDstBuf,int i_iDstBufMaxLen);


    Buffer * m_pbTransBuf;
    T_AudioCodecParam m_tSrcCodecParam;
    T_AudioCodecParam m_tDstCodecParam;
};






#endif

