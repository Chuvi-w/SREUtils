#pragma once
#include <stdint.h>
#include <memory>
#include <vector>

template<typename ResT> class CHashType
{
public:
    using ResultType = ResT;
    virtual ~CHashType()
    {
    }
    virtual void init() = 0;
    virtual void update(const void *message, uint64_t Len) = 0;
    void update_const(uint8_t nByte, uint64_t Len)
    {
        std::vector<uint8_t> vData(Len, nByte);
        update(vData.data(), Len);
    }

    virtual void Finalize() = 0;
    ResultType GetResult() const
    {
        if(!m_bFinalized)
        {
            throw ("Not finalized");
        }
        return m_Result;
    }
    ResultType GetResult()
    {
        Finalize();
        return m_Result;
    }

protected:
    ResultType m_Result;
    bool m_bFinalized;
};
