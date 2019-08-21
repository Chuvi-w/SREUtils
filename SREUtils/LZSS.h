#pragma once

#include <vector>
#include <stdint.h>

#define LZSS_DECOMPRESSOR_ENABLED
#define LZSS_COMPRESSOR_ENABLED
std::vector<uint8_t> LZSS_VCompress(const std::vector<uint8_t> vInput);
uint32_t LZSS_VGetActualSize(const std::vector<uint8_t> &vInput);
std::vector<uint8_t> LZSS_VUncompress(const std::vector<uint8_t> &vInput);
