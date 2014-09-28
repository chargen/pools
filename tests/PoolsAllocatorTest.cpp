/*
Copyright (c) 2014, Jeff Koftinoff <jeffk@jdkoftinoff.com>
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

* Redistributions of source code must retain the above copyright notice, this
  list of conditions and the following disclaimer.

* Redistributions in binary form must reproduce the above copyright notice,
  this list of conditions and the following disclaimer in the documentation
  and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <vector>
#include <string>
#include <iostream>

#include "PoolsAllocator.hpp"

void *my_allocation( size_t sz ) { return malloc( sz ); }

void my_free( void *p ) { free( p ); }

using my_string = std::basic_string<char, std::char_traits<char>, PoolsAllocator::pools_allocator<char> >;
using my_vector = std::vector<my_string, PoolsAllocator::pools_allocator<my_string> >;

template <typename T>
using my_allocator = PoolsAllocator::pools_allocator<T>;

int my_print( const char *s )
{
    std::cout << s << std::endl;
    return 0;
}

int main()
{
    Pools my_pools;
    Pools_init( &my_pools, "my_pools", my_allocation, my_free );
    //    Pools_add( &my_pools, 64, 1024 );
    //    Pools_add( &my_pools, 256, 1024 );

    my_allocator<my_string> all( &my_pools );

    {
        my_vector v{all};

        v.emplace_back( "one" );
        v.push_back( "two" );
        v.push_back( "three" );
        v.push_back( my_string(
            "123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef012"
            "3456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef01234"
            "56789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456"
            "789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef012345678"
            "9abcdef0123456789abcdef0123456789abcdef0123456789abcdef0" ) );
        v.push_back( "123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0" );

        for ( auto i = begin( v ); i != end( v ); ++i )
        {
            std::cout << *i << std::endl;
        }
        Pools_diagnostics( &my_pools, "", my_print );
    }
    Pools_diagnostics( &my_pools, "", my_print );

    Pools_terminate( &my_pools );
}
