#include <fstream>
#include "fs.h"
using namespace std;

bool file_to_memory (string const & filename, char* data, streamsize const bytes, streampos const offset)
{
    ifstream byte_file (filename, fstream::in | fstream::binary);
    if (byte_file)
    {
	byte_file.seekg(offset);
	byte_file.read(data, bytes);
	return (byte_file.good());
    }
    return false;
}

bool memory_to_file (string const & filename, char const * data, streamsize const bytes)
{
    ofstream byte_file (filename, fstream::out | fstream::binary | fstream::trunc);
    if (byte_file)
    {
	byte_file.write(data, bytes);
	return (byte_file.good());
    }
    return false;
}

streamsize get_size (string const & filename)
{
    ifstream byte_file (filename, fstream::in | fstream::binary);
    if (byte_file)
    {
	byte_file.seekg(0, byte_file.end);
    	return byte_file.tellg();
    }
    return -1;
}
