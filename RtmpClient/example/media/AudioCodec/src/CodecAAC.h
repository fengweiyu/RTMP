/*****************************************************************************
* Copyright (C) 2023-2028 Hanson Yu  All rights reserved.
------------------------------------------------------------------------------
* File Module		: 	CodecAAC.h
* Description		: 	CodecAAC operation center
* Created			: 	2017.09.21.
* Author			: 	Yu Weifeng
* Function List		: 	
* Last Modified 	: 	
* History			: 	
******************************************************************************/
#ifndef CODEC_AAC_H
#define CODEC_AAC_H

#include <stdlib.h>
#include <stdio.h>
#include <string>
#include "AudioCodecAdapter.h"
#include "aacenc_lib.h"
#include "aacdecoder_lib.h"

using std::string;




/*****************************************************************************
-Class			: CodecAAC
-Description	: 
* Modify Date	  Version		 Author 		  Modification
* -----------------------------------------------
* 2023/09/21	  V1.0.0		 Yu Weifeng 	  Created
******************************************************************************/
class CodecAAC : public AudioCodecInterface
{
public:
    CodecAAC();
    virtual ~CodecAAC();
    int Init();
    int InitEncode(T_AudioCodecParam i_tSrcCodecParam);
    int Encode(unsigned char * i_abSrcBuf,int i_iSrcBufLen,unsigned char * o_abDstBuf,int i_iDstBufMaxLen);
    int InitDecode();
    int Decode(unsigned char * i_abSrcBuf,int i_iSrcBufLen,unsigned char * o_abDstBuf,int i_iDstBufMaxLen,T_AudioCodecParam *o_ptCodecParam);

    
private:
    Buffer * m_pbEncodeBuf;
    int m_iEncodeBufLen;
    T_AudioCodecParam m_tEncCodecParam;
    HANDLE_AACENCODER m_ptEncoder;
    HANDLE_AACDECODER m_ptDecoder;
};






#endif

