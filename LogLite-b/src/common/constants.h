#include <c++/10/new>
#ifndef CONSTANTS_H_
#define CONSTANTS_H_

// Parameters that can be adjusted
constexpr int EACH_WINDOW_SIZE_COUNT = 3;
constexpr int STREAM_ENCODER_COUNT = 13 + 8;
constexpr int ORIGINAL_LENGTH_COUNT = 15 + 8;
constexpr int MAX_LEN = 10000;



constexpr int RLE_COUNT = 8;
constexpr int RLE_POW_COUNT = 1 << RLE_COUNT;
constexpr int RLE_SKIM = 8;
constexpr int EACH_WINDOW_SIZE = 1 << EACH_WINDOW_SIZE_COUNT;
constexpr size_t simd_width32 = 32;
constexpr size_t simd_width16 = 16;

#endif // CONSTANTS_H