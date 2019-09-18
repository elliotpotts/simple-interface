#ifndef SI_UTIL_HPP_INCLUDED
#define SI_UTIL_HPP_INCLUDED

#include <cstddef>
#include <vector>
#include <string>

namespace si {
    std::vector<std::byte> file_contents(std::string filename);
}

#endif
