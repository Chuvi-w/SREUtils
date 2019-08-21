#include "CMemorySection.h"
#include <fstream>
#include <algorithm>
#include <iomanip>
#include <string.h>
#define MinEmptySpaceLen (4)

CMemorySection::CMemorySection(uint32_t nStartAddr, const void *pData, size_t nDataSize)
        : m_Addr(nStartAddr), m_bWriteProtect(false)
{
    if(pData&&nDataSize)
    {
        m_Data.resize(nDataSize);
        memcpy(m_Data.data(), pData, nDataSize);
    }
}

CMemorySection::CMemorySection(const CMemorySection &pOther)
        : m_Addr(pOther.m_Addr), m_Data(pOther.m_Data), m_bWriteProtect(false)
{
}
CMemorySection::~CMemorySection()
{
    m_Data.clear();
    m_Addr=0;
}

void CMemorySection::SetPartAddr(uint32_t nStartAddr)
{
    m_Addr=nStartAddr;
}
uint32_t CMemorySection::GetStart() const
{
    return m_Addr;
}
uint32_t CMemorySection::GetEnd() const
{
    return m_Addr+m_Data.size();
}
bool CMemorySection::HasAddr(uint32_t Addr) const
{
    return Addr>=m_Addr&&(Addr)<GetEnd();
}

size_t CMemorySection::AddData(const void *pDataBuf, size_t nBuffSize)
{
    m_Data.insert(m_Data.end(), reinterpret_cast<const uint8_t *>(pDataBuf), reinterpret_cast<const uint8_t *>(pDataBuf)+nBuffSize);
    return nBuffSize;
}

size_t CMemorySection::GetData(uint32_t Addr, void *pDataBuf, size_t nBuffSize) const
{
    if(!HasAddr(Addr)||!pDataBuf||!nBuffSize)
    {
        return 0;
    }
    uint32_t AvaiableSize=GetEnd()-Addr;
    if(AvaiableSize<nBuffSize)
    {
        nBuffSize=AvaiableSize;
    }
    memcpy(pDataBuf, &m_Data[Addr-m_Addr], nBuffSize);
    return nBuffSize;
}

bool CMemorySection::SetData(uint32_t Addr, const void *pDataBuf, size_t nBuffSize)
{
    if(m_bWriteProtect||Addr<GetStart()||Addr+nBuffSize>GetEnd())
    {

        return false;
    }
    memcpy(reinterpret_cast<void *>((size_t) (m_Data.data())+(Addr-GetStart())), pDataBuf, nBuffSize);
    return true;;
}

uint32_t CMemorySection::GetSize() const
{
    return m_Data.size();
}
bool CMemorySection::IsValid() const
{
    return GetSize()!=0;
}

std::vector<DataRange_t> CMemorySection::GetEmptyMap() const
{
    std::vector<DataRange_t> vRes;
    int nEmptyCount=0;
    bool bIsEmpty=false;
    size_t nMaxUint32Count=m_Data.size()/sizeof(uint32_t);
    size_t nEmptyStart=0;
    for(size_t i=0; i<nMaxUint32Count; i++)
    {
        if((reinterpret_cast<const uint32_t *>(m_Data.data()))[i]==0xFFFFFFFF)
        {
            if(!bIsEmpty)
            {
                bIsEmpty=true;
                nEmptyStart=i*sizeof(uint32_t);
            }
            nEmptyCount++;
        }
        else
        {
            if(nEmptyCount>=(MinEmptySpaceLen))
            {
                vRes.emplace_back(std::make_pair(nEmptyStart, i*sizeof(uint32_t)));
            }
            bIsEmpty=false;
            nEmptyCount=0;
        }
    }
    if(nEmptyCount>=(MinEmptySpaceLen))
    {
        vRes.emplace_back(std::make_pair(nEmptyStart, nMaxUint32Count*sizeof(uint32_t)));
    }
    return vRes;
}

std::list<CMemorySection::Ptr> CMemorySection::RemoveEmptyData(bool &bShouldRemove) const
{
    //Разбивает секцию на несколько подсекций, не содержащих пустые области. В случае если bShouldRemove==true секцию следует удалить
    std::list<CMemorySection::Ptr> vRes;
    uint32_t DataStart=0;
    uint32_t DataEnd=0;
    auto EmptyMap=GetEmptyMap();
    bShouldRemove=!EmptyMap.empty();

    for(auto &e : EmptyMap)
    {
        DataEnd=e.first;
        if(DataEnd-DataStart>0)//#ToDo: WTF?
        {
            vRes.emplace_back(CMemorySection::Create(m_Addr+DataStart, &m_Data[DataStart], DataEnd-DataStart));
        }
        DataStart=e.second;
    }
    if(DataStart&&DataStart!=m_Data.size())
    {
        DataEnd=m_Data.size();
        auto sz=m_Data.size()-DataStart;
        if(sz)
        {
            vRes.emplace_back(CMemorySection::Create(m_Addr+DataStart, &m_Data[DataStart], DataEnd-DataStart));
        }
    }
    return vRes;
}

bool CMemorySection::operator==(const CMemorySection &pOther) const
{
    if(pOther.GetStart()!=GetStart()||pOther.GetSize()!=GetSize())
    {
        return false;
    }
    if(memcmp(pOther.GetFullData().data(), GetFullData().data(), GetSize()))
    {
        return false;
    }
    return true;
}

bool CMemorySection::operator<(const CMemorySection &pOther) const
{
    return GetStart()<pOther.GetStart();
}
