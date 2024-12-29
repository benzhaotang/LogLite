#include "rle.h"

namespace XORC
{
    static bool isContinuous(const std::string &input, int i, int &i_len)
    {
        i++;
        while (input[i] == '\0' && i < input.size())
        {
            i_len++;
            i++;
            if (i_len == RLE_POW_COUNT - 1)
            {
                break;
            }
        }
        if (i_len >= RLE_COUNT / 8 + 1)
        {
            return true;
        }
        else
        {
            return false;
        }
    }

    static void encoder(boost::dynamic_bitset<> &output_data, uint64_t &len_output_data, size_t &length_encoded_bitset, bool isRLE, int &i, int i_len, const std::string &original_data)
    {
        if (isRLE)
        {
            output_data[len_output_data++] = 0;
            ++length_encoded_bitset;
            for (size_t j = 0; j < RLE_COUNT; ++j)
            {
                output_data[len_output_data++] = (i_len >> j) & 1;
                ++length_encoded_bitset;
            }
            i = i + i_len;
        }
        else
        {
            output_data[len_output_data++] = 1;
            ++length_encoded_bitset;
            for (int j = 0; j < 8; ++j)
            {
                output_data[len_output_data++] = (original_data[i] >> j) & 1;
                ++length_encoded_bitset;
            }
            ++i;
        }
    }

    size_t runLengthEncodeString(const std::string &input, boost::dynamic_bitset<> &output_data, uint64_t &len_output_data, const std::string &original_data)
    {

        const int len_input = input.size();

        size_t length_encoded_bitset = 0;

        int i = 0;
        int i_len;
        bool isRLE;
        while (i < len_input)
        {
            if (input[i] == '\0')
            {
                i_len = 1;
                isRLE = isContinuous(input, i, i_len);
                encoder(output_data, len_output_data, length_encoded_bitset, isRLE, i, i_len, original_data);
            }
            else
            {
                i_len = 0;
                encoder(output_data, len_output_data, length_encoded_bitset, false, i, i_len, original_data);
            }
        }

        return length_encoded_bitset;
    }
}