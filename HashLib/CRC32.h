#pragma once
#include "CHashType.h"
#include "CHashResult.h"
#define CRC32_DIGEST_SIZE (4)

class CRC32;

class CRC32Value : public CHashResult<CRC32_DIGEST_SIZE, CRC32>
{
private:
};

class CRC32 : public CHashType<CRC32Value>
{

public:
    CRC32();
    virtual void init() override;

    virtual void update(const void *message, uint64_t Len) override;
    virtual void Finalize() override;

private:
    static const uint32_t CRC32_INIT_VALUE=0xFFFFFFFFUL;
    static const uint32_t CRC32_XOR_VALUE=0xFFFFFFFFUL;
};