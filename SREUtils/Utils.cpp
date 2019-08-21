#include "Utils.h"

#include <stdio.h>

#if defined (__linux__)
#define _fseeki64  fseeko64
#define _ftelli64  ftello64
#endif

std::vector<uint8_t> ReadFileInMemory(const char *sFileName)
{

    size_t fSize=0;
    std::vector<uint8_t> vOut;
    FILE *f=fopen(sFileName, "rb");
    if(f!=nullptr)
    {
        _fseeki64(f, 0, SEEK_END);
        fSize=_ftelli64(f);
        _fseeki64(f, 0, SEEK_SET);
        vOut.resize(fSize);
        fread(vOut.data(), 1, fSize, f);
        fclose(f);
    }
    return vOut;
}

std::vector<uint8_t> ReadFileInMemory(const wchar_t *sFileName)
{

    size_t fSize=0;
    std::vector<uint8_t> vOut;
    FILE *f=nullptr;
#if defined (__linux__)
    const wchar_t *ss=sFileName;

    int i=(int) wcsrtombs(NULL, &ss, 0, NULL);
    if(i<0)
    {
        return vOut;
    }
    char *d=(char *) malloc(i+1);
    wcsrtombs(d, &sFileName, i, NULL);
    d[i]=0;

    f=fopen(d, "rb");
#else
    f= _wfopen(sFileName, L"rb");
#endif

    if(f!=nullptr)
    {
        _fseeki64(f, 0, SEEK_END);
        fSize=_ftelli64(f);
        _fseeki64(f, 0, SEEK_SET);
        vOut.resize(fSize);
        fread(vOut.data(), 1, fSize, f);
        fclose(f);
    }
    return vOut;
}

std::string SplitLargeStringToStrArray(const std::string &sStrName, const std::string &sLargeStr)
{
    std::string sOutText="const char * const "+std::string(sStrName)+"[]={\r\n";
    size_t DefStrLen=0x3FFC;
    size_t StrLen;
    size_t nCurPos=0;
    std::string sCurStr;
    size_t nNumStrings=0;
    if(sLargeStr.empty()==false)
    {
        nNumStrings=sLargeStr.length()/DefStrLen;

        do
        {
            StrLen=sLargeStr.length()-nCurPos;
            if(StrLen>DefStrLen)
            {
                StrLen=DefStrLen;
            }
            sCurStr=sLargeStr.substr(nCurPos, StrLen);
            //sBase64Data=sBase64Data.substr(StrLen, sBase64Data.length());
            sOutText+="\t\""+sCurStr+"\"";
            nCurPos+=StrLen;
            if(nCurPos<sLargeStr.length())
            {
                sOutText+=",";
            }
            sOutText+="\r\n";
        }
        while(nCurPos<sLargeStr.length());
    }
    sOutText+="};\r\n";
    return sOutText;
}

std::string CombineStrArray(const char *const *pStr, size_t nStrCount)
{
    std::string OutStr;
    for(size_t i=0; i<nStrCount; i++)
    {
        OutStr.append(pStr[i]);
    }
    return OutStr;
}
