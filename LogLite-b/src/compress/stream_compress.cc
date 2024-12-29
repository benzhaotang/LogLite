#include "stream_compress.h"

namespace XORC
{

    static __m256i zero_vec32 = _mm256_set1_epi8('\0');

    static __m128i zero_vec16 = _mm_set1_epi8('\0');

    static boost::dynamic_bitset<> vectorCharToBitset(const std::vector<char> &data)
    {
        boost::dynamic_bitset<> bitset(data.size() * 8);

        for (size_t i = 0; i < data.size(); ++i)
        {
            for (size_t j = 0; j < 8; ++j)
            {
                if (data[i] & (1 << j))
                {
                    bitset[i * 8 + j] = 1;
                }
            }
        }

        return bitset;
    }

    static std::vector<char> bitsetToVectorChar(const boost::dynamic_bitset<> &bitset)
    {

        std::vector<char> data(bitset.size() / 8);

        for (size_t i = 0; i < data.size(); ++i)
        {
            char c = 0;
            for (size_t j = 0; j < 8; ++j)
            {
                if (bitset[i * 8 + j])
                {
                    c |= (1 << j);
                }
            }
            data[i] = c;
        }

        return data;
    }

    static void integerToBitset(size_t value, boost::dynamic_bitset<> &output_data, uint64_t &len_output_data, size_t bit_count = STREAM_ENCODER_COUNT)
    {
        for (size_t i = 0; i < bit_count; ++i)
        {
            output_data[len_output_data++] = (value >> i) & 1;
        }
    }

    Stream_Compress::Stream_Compress() {}
    Stream_Compress::~Stream_Compress() {}

    void Stream_Compress::stream_compress(const std::string &single_data, boost::dynamic_bitset<> &output_data, uint64_t &len_output_data)
    {
        const size_t len_single_data = single_data.size();

        if (this->window.find(len_single_data) != this->window.end())
        {
            std::string xor_result;
            xor_result.resize(len_single_data);

            float min_compress_rate = 3.0;
            int min_index = -1;
            std::string min_xor_result;
            min_xor_result.reserve(len_single_data);

            int count = 0;
            float tem_rate;

            // int i = 0;
            for (int j = this->window[len_single_data].size() - 1; j >= 0; --j)
            {

                XORC::bitwiseXor(single_data, this->window[len_single_data][j], xor_result);

                count = 0;

                size_t i = 0;

                for (; i + simd_width32 <= len_single_data; i += simd_width32)
                {

                    __m256i data = _mm256_loadu_si256(reinterpret_cast<const __m256i *>(xor_result.data() + i));

                    __m256i result = _mm256_cmpeq_epi8(data, zero_vec32);

                    count += _mm_popcnt_u32(_mm256_movemask_epi8(result));
                }

                for (; i < len_single_data; ++i)
                {
                    if (xor_result[i] == '\0')
                    {
                        ++count;
                    }
                }

                tem_rate = 1.0f - static_cast<float>(count) / len_single_data;

                if (tem_rate <= min_compress_rate)
                {
                    min_compress_rate = tem_rate;
                    min_index = j;
                    min_xor_result = xor_result;

                    if (min_compress_rate <= 0.15)
                    {
                        break;
                    }
                }
            }

            output_data[len_output_data++] = 1;

            integerToBitset(min_index, output_data, len_output_data, EACH_WINDOW_SIZE_COUNT);

            size_t tem_index = len_output_data;
            len_output_data += STREAM_ENCODER_COUNT;

            size_t len_xor_rle_bitset = XORC::runLengthEncodeString(min_xor_result, output_data, len_output_data, single_data);

            for (size_t i = 0; i < STREAM_ENCODER_COUNT; ++i)
            {
                output_data[tem_index + i] = (len_xor_rle_bitset >> i) & 1;
            }

            if (this->window[len_single_data].size() < EACH_WINDOW_SIZE)
            {
                this->window[len_single_data].push_back(single_data);
            }
            else
            {
                this->window[len_single_data].pop_front();
                this->window[len_single_data].push_back(single_data);
            }
        }
        else if (len_single_data >= MAX_LEN || len_single_data == 0)
        {
            output_data[len_output_data++] = 0;

            integerToBitset(len_single_data, output_data, len_output_data, ORIGINAL_LENGTH_COUNT);

            for (size_t i = 0; i < len_single_data; i++)
            {
                for (size_t j = 0; j < 8; ++j)
                {
                    output_data[len_output_data++] = (single_data[i] >> j) & 1;
                }
            }
        }
        else
        {
            std::deque<std::string> newDeque;
            newDeque.push_back(single_data);
            this->window[len_single_data] = newDeque;

            output_data[len_output_data++] = 0;

            integerToBitset(len_single_data, output_data, len_output_data, ORIGINAL_LENGTH_COUNT);

            for (size_t i = 0; i < len_single_data; i++)
            {
                for (size_t j = 0; j < 8; ++j)
                {
                    output_data[len_output_data++] = (single_data[i] >> j) & 1;
                }
            }
        }
    }

    static std::string bitsetToString(const boost::dynamic_bitset<> &bitset)
    {

        std::string data;
        data.resize(bitset.size() / 8);
        char c = 0;
        for (size_t i = 0; i < data.size(); ++i)
        {
            c = 0;
            for (size_t j = 0; j < 8; ++j)
            {
                if (bitset[i * 8 + j])
                {
                    c |= (1 << j);
                }
            }
            data[i] = c;
        }

        return data;
    }

    static void simdReplaceNullCharacters(std::string &xor_result, const std::string &pattern)
    {
        size_t len = xor_result.size();

        size_t i = 0;

        const char *xor_data = xor_result.data();
        const char *pattern_data = pattern.data();

        for (; i + simd_width32 <= len; i += simd_width32)
        {

            __m256i xor_vec = _mm256_loadu_si256((__m256i *)(xor_data + i));
            __m256i pattern_vec = _mm256_loadu_si256((const __m256i *)(pattern_data + i));

            __m256i cmp_mask = _mm256_cmpeq_epi8(xor_vec, zero_vec32);

            __m256i result_vec = _mm256_blendv_epi8(xor_vec, pattern_vec, cmp_mask);

            _mm256_storeu_si256((__m256i *)(xor_data + i), result_vec);
        }

        if (i + simd_width16 <= len)
        {

            __m128i xor_vec = _mm_loadu_si128((__m128i *)(xor_data + i));
            __m128i pattern_vec = _mm_loadu_si128((const __m128i *)(pattern_data + i));

            __m128i cmp_mask = _mm_cmpeq_epi8(xor_vec, zero_vec16);

            __m128i result_vec = _mm_blendv_epi8(xor_vec, pattern_vec, cmp_mask);

            _mm_storeu_si128((__m128i *)(xor_data + i), result_vec);

            i += simd_width16;
        }

        for (; i < len; ++i)
        {
            if (xor_result[i] == '\0')
            {
                xor_result[i] = pattern[i];
            }
        }
    }

    void Stream_Compress::stream_decompress(const boost::dynamic_bitset<> &single_data, const bool isRLE, const int original_length_or_window_id, std::string &output_data, std::string &xor_result)
    {
        xor_result.clear();
        if (isRLE)
        {

            size_t len_single_data = single_data.size();
            size_t i = 0;

            int zero_count = 0;

            unsigned char byte = 0;

            while (i < len_single_data)
            {

                if (single_data[i])
                {
                    i++;

                    byte = 0;
                    for (std::size_t j = 0; j < 8; ++j)
                    {
                        byte |= (single_data[i + j]) << j;
                    }
                    i += RLE_SKIM;

                    xor_result.push_back(byte);
                }
                else
                {
                    i++;

                    zero_count = 0;
                    for (size_t j = 0; j < RLE_COUNT; ++j, ++i)
                    {
                        if (single_data[i])
                        {
                            zero_count |= (1 << j);
                        }
                    }

                    xor_result.append(zero_count, '\0');
                }
            }

            int len_xor_result = xor_result.size();
            std::string &pattern = this->window[len_xor_result][original_length_or_window_id];

            simdReplaceNullCharacters(xor_result, pattern);

            output_data += xor_result;
            output_data += "\n";
            // output_data += "\r\n";

            if (this->window[len_xor_result].size() < EACH_WINDOW_SIZE)
            {
                this->window[len_xor_result].push_back(xor_result);
            }
            else
            {
                this->window[len_xor_result].pop_front();
                this->window[len_xor_result].push_back(xor_result);
            }
        }
        else
        {

            std::string tem = bitsetToString(single_data);
            output_data += tem;
            output_data += "\n";
            // output_data += "\r\n";

            if (original_length_or_window_id < MAX_LEN)
            {
                std::deque<std::string> newDeque;
                newDeque.push_back(tem);
                this->window[original_length_or_window_id] = newDeque;
            }
        }
    }

}