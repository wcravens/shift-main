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

#include "crossguid/Guid.h"

#include <iostream>

auto test(std::ostream& outStream) -> int
{
    int failed = 0;

    /*************************************************************************
	 * HAPPY PATH TESTS
	 *************************************************************************/

    auto r1 = shift::crossguid::newGuid();
    auto r2 = shift::crossguid::newGuid();
    auto r3 = shift::crossguid::newGuid();

    outStream << r1 << std::endl
              << r2 << std::endl
              << r3 << std::endl;

    shift::crossguid::Guid s1("7bcd757f-5b10-4f9b-af69-1a1f226f3b3e");
    shift::crossguid::Guid s2("16d1bd03-09a5-47d3-944b-5e326fd52d27");
    shift::crossguid::Guid s3("fdaba646-e07e-49de-9529-4499a5580c75");
    shift::crossguid::Guid s4("7bcd757f-5b10-4f9b-af69-1a1f226f3b3e");

    if (r1 == r2 || r1 == r3 || r2 == r3) {
        outStream << "FAIL - not all random guids are different" << std::endl;
        ++failed;
    }

    if (s1 == s2) {
        outStream << "FAIL - s1 and s2 should be different" << std::endl;
        ++failed;
    }

    if (s1 != s4) {
        outStream << "FAIL - s1 and s4 should be equal" << std::endl;
        ++failed;
    }

    std::stringstream ss1;
    ss1 << s1;
    if (ss1.str() != "7bcd757f-5b10-4f9b-af69-1a1f226f3b3e") {
        outStream << "FAIL - string from s1 stream is wrong" << std::endl;
        outStream << "--> " << ss1.str() << std::endl;
        ++failed;
    }

    if (s1.str() != "7bcd757f-5b10-4f9b-af69-1a1f226f3b3e") {
        outStream << "FAIL - string from s1.str() is wrong" << std::endl;
        outStream << "--> " << s1.str() << std::endl;
        ++failed;
    }

    std::stringstream ss2;
    ss2 << s2;
    if (ss2.str() != "16d1bd03-09a5-47d3-944b-5e326fd52d27") {
        outStream << "FAIL - string generated from s2 is wrong" << std::endl;
        outStream << "--> " << ss2.str() << std::endl;
        return 1;
    }

    std::stringstream ss3;
    ss3 << s3;
    if (ss3.str() != "fdaba646-e07e-49de-9529-4499a5580c75") {
        outStream << "FAIL - string generated from s3 is wrong" << std::endl;
        outStream << "--> " << ss3.str() << std::endl;
        ++failed;
    }

    auto swap1 = shift::crossguid::newGuid();
    auto swap2 = shift::crossguid::newGuid();
    auto swap3 = swap1;
    auto swap4 = swap2;

    if (swap1 != swap3 || swap2 != swap4 || swap1 == swap2) {
        outStream << "FAIL - swap guids have bad initial state" << std::endl;
        ++failed;
    }

    swap1.swap(swap2);

    if (swap1 != swap4 || swap2 != swap3 || swap1 == swap2) {
        outStream << "FAIL - swap didn't swap" << std::endl;
        ++failed;
    }

    std::array<unsigned char, 16> bytes = {
        { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
            0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0xdd }
    };
    shift::crossguid::Guid guidFromBytes(bytes);
    shift::crossguid::Guid guidFromString("0102030405060708090a0b0c0d0e0fdd");
    if (guidFromBytes != guidFromString) {
        outStream << "FAIL - String/bytes make different guids" << std::endl;
        ++failed;
    }

    if (!std::equal(guidFromBytes.bytes().begin(), guidFromBytes.bytes().end(), bytes.begin())) {
        outStream << "FAIL - array returned from bytes() is wrong" << std::endl;
        ++failed;
    }

    /*************************************************************************
	* ERROR HANDLING
	*************************************************************************/

    shift::crossguid::Guid empty;
    shift::crossguid::Guid twoTooFew("7bcd757f-5b10-4f9b-af69-1a1f226f3b");
    if (twoTooFew != empty || twoTooFew.isValid()) {
        outStream << "FAIL - Guid from two too few chars" << std::endl;
        ++failed;
    }

    shift::crossguid::Guid oneTooFew("16d1bd03-09a5-47d3-944b-5e326fd52d2");
    if (oneTooFew != empty || oneTooFew.isValid()) {
        outStream << "FAIL - Guid from one too few chars" << std::endl;
        ++failed;
    }

    shift::crossguid::Guid twoTooMany("7bcd757f-5b10-4f9b-af69-1a1f226f3beeff");
    if (twoTooMany != empty || twoTooMany.isValid()) {
        outStream << "FAIL - Guid from two too many chars" << std::endl;
        ++failed;
    }

    shift::crossguid::Guid oneTooMany("16d1bd03-09a5-47d3-944b-5e326fd52d27a");
    if (oneTooMany != empty || oneTooMany.isValid()) {
        outStream << "FAIL - Guid from one too many chars" << std::endl;
        ++failed;
    }

    shift::crossguid::Guid badString("!!bad-guid-string!!");
    if (badString != empty || badString.isValid()) {
        outStream << "FAIL - Guid from bad string" << std::endl;
        ++failed;
    }

    if (failed > 0) {
        outStream << failed << " tests failed." << std::endl;
        return 1;
    }

    outStream << "All tests passed!" << std::endl;
    return 0;
}

auto main(int argc, char** argv) -> int
{
    return test(std::cout);
}
