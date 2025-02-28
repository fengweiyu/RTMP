/*****************************************************************************
* Copyright (C) 2020-2025 Hanson Yu  All rights reserved.
------------------------------------------------------------------------------
* File Module           :       MediaConvert.cpp
* Description           : 	    
* Created               :       2020.01.13.
* Author                :       Yu Weifeng
* Function List         : 	
* Last Modified         : 	
* History               : 	
******************************************************************************/
#ifndef MEDIA_CONVERT_H
#define MEDIA_CONVERT_H

#include <stdio.h>  
#include <stdlib.h>
#include <string.h>
#include <string>
#include <list>

#include "AudioCodec.h"
#include "MediaHandle.h"
#ifdef SUPPORT_PRI
#include "XStreamParser.h"
#include "STDStream.h"
#endif
using std::string;
using std::list;


//EMSCRIPTEN_KEEPALIVE 同样能导出一个函数， 它跟将函数加到EXPORTED_FUNCTIONS中效果一样。
//为了防止name mangling， 在导出函数一定要使用extern “C” 来修饰

#ifndef EM_EXPORT_API
    #if defined(__EMSCRIPTEN__)
        #include <emscripten.h>
        #if defined(__cplusplus)
            #define EM_EXPORT_API(rettype) extern "C" rettype EMSCRIPTEN_KEEPALIVE
        #else
            #define EM_EXPORT_API(rettype) rettype EMSCRIPTEN_KEEPALIVE
        #endif
    #else
        #if defined(__cplusplus)
            #define EM_EXPORT_API(rettype) extern "C" rettype
        #else
            #define EM_EXPORT_API(rettype) rettype
        #endif
    #endif
#endif

/*****************************************************************************
-Class			: DataBuf
-Description	: 
* Modify Date	  Version		 Author 		  Modification
* -----------------------------------------------
* 2019/09/21	  V1.0.0		 Yu Weifeng 	  Created
******************************************************************************/
class DataBuf
{
public:
    DataBuf(int i_iBufMaxLen)
    {
        pbBuf = new unsigned char [i_iBufMaxLen];
        iBufLen = 0;
        iBufMaxLen = i_iBufMaxLen;
    }
    virtual ~DataBuf()
    {
        delete [] pbBuf;
    }
    void Copy(unsigned char * i_pbSrcData,int i_iSrcDataLen)
    {
        if(i_iSrcDataLen+iBufLen<=iBufMaxLen)
        {
            memcpy(pbBuf+iBufLen,i_pbSrcData,i_iSrcDataLen);
            iBufLen += i_iSrcDataLen;
            return;
        }
        unsigned char * pBuf = new unsigned char [i_iSrcDataLen+iBufLen];
        memcpy(pBuf,pbBuf,iBufLen);
        memcpy(pBuf+iBufLen,i_pbSrcData,i_iSrcDataLen);
        delete [] pbBuf;
        pbBuf=pBuf;
        iBufLen = i_iSrcDataLen+iBufLen;
        iBufMaxLen = i_iSrcDataLen+iBufLen;
    }
    void Delete(int i_iProcessedLen)
    {
        if(i_iProcessedLen>=iBufLen)
        {
            iBufLen=0;
            return;
        }
        memmove(pbBuf,pbBuf+i_iProcessedLen,iBufLen-i_iProcessedLen);
        iBufLen = iBufLen-i_iProcessedLen;
    }
    unsigned char * pbBuf;//只允许只读操作
    int iBufLen;//只允许只读操作
    int iBufMaxLen;
};

/*****************************************************************************
-Class			: MediaConvert
-Description	: 
* Modify Date	  Version		 Author 		  Modification
* -----------------------------------------------
* 2019/09/21	  V1.0.0		 Yu Weifeng 	  Created
******************************************************************************/
class MediaConvert
{
public:
	MediaConvert();
	virtual ~MediaConvert();
    static MediaConvert * Instance();
    int ConvertFromPri(unsigned char * i_pbSrcData,int i_iSrcDataLen,E_StreamType i_eDstStreamType);
    int Convert(unsigned char * i_pbSrcData,int i_iSrcDataLen,E_MediaEncodeType i_eSrcEncType,E_StreamType i_eSrcStreamType,E_StreamType i_eDstStreamType);
    int GetData(unsigned char * o_pbData,int i_iMaxDataLen);
    int GetEncodeType(unsigned char * o_pbVideoEncBuf,int i_iMaxVideoEncBufLen,unsigned char * o_pbAudioEncBuf,int i_iMaxAudioEncBufLen);
private:
    int AudioTranscode(T_MediaFrameInfo * m_pbAudioFrame);

    int SetH264NaluData(unsigned char i_bNaluType,unsigned char i_bStartCodeLen,unsigned char *i_pbNaluData,int i_iNaluDataLen,T_MediaFrameInfo *m_ptFrame);
    int SetH265NaluData(unsigned char i_bNaluType,unsigned char i_bStartCodeLen,unsigned char *i_pbNaluData,int i_iNaluDataLen,T_MediaFrameInfo *m_ptFrame);
    int ParseH264NaluFromFrame(T_MediaFrameInfo *m_ptFrame);
    int ParseH265NaluFromFrame(T_MediaFrameInfo *m_ptFrame);
    int ParseNaluFromFrame(T_MediaFrameInfo * o_ptFrameInfo);
#ifdef SUPPORT_PRI
    FRAME_INFO* MediaToPri(T_MediaFrameInfo * i_ptMediaFrame);
    int PriToMedia(FRAME_INFO * i_pFrame,T_MediaFrameInfo *o_ptMediaFrameInfo);
	CSTDStream m_streamer;
#endif
    MediaHandle m_oMediaHandle;
    DataBuf * m_pbInputBuf;
    E_MediaEncodeType m_eDstVideoEncType;
    E_MediaEncodeType m_eDstAudioEncType;
    list<DataBuf *> m_pDataBufList;
    AudioCodec * m_pAudioCodec;
    unsigned char * m_pAudioTranscodeBuf;
    
    static MediaConvert *m_pInstance;
};
EM_EXPORT_API(int) InputData(unsigned char * i_pbSrcData,int i_iSrcDataLen,const char *i_strSrcName,const char *i_strDstName);
EM_EXPORT_API(int) GetData(unsigned char * o_pbData,int i_iMaxDataLen);
EM_EXPORT_API(int) GetEncodeType(unsigned char * o_pbVideoEncBuf,int i_iMaxVideoEncBufLen,unsigned char * o_pbAudioEncBuf,int i_iMaxAudioEncBufLen);


#endif
