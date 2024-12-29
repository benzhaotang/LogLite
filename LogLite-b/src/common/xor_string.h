#ifndef XOR_STRING_H_
#define XOR_STRING_H_

#include <iostream>
#include <string>
#include <immintrin.h>
#include <stdexcept>
#include <chrono>

#include "common/constants.h"

namespace XORC
{

    std::string bitwiseXor(const std::string &a, const std::string &b);
    void bitwiseXor(const std::string &a, const std::string &b, std::string &result);
}

#endif