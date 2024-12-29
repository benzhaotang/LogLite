#include <ctime>
#include <iostream>
#include <vector>
#include <string.h>
#include <boost/dynamic_bitset.hpp>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <filesystem>

#include "common/file.h"
#include "compress/stream_compress.h"

static struct config
{
    bool stream_compress;
    bool stream_decompress;
    bool is_test;

    const char *file_path;
    const char *output_path;
} config;

static void parseOptions(int argc, const char **argv)
{
    // Default values
    config.stream_compress = false;
    config.stream_decompress = false;
    config.is_test = false;

    for (int i = 1; i < argc; i++)
    {
        int lastarg = (i == argc - 1);
        if (!strcmp(argv[i], "--compress") && !lastarg)
        {
            config.stream_compress = true;
        }
        else if (!strcmp(argv[i], "--decompress") && !lastarg)
        {
            config.stream_decompress = true;
        }
        else if (!strcmp(argv[i], "--test") && !lastarg)
        {
            config.is_test = true;
        }
        else if (!strcmp(argv[i], "--file-path") && !lastarg)
        {
            config.file_path = const_cast<char *>(argv[++i]);
        }
        else if (!strcmp(argv[i], "--output-path") && !lastarg)
        {
            config.output_path = const_cast<char *>(argv[++i]);
        }
        else
        {
            std::cerr << "Unknown option: " << argv[i] << std::endl;
            exit(1);
        }
    }
}

std::streampos file_size(const char *filename)
{
    std::ifstream file(filename, std::ios::binary | std::ios::ate);
    if (!file)
    {
        throw std::runtime_error("Failed to open file for size calculation.");
    }
    return file.tellg();
}

bool areFilesEqual(const std::string &filePath1, const std::string &filePath2)
{
    std::ifstream file1(filePath1, std::ifstream::binary | std::ifstream::ate);
    std::ifstream file2(filePath2, std::ifstream::binary | std::ifstream::ate);

    if (!file1.is_open() || !file2.is_open())
    {
        std::cerr << "Error: Could not open one of the files." << std::endl;
        return false;
    }

    if (file1.tellg() != file2.tellg())
    {
        return false;
    }

    file1.seekg(0, std::ifstream::beg);
    file2.seekg(0, std::ifstream::beg);

    std::istreambuf_iterator<char> begin1(file1), begin2(file2);
    std::istreambuf_iterator<char> end;
    return std::equal(begin1, end, begin2);
}

int main(int argc, const char *argv[])
{
    // Parse command line options
    parseOptions(argc, argv);

    if (config.is_test)
    {
        std::cout << "Test mode: Compressing and Decompressing the file in sequence..." << std::endl;
    }

    if (config.stream_compress || config.is_test)
    {
        if (config.is_test)
        {
            std::cout << "Testing Compression..." << std::endl;
        }

        std::cout << "-----Using Stream Compress-----" << std::endl;
        std::cout << "Raw file path: " << config.file_path << std::endl;
        std::cout << "Compressed output file path: " << config.output_path << std::endl;

        std::string all_data;
        XORC::read_string_from_file(all_data, config.file_path);

        boost::dynamic_bitset<> output_data(all_data.size() * 8);
        uint64_t len_output_data = 0;

        std::vector<std::string> split_all_data;

        std::istringstream stream(all_data);
        std::string token;

        int line_count = 0;
        while (std::getline(stream, token, '\n'))
        {
            if (!token.empty() && token.back() == '\r')
            {
                token.pop_back();
            }
            split_all_data.push_back(token);
            ++line_count;
        }

        XORC::Stream_Compress *sc = new XORC::Stream_Compress();

        clock_t start_time, end_time;
        start_time = clock();
        for (size_t i = 0; i < split_all_data.size(); ++i)
        {
            sc->stream_compress(split_all_data[i], output_data, len_output_data);
        }
        end_time = clock();

        output_data.resize(len_output_data);
        XORC::write_bitset_to_file(output_data, config.output_path);

        int64_t raw_size = file_size(config.file_path);
        int64_t compressed_size = file_size(config.output_path);
        std::cout << "Compression rate (with separator): "
                  << static_cast<double>(compressed_size) / static_cast<double>(raw_size)
                  << std::endl;

        std::cout << "Compression speed: "
                  << (double)(raw_size) / 8 / (1024 * 1024) /
                         ((double)(end_time - start_time) / CLOCKS_PER_SEC)
                  << " MB/s" << std::endl;

        delete sc;
    }

    if (config.stream_decompress || config.is_test)
    {
        if (config.is_test)
        {
            std::cout << "Testing Decompression..." << std::endl;
        }

        std::cout << "-----Using Stream Decompress-----" << std::endl;
        std::cout << "Compressed file path: " << config.file_path << std::endl;
        std::cout << "Decompressed output file path: " << config.output_path << std::endl;

        boost::dynamic_bitset<> compressed_bitset;
        XORC::read_bitset_from_file(compressed_bitset, config.file_path);

        std::vector<boost::dynamic_bitset<>> split_compressed_bitset;
        std::vector<bool> isRLE;
        std::vector<int> original_length_or_window_id;
        size_t len_compressed_bitset = compressed_bitset.size();
        size_t i = 0;
        while (i < len_compressed_bitset)
        {
            if (compressed_bitset[i] == 0)
            {
                isRLE.push_back(false);
                i++;

                int tem_original_length = 0;
                for (size_t j = 0; j < ORIGINAL_LENGTH_COUNT; ++j, ++i)
                {
                    if (compressed_bitset[i])
                    {
                        tem_original_length |= (1 << j);
                    }
                }
                original_length_or_window_id.push_back(tem_original_length);

                boost::dynamic_bitset<> tem_bitset(tem_original_length * 8);
                for (size_t j = 0; j < tem_original_length * 8; j++)
                {
                    tem_bitset[j] = compressed_bitset[i + j];
                }
                i += tem_original_length * 8;

                split_compressed_bitset.push_back(tem_bitset);
            }
            else
            {
                isRLE.push_back(true);
                i++;
                int tem_window_id = 0;
                for (size_t j = 0; j < EACH_WINDOW_SIZE_COUNT; ++j, ++i)
                {
                    if (compressed_bitset[i])
                    {
                        tem_window_id |= (1 << j);
                    }
                }
                original_length_or_window_id.push_back(tem_window_id);

                int len_single_data = 0;
                for (size_t j = 0; j < STREAM_ENCODER_COUNT; ++j, ++i)
                {
                    if (compressed_bitset[i])
                    {
                        len_single_data |= (1 << j);
                    }
                }

                boost::dynamic_bitset<> tem_bitset(len_single_data);
                for (size_t j = 0; j < len_single_data; j++)
                {
                    tem_bitset[j] = compressed_bitset[i + j];
                }
                i += len_single_data;

                split_compressed_bitset.push_back(tem_bitset);
            }
        }

        std::string all_data;
        all_data.reserve(static_cast<size_t>(1024) * 1024 * 1024 * 33);

        XORC::Stream_Compress *sc = new XORC::Stream_Compress();

        std::string xor_result;
        xor_result.reserve(500);

        clock_t start_time, end_time;
        start_time = clock();
        for (size_t i = 0; i < split_compressed_bitset.size(); ++i)
        {
            sc->stream_decompress(split_compressed_bitset[i], isRLE[i], original_length_or_window_id[i], all_data, xor_result);
        }
        end_time = clock();

        XORC::write_string_to_file(all_data, config.output_path);

        int64_t raw_size = static_cast<int64_t>(file_size(config.output_path)) - 2 * split_compressed_bitset.size();
        std::cout << "Decompression speed: "
                  << (double)(raw_size) / (1024 * 1024) /
                         ((double)(end_time - start_time) / CLOCKS_PER_SEC)
                  << " MB/s" << std::endl;

        delete sc;
    }

    return 0;
}













// #include <ctime>
// #include <iostream>
// #include <vector>
// #include <string.h>
// #include <boost/dynamic_bitset.hpp>
// #include <sys/stat.h>
// #include <sys/mman.h>
// #include <fcntl.h>
// #include <unistd.h>
// #include <stdio.h>
// #include <filesystem>

// #include "common/file.h"
// #include "compress/stream_compress.h"

// static struct config
// {
//     bool stream_compress;
//     bool stream_decompress;
//     bool is_test;

//     const char *file_path;
//     const char *output_path;
// } config;

// static void parseOptions(int argc, const char **argv)
// {
//     for (int i = 1; i < argc; i++)
//     {
//         int lastarg = (i == argc - 1);
//         if (!strcmp(argv[i], "--stream-compress") && !lastarg)
//         {
//             config.stream_compress = true;
//         }
//         else if (!strcmp(argv[i], "--file-path") && !lastarg)
//         {
//             config.file_path = const_cast<char *>(argv[++i]);
//         }
//         else if (!strcmp(argv[i], "--output-path") && !lastarg)
//         {
//             config.output_path = const_cast<char *>(argv[++i]);
//         }
//         else if (!strcmp(argv[i], "--test") && !lastarg)
//         {
//             config.is_test = true;
//         }
//     }
// }

// std::streampos file_size(const char *filename)
// {
//     std::ifstream file(filename, std::ios::binary | std::ios::ate);
//     if (!file)
//     {
//         throw std::runtime_error("Failed to open file for size calculation.");
//     }
//     return file.tellg();
// }

// bool areFilesEqual(const std::string &filePath1, const std::string &filePath2)
// {
//     std::ifstream file1(filePath1, std::ifstream::binary | std::ifstream::ate);
//     std::ifstream file2(filePath2, std::ifstream::binary | std::ifstream::ate);

//     if (!file1.is_open() || !file2.is_open())
//     {
//         std::cerr << "Error: Could not open one of the files." << std::endl;
//         return false;
//     }

//     if (file1.tellg() != file2.tellg())
//     {

//         return false;
//     }

//     file1.seekg(0, std::ifstream::beg);
//     file2.seekg(0, std::ifstream::beg);

//     std::istreambuf_iterator<char> begin1(file1), begin2(file2);
//     std::istreambuf_iterator<char> end;
//     return std::equal(begin1, end, begin2);
// }

// int main(int argc, const char *argv[])
// {
//     config.stream_compress = false;
//     config.stream_decompress = false;
//     config.is_test = false;

//     parseOptions(argc, argv);

//     std::string name = "postgresql_stderr";

//     config.stream_compress = true;

//     if (config.stream_compress && !config.is_test)
//     {

//         std::string file_path = "/home/tangbenzhao/Development/cDev/postgresql/src/postgresql_stderr.log";
//         std::string output_path = "../test/mytestStream/data01/postgresql_stderrStreamCompress";

//         config.file_path = file_path.c_str();
//         config.output_path = output_path.c_str();

//         std::cout << "-----using stream compress-----" << std::endl;
//         std::cout << "raw file path:" << config.file_path << std::endl;
//         std::cout << "compressed output file path:" << config.output_path << std::endl;

//         std::string all_data;
//         XORC::read_string_from_file(all_data, config.file_path);

//         boost::dynamic_bitset<> output_data(all_data.size() * 8);
//         uint64_t len_output_data = 0;

//         std::vector<std::string> split_all_data;

//         std::istringstream stream(all_data);
//         std::string token;

//         int line_count = 0;
//         while (std::getline(stream, token, '\n'))
//         {
//             if (!token.empty() && token.back() == '\r')
//             {
//                 token.pop_back();
//             }
//             split_all_data.push_back(token);
//             ++line_count;
//         }

//         XORC::Stream_Compress *sc = new XORC::Stream_Compress();

//         clock_t start_time, end_time;
//         start_time = clock();
//         for (size_t i = 0; i < split_all_data.size(); ++i)
//         {
//             sc->stream_compress(split_all_data[i], output_data, len_output_data);
//         }
//         end_time = clock();

//         output_data.resize(len_output_data);
//         XORC::write_bitset_to_file(output_data, config.output_path);

//         std::cout << "line_count:"
//                   << line_count
//                   << std::endl;

//         int64_t raw_size = file_size(config.file_path);
//         int64_t compressed_size = file_size(config.output_path);
//         std::cout << "compression rate (with separator):"
//                   << static_cast<double>(compressed_size) / static_cast<double>(raw_size)
//                   << std::endl;

//         int64_t raw_size1 = (all_data.size() - 1 * line_count) * 8;
//         int64_t compressed_size1 = len_output_data - line_count * STREAM_ENCODER_COUNT;
//         std::cout << "compression rate (without separator):"
//                   << static_cast<double>(compressed_size1) / static_cast<double>(raw_size1)
//                   << std::endl;

//         std::cout << "compression speed: "
//                   << (double)raw_size1 / 8 / (double)1024 / (double)1024 /
//                          (static_cast<double>(end_time - start_time) / CLOCKS_PER_SEC)
//                   << "MB/s" << std::endl;

//         delete sc;
//     }

//     // config.stream_decompress=true;

//     if (config.stream_decompress && !config.is_test)
//     {

//         std::string file_path = "/data/tangbenzhao/xorc-temp/xorc-b/" + name + "StreamCompress";
//         std::string output_path = "/data/tangbenzhao/xorc-temp/xorc-b/" + name + "StreamDecompress";

//         config.file_path = file_path.c_str();
//         config.output_path = output_path.c_str();

//         std::cout << "-----using stream decompress-----" << std::endl;
//         std::cout << "compressed file path:" << config.file_path << std::endl;
//         std::cout << "decompressed output file path:" << config.output_path << std::endl;

//         boost::dynamic_bitset<> compressed_bitset;
//         XORC::read_bitset_from_file(compressed_bitset, config.file_path);

//         std::vector<boost::dynamic_bitset<>> split_compressed_bitset;
//         std::vector<bool> isRLE;
//         std::vector<int> original_length_or_window_id;
//         size_t len_compressed_bitset = compressed_bitset.size();
//         size_t i = 0;
//         while (i < len_compressed_bitset)
//         {
//             if (compressed_bitset[i] == 0)
//             {
//                 isRLE.push_back(false);
//                 i++;

//                 int tem_original_length = 0;
//                 for (size_t j = 0; j < ORIGINAL_LENGTH_COUNT; ++j, ++i)
//                 {
//                     if (compressed_bitset[i])
//                     {
//                         tem_original_length |= (1 << j);
//                     }
//                 }
//                 original_length_or_window_id.push_back(tem_original_length);

//                 boost::dynamic_bitset<> tem_bitset(tem_original_length * 8);
//                 for (size_t j = 0; j < tem_original_length * 8; j++)
//                 {
//                     tem_bitset[j] = compressed_bitset[i + j];
//                 }
//                 i += tem_original_length * 8;

//                 split_compressed_bitset.push_back(tem_bitset);
//             }
//             else
//             {
//                 isRLE.push_back(true);
//                 i++;
//                 int tem_window_id = 0;
//                 for (size_t j = 0; j < EACH_WINDOW_SIZE_COUNT; ++j, ++i)
//                 {
//                     if (compressed_bitset[i])
//                     {
//                         tem_window_id |= (1 << j);
//                     }
//                 }
//                 original_length_or_window_id.push_back(tem_window_id);

//                 int len_single_data = 0;
//                 for (size_t j = 0; j < STREAM_ENCODER_COUNT; ++j, ++i)
//                 {
//                     if (compressed_bitset[i])
//                     {
//                         len_single_data |= (1 << j);
//                     }
//                 }

//                 boost::dynamic_bitset<> tem_bitset(len_single_data);
//                 for (size_t j = 0; j < len_single_data; j++)
//                 {
//                     tem_bitset[j] = compressed_bitset[i + j];
//                 }
//                 i += len_single_data;

//                 split_compressed_bitset.push_back(tem_bitset);
//             }
//         }

//         std::string all_data;
//         all_data.reserve(static_cast<size_t>(1024) * 1024 * 1024 * 33);

//         XORC::Stream_Compress *sc = new XORC::Stream_Compress();

//         std::string xor_result;
//         xor_result.reserve(500);

//         clock_t start_time, end_time;
//         start_time = clock();
//         for (size_t i = 0; i < split_compressed_bitset.size(); ++i)
//         {
//             sc->stream_decompress(split_compressed_bitset[i], isRLE[i], original_length_or_window_id[i], all_data, xor_result);
//         }
//         end_time = clock();

//         XORC::write_string_to_file(all_data, config.output_path);

//         int64_t raw_size = static_cast<int64_t>(file_size(config.output_path)) - 2 * split_compressed_bitset.size();
//         std::cout << "decompression speed: "
//                   << (double)raw_size / (double)1024 / (double)1024 /
//                          (static_cast<double>(end_time - start_time) / CLOCKS_PER_SEC)
//                   << "MB/s" << std::endl;

//         // std::string original_file_path = "../test/mytestStream/data01/" + name + ".log";
//         // bool isEqual=areFilesEqual(original_file_path,output_path);
//         // std::cout << "is Equal?  "<< (isEqual?"yes":"no") << std::endl;

//         delete sc;
//     }
// }