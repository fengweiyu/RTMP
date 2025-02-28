/*****************************************************************************
* Copyright (C) 2023-2028 Hanson Yu  All rights reserved.
------------------------------------------------------------------------------
* File Module		: 	CodecWAV.h
* Description		: 	CodecWAV operation center
* Created			: 	2017.09.21.
* Author			: 	Yu Weifeng
* Function List		: 	
* Last Modified 	: 	
* History			: 	
******************************************************************************/
#ifndef CODEC_WAV_H
#define CODEC_WAV_H

#include <stdlib.h>
#include <stdio.h>
#include <string>
#include "AudioCodecAdapter.h"

using std::string;




/*****************************************************************************
-Class			: CodecWAV
-Description	: 
* Modify Date	  Version		 Author 		  Modification
* -----------------------------------------------
* 2023/09/21	  V1.0.0		 Yu Weifeng 	  Created
******************************************************************************/
class CodecWAV   : public AudioCodecInterface
{
public:
    CodecWAV();
    virtual ~CodecWAV();
    int Init();
    int InitEncode(T_AudioCodecParam i_tSrcCodecParam);
    int Encode(unsigned char * i_abSrcBuf,int i_iSrcBufLen,unsigned char * o_abDstBuf,int i_iDstBufMaxLen);
    int InitDecode();
    int Decode(unsigned char * i_abSrcBuf,int i_iSrcBufLen,unsigned char * o_abDstBuf,int i_iDstBufMaxLen,T_AudioCodecParam *o_ptCodecParam);

    
private:
    T_AudioCodecParam m_tCodecEncParam;//目标编码参数
    AudioCodecInterface *m_pAudioEncoder;
};






#endif

