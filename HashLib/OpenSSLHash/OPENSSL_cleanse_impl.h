#ifndef OPENSSL_cleanse_impl_h__
#define OPENSSL_cleanse_impl_h__
#include <stdint.h>
void OPENSSL_cleanse(void *ptr, uint64_t len);
#endif // OPENSSL_cleanse_impl_h__
