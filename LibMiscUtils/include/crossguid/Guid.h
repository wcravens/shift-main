/*
The MIT License (MIT)

Copyright (c) 2014 Graeme Hill (http://graemehill.ca)

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/

#pragma once

#ifdef _WIN32
#pragma comment(lib, "ole32.lib")
#endif

#include "../MiscUtils_EXPORTS.h"

#include <array>
#include <functional>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <utility>

namespace shift::crossguid {

// class to represent a GUID/UUID. Each instance acts as a wrapper around a
// 16 byte value that can be passed around by value. It also supports
// conversion to string (via the stream operator <<) and conversion from a
// string via constructor.
class MISCUTILS_EXPORTS Guid {
public:
    Guid(const std::array<unsigned char, 16>& bytes);
    Guid(const unsigned char* bytes);
    Guid(const std::string& fromString);
    Guid();
    Guid(const Guid& other);
    auto operator=(const Guid& other) -> Guid&;
    auto operator==(const Guid& other) const -> bool;
    auto operator!=(const Guid& other) const -> bool;

    auto str() const -> std::string;
    operator std::string() const;
    auto bytes() const -> const std::array<unsigned char, 16>&;
    void swap(Guid& other);
    auto isValid() const -> bool;

private:
    void zeroify();

    // actual data
    std::array<unsigned char, 16> _bytes;

    // make the << operator a friend so it can access _bytes
    friend auto operator<<(std::ostream& s, const Guid& guid) -> std::ostream&;
};

Guid newGuid();

} // shift::crossguid

namespace std {

// template specialization for std::swap<Guid>() --
// see guid.cpp for the function definition
// template <>
// void swap(shift::crossguid::Guid& guid0, shift::crossguid::Guid& guid1);

// specialization for std::hash<Guid> -- this implementation
// uses std::hash<std::string> on the stringification of the guid
// to calculate the hash
template <>
struct hash<shift::crossguid::Guid> {
    typedef shift::crossguid::Guid argument_type;
    typedef std::size_t result_type;

    auto operator()(argument_type const& guid) const -> result_type
    {
        std::hash<std::string> hasher;
        return static_cast<result_type>(hasher(guid.str()));
    }
};

} // std
