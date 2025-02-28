/*****************************************************************************
* Copyright (C) 2020-2025 Hanson Yu  All rights reserved.
------------------------------------------------------------------------------
* File Module           :       main.cpp
* Description           : 	    
* Created               :       2020.01.13.
* Author                :       Yu Weifeng
* Function List         : 	
* Last Modified         : 	
* History               : 	
******************************************************************************/
#include <stdio.h>  
#include <stdlib.h>
#include <string.h>

#include "MediaConvert.h"

#define MEDIA_BUF_FORMAT_MAX_LEN	(10*1024*1024) 

static int proc(const char * i_strSrcFilePath,const char *i_strDstFilePath);
static void PrintUsage(char *i_strProcName);

/*****************************************************************************
-Fuction        : main
-Description    : main
-Input          : 
-Output         : 
-Return         : 
* Modify Date     Version             Author           Modification
* -----------------------------------------------
* 2020/01/01      V1.0.0              Yu Weifeng       Created
******************************************************************************/
int main(int argc, char* argv[]) 
{
    int iRet = -1;
    
    if(argc !=3)
    {
        PrintUsage(argv[0]);
        return proc("H264AAC.flv","H264AAC.ts");//proc("H264G711A.flv","H264G711A.mp4");H264AAC
    }
    return proc(argv[1],argv[2]);
}
/*****************************************************************************
-Fuction        : ReadFile
-Description    : ReadFile
-Input          : 
-Output         : 
-Return         : 
* Modify Date     Version             Author           Modification
* -----------------------------------------------
* 2020/01/01      V1.0.0              Yu Weifeng       Created
******************************************************************************/
int ReadFile(const char * i_strSrcFilePath,unsigned char **o_ppBuffer) 
{  
    FILE *pFile;  
    int iBytesRead;  
    int iFileSize;  
    unsigned char *pBuffer=NULL;
    int iRet = -1;

    
    // 打开二进制文件  
    pFile= fopen(i_strSrcFilePath, "rb");  
    if (pFile == NULL) 
    {  
        printf("ReadFile err");  
        return iRet;  
    }  

    do
    {
        // 获取文件大小  
        fseek(pFile, 0, SEEK_END);  
        iFileSize = ftell(pFile);  
        fseek(pFile, 0, SEEK_SET); // 重置文件指针到开头  
        
        // 分配内存以存储文件内容  
        pBuffer = new unsigned char [iFileSize];
        if (pBuffer == NULL) 
        {  
            printf("malloc err %d",iFileSize);  
            break;  
        }  
        
        // 读取文件内容  
        iBytesRead = fread(pBuffer, sizeof(unsigned char), iFileSize, pFile);  
        if (iBytesRead != iFileSize && !feof(pFile)) 
        {  
            printf("fread err %d",iFileSize);  
            break;  
        }  
        iRet = iBytesRead;
    }while(0);
    fclose(pFile);  
    if (iRet<= 0 && pBuffer != NULL) 
    {  
        delete [] pBuffer;//free(pBuffer); 
        return iRet;  
    }  
    *o_ppBuffer=pBuffer;
    return iRet;  
} 

/*****************************************************************************
-Fuction        : proc
-Description    : proc
-Input          : 
-Output         : 
-Return         : 
* Modify Date     Version             Author           Modification
* -----------------------------------------------
* 2020/01/01      V1.0.0              Yu Weifeng       Created
******************************************************************************/
static int proc(const char * i_strSrcFilePath,const char *i_strDstFilePath)
{
    unsigned char * pbSrcFileBuf=NULL;
    unsigned char * pbFileBuf=NULL;
	int iRet = -1,iReadLen = -1,iWriteLen=0;
	int iMaxLen=0;
    FILE *pDstFile=NULL;  


    iReadLen=ReadFile(i_strSrcFilePath,&pbSrcFileBuf);
    if(iReadLen <= 0)
    {
        printf("ReadFile err %s\r\n",i_strSrcFilePath);
        return iRet;
    } 
    do
    {
        iRet=InputData(pbSrcFileBuf,iReadLen,i_strSrcFilePath,i_strDstFilePath);
        if(iRet <= 0)
        {
            printf("InputData err %s %s\r\n",i_strSrcFilePath,i_strDstFilePath);
            //break;
        } 
        iMaxLen=iReadLen+MEDIA_BUF_FORMAT_MAX_LEN;
        pbFileBuf = new unsigned char [iMaxLen];
        if(NULL == pbFileBuf)
        {
            printf("NULL == pbFileBuf err\r\n");
            break;
        } 
        iWriteLen=0;
        iRet=0;
        do
        {
            iWriteLen+=iRet;
            iRet=GetData(pbFileBuf+iWriteLen,iMaxLen-iWriteLen);
        } while(iRet>0);

        
        pDstFile = fopen(i_strDstFilePath,"wb");//
        if(NULL == pDstFile)
        {
            printf("fopen %s err\r\n",i_strDstFilePath);
            break;
        } 
        iRet = fwrite(pbFileBuf,1,iWriteLen, pDstFile);
        if(iRet != iWriteLen)
        {
            printf("fwrite err %d iWriteLen%d\r\n",iRet,iWriteLen);
            break;
        }
    }while(0);
    
    if(NULL != pbFileBuf)
    {
        delete [] pbFileBuf;
    } 
    if(NULL != pbSrcFileBuf)
    {
        delete [] pbSrcFileBuf;
    } 
    if(NULL != pDstFile)
    {
        fclose(pDstFile);//fseek(m_pMediaFile,0,SEEK_SET); 
    } 
    return iRet;
}
/*****************************************************************************
-Fuction        : PrintUsage
-Description    : PrintUsage
-Input          : 
-Output         : 
-Return         : 
* Modify Date     Version             Author           Modification
* -----------------------------------------------
* 2020/01/01      V1.0.0              Yu Weifeng       Created
******************************************************************************/
static void PrintUsage(char *i_strProcName)
{
    printf("Usage: %s inputFile outputFile\r\n",i_strProcName);
    //printf("eg: %s 9112 77.72.169.210 3478\r\n",i_strProcName);
    printf("run default args: %s H264G711A.flv H264G711A.mp4\r\n",i_strProcName);
}

