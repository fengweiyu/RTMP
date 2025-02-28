/*****************************************************************************
* Copyright (C) 2023-2028 Hanson Yu  All rights reserved.
------------------------------------------------------------------------------
* File Module		: 	CodecG711U.h
* Description		: 	CodecG711U operation center
* Created			: 	2017.09.21.
* Author			: 	Yu Weifeng
* Function List		: 	
* Last Modified 	: 	
* History			: 	
******************************************************************************/
#ifndef CODEC_G711U_H
#define CODEC_G711U_H

#include <stdlib.h>
#include <stdio.h>
#include <string>
#include "AudioCodecAdapter.h"

using std::string;




/*****************************************************************************
-Class			: CodecG711U
-Description	: 
* Modify Date	  Version		 Author 		  Modification
* -----------------------------------------------
* 2023/09/21	  V1.0.0		 Yu Weifeng 	  Created
******************************************************************************/
class CodecG711U   : public AudioCodecInterface
{
public:
    CodecG711U();
    virtual ~CodecG711U();
    int Init();
    int InitEncode(T_AudioCodecParam i_tSrcCodecParam);
    int Encode(unsigned char * i_abSrcBuf,int i_iSrcBufLen,unsigned char * o_abDstBuf,int i_iDstBufMaxLen);
    int InitDecode();
    int Decode(unsigned char * i_abSrcBuf,int i_iSrcBufLen,unsigned char * o_abDstBuf,int i_iDstBufMaxLen,T_AudioCodecParam *o_ptCodecParam);

    
private:

    T_AudioCodecParam m_tCodecEncParam;
};






#endif

