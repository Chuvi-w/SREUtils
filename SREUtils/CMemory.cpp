#include "CMemory.h"
#include <algorithm>
#include <stdarg.h>
#include <chrono>
#include "base64.h"
#include <CRC32.h>
#include <CHashFunc.h>
#include <string.h>
#include "LZSS.h"
#include "Utils.h"

enum
{
    PACKED_MEMORY_SIGNATURE=0x4b41504d/*(unsigned int)'MPAK'*/, PACKED_SECTION_SIGNATURE=0x4b415053/*(unsigned int)'SPAK'*/
};
typedef struct PacketMemoryHeader_s
{
    uint32_t nSignature;
    uint32_t nSectionsCount;
    uint32_t nReadSize;
    uint32_t nDataSize;
    uint32_t nHeaderSize;
    uint32_t nExecAddr;
    uint32_t nCRC32;
} PacketMemoryHeader_t;

typedef struct PackedSectionHeader_s
{
    uint32_t nSignature;
    uint32_t nSecAddr;
    uint32_t nDataSize;
    uint32_t nCRC32;
} PackedSectionHeader_t;

#ifndef SORT_VALIDATION
#define SORT_CHECK()
#else
#define SORT_CHECK() CheckSorted()
#include <windows.h>
bool CMemory::CheckSorted() const
{
    auto pSect=m_Sections.begin();
    auto pNext=std::next(pSect);
    //    auto ShowMsgBox = [](const char *Msg)
    //    {
    //      typedef  int  (__stdcall *pfnMessageBoxA)(void *hWnd, const char* lpText, const char* lpCaption, int uType);
    //
    //      auto UserLib=GetModuleHandleA()
    //    };
    size_t nErrNum=0;
#define ASSERT_FATAL(exp) if(exp){  MessageBoxA(0, "Error: "#exp, "FAIL", MB_ICONERROR); /*fwrite(stderr,"%u %x:%x  %x:%x",nErrNum,pSect->GetStart(),pSect->GetEnd(),pNext->GetStart(),pNext->GetEnd());*/    exit(0);}
    std::vector<uint8_t> vData;
    while(pSect!=m_Sections.end()&&pNext!=m_Sections.end())
    {
        ASSERT_FATAL((*pSect)->GetStart()>=(*pNext)->GetStart());
        ASSERT_FATAL((*pSect)->GetEnd()>(*pNext)->GetStart());
        ASSERT_FATAL((*pSect)->GetStart()>=(*pNext)->GetEnd());
        ASSERT_FATAL((*pSect)->GetEnd()>=(*pNext)->GetEnd());
        ++pSect;
        pNext=std::next(pSect);
        ++nErrNum;
    }

    return true;
}
#endif

#ifdef DISABLED_CODE



bool CMemory::Init(const MemoryData_t& pData)
{
    bool                bValid=true;
    CMemorySection NewSect;
    Reset();

    if(pData.nHeaderSz)
    {
        m_Header.clear();
        m_Header.insert(m_Header.end(), pData.szHeader, pData.szHeader+pData.nHeaderSz);
    }
    for(size_t i=0; i<pData.nSections; i++)
    {


        if(!NewSect.Init(pData.pSection[i]))
        {
            bValid=false;
            break;
        }
        m_Sections.push_back(NewSect);
    }
    m_ExecAddr=pData.nExecAddr;
    if(!bValid)
    {
        Reset();
        return false;
    }

    // ______ VALIDATION _____
    uint32_t             nNumSections=m_Sections.size();
    uint32_t             nHeaderSz=m_Header.length();
    std::vector<uint8_t> sData;
    uint32_t             nEmptySpaceSz=0;
#ifdef HAVE_MD5
    CommonTools::MD5_CTX MD5Ctx;
    uint64_t             MD5Sum[MD5_DIGEST_LENGTH_64];
    CommonTools::MD5Init(&MD5Ctx);
    CommonTools::MD5Update(&MD5Ctx, &m_ExecAddr, sizeof(m_ExecAddr));
    CommonTools::MD5Update(&MD5Ctx, &nNumSections, sizeof(nNumSections));
    CommonTools::MD5Update(&MD5Ctx, &nHeaderSz, sizeof(nHeaderSz));
#endif

#ifdef HAVE_CRC32
    uint32_t ResCrc=0;
    CommonTools::CRC32_Init(reinterpret_cast<CommonTools::CRC32_t*>(&ResCrc));
    CommonTools::CRC32_ProcessBuffer(reinterpret_cast<CommonTools::CRC32_t*>(&ResCrc), &m_ExecAddr, sizeof(m_ExecAddr));
    CommonTools::CRC32_ProcessBuffer(reinterpret_cast<CommonTools::CRC32_t*>(&ResCrc), &nNumSections, sizeof(nNumSections));
    CommonTools::CRC32_ProcessBuffer(reinterpret_cast<CommonTools::CRC32_t*>(&ResCrc), &nHeaderSz, sizeof(nHeaderSz));
#endif

    if(!m_Header.empty())
    {
#ifdef HAVE_CRC32
        CommonTools::CRC32_ProcessBuffer(reinterpret_cast<CommonTools::CRC32_t*>(&ResCrc), m_Header.c_str(), m_Header.length());
#endif

#ifdef HAVE_MD5
        CommonTools::MD5Update(&MD5Ctx, m_Header.c_str(), m_Header.length());
#endif
    }

    int t=0;
    for(auto p=m_Sections.begin(); p!=m_Sections.end(); ++p, ++t)
    {
        sData=p->GetFullData();
#ifdef HAVE_CRC32
        CommonTools::CRC32_ProcessBuffer(reinterpret_cast<CommonTools::CRC32_t*>(&ResCrc), &sData[0], sData.size());
#endif

#ifdef HAVE_MD5
        CommonTools::MD5Update(&MD5Ctx, &sData[0], sData.size());
#endif
        if(std::next(p)!=m_Sections.end())
        {
            nEmptySpaceSz=(std::next(p)->GetStart()-p->GetEnd());
#ifdef HAVE_CRC32
            for(size_t i=0; i<nEmptySpaceSz; i++)
            {
                CommonTools::CRC32_ProcessByte(reinterpret_cast<CommonTools::CRC32_t*>(&ResCrc), 0xFF);
            }
#endif
#ifdef HAVE_MD5
            MD5UpdateConst(&MD5Ctx, 0xFF, nEmptySpaceSz);
#endif
        }
    }

#ifdef HAVE_CRC32
    CommonTools::CRC32_Final(reinterpret_cast<CommonTools::CRC32_t*>(&ResCrc));
#endif
#ifdef HAVE_MD5
    MD5Final(reinterpret_cast<unsigned char*>(&MD5Sum), &MD5Ctx);
#endif

    if(false
#ifdef HAVE_CRC32
       ||(pData.nFlags & FLAG_CRC32 && ResCrc!=pData.CRC32)
#endif
#ifdef HAVE_MD5
       ||(pData.nFlags & FLAG_MD5&&(MD5Sum[0]!=pData.MD5[0]||MD5Sum[1]!=pData.MD5[1]))
#endif
       )
    {
        printf("Checksum verification error. In:{%08x,{0x%016I64x,0x%016I64x}}, Calc:{%08x,{0x%016I64x,0x%016I64x}}\n",
               pData.CRC32,
               pData.MD5[0],
               pData.MD5[1],
#ifdef HAVE_CRC32
               ResCrc,
#else
               0,
#endif
#ifdef HAVE_MD5
               MD5Sum[0],
               MD5Sum[1]
#else
               0,
               0
#endif
        );
        bValid=false;
    }
    if(!bValid)
    {
        Reset();
        return false;
    }
    return bValid;
}

#endif

CMemory::CMemory()
{
}

CMemory::~CMemory()
{
}

void CMemory::Reset()
{
    m_Header.clear();
    m_Sections.clear();
}

std::string CMemory::WriteS19(bool bFillEmpty, uint32_t NumWords) const
{
    uint32_t Addr=0;
    std::string sOutStr;
    if(m_Header.empty()==false)
    {
        sOutStr+=GetSreHeader()+"\r\n";
    }
    auto vDataStrings=GetSreDataStrVec(bFillEmpty, NumWords);
    for(const auto &DatStr:vDataStrings)
    {
        sOutStr+=DatStr+"\r\n";
    }
    if(m_ExecAddr.second==true)
    {
        sOutStr+=GetSreExec()+"\r\n";
    }

    return sOutStr;
}

bool CMemory::SetData(uint32_t pAddr, const void *pData, size_t nDataSize)
{

    typedef struct WriteRange_s
    {
        uint32_t nStart;
        uint32_t nEnd;
        decltype(m_Sections)::iterator pInsPos;
    } WriteRange_t;
    bool bNoWrite=false;
    uint32_t nStart=pAddr;
    uint32_t nEndAddr=pAddr+nDataSize;
    std::vector<WriteRange_t> vRanges;

    CMemorySection::Ptr OldSec, NewSec;
    if(m_Sections.empty())
    {
        //Если нет секций - создаём новую.
        //m_Sections.emplace_back(pAddr, pData, nDataSize);
        m_Sections.emplace_back(CMemorySection::Create(pAddr, pData, nDataSize));
        SORT_CHECK();
    }
    else if(m_Sections.front()->GetStart()==nEndAddr)
    {
        //Если новые данные находятся перед первой секцией, дописываем их в начало.
        OldSec=m_Sections.front();
        m_Sections.pop_front();
        NewSec=CMemorySection::Create(pAddr, pData, nDataSize);
        NewSec->AddData(OldSec->GetFullData().data(), OldSec->GetSize());
        m_Sections.insert(m_Sections.begin(), NewSec);
        SORT_CHECK();
        //m_Sections.push_back(NewSec);
    }
    else if(m_Sections.back()->GetEnd()==nStart)
    {
        //Если новые данные находятся сразу за последней секцией- дописываем их в конец.
        m_Sections.back()->AddData(pData, nDataSize);
        SORT_CHECK();
    }
    else if(nEndAddr<m_Sections.front()->GetStart())
    {
        //Если данные находятся до первой секции и не пересекаются - создаём новую секцию.
        // m_Sections.emplace_back(pAddr, pData, nDataSize);
        m_Sections.insert(m_Sections.begin(), CMemorySection::Create(pAddr, pData, nDataSize));
        SORT_CHECK();
    }
    else if(nStart>m_Sections.back()->GetEnd())
    {
        //Если данные находятся после последней секции и не пересекаются - создаём новую секцию.
        //m_Sections.emplace_back(pAddr, pData, nDataSize);
        m_Sections.insert(m_Sections.end(), CMemorySection::Create(pAddr, pData, nDataSize));
        SORT_CHECK();
    }
    else
    {
        //Иначе данные лежат между секциями. И тут могут быть перекрытия.
        //Вектор vRanges хранит в себе диапазоны данных, не попавшие в имеющиеся сектора.
        if(nStart<m_Sections.front()->GetStart())
        {
            //Сначала сохраним диапазон до первого сектора, если он есть.
            vRanges.push_back({nStart, m_Sections.front()->GetStart(), m_Sections.begin()});
            nStart=m_Sections.front()->GetStart();
        }
        if(nEndAddr>m_Sections.back()->GetEnd())
        {
            //Так же сохраним диапазон за последним сектором.
            vRanges.push_back({m_Sections.back()->GetEnd(), nEndAddr, m_Sections.end()});
            nEndAddr=m_Sections.back()->GetEnd();
        }

        for(auto pSect=m_Sections.begin(); pSect!=m_Sections.end(); ++pSect)
        {

            if(nStart==nEndAddr)
            {
                //Такое бывает. Выходим из цикла.
                break;
            }

            if(nStart>=(*pSect)->GetStart()&&nStart<(*pSect)->GetEnd())
            {
                //Проверили, что данные попадают в область текущей секции.
                if(nEndAddr<(*pSect)->GetEnd())
                {
                    //Если данные заканчиваются в текущей секции - сохраняем их и выходим.
                    if(OnOverwrite(nStart, nEndAddr-nStart))
                    {
                        (*pSect)->SetData(nStart, reinterpret_cast<const void *>((size_t) (pData)+(nStart-pAddr)), nEndAddr-nStart);
                    }
                    break;
                }
                else
                {
                    //Иначе сохраняем область, попавшую в секцию и смещаем адрес начала данных.
                    if(OnOverwrite(nStart, (*pSect)->GetEnd()-nStart))
                    {
                        (*pSect)->SetData(nStart, reinterpret_cast<const void *>((size_t) (pData)+(nStart-pAddr)), (*pSect)->GetEnd()-nStart);
                    }
                    nStart=(*pSect)->GetEnd();
                }
            }
            auto pNext=std::next(pSect);
            if(pNext!=m_Sections.end())
            {
                //Если существует следующая секция, нужно сохранить диапазон данных, находящийся между секциями.
                if(nStart>=(*pSect)->GetEnd()&&nStart<(*pNext)->GetStart())
                {
                    if(nEndAddr<(*pNext)->GetStart())
                    {
                        vRanges.push_back({nStart, nEndAddr, pNext});
                        break;
                    }
                    else
                    {
                        vRanges.push_back({nStart, (*pNext)->GetStart(), pNext});
                        nStart=(*pNext)->GetStart();
                    }
                }
            }
        }

        std::sort(vRanges.begin(), vRanges.end(), [](const WriteRange_t &a, const WriteRange_t &b)
        {
            //Сортировка, возможно, не нужна.
            return a.nStart<b.nStart;
        });

        for(const auto &Range:vRanges)
        {
            m_Sections.insert(Range.pInsPos, CMemorySection::Create(Range.nStart, reinterpret_cast<const void *>((size_t) (pData)+(Range.nStart-pAddr)), Range.nEnd-Range.nStart));
        }
        SORT_CHECK();
    }

    MergeSections();
    SORT_CHECK();
    return true;
}

CMemory::ParseSreLineResult_t CMemory::ParseSRELine(const char *sLine)
{
#define aToHex(c) ((uint8_t)((c)>='0'&&(c)<='9')?((c)-'0'):(((c)>='A'&&(c)<='F')?(0xA+((c)-'A')):(((c)>='a'&&(c)<='f')?(0xA+((c)-'a')):0)))
#define aToHex2(c, d) ((uint8_t)( aToHex((d)) | (aToHex((c)) << 4)))
    ParseSreLineResult_t ParseRes;
    uint8_t RecvByte, AddrLen=0;
    size_t DeclaredByteLen=0, LineByteLength=0;
    const char *c;
    //uint8_t pDataBuf[0xFF]; //Больше в SRE-шную строку точно не влезет
    int LineCheckSum;
    std::vector<uint8_t> vLineData;
    size_t LineLength=0;

    ParseRes.nType=INVALID;

    auto OnSRELineParseError=[&ParseRes, &sLine, &LineLength](const char *Msg, ...)
    {
        /*std::string sFmtError;*/
        int FmtLen=0;

        va_list args;
        va_start(args, Msg);
        // vsnprintf(szError, StrLen - 1, Msg, args);
        FmtLen=vsnprintf(nullptr, NULL, Msg, args);
        va_end(args);
        if(FmtLen>0)
        {
            FmtLen+=1;
            ParseRes.sErrorText.resize(FmtLen);
            va_start(args, Msg);
            vsnprintf(&ParseRes.sErrorText[0], FmtLen, Msg, args);
            va_end(args);
        }
        //printf("%s\n", sFmtError.c_str());
        if(LineLength>0&&sLine)
        {
            ParseRes.sRawLine=std::string(sLine, sLine+LineLength);
        }
        ParseRes.vData.clear();
        ParseRes.nType=INVALID;
        ParseRes.nRecvAddr=0;
        return ParseRes;
    };
    if(!sLine||!*sLine)
    {
        return OnSRELineParseError("Empty line");
    }

    c=sLine;
    do
    {
        if(*c=='\r'||*c=='\n'||*c==0)
        {
            LineLength=(c-sLine);
            break;
        }
        c++;
    }
    while(true);

    if(sLine[0]!='s'&&sLine[0]!='S')
    {
        return OnSRELineParseError("1-st char should be 's' or 'S.");
    }

    if(LineLength<=6)
    {
        return OnSRELineParseError("Line too short(%u chars).", LineLength);
    }
    if(LineLength%2!=0)
    {
        return OnSRELineParseError("Line length should be even. Current length=%u.", LineLength);
    }
    if(sLine[1]<'0'||sLine[1]>'9')
    {
        return OnSRELineParseError("Unknown line type ('%c').", sLine[1]);
    }
    ParseRes.nType=(CMemory::SRELineType_t) (sLine[1]-'0');
    if(ParseRes.nType==RESERVED||ParseRes.nType==DATA16||ParseRes.nType==DATA24||ParseRes.nType==TERM24)
    {
        return OnSRELineParseError("Not supported line type ('%c').", sLine[1]);
    }

    DeclaredByteLen=aToHex2(sLine[2], sLine[3]);
    LineByteLength=(LineLength/2)-2;
    if(DeclaredByteLen!=LineByteLength)
    {
        return OnSRELineParseError("Invalid line length. Expected %u bytes. Received %u.", DeclaredByteLen, LineByteLength);
    }

    vLineData.resize(LineByteLength-1);
    LineCheckSum=aToHex2(sLine[0*2+2], sLine[0*2+3]);
    for(unsigned int i=1; i<LineByteLength; i++)
    {
        RecvByte=aToHex2(sLine[i*2+2], sLine[i*2+3]);
        vLineData[i-1]=RecvByte;
        LineCheckSum+=RecvByte;
    }
    LineCheckSum=(~LineCheckSum)&0xFF;

    auto DeclaredLineCheckSum=aToHex2(sLine[LineLength-2], sLine[LineLength-1]);
    if(LineCheckSum!=DeclaredLineCheckSum)
    {
        return OnSRELineParseError("Checksum error. Expected '%02x'. Received '%02x'.", LineCheckSum, DeclaredLineCheckSum);
    }
    switch(ParseRes.nType)
    {
        case HEADER:
        case DATA16:
        case COUNT16:
        case TERM16:
            AddrLen=2;
            break;
        case DATA24:
        case COUNT24:
        case TERM24:
            AddrLen=3;
        case DATA32:
        case TERM32:
            AddrLen=4;
        default:
            break;
    }
    if(vLineData.size()<AddrLen)
    {
        return OnSRELineParseError("Line data length< length of addr.(%i<%i).", vLineData.size(), AddrLen);
    }
    ParseRes.nRecvAddr=0;
    for(unsigned int i=0; i<AddrLen; i++)
    {
        ParseRes.nRecvAddr=(ParseRes.nRecvAddr<<8)|vLineData[i];
    }
    ParseRes.vData=std::vector<uint8_t>(vLineData.begin()+AddrLen, vLineData.end());
    return ParseRes;
#undef aToHex
#undef aToHex2
}

std::string CMemory::DataBufToSreString(SRELineType_t type, const std::vector<uint8_t> &vData) const
{

    std::string OutStr;
    uint8_t Cur=0;
    size_t CurPos=0;
    uint32_t LineCheckSum=0;
    size_t TextLen=0;
    auto LineLength=vData.size()+1; //В строке пока нет контрольной суммы. Для её расчёта нужно вписать длину строки в данные.
    if(LineLength>1&&LineLength<=0xFF) //Если строка состоит только из контрольной суммы, или её длина не лезет в 1 байт (что странно), то это печально.
    {

        TextLen=2+2+(vData.size()*2)+2; //(S+Type(1)+Length(2)+Data+CheckSum);
        OutStr.resize(TextLen);

        LineCheckSum=LineLength;

        //OutStr.push_back('S');
        //OutStr.push_back((char)('0' + type));
        OutStr[CurPos++]='S';
        OutStr[CurPos++]=((char) ('0'+type));
        Cur=LineLength>>4;
        Cur+=(Cur>=0xa)?('A'-0xA):'0';
        //OutStr.push_back(Cur);
        OutStr[CurPos++]=Cur;
        Cur=LineLength&0xf;
        Cur+=(Cur>=0xa)?('A'-0xA):'0';
        //OutStr.push_back(Cur);
        OutStr[CurPos++]=Cur;

        for(auto b:vData)
        {
            LineCheckSum+=b;
            Cur=b>>4;
            Cur+=(Cur>=0xa)?('A'-0xA):'0';
            //OutStr.push_back(Cur);
            OutStr[CurPos++]=Cur;
            Cur=b&0xf;
            Cur+=(Cur>=0xa)?('A'-0xA):'0';
            //OutStr.push_back(Cur);
            OutStr[CurPos++]=Cur;
        }
        LineCheckSum=(~LineCheckSum)&0xFF;

        Cur=LineCheckSum>>4;
        Cur+=(Cur>=0xa)?('A'-0xA):'0';
        //OutStr.push_back(Cur);
        OutStr[CurPos++]=Cur;
        Cur=LineCheckSum&0xf;
        Cur+=(Cur>=0xa)?('A'-0xA):'0';
        //OutStr.push_back(Cur);
        OutStr[CurPos++]=Cur;
        // Data.insert(Data.end(), *reinterpret_cast<uint8_t*>(&LineCheckSum));
    }

    return OutStr;
}

std::string CMemory::GetSreHeader() const
{

    uint32_t Addr;
    std::vector<uint8_t> Data;
    Addr=0;
    Data.insert(Data.end(), (uint8_t *) &Addr, (uint8_t *) (&Addr)+2);
    if(!m_Header.empty())
    {
        Data.insert(Data.end(), m_Header.begin(), m_Header.end());
    }
    else
    {
        //Если нет заголовка, вставим туда 8 ноликов. Просто, чтоб было.
        Data.insert(Data.end(), (uint8_t *) &Addr, (uint8_t *) (&Addr)+4);
        Data.insert(Data.end(), (uint8_t *) &Addr, (uint8_t *) (&Addr)+4);
    }
    return DataBufToSreString(HEADER, Data);
}

std::string CMemory::GetSreExec() const
{
    uint32_t Addr=m_ExecAddr.second?m_ExecAddr.first:0;
    std::vector<uint8_t> Data;
    for(int i=3; i>=0; i--)
    {
        Data.push_back(((uint8_t *) &Addr)[i]);
    }
    return DataBufToSreString(TERM32, Data);
}

std::vector<std::string> CMemory::GetSreDataStrVec(bool bFillEmpty, uint32_t nNumWordsInLine /*= 4*/) const
{
    const int MaxWordsInLine=(0xFF-sizeof(uint32_t)-sizeof(char))/4; //(Длина всей строки - длина адреса - длина контрольной суммы строки)
    std::vector<std::string> vOutStr;
    std::vector<uint8_t> vData;
    uint32_t nAddr=GetStartAddr();
    uint32_t DataLength=0;
    auto vRange=GetDataRange();
    if(nNumWordsInLine>MaxWordsInLine)
    {
        nNumWordsInLine=MaxWordsInLine;
    }
    for(const auto &Sec:m_Sections)
    {
        if(bFillEmpty)
        {
            while(nAddr<Sec->GetStart())
            {
                vData.clear();
                DataLength=nNumWordsInLine*sizeof(uint32_t);
                if(nAddr+DataLength>=Sec->GetStart())
                {
                    DataLength=(Sec->GetStart()-nAddr);
                }
                vData.resize(DataLength+sizeof(uint32_t));
                for(int i=3; i>=0; i--)
                {
                    vData[3-i]=(((uint8_t *) &nAddr)[i]);
                }
                memset((void *) ((size_t) vData.data()+sizeof(uint32_t)), 0xFF, DataLength);
                vOutStr.push_back(DataBufToSreString(DATA32, vData));
                nAddr+=DataLength;
            }
        }
        else
        {
            nAddr=Sec->GetStart();
        }

        printf("");
        while(nAddr<Sec->GetEnd())
        {
            vData.clear();
            DataLength=nNumWordsInLine*sizeof(uint32_t);
            if(nAddr+DataLength>=Sec->GetEnd())
            {
                DataLength=(Sec->GetEnd()-nAddr);
            }
            vData.resize(DataLength+sizeof(uint32_t));
            for(int i=3; i>=0; i--)
            {
                vData[3-i]=(((uint8_t *) &nAddr)[i]);
            }
            auto vRecv=Sec->GetData(nAddr, (void *) ((size_t) vData.data()+sizeof(uint32_t)), DataLength);

            vOutStr.push_back(DataBufToSreString(DATA32, vData));
            nAddr+=DataLength;
        }
    }

    return vOutStr;
}

std::string CMemory::WriteSRELine(SRELineType_t type, uint32_t &Addr, bool bFillEmpty, uint32_t NumWords) const
{

#if 1

    const int MaxWordsInLine=(0xFF-sizeof(uint32_t)-sizeof(char))/4; //(Длина всей строки - длина адреса - длина контрольной суммы строки)
    std::vector<uint8_t> Data;
    std::string OutLine;;
    uint32_t nDataSize=0;
    uint32_t EndAddr=0;

    if(NumWords>MaxWordsInLine)
    {
        NumWords=MaxWordsInLine;
    }

    switch(type)
    {
        case HEADER:
        {
            Addr=0;
            Data.insert(Data.end(), (uint8_t *) &Addr, (uint8_t *) (&Addr)+2);
            if(!m_Header.empty())
            {
                Data.insert(Data.end(), m_Header.begin(), m_Header.end());
            }
            else
            {
                //Если нет заголовка, вставим туда 8 ноликов. Просто, чтоб было.
                Data.insert(Data.end(), (uint8_t *) &Addr, (uint8_t *) (&Addr)+4);
                Data.insert(Data.end(), (uint8_t *) &Addr, (uint8_t *) (&Addr)+4);
            }
            break;
        }
        case TERM32:
        {
            for(int i=3; i>=0; i--)
            {
                Data.push_back(((uint8_t *) &Addr)[i]);
            }
            //Адрес должен быть в "прямом" порядке.
            break;
        }
        case COUNT16:
        {
            //У нас не используется. Здесь вместо адреса должно быть кол-во предыдущих строк с данными.
            for(int i=1; i>=0; i--)
            {
                Data.push_back(((uint8_t *) &Addr)[i]);
            }
            // Data.insert(Data.end(), (uint8_t*)&Addr, (uint8_t*)(&Addr) + 2);
            break;
        }
        case DATA32:
        {
            if(Addr<GetEndAddr())
            {
                if(Addr<GetStartAddr())
                {
                    Addr=GetStartAddr();
                }

                if(bFillEmpty||GetNextAddr(Addr))
                {
                }
            }
            else
            {
                Addr=0;
            }

            break;
        }

        default:
        {
            break;
        }
    }

    auto STR=DataBufToSreString(type, Data);
    return STR;

#endif
}

void CMemory::MergeSections()
{
    //Объеденение секций, если конец секции совпадает с началом следующей
    auto pSect=m_Sections.begin();
    auto pNext=std::next(pSect);

    std::vector<uint8_t> vData;
    while(pSect!=m_Sections.end()&&pNext!=m_Sections.end())
    {

        if((*pNext)->GetStart()==(*pSect)->GetEnd())
        {
            vData=(*pNext)->GetFullData();
            (*pSect)->AddData(vData.data(), vData.size());
            m_Sections.erase(pNext);
            // RemoveSection(*(i + 1));
        }
        else
        {
            ++pSect;
        }
        pNext=std::next(pSect);
    }
    SORT_CHECK();
}

void CMemory::RemoveEmptyData()
{

    //return;
    bool bShouldRemove;
    std::list<CMemorySection::Ptr> SplitSects;
    bool bHaveChanges=false;
    do
    {
        bHaveChanges=false;
        for(auto &Section:m_Sections)
        {
            SplitSects=Section->RemoveEmptyData(bShouldRemove);
            if(bShouldRemove)
            {
                m_Sections.remove(Section);
                bHaveChanges=true;
                break;
            }
        }

        if(bHaveChanges)
        {
            if(!SplitSects.empty())
            {
                m_Sections.insert(m_Sections.end(), SplitSects.begin(), SplitSects.end());
                // CheckSorted();
                m_Sections.sort();
            }
        }
    }
    while(bHaveChanges);
    SORT_CHECK();
}

bool CMemory::GetData(uint32_t pAddr, void *pData, size_t nDataSize) const
{
    if(!pData||!nDataSize||pAddr+nDataSize<GetStartAddr()||pAddr>=GetEndAddr())
    {
        return false;
    }
    uint32_t StartAddr=pAddr;
    uint32_t EndAddr=pAddr+nDataSize;


    //Init data with all 0xFF.
    memset(pData, 0xFF, nDataSize);

    for(auto pSec=m_Sections.begin(); pSec!=m_Sections.end(); ++pSec)
    {
        if((*pSec)->GetEnd()>StartAddr&&(*pSec)->GetStart()<EndAddr)
        {
            if(StartAddr<(*pSec)->GetStart())
            {
                StartAddr=(*pSec)->GetStart();
            }
            if(EndAddr<(*pSec)->GetEnd())
            {
                //Тут уже, вроде как, всё.
                (*pSec)->GetData(StartAddr, (void *) ((size_t) (pData)+(StartAddr-pAddr)), EndAddr-StartAddr);
                break;
            }
            else
            {
                (*pSec)->GetData(StartAddr, (void *) ((size_t) (pData)+(StartAddr-pAddr)), (*pSec)->GetEnd()-StartAddr);
            }
        }
    }
    return true;
}

bool CMemory::ReadS19File(const char *sFileName, int32_t Offset)
{
    bool bLoaded=false;
    auto pFileData=ReadFileInMemory(sFileName);
    if(!pFileData.empty())
    {
        m_CurReadFile=sFileName;
        bLoaded=ReadS19(reinterpret_cast<const char *>(pFileData.data()), pFileData.size(), Offset);
        m_CurReadFile.clear();
    }
    return bLoaded;
}

bool CMemory::ReadS19(const char *sData, size_t nDataSize, int32_t Offset/*=0*/)
{
    if(!sData||!nDataSize)
    {
        return false;
    }
    ParseSreLineResult_t ParseRes;
    const char *szLineStart=sData;
    bool bLineEnd=true; //Should be true;
    bool bExpectData=true;
    bool bHaveHeader=false;
    bool bReceivingData=false;
    bool bHaveCountRec=false;
    bool bHaveTerminate=false;
    while((szLineStart-sData)<nDataSize&&*szLineStart!='\0')
    {
        if(*szLineStart=='\r'||*szLineStart=='\n')
        {
            bLineEnd=true;
        }
        else
        {
            if(bLineEnd==true)
            {
                ParseRes=ParseSRELine(szLineStart);
                if(!ParseRes.sErrorText.empty()||ParseRes.nType==INVALID)
                {
                    return false;
                }
                if(ParseRes.nType==HEADER)
                {
                    if(bReceivingData)
                    {
                        return false;
                    }
                    if(bHaveHeader)
                    {
                        return false;
                    }
                    ParseRes.vData.push_back(0);
                    SetHeader((const char *) ParseRes.vData.data());
                    bHaveHeader=true;
                }
                else if(ParseRes.nType==DATA32)
                {
                    if(bHaveCountRec||bHaveTerminate)
                    {
                        return false;
                    }
                    SetData(ParseRes.nRecvAddr, ParseRes.vData.data(), ParseRes.vData.size());
                }
                else if(ParseRes.nType==COUNT16||ParseRes.nType==COUNT24)
                {
                    bHaveCountRec=true;
                }
                else if(ParseRes.nType==TERM16||ParseRes.nType==TERM24||ParseRes.nType==TERM32)
                {
                    bHaveTerminate=true;
                    if(ParseRes.nType==TERM32)
                    {
                        SetExecAddr(ParseRes.nRecvAddr);
                    }
                }
                bLineEnd=false;
            }
        }
        szLineStart++;
    }
    MergeSections();
    RemoveEmptyData();
    SORT_CHECK();
    return true;
}

bool CMemory::ReadBinary(const char *sFileName, uint32_t nAddr)
{
    auto FileData=ReadFileInMemory(sFileName);
    if(!FileData.empty())
    {
        if(SetData(nAddr, FileData.data(), FileData.size()))
        {
            MergeSections();
            RemoveEmptyData();
            return true;
        }
    }
    return false;
}

bool CMemory::WriteBinary(const char *sFileName)
{
    if(GetStartAddr()==GetEndAddr())
    {
        return false;
    }
    uint8_t WriteBuf[2048];
    uint32_t TestData=2545;
    uint32_t Addr=GetStartAddr();
    uint32_t WriteLen;

    FILE *fOut=fopen(sFileName, "wb");
    if(!fOut)
    {
        return false;
    }
    do
    {
        WriteLen=(std::min)(sizeof(WriteBuf), (size_t) (GetEndAddr()-Addr));
        if(GetData(Addr, WriteBuf, WriteLen))
        {
            fwrite(WriteBuf, WriteLen, 1, fOut);
        }
        else
        {
            printf("");
        }
        Addr+=WriteLen;
    }
    while(Addr<GetEndAddr());
    fclose(fOut);
    return true;
}

bool CMemory::WriteS19File(const char *sFileName, bool bFillEmpty, uint32_t NumWords) const
{
    if(GetStartAddr()==GetEndAddr())
    {
        return false;
    }

    FILE *fOut=fopen(sFileName, "wb");
    if(!fOut)
    {
        return false;
    }

    auto WriteStr=WriteS19(bFillEmpty, NumWords);
    fwrite(WriteStr.c_str(), WriteStr.length(), 1, fOut);

    fclose(fOut);
    return true;
}

bool CMemory::LoadMemPack(const void *pData, size_t nDataSize)
{
    Reset();
    std::string sHeader;
    PacketMemoryHeader_t MemHeader;
    PackedSectionHeader_t *pSecHeader;
    size_t ReqStorageSize;
    uint32_t CalcCRC, RecvCRC;
    std::vector<uint8_t> vData=std::vector<uint8_t>((uint8_t *) pData, (uint8_t *) pData+nDataSize);
    size_t LastDataAddr;

    if(vData.empty()||vData.size()<sizeof(PacketMemoryHeader_t))
    {
        return false;
    }
    MemHeader=*reinterpret_cast<PacketMemoryHeader_t *>(vData.data());
    if(MemHeader.nSignature!=PACKED_MEMORY_SIGNATURE)
    {
        return false;
    }
    reinterpret_cast<PacketMemoryHeader_t *>(vData.data())->nCRC32=0;
    CalcCRC=HashSingleBuffer<CRC32>(vData.data(), vData.size()).GetType<uint32_t>();
    if(CalcCRC!=MemHeader.nCRC32)
    {
        return false;
    }
    ReqStorageSize=sizeof(PacketMemoryHeader_t)+MemHeader.nReadSize;
    if(vData.size()<ReqStorageSize)
    {
        return false;
    }

    if(MemHeader.nReadSize>MemHeader.nDataSize)
    {
        return false;
    }
    vData=std::vector<uint8_t>(vData.begin()+sizeof(PacketMemoryHeader_t), vData.end());

    if(MemHeader.nReadSize<MemHeader.nDataSize)
    {
        vData=LZSS_VUncompress(vData);
        if(vData.empty()||vData.size()!=MemHeader.nDataSize)
        {
            return false;
        }
    }

    pSecHeader=reinterpret_cast<PackedSectionHeader_t *>(vData.data());
    if(MemHeader.nHeaderSize)
    {
        pSecHeader=reinterpret_cast<PackedSectionHeader_t *>((size_t) pSecHeader+MemHeader.nHeaderSize);
        sHeader.insert(sHeader.end(), vData.begin(), vData.begin()+MemHeader.nHeaderSize);
        SetHeader(sHeader.c_str());
    }
    LastDataAddr=size_t(vData.data())+vData.size();
    size_t SecEnd;
    do
    {
        RecvCRC=pSecHeader->nCRC32;
        pSecHeader->nCRC32=0;
        SecEnd=(size_t) pSecHeader+sizeof(PackedSectionHeader_t)+pSecHeader->nDataSize;
        if(SecEnd>LastDataAddr)
        {
            Reset();
            return false;
        }
        CalcCRC=HashSingleBuffer<CRC32>(pSecHeader, sizeof(PackedSectionHeader_t)+pSecHeader->nDataSize).GetType<uint32_t>();
        if(RecvCRC!=CalcCRC)
        {
            Reset();
            return false;
        }
        SetData(pSecHeader->nSecAddr, (void *) ((size_t) pSecHeader+sizeof(PackedSectionHeader_t)), pSecHeader->nDataSize);
        pSecHeader=reinterpret_cast<PackedSectionHeader_t *>((size_t) pSecHeader+sizeof(PackedSectionHeader_t)+pSecHeader->nDataSize);
    }
    while((size_t) pSecHeader<LastDataAddr);
    if(MemHeader.nExecAddr!=0xFFFFFFFF)
    {
        SetExecAddr(MemHeader.nExecAddr);
    }

    return true;
}

std::vector<uint8_t> CMemory::GetMemPack(bool bCompressData/*=true*/) const
{
    std::vector<uint8_t> vDataStorage, vSection, vSecData;
    PacketMemoryHeader_t MemHeader;
    PackedSectionHeader_t *pSecHeader=nullptr;
    if(!m_Header.empty())
    {
        vDataStorage.insert(vDataStorage.end(), m_Header.begin(), m_Header.end());
    }

    for(const auto &Sec:m_Sections)
    {
        vSection.clear();
        vSection.resize(sizeof(PackedSectionHeader_t));
        pSecHeader=reinterpret_cast<PackedSectionHeader_s *>(vSection.data());
        pSecHeader->nSignature=PACKED_SECTION_SIGNATURE;
        pSecHeader->nDataSize=Sec->GetSize();
        pSecHeader->nSecAddr=Sec->GetStart();
        pSecHeader->nCRC32=0;
        vSecData=Sec->GetFullData();
        vSection.insert(vSection.end(), vSecData.begin(), vSecData.end());
        reinterpret_cast<PackedSectionHeader_s *>(vSection.data())->nCRC32=HashSingleBuffer<CRC32>(vSection.data(), vSection.size()).GetType<uint32_t>();
        vDataStorage.insert(vDataStorage.end(), vSection.begin(), vSection.end());
    }
    MemHeader.nSignature=PACKED_MEMORY_SIGNATURE;
    MemHeader.nDataSize=vDataStorage.size();
    MemHeader.nSectionsCount=m_Sections.size();
    MemHeader.nHeaderSize=m_Header.size();
    MemHeader.nExecAddr=m_ExecAddr.second?m_ExecAddr.first:0xFFFFFFFF;
    MemHeader.nCRC32=0;

    if(bCompressData)
    {
        auto vPackedData=LZSS_VCompress(vDataStorage);
        if(!vPackedData.empty())
        {
            vDataStorage=vPackedData;
        }
    }
    MemHeader.nReadSize=vDataStorage.size();
    vDataStorage.insert(vDataStorage.begin(), (uint8_t *) &MemHeader, (uint8_t *) &MemHeader+sizeof(MemHeader));
    reinterpret_cast<PacketMemoryHeader_t *>(vDataStorage.data())->nCRC32=HashSingleBuffer<CRC32>(vDataStorage.data(), vDataStorage.size()).GetType<uint32_t>();

    return vDataStorage;
}

std::string CMemory::GetBase64DataString(bool bCompressData) const
{

    std::string sBase64Data;
    std::string sOutText;
    std::vector<uint8_t> vDataStorage, vSection, vSecData;
    PacketMemoryHeader_t MemHeader;
    PackedSectionHeader_t *pSecHeader=nullptr;
    if(!m_Header.empty())
    {
        vDataStorage.insert(vDataStorage.end(), m_Header.begin(), m_Header.end());
    }

    for(const auto &Sec:m_Sections)
    {
        vSection.clear();
        vSection.resize(sizeof(PackedSectionHeader_t));
        pSecHeader=reinterpret_cast<PackedSectionHeader_s *>(vSection.data());
        pSecHeader->nSignature=PACKED_SECTION_SIGNATURE;
        pSecHeader->nDataSize=Sec->GetSize();
        pSecHeader->nSecAddr=Sec->GetStart();
        pSecHeader->nCRC32=0;
        vSecData=Sec->GetFullData();
        vSection.insert(vSection.end(), vSecData.begin(), vSecData.end());
        reinterpret_cast<PackedSectionHeader_s *>(vSection.data())->nCRC32=HashSingleBuffer<CRC32>(vSection.data(), vSection.size()).GetType<uint32_t>();
        vDataStorage.insert(vDataStorage.end(), vSection.begin(), vSection.end());
    }
    MemHeader.nSignature=PACKED_MEMORY_SIGNATURE;
    MemHeader.nDataSize=vDataStorage.size();
    MemHeader.nSectionsCount=m_Sections.size();
    MemHeader.nHeaderSize=m_Header.size();
    MemHeader.nExecAddr=m_ExecAddr.second?m_ExecAddr.first:0xFFFFFFFF;
    MemHeader.nCRC32=0;

    if(bCompressData)
    {
        auto vPackedData=LZSS_VCompress(vDataStorage);
        if(!vPackedData.empty())
        {
            vDataStorage=vPackedData;
        }
    }
    MemHeader.nReadSize=vDataStorage.size();
    vDataStorage.insert(vDataStorage.begin(), (uint8_t *) &MemHeader, (uint8_t *) &MemHeader+sizeof(MemHeader));
    reinterpret_cast<PacketMemoryHeader_t *>(vDataStorage.data())->nCRC32=HashSingleBuffer<CRC32>(vDataStorage.data(), vDataStorage.size()).GetType<uint32_t>();
    sBase64Data=std::string(BASE64::Encode(vDataStorage.data(), vDataStorage.size()).c_str());

    return sBase64Data;
}

void CMemory::SetHeader(const char *sHeader)
{
    if(!sHeader||!*sHeader)
    {
        m_Header.clear();
    }
    else
    {
        m_Header=sHeader;
    }
}

const char *CMemory::GetHeader() const
{
    return m_Header.c_str();
}

void CMemory::SetExecAddr(uint32_t nAddr)
{
    m_ExecAddr.first=nAddr;
    m_ExecAddr.second=true;
}

uint32_t CMemory::GetExecAddr() const
{
    return m_ExecAddr.first;
}

bool CMemory::HaveExecAddr() const
{
    return m_ExecAddr.second;
}

void CMemory::RemoveExecAddr()
{
    m_ExecAddr.second=false;
}

uint32_t CMemory::GetStartAddr() const
{
    return GetNumSections()==0?0:m_Sections.front()->GetStart();
}

uint32_t CMemory::GetEndAddr() const
{
    return GetNumSections()==0?0:m_Sections.back()->GetEnd();
}

uint32_t CMemory::GetNumSections() const
{
    return m_Sections.size();
}

std::vector<DataRange_t> CMemory::GetDataRange() const
{
    std::vector<DataRange_t> vRanges;
    for(auto &s:m_Sections)
    {
        //vRanges.emplace_back(s->GetStart(), s->GetEnd());
		vRanges.push_back(std::make_pair(s->GetStart(), s->GetEnd()));
    }
    return vRanges;
}

bool CMemory::GetNextAddr(uint32_t &pAddr) const
{
    if(m_Sections.empty()||pAddr>=m_Sections.back()->GetEnd())
    {
        return false;
    }
    for(auto &s:m_Sections)
    {
        if(pAddr<s->GetStart())
        {
            pAddr=s->GetStart();
            return true;
        }
        else if(pAddr>=s->GetStart()&&pAddr<s->GetEnd())
        {
            return true;
        }
    }
    return false;
}

bool CMemory::OnOverwrite(uint32_t pAddr, size_t nDataSize)
{
    return true;
}




