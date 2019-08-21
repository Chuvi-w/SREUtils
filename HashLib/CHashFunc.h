#pragma once
#include "CHashType.h"
#include "CHashResult.h"
#include "OpenSSLHash/sha_c.h"
#include "OpenSSLHash/md5_c.h"

#if defined (__linux__)
#include <experimental/filesystem>
#else

    #if defined(__MINGW32__) or  defined(__MINGW32__)
    #include <experimental/filesystem>
    #else
    #include <filesystem>
    #endif

#endif
namespace fs = std::experimental::filesystem;

#define CreateHashClassDecl(Type) \
   class C##Type;\
   class Type##Value: public CHashResult<Type##_DIGEST_LENGTH, C##Type>{};\
class C##Type: public  CHashType<Type##Value>\
{\
public:\
   C##Type();\
   void init() override; \
   void update(const void* message, uint64_t Len) override;\
   void       Finalize()  override;\
private:\
   Type##_CTX m_CTX;\
};

using SHA224_CTX = SHA256_CTX;
using SHA384_CTX = SHA512_CTX;
using SHA1_CTX = SHA_CTX;
#define SHA1_DIGEST_LENGTH SHA_DIGEST_LENGTH
CreateHashClassDecl(MD5);
CreateHashClassDecl(SHA1);
CreateHashClassDecl(SHA224);
CreateHashClassDecl(SHA256);
CreateHashClassDecl(SHA384);
CreateHashClassDecl(SHA512);

#if 1
template<typename Hasher, typename ResT = typename Hasher::ResultType> ResT HashString(const char *str)
{
    Hasher Ctx;
    Ctx.init();
    if(str)
    {
        Ctx.update(str, strlen(str));
    }
    Ctx.Finalize();
    return Ctx.GetResult();
}

template<typename Hasher, typename ResT = typename Hasher::ResultType> ResT HashSingleBuffer(const void *p, size_t len)
{
    Hasher Ctx;
    const size_t HashBufMaxLen=Hasher::ResultType::GetSize();//1024 * 1024;// *500;
    size_t ReadPos=0;
    size_t HashLen=0;

    Ctx.init();
    if(p&&len)
    {
        do
        {
            HashLen=len-ReadPos;
            if(HashLen>HashBufMaxLen)
            {
                HashLen=HashBufMaxLen;
            }
            Ctx.update(&(reinterpret_cast<const uint8_t *>(p)[ReadPos]), HashLen);
            ReadPos+=HashLen;
        }
        while(ReadPos<len);
    }
    Ctx.Finalize();
    return Ctx.GetResult();
}

template<typename HasherRes> bool HashFile(const fs::path &sFilePath, HasherRes &Val)
{
    const size_t DataBuffSize=512*1024;
    typename HasherRes::Parent_t Hash;
    uint8_t DataBuff[DataBuffSize];
    size_t fReadPos=0, fReadSize;
    if(sFilePath.empty()||!fs::is_regular_file(sFilePath))
    {
        return false;
    }

    FILE *fIn=fopen(sFilePath.string().c_str(), "rb");
    if(!fIn)
    {
        return false;
    }
    Hash.init();
    size_t fSize=0;
#if defined (__linux__)
    fseeko64(fIn, 0, SEEK_END);
    fSize=ftello64(fIn);
    fseeko64(fIn, 0, SEEK_SET);
#else
    _fseeki64(fIn, 0, SEEK_END);
    fSize = _ftelli64(fIn);
   _fseeki64(fIn, 0, SEEK_SET);
#endif

    do
    {
        fReadSize=fSize-fReadPos;
        if(fReadSize>DataBuffSize)
        {
            fReadSize=DataBuffSize;
        }
        fread(DataBuff, 1, fReadSize, fIn);

        fReadPos+=fReadSize;
        Hash.update(DataBuff, fReadSize);
    }
    while(fReadPos<fSize);
    fclose(fIn);
    Hash.Finalize();
    Val=Hash.GetResult();
    // printf("%s\n", Val.GetString().c_str());
    return true;
}
#endif
