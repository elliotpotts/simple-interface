#include <si/util.hpp>
#include <fstream>

std::vector<std::byte> si::file_contents(std::string fname) {
    std::basic_ifstream<char> file(fname, std::ios::ate | std::ios::binary);
    if (file) {
        std::vector<std::byte> contents(file.tellg());
        file.seekg(0);
        file.read(reinterpret_cast<char*>(contents.data()), contents.size());
        return contents;
    } else {
        throw std::runtime_error("Couldn't open file");
    }    
}
