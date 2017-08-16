#ifndef __TT_FS_INCLUDED__
#define __TT_FS_INCLUDED__

#include <cstddef>
#include <ios>
#include <string>

bool file_to_memory (std::string const &, char*, std::streamsize const, std::streampos const = 0);
bool memory_to_file (std::string const &, char const *, std::streamsize const);
std::streamsize get_size (std::string const & filename);

#endif
