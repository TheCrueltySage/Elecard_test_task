#include <fstream>
#include "fs.h"

bool file_to_memory (std::string const & filename, char* data, std::streamsize const bytes, std::streampos const offset)
{
    std::ifstream byte_file (filename, std::fstream::in | std::fstream::binary);
    if (byte_file)
    {
	byte_file.seekg(offset);
	byte_file.read(data, bytes);
	return (byte_file.good());
    }
    return false;
}

bool memory_to_file (std::string const & filename, char const * data, std::streamsize const bytes)
{
    std::ofstream byte_file (filename, std::fstream::out | std::fstream::binary | std::fstream::trunc);
    if (byte_file)
    {
	byte_file.write(data, bytes);
	return (byte_file.good());
    }
    return false;
}

std::streamsize get_size (std::string const & filename)
{
    std::ifstream byte_file (filename, std::fstream::in | std::fstream::binary);
    if (byte_file)
    {
	byte_file.seekg(0, byte_file.end);
    	return byte_file.tellg();
    }
    return -1;
}
