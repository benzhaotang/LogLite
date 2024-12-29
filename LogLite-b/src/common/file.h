#ifndef FILE_H_
#define FILE_H_

#include <string>
#include <unordered_map>
#include <vector>
#include <sstream>
#include <fstream>
#include <iostream>
#include <boost/dynamic_bitset.hpp>

namespace XORC
{
    void write_bitset_to_file(const boost::dynamic_bitset<> &bitset, const char *filename);
    void read_bitset_from_file(boost::dynamic_bitset<> &bitset, const char *filename);

    void write_string_to_file(const std::string &content, const char *filename);
    void read_string_from_file(std::string &content, const char *filename);

    bool write_vector_to_file(const std::vector<uint16_t> &data, const std::string &filename);
    bool read_vector_from_file(std::vector<uint16_t> &data, const std::string &filename);

    void write_bytes_to_file(const std::vector<unsigned char> &data, const std::string &filename);
    void read_bytes_from_file(std::vector<unsigned char> &data, const std::string &filename);

    void write_sizetvector_to_file(const std::vector<size_t> &data, const std::string &filename);
    void read_sizetvector_from_file(std::vector<size_t> &data, const std::string &filename);

}

#endif