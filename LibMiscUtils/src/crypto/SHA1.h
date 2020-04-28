/*
    sha1.hpp - header of

    ============
    SHA-1 in C++
    ============

    100% Public Domain.

    Original C Code
        -- Steve Reid <steve@edmweb.com>
    Small changes to fit into bglibs
        -- Bruce Guenter <bruce@untroubled.org>
    Translation to simpler C++ Code
        -- Volker Diels-Grabsch <v@njh.eu>
    Safety fixes
        -- Eugene Hopkinson <slowriot at voxelstorm dot com>
*/

#pragma once

#include <cstdint>
#include <iostream>
#include <string>

class SHA1 {
public:
    SHA1();
    void update(const std::string& s);
    void update(std::istream& is);
    auto final() -> std::string;
    static auto from_file(const std::string& filename) -> std::string;

private:
    uint32_t digest[5];
    std::string buffer;
    uint64_t transforms;
};
