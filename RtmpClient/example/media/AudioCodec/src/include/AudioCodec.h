/*****************************************************************************
* Copyright (C) 2023-2028 Hanson Yu  All rights reserved.
------------------------------------------------------------------------------
* File Module		: 	AudioCodec.h
* Description		: 	AudioCodec operation center
* Created			: 	2017.09.21.
* Author			: 	Yu Weifeng
* Function List		: 	
* Last Modified 	: 	
* History			: 	
******************************************************************************/
#ifndef AUDIO_CODEC_H
#define AUDIO_CODEC_H

#include <stdlib.h>
#include <stdio.h>
#include <string>
#include "AudioCodecAdapter.h"

using std::string;




/*****************************************************************************
-Class			: AudioCodec
-Description	: 
* Modify Date	  Version		 Author 		  Modification
* -----------------------------------------------
* 2023/09/21	  V1.0.0		 Yu Weifeng 	  Created
******************************************************************************/
class AudioCodec
{
public:
    AudioCodec();
    virtual ~AudioCodec();
    int Init();
    int Transcode(unsigned char * i_abSrcBuf,int i_iSrcBufLen,T_AudioCodecParam i_tSrcCodecParam,unsigned char * o_abDstBuf,int i_iDstBufMaxLen,T_AudioCodecParam i_tDstCodecParam);
    int InitEncode(T_AudioCodecParam i_tCodecParam);
    int Encode(unsigned char * i_abSrcBuf,int i_iSrcBufLen,unsigned char * o_abDstBuf,int i_iDstBufMaxLen);
    int InitDecode(E_AudioCodecType i_eAudioCodecType);
    int Decode(unsigned char * i_abSrcBuf,int i_iSrcBufLen,unsigned char * o_abDstBuf,int i_iDstBufMaxLen,T_AudioCodecParam *o_ptCodecParam);

    
private:
    unsigned char * m_pbDecodeBuf;//解码出pcm裸数据，存放在此buf
    AudioCodecInterface * m_ptEncoder;
    AudioCodecInterface * m_ptDecoder;
    void * m_pRawDataHandle;//原始(未压缩)音频数据处理对象
    Buffer * m_pbEncHeaderBuf;
    AudioCodecInterface *m_pEncoderHeader;
    Buffer * m_pbSubDecBuf;
    AudioCodecInterface *m_pSubDecoder;
};






#endif

