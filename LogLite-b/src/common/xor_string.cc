#include "xor_string.h"

namespace XORC
{

    std::string bitwiseXor(const std::string &a, const std::string &b)
    {

        std::string result;
        result.resize(a.length());

        size_t i = 0;

        for (; i + simd_width32 <= a.length(); i += simd_width32)
        {
            __m256i v_a = _mm256_loadu_si256((__m256i *)&a[i]);
            __m256i v_b = _mm256_loadu_si256((__m256i *)&b[i]);
            __m256i v_result = _mm256_xor_si256(v_a, v_b);
            _mm256_storeu_si256((__m256i *)&result[i], v_result);
        }

        if (i + simd_width16 <= a.length())
        {
            __m128i v_a = _mm_loadu_si128((__m128i *)&a[i]);
            __m128i v_b = _mm_loadu_si128((__m128i *)&b[i]);
            __m128i v_result = _mm_xor_si128(v_a, v_b);
            _mm_storeu_si128((__m128i *)&result[i], v_result);
            i += simd_width16;
        }

        for (; i < a.length(); ++i)
        {
            result[i] = a[i] ^ b[i];
        }

        return result;
    }

    void bitwiseXor(const std::string &a, const std::string &b, std::string &result)
    {

        size_t i = 0;

        for (; i + simd_width32 <= a.length(); i += simd_width32)
        {
            __m256i v_a = _mm256_loadu_si256((__m256i *)&a[i]);
            __m256i v_b = _mm256_loadu_si256((__m256i *)&b[i]);
            __m256i v_result = _mm256_xor_si256(v_a, v_b);
            _mm256_storeu_si256((__m256i *)&result[i], v_result);
        }

        if (i + simd_width16 <= a.length())
        {
            __m128i v_a = _mm_loadu_si128((__m128i *)&a[i]);
            __m128i v_b = _mm_loadu_si128((__m128i *)&b[i]);
            __m128i v_result = _mm_xor_si128(v_a, v_b);
            _mm_storeu_si128((__m128i *)&result[i], v_result);
            i += simd_width16;
        }

        for (; i < a.length(); ++i)
        {
            result[i] = a[i] ^ b[i];
        }
    }

} // namespace XORC