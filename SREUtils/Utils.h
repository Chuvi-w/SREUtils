#pragma once
#include <vector>
#include <cstdint>
#include <string>

std::vector<uint8_t> ReadFileInMemory(const char *sFileName);
std::vector<uint8_t> ReadFileInMemory(const wchar_t *sFileName);
std::string SplitLargeStringToStrArray(const std::string &sStrName, const std::string &sLargeStr);

std::string CombineStrArray(const char *const *pStr, size_t nStrCount);