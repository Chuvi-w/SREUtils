#pragma once
#define _CRT_SECURE_NO_WARNINGS
#include <stdint.h>
#include <string>
#include <vector>
#include <exception>
#include <stdexcept>
#include <sstream>
#include <memory>
#include <list>

using DataRange_t = std::pair<uint32_t, uint32_t>;
class CMemorySection
{
    friend class CMemory;
private:

    CMemorySection(const CMemorySection &pOther);
    CMemorySection(uint32_t nStartAddr=0, const void *pData=nullptr, size_t nDataSize=0);
public:
    using Ptr = std::shared_ptr<CMemorySection>;

    static Ptr Create(uint32_t nStartAddr=0, const void *pData=nullptr, size_t nDataSize=0)
    {
        return std::shared_ptr<CMemorySection>(new CMemorySection(nStartAddr, pData, nDataSize));
    }
    ~CMemorySection();

    void SetPartAddr(uint32_t nStartAddr);
    uint32_t GetStart() const;
    uint32_t GetEnd() const;
    bool HasAddr(uint32_t Addr) const;

    size_t AddData(const void *pDataBuf, size_t nBuffSize);
    size_t GetData(uint32_t Addr, void *pDataBuf, size_t nBuffSize) const;
    bool SetData(uint32_t Addr, const void *pDataBuf, size_t nBuffSize);

    const std::vector<uint8_t> GetFullData() const
    {
        return m_Data;
    }
    uint32_t GetSize() const;
    bool IsValid() const;
    bool operator<(const CMemorySection &pOther) const;
    std::vector<DataRange_t> GetEmptyMap() const;
    bool operator==(const CMemorySection &pOther) const;
    std::list<CMemorySection::Ptr> RemoveEmptyData(bool &bShouldRemove) const;

protected:

private:
    uint32_t m_Addr;
    std::vector<uint8_t> m_Data;
    bool m_bWriteProtect;
};
