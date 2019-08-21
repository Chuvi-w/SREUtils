#pragma once
#include <list>
#include <memory>
#include "CMemorySection.h"

#ifdef DISABLED_CODE
typedef struct MemoryData_s
{

   uint32_t       nExecAddr;
   uint32_t       nHeaderSz;
   char*          szHeader;
   uint32_t       nSections;
   MemorySection_t* pSection;
   uint32_t       nFlags;
   uint32_t       CRC32;
   uint64_t       MD5[/*MD5_DIGEST_LENGTH_64*/ 2];
} MemoryData_t;
bool Init(const MemoryData_t& pData);
#endif

class CMemData
{
    /*

    */
    friend class CMemory;
protected:
    CMemData(const CMemData &d) : m_pMem(d.m_pMem), m_BaseAddr(d.m_BaseAddr)
    {
    }
    CMemData(class CMemory *pMem, uint32_t pAddr) : m_pMem(pMem), m_BaseAddr(pAddr)
    {
    }
public:

    CMemData operator=(const CMemData &) const = delete;

    template<typename T> CMemData &operator=(T val);
    template<typename T> operator T() const;

#if 0
    CMemData&  operator=(int8_t   val);
    CMemData&  operator=(uint8_t  val);
    CMemData&  operator=(int16_t  val);
    CMemData&  operator=(uint16_t val);
    CMemData&  operator=(int32_t  val);
    CMemData&  operator=(uint32_t val);
    CMemData&  operator=(int64_t  val);
    CMemData&  operator=(uint64_t val);
    CMemData&  operator=(float    val);
    CMemData&  operator=(double   val);
    operator int8_t() const;
    operator uint8_t() const;
    operator int16_t() const;
    operator uint16_t() const;
    operator int32_t() const;
    operator uint32_t() const;
    operator int64_t() const;
    operator uint64_t() const;
    operator float() const;
    operator double() const;
#endif

private:
    class CMemory *m_pMem;
    uint32_t m_BaseAddr;
};

class CMemory : public std::enable_shared_from_this<CMemory>
{
private:

public:
    CMemory();
    ~CMemory();
    typedef enum SRELineType
    {
        INVALID=-1, /*S0*/ HEADER, /*S1*/ DATA16, /*S2*/ DATA24, /*S3*/ DATA32, /*S4*/ RESERVED, /*S5*/ COUNT16, /*S6*/ COUNT24, /*S7*/ TERM32, /*S8*/ TERM24, /*S9*/ TERM16,
    } SRELineType_t;

public:

    CMemData operator[](uint32_t i)
    {
        return CMemData(this, i);
    }

    //    using Ptr=std::shared_ptr<CMemory>;
    //    static Ptr Create()
    //    {
    //       return std::shared_ptr<CMemory>(new CMemory);
    //    }
#ifdef SORT_VALIDATION
    bool CheckSorted() const;
#endif

    void Reset();
    bool ReadS19File(const char *sFileName, int32_t Offset=0);
    bool ReadS19(const char *sData, size_t nDataSize, int32_t Offset=0);
    bool ReadBinary(const char *sFileName, uint32_t nAddr);
    bool WriteBinary(const char *sFileName);
    bool WriteS19File(const char *sFileName, bool bFillEmpty, uint32_t NumWords) const;

    bool LoadMemPack(const void *pData, size_t nDataSize);
    std::vector<uint8_t> GetMemPack(bool bCompressData=true) const;
    std::string GetBase64DataString(bool bCompressData=true) const;
    std::string WriteS19(bool bFillEmpty, uint32_t NumWords=4) const;

    bool SetData(uint32_t pAddr, const void *pData, size_t nDataSize);
    void SetHeader(const char *sHeader);
    const char *GetHeader() const;
    void SetExecAddr(uint32_t nAddr);
    uint32_t GetExecAddr() const;
    bool HaveExecAddr() const;
    void RemoveExecAddr();
    uint32_t GetStartAddr() const;
    uint32_t GetEndAddr() const;
    uint32_t GetNumSections() const;

    std::vector<DataRange_t> GetDataRange() const;
    bool GetNextAddr(uint32_t &pAddr) const;
    bool GetData(uint32_t pAddr, void *pData, size_t nDataSize) const;

    std::string DataBufToSreString(SRELineType_t type, const std::vector<uint8_t> &vData) const;
    std::string GetSreHeader() const;
    std::string GetSreExec() const;
    std::vector<std::string> GetSreDataStrVec(bool bFillEmpty, uint32_t nNumWordsInLine=4) const;
    std::string WriteSRELine(SRELineType_t type, uint32_t &Addr, bool bFillEmpty, uint32_t NumWords=4) const;
	void RemoveEmptyData();
	void MergeSections();
private:

   
    typedef struct ParseSreLineResult_s
    {
        SRELineType_t nType;
        uint32_t nRecvAddr;
        std::vector<uint8_t> vData;
        std::string sErrorText, sRawLine;
    } ParseSreLineResult_t;
    static ParseSreLineResult_t ParseSRELine(const char *sLine);

    void OnSRELineParseError(const char *sRawLine, const char *sFileName, size_t nReadOffset, const char *Msg, ...) const;
    bool OnOverwrite(uint32_t pAddr, size_t nDataSize);

    std::string m_Header;
    std::pair<uint32_t, bool> m_ExecAddr;

    std::string m_CurReadFile;
    std::list<CMemorySection::Ptr> m_Sections;
    mutable std::string sError;
};

template<typename T> CMemData &CMemData::operator=(T val)
{
    m_pMem->SetData(m_BaseAddr, &val, sizeof(T));
    return *this;
}

template<typename T> CMemData::operator T() const
{
    T Data;
    m_pMem->GetData(m_BaseAddr, &Data, sizeof(T));
    return Data;
}