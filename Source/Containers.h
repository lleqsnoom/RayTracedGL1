// Copyright (c) 2022 Sultim Tsyrendashiev
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#pragma once

#include <string>
#include <string_view>

#include "Hashmap/robin_hood.h"

namespace rgl
{

template< typename Key, typename T >
using unordered_map = robin_hood::unordered_map< Key, T >;

template< typename Key >
using unordered_set = robin_hood::unordered_set< Key >;

// Transparent string hashing/equality, so a std::string-keyed map can be probed with a
// std::string_view or const char* without constructing a temporary std::string.
// robin_hood::hash<std::string> and hash<std::string_view> both use hash_bytes over the
// character range, so hashes agree for equal content.
struct StringHash
{
    using is_transparent = void;

    size_t operator()( std::string_view sv ) const noexcept
    {
        return robin_hood::hash< std::string_view >{}( sv );
    }
};

struct StringEqual
{
    using is_transparent = void;

    template< typename A, typename B >
    bool operator()( const A& a, const B& b ) const noexcept
    {
        return std::string_view( a ) == std::string_view( b );
    }
};

template< typename T >
using string_map = robin_hood::unordered_map< std::string, T, StringHash, StringEqual >;

using string_set = robin_hood::unordered_set< std::string, StringHash, StringEqual >;

}