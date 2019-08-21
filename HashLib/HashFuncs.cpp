#include <cstring>
#include <fstream>
#include <string>
#include <stdint.h>
#include "CRC32.h"
#include "CHashFunc.h"

#define CreateHashClassImpl(Type) \
C##Type::C##Type(){init();}\
void C##Type::init(){ m_bFinalized = false;Type##_Init(&m_CTX);}\
void C##Type::update(const void* message, uint64_t Len){Type##_Update(&m_CTX, message, Len);}\
void C##Type::Finalize(){if(!m_bFinalized){Type##_Final(m_Result.m_Digest, &m_CTX);m_bFinalized = true; }}

CreateHashClassImpl(MD5);
CreateHashClassImpl(SHA1);
CreateHashClassImpl(SHA224);
CreateHashClassImpl(SHA256);
CreateHashClassImpl(SHA384);
CreateHashClassImpl(SHA512);
