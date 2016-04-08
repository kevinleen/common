#pragma once

#include <string>

std::string base64_encode(const std::string& data, bool change_line = false);

std::string base64_decode(const std::string& data);

int base64_encode(const char* src, int src_len, char* dst,
                  bool change_line = false);

int base64_decode(const char* src, int src_len, char* dst);
