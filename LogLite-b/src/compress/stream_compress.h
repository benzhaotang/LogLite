#ifndef XORC_STREAM_COMPRESS_COMPRESS_H_
#define XORC_STREAM_COMPRESS_COMPRESS_H_

#include <string>
#include <vector>
#include <sstream>
#include <fstream>
#include <iostream>
#include <boost/dynamic_bitset.hpp>
#include <deque>
#include <unordered_map>
#include <chrono>

#include "common/xor_string.h"
#include "common/rle.h"
#include "common/constants.h"

namespace XORC
{

    class Stream_Compress
    {
    private:
        std::unordered_map<size_t, std::deque<std::string>> window;

    public:
        Stream_Compress();
        ~Stream_Compress();

        void stream_compress(const std::string &single_data, boost::dynamic_bitset<> &output_data, uint64_t &len_output_data);
        void stream_decompress(const boost::dynamic_bitset<> &single_data, const bool isRLE, const int window_id, std::string &output_data, std::string &xor_result);
    };

}

#endif