#ifndef RLE_H_
#define RLE_H_

#include <iostream>
#include <string>
#include <boost/dynamic_bitset.hpp>
#include <vector>
#include <immintrin.h>

#include "common/constants.h"

namespace XORC
{

    size_t runLengthEncodeString(const std::string &input, boost::dynamic_bitset<> &output_data, uint64_t &len_output_data, const std::string &single_data);

}

#endif