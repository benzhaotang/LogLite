#include "file.h"

namespace XORC
{

    void write_bitset_to_file(const boost::dynamic_bitset<> &bitset, const char *filename)
    {
        std::ofstream file(filename, std::ios::binary);
        if (!file.is_open())
        {
            throw std::runtime_error("Failed to open file for writing.");
        }

        std::vector<unsigned long> blocks(bitset.num_blocks());
        boost::to_block_range(bitset, blocks.begin());

        file.write(reinterpret_cast<const char *>(blocks.data()), blocks.size() * sizeof(unsigned long));

        size_t last_block_bits = bitset.size() % (sizeof(unsigned long) * 8);
        if (last_block_bits == 0 && bitset.size() != 0)
        {
            last_block_bits = sizeof(unsigned long) * 8;
        }
        file.write(reinterpret_cast<const char *>(&last_block_bits), sizeof(size_t));
        file.close();
    }

    void read_bitset_from_file(boost::dynamic_bitset<> &bitset, const char *filename)
    {
        std::ifstream file(filename, std::ios::binary);
        if (!file.is_open())
        {
            throw std::runtime_error("Failed to open file for reading.");
        }

        file.seekg(0, std::ios::end);
        size_t file_size = file.tellg();
        file.seekg(0, std::ios::beg);

        size_t last_block_bits;
        file.seekg(file_size - sizeof(size_t), std::ios::beg);
        file.read(reinterpret_cast<char *>(&last_block_bits), sizeof(size_t));
        file.seekg(0, std::ios::beg);

        size_t num_blocks = (file_size - sizeof(size_t)) / sizeof(unsigned long);
        std::vector<unsigned long> blocks(num_blocks);

        file.read(reinterpret_cast<char *>(blocks.data()), blocks.size() * sizeof(unsigned long));
        file.close();

        bitset.resize(file_size * 8);
        boost::from_block_range(blocks.begin(), blocks.end(), bitset);

        size_t bitset_size = (num_blocks - 1) * sizeof(unsigned long) * 8 + last_block_bits;
        bitset.resize(bitset_size);
    }

    void write_string_to_file(const std::string &content, const char *filename)
    {
        std::ofstream file(filename, std::ios::out | std::ios::binary);
        if (!file.is_open())
        {
            throw std::runtime_error("Failed to open file for writing.");
        }

        file.write(content.data(), content.size());
        if (!file)
        {
            throw std::runtime_error("Failed to write content to file.");
        }

        file.close();
    }

    void read_string_from_file(std::string &content, const char *filename)
    {
        std::ifstream file(filename, std::ios::in | std::ios::binary);
        if (!file.is_open())
        {
            throw std::runtime_error("Failed to open file for reading.");
        }

        content.assign((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

        if (file.fail() && !file.eof())
        {
            throw std::runtime_error("Failed to read content from file.");
        }

        file.close();
    }

    bool write_vector_to_file(const std::vector<uint16_t> &data, const std::string &filename)
    {
        std::ofstream outFile(filename, std::ios::binary);

        if (!outFile)
        {
            std::cerr << "Unable to open file " << filename << std::endl;
            return false;
        }

        outFile.write(reinterpret_cast<const char *>(data.data()), data.size() * sizeof(uint16_t));

        if (!outFile)
        {
            std::cerr << "Failed to write to file: " << filename << std::endl;
            return false;
        }

        outFile.close();

        return true;
    }

    bool read_vector_from_file(std::vector<uint16_t> &data, const std::string &filename)
    {
        std::ifstream inFile(filename, std::ios::binary);

        if (!inFile)
        {
            std::cerr << "Unable to open file " << filename << std::endl;
            return false;
        }

        inFile.seekg(0, std::ios::end);
        std::streampos fileSize = inFile.tellg();
        inFile.seekg(0, std::ios::beg);

        size_t numElements = fileSize / sizeof(uint16_t);

        data.resize(numElements);

        inFile.read(reinterpret_cast<char *>(data.data()), fileSize);

        if (!inFile)
        {
            std::cerr << "Failed to read the file:" << filename << std::endl;
            return false;
        }

        inFile.close();

        return true;
    }

    void write_bytes_to_file(const std::vector<unsigned char> &data, const std::string &filename)
    {

        std::ofstream file(filename, std::ios::binary);

        if (!file.is_open())
        {
            std::cerr << "Error opening file for writing: " << filename << std::endl;
            return;
        }

        if (!data.empty())
        {
            file.write(reinterpret_cast<const char *>(data.data()), data.size());
        }

        file.close();

        if (!file)
        {
            std::cerr << "Error writing to file: " << filename << std::endl;
        }
    }

    void read_bytes_from_file(std::vector<unsigned char> &data, const std::string &filename)
    {

        std::ifstream file(filename, std::ios::binary);

        if (!file.is_open())
        {
            std::cerr << "Error opening file for reading: " << filename << std::endl;
            return;
        }

        file.seekg(0, std::ios::end);
        size_t size = file.tellg();
        file.seekg(0, std::ios::beg);

        data.resize(size);
        file.read(reinterpret_cast<char *>(data.data()), size);

        file.close();

        if (!file)
        {
            std::cerr << "Error reading file: " << filename << std::endl;
            return;
        }
    }

    void write_sizetvector_to_file(const std::vector<size_t> &data, const std::string &filename)
    {
        std::ofstream file(filename, std::ios::binary);

        if (!file.is_open())
        {
            std::cerr << "Error opening file for writing: " << filename << std::endl;
            return;
        }

        size_t size = data.size();
        file.write(reinterpret_cast<const char *>(&size), sizeof(size));

        if (!data.empty())
        {
            file.write(reinterpret_cast<const char *>(data.data()), size * sizeof(size_t));
        }

        file.close();

        if (!file)
        {
            std::cerr << "Error writing to file: " << filename << std::endl;
        }
        else
        {
            // std::cout << "Data successfully written to " << filename << std::endl;
        }
    }

    void read_sizetvector_from_file(std::vector<size_t> &data, const std::string &filename)
    {

        std::ifstream file(filename, std::ios::binary);

        if (!file.is_open())
        {
            std::cerr << "Error opening file for reading: " << filename << std::endl;
            return;
        }

        size_t size = 0;
        file.read(reinterpret_cast<char *>(&size), sizeof(size));

        data.resize(size);
        file.read(reinterpret_cast<char *>(data.data()), size * sizeof(size_t));

        file.close();

        if (!file)
        {
            std::cerr << "Error reading from file: " << filename << std::endl;
        }
        else
        {
            // std::cout << "Data successfully read from " << filename << std::endl;
        }
    }

}