#pragma once
#include <stdint.h>
#include <array>
#include <string>
#include <string.h>

template<uint32_t nDigestSize, class ParentClass> class CHashResult
{
    friend ParentClass;

public:
    using Parent_t = ParentClass; \
   using Arr_t = std::array<uint8_t, nDigestSize>;
    CHashResult()
    {
        memset(m_Digest, 0, nDigestSize);
    }
    ~CHashResult()
    {
    }

    static uint32_t GetSize()
    {
        return nDigestSize;
    }

    const void *GetData() const
    {
        return m_Digest;
    }

    Arr_t GetArray() const
    {
        Arr_t OutArr;
        std::copy(std::begin(m_Digest), std::end(m_Digest), OutArr.begin());
        return OutArr;
    }

    std::string GetString() const
    {
        std::string sRes;
        sRes.resize(nDigestSize*2+1);
        for(uint32_t i=0; i<nDigestSize; i++)
        {
#if(defined(_MSC_VER)&&(defined(_WIN32)||defined(_WIN64)))&&!defined(_CRT_SECURE_NO_WARNINGS)
            sprintf_s(&sRes[i * 2], sRes.length() - (i * 2), "%02x", m_Digest[i]);
#else
            sprintf(&sRes[i*2], "%02x", m_Digest[i]);
#endif
        }
        return sRes;
    }
    template<typename T> T GetType() const
    {
        static_assert(sizeof(T)==nDigestSize, "sss");
        return *reinterpret_cast<const T *>(m_Digest);
    }

    bool operator==(const CHashResult &pOther) const
    {
        return 0==memcmp(m_Digest, pOther.m_Digest, nDigestSize);
    }
    bool operator!=(const CHashResult &pOther) const
    {
        return 0!=memcmp(m_Digest, pOther.m_Digest, nDigestSize);
    }
protected:
    template<typename T> T &_GetType()
    {
        static_assert(sizeof(T)==nDigestSize, "sss");
        return *reinterpret_cast<T *>(m_Digest);
    }
    template<typename T> T &_GetAt(size_t nIndex)
    {
        static_assert(nDigestSize%sizeof(T)==0, "sss");
        return reinterpret_cast<T *>(m_Digest)[nIndex];
    }
protected:
    uint8_t m_Digest[nDigestSize];
};
