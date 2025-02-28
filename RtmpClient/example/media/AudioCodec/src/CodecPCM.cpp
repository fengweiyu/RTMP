/*****************************************************************************
* Copyright (C) 2023-2028 Hanson Yu  All rights reserved.
------------------------------------------------------------------------------
* File Module		: 	CodecPCM.cpp
* Description		: 	CodecPCM operation center
                        无损压缩裸数据处理，主要是重采样
处理源数据和目标数据采样率，采样位数，通道不一致的问题
1.采样率不一致，
    要升采样(插值算法,简单线性插值、立方插值、傅里叶变换等)
    要降采样(过滤算法,低通滤波器、抗混叠滤波)
2.采样位数不一致，
    一般编码器里面支持不同的采样位数，如果这里处理，
    对于高位数转低位数，则保留高位过滤低位
        (将 24 位样本转换为 16 位样本，通常做法是直接保留高 16 位部分)
    对于低位数转高位数，则通过简单的零填充或扩展音频样本的值来实现
        (将每个 16 位样本的高 16 位扩展为 24 位，可以通过简单的左移和填充零)
3.通道不一致，
    单声道到立体声：
        可以将单声道音频的样本复制到两个声道中，创建一个相同的立体声输出。
    立体声到单声道：
        将立体声的两个声道混合成一个声道，通常通过计算两个声道的平均值来实现。
    多声道到立体声：
        如果有更多的声道（比如 5.1 声道），你可以选择特定的声道进行混合或直接选择其中两个声道作为立体声。
    立体声到多声道：
        如果需要扩展立体声到多个声道，可以通过复制声道或使用效果处理模板来生成其他声道。
* Created			: 	2023.09.21.
* Author			: 	Yu Weifeng
* Function List		: 	
* Last Modified 	: 	
* History			: 	
******************************************************************************/
#include <string.h>

#include "CodecPCM.h"



/*****************************************************************************
-Fuction		: CodecPCM
-Description	: 
-Input			: 
-Output 		: 
-Return 		: 
* Modify Date	  Version		 Author 		  Modification
* -----------------------------------------------
* 2023/09/21	  V1.0.0		 Yu Weifeng 	  Created
******************************************************************************/
CodecPCM::CodecPCM()
{
    m_pbTransBuf=NULL;
    memset(&m_tSrcCodecParam,0,sizeof(T_AudioCodecParam));
    memset(&m_tDstCodecParam,0,sizeof(T_AudioCodecParam));
}
/*****************************************************************************
-Fuction		: ~CodecPCM
-Description	: ~CodecPCM
-Input			: 
-Output 		: 
-Return 		: 
* Modify Date	  Version		 Author 		  Modification
* -----------------------------------------------
* 2023/09/21	  V1.0.0		 Yu Weifeng 	  Created
******************************************************************************/
CodecPCM::~CodecPCM()
{
    if(NULL != m_pbTransBuf)
        delete m_pbTransBuf;
}

/*****************************************************************************
-Fuction		: CodecPCM::Init
-Description	: 
-Input			: 
-Output 		: 
-Return 		: 
* Modify Date	  Version		 Author 		  Modification
* -----------------------------------------------
* 2023/09/21	  V1.0.0		 Yu Weifeng 	  Created
******************************************************************************/
int CodecPCM::Init(T_AudioCodecParam i_tSrcCodecParam,T_AudioCodecParam i_tDstCodecParam)
{
    memcpy(&m_tSrcCodecParam,&i_tSrcCodecParam,sizeof(T_AudioCodecParam));
    memcpy(&m_tDstCodecParam,&i_tDstCodecParam,sizeof(T_AudioCodecParam));
	return 0;
}

/*****************************************************************************
-Fuction		: TransSampleRate
-Description	: 
-Input			: 源样本数据i_abBuf 和长度i_iBufLen
-Output 		: 转换后的样本数据i_abBuf
-Return 		: 转换后的样本长度
* Modify Date	  Version		 Author 		  Modification
* -----------------------------------------------
* 2023/09/21	  V1.0.0		 Yu Weifeng 	  Created
******************************************************************************/
int CodecPCM::TransSampleRate(unsigned char * i_abBuf,int i_iBufLen,int i_iBufMaxLen)
{
    int iRet = -1;
    
    if(NULL == i_abBuf || i_iBufLen<=0)
    {
        AC_LOGE("TransSampleRate err NULL %d\r\n",i_iBufLen);
        return iRet;
    }
    AC_LOGD("CodecPCM dwSampleRate %d %d ,dwChannels %d %d ,dwBitsPerSample %d %d\r\n",m_tSrcCodecParam.dwSampleRate,m_tDstCodecParam.dwSampleRate,
    m_tSrcCodecParam.dwChannels,m_tDstCodecParam.dwChannels,m_tSrcCodecParam.dwBitsPerSample,m_tDstCodecParam.dwBitsPerSample);
    if(m_tSrcCodecParam.dwSampleRate==m_tDstCodecParam.dwSampleRate)
    {
        return i_iBufLen;
    }
    if(NULL == m_pbTransBuf)
    {
        m_pbTransBuf = new Buffer(i_iBufLen);
        m_pbTransBuf->Copy(i_abBuf,i_iBufLen);
    }
    else
    {
        m_pbTransBuf->Copy(i_abBuf,i_iBufLen);
    }
    if(m_tSrcCodecParam.dwSampleRate>m_tDstCodecParam.dwSampleRate)
    {
        iRet=DownSampleRate(m_pbTransBuf->pbBuf,m_pbTransBuf->iBufLen,i_abBuf,i_iBufMaxLen);
        m_pbTransBuf->Delete(m_pbTransBuf->iBufLen);
        return iRet;
    }
    if(m_tSrcCodecParam.dwSampleRate<m_tDstCodecParam.dwSampleRate)
    {
        iRet=UpSampleRate(m_pbTransBuf->pbBuf,m_pbTransBuf->iBufLen,i_abBuf,i_iBufMaxLen);
        m_pbTransBuf->Delete(m_pbTransBuf->iBufLen);
    }
    return iRet;
}

/*****************************************************************************
-Fuction		: UpSampleRate
-Description	: 升采样
线性插值法
-Input			: 
-Output 		: 
-Return 		: 
* Modify Date	  Version		 Author 		  Modification
* -----------------------------------------------
* 2023/09/21	  V1.0.0		 Yu Weifeng 	  Created
******************************************************************************/
int CodecPCM::UpSampleRate(unsigned char * i_abSrcBuf,int i_iSrcBufLen,unsigned char * o_abDstBuf,int i_iDstBufMaxLen)
{
    int iRet = -1;
    short * pwSrcData=NULL;
    short * pwDstData=NULL;
    int i=0,j=0;
    int iDiffSampleRate=0;
    int iDstSampleCnt=0;
    int iCurSampleCnt=0;

    if(m_tSrcCodecParam.dwBitsPerSample!=m_tDstCodecParam.dwBitsPerSample)
    {//暂不支持采样位数转换
        AC_LOGE("UpSampleRate.dwBitsPerSample %d!=m_tDstCodecParam.dwBitsPerSample %d\r\n",m_tSrcCodecParam.dwBitsPerSample,m_tDstCodecParam.dwBitsPerSample);
        return iRet;
    }
    if(m_tSrcCodecParam.dwBitsPerSample!=16)//暂时只支持pcm16的数据处理
    {
        AC_LOGE("UpSampleRate dwBitsPerSample err %d %d\r\n",m_tSrcCodecParam.dwBitsPerSample,m_tDstCodecParam.dwBitsPerSample);
        return iRet;
    }
    iDstSampleCnt=(float)i_iSrcBufLen*m_tDstCodecParam.dwSampleRate/(sizeof(short)*m_tSrcCodecParam.dwSampleRate);//优化计算后
    iDiffSampleRate=m_tDstCodecParam.dwSampleRate/m_tSrcCodecParam.dwSampleRate;
    pwSrcData=(short *)i_abSrcBuf;
    pwDstData=(short *)o_abDstBuf;
    j=0;
    for (i = 0; i < i_iSrcBufLen/sizeof(short); i ++) // 计算每个输入样本对应的输出采样频率增量  
    {  
        if(i*iDiffSampleRate*sizeof(short)>i_iDstBufMaxLen)
        {
            AC_LOGE("DownSampleRate err DstBufMaxLen %d %d\r\n",i_iSrcBufLen,i_iDstBufMaxLen);
            return iRet;
        }
        pwDstData[iCurSampleCnt] = pwSrcData[i]; // 将每个输入样本复制到输出的相应位置，每隔5个插入一份  ,这里放第一个
        // 进行线性插值的简单实现  
        if (i < i_iSrcBufLen/sizeof(short) - 1) //如果大于即输入样本数不够，则使用计算后的最后一个样本值统一赋值或者不处理填0无声音
        {  
            for (j = 1; j < iDiffSampleRate; j++) 
            { //第一个后面的由这里插入
                pwDstData[iCurSampleCnt + j] = (pwSrcData[i] * (iDiffSampleRate - j) + pwSrcData[i + 1] * j) / iDiffSampleRate;//比如第一个是前一个五分之四加后一个五分之一  
            } //第一个等于前一个加后一个值与前一个差值的百分之二十，第二个是百分之四十，实现线性插值
        }
        else //i = i_iSrcBufLen/2 - 1 ,最后一个源数据
        {
            for (j = 1; j < iDiffSampleRate; j++) 
            { 
                pwDstData[iCurSampleCnt + j] = pwSrcData[i];//最后一个样本值统一赋值或者不处理填0无声音
            } 
        }
        iCurSampleCnt+=iDiffSampleRate;
        
        //动态调整算法
        iDstSampleCnt=(float)(i+1)*m_tDstCodecParam.dwSampleRate/m_tSrcCodecParam.dwSampleRate;//计算目标采样率同样时间对应的样本数(目标样本数)
        if(iCurSampleCnt>iDstSampleCnt)//
        {//样本数多了，则采集少些
            iDiffSampleRate=m_tDstCodecParam.dwSampleRate/m_tSrcCodecParam.dwSampleRate;//iDiffSampleRate--;//由于计算误差，这样会导致采样间隙过大
        }
        else if(iCurSampleCnt<iDstSampleCnt)
        {//样本数少了，则采集多些
            iDiffSampleRate=m_tDstCodecParam.dwSampleRate/m_tSrcCodecParam.dwSampleRate+1;//iDiffSampleRate++;//由于计算误差，这样会导致采样间隙过大
        }
        //AC_LOGD("%d ",iCurSampleCnt);
    }  
    iRet=iCurSampleCnt*sizeof(short);
    return iRet;
}

/*****************************************************************************
-Fuction		: DownSampleRate
-Description	: 降采样
降采样步骤
滤波：应用低通滤波器。
取样：从每个 5.51 个样本中选择一个样本。44.1kHz 和 8kHz 的比例是44100/8000 = 5.5125，
可以取整为每 5 个样本取一个
(采集样本会多，会导致播放时间变长，时间戳应该会不准
如果最后统一丢样本会导致丢数据，可以间隔一定时间丢样本，比如100ms ,
这个间隔时间需要调试得到
更准确的方法是，算出源样本对应的时间得到目标总样本数，前面算出的总样本数
除以目标总样本数，得到倍数，根据这个间隔倍数再丢样本，就会准一些(1.1倍难处理，和前面扩展丢的精度没区别)，
但是要一样必须间隔时间或间隔样本丢)
-Input			: 
-Output 		: 
-Return 		: 
* Modify Date	  Version		 Author 		  Modification
* -----------------------------------------------
* 2023/09/21	  V1.0.0		 Yu Weifeng 	  Created
******************************************************************************/
int CodecPCM::DownSampleRate(unsigned char * i_abSrcBuf,int i_iSrcBufLen,unsigned char * o_abDstBuf,int i_iDstBufMaxLen)
{
    int iRet = -1;
    short * pwSrcData=NULL;
    short * pwDstData=NULL;
    int i=0,j=0;
    int iDiffSampleRate=0;
    float fSrcSampleTime=0;
    int iDstSampleCnt=0;

    if(m_tSrcCodecParam.dwBitsPerSample!=m_tDstCodecParam.dwBitsPerSample)
    {//暂不支持采样位数转换
        AC_LOGE("m_tSrcCodecParam.dwBitsPerSample %d!=m_tDstCodecParam.dwBitsPerSample %d\r\n",m_tSrcCodecParam.dwBitsPerSample,m_tDstCodecParam.dwBitsPerSample);
        return iRet;
    }
    if(m_tSrcCodecParam.dwBitsPerSample!=16)//暂时只支持pcm16的数据处理
    {
        AC_LOGE("dwBitsPerSample err %d %d\r\n",m_tSrcCodecParam.dwBitsPerSample,m_tDstCodecParam.dwBitsPerSample);
        return iRet;
    }
    fSrcSampleTime=(float)i_iSrcBufLen/sizeof(short)*1000/m_tSrcCodecParam.dwSampleRate;//采样次数除以采样率就是时间，这里是ms单位
    iDstSampleCnt=fSrcSampleTime*m_tDstCodecParam.dwSampleRate/1000;//采样时间乘以采样率就是采样次数(样本数)
    iDstSampleCnt=(float)i_iSrcBufLen*m_tDstCodecParam.dwSampleRate/(sizeof(short)*m_tSrcCodecParam.dwSampleRate);//优化计算后
    
    iDiffSampleRate=m_tSrcCodecParam.dwSampleRate/m_tDstCodecParam.dwSampleRate;
    pwSrcData=(short *)i_abSrcBuf;
    pwDstData=(short *)o_abDstBuf;
    j=0;
    for (i = 0; i < i_iSrcBufLen/sizeof(short); i += iDiffSampleRate) 
    {  
        if(j*sizeof(short)>i_iDstBufMaxLen)
        {
            AC_LOGE("DownSampleRate err DstBufMaxLen %d %d\r\n",i_iSrcBufLen,i_iDstBufMaxLen);
            return iRet;
        }
        pwDstData[j] = pwSrcData[i]; // 每5个样本取一个  
        j++;

        //动态调整算法
        iDstSampleCnt=(float)i*m_tDstCodecParam.dwSampleRate/m_tSrcCodecParam.dwSampleRate;//计算目标采样率同样时间对应的样本数(目标样本数)
        if(j>iDstSampleCnt)//
        {//样本数多了，则采集少些
            iDiffSampleRate=m_tSrcCodecParam.dwSampleRate/m_tDstCodecParam.dwSampleRate+1;//iDiffSampleRate++;//由于计算误差，这样会导致采样间隙过大
        }
        else if(j<iDstSampleCnt)
        {//样本数少了，则采集多些
            iDiffSampleRate=m_tSrcCodecParam.dwSampleRate/m_tDstCodecParam.dwSampleRate;//iDiffSampleRate--;//由于计算误差，这样会导致采样间隙过大
        }
        //AC_LOGD("%d ",i);
    }  
    iRet=j*sizeof(short);
    return iRet;
}

/*
// 降采样函数  
//取样：从每个 551 个样本中选择一个样本。44.1kHz 和 8kHz 的比例是44100/8000 = 5.5125，
//可以取整为每 551 个样本取一个
void downsample(int16_t *input, int16_t *output) {  
    for (int i = 0, j = 0; j < OUTPUT_SAMPLES; i += 551, j++) {  
        output[j] = input[i]; // 每551个样本取一个  
    }  
} 
#define INPUT_SAMPLES 8000  // 假设 8kHz输入样本数量（1秒）  
#define OUTPUT_SAMPLES 44100 // 对应 44.1kHz输出样本数量  

void upsample(int16_t *input, int16_t *output) 
{  
    // 计算每个输入样本对应的输出采样频率增量  
    for (int i = 0; i < INPUT_SAMPLES; i++) 
    {  
        output[i * 5] = input[i]; // 将每个输入样本复制到输出的相应位置，每隔5个插入一份  
        // 进行线性插值的简单实现  
        if (i < INPUT_SAMPLES - 1) //如果大于即输入样本数不够，则使用计算后的最后一个样本值统一赋值或者不处理填0无声音
        {  
            for (int j = 1; j < 5; j++) {  
                output[i * 5 + j] = (input[i] * (5 - j) + input[i + 1] * j) / 5;//比如第一个是前一个五分之四加后一个五分之一  
            }  //第一个等于前一个加后一个值与前一个差值的百分之二十，第二个是百分之四十，实现线性插值
        }  
    }  
}
*/
