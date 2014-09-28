#ifndef pools_allocator_hpp
#define pools_allocator_hpp

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

#include <memory>

extern "C" {
#include "pool.h"
#include "pools.h"
}

namespace PoolsAllocator
{
template <typename T>
struct pools_allocator : public std::allocator<T>
{
    typedef size_t size_type;
    typedef T *pointer;
    typedef const T *const_pointer;

    template <typename U>
    struct rebind
    {
        typedef pools_allocator<U> other;
    };

    pointer allocate( size_type n, const void *hint = 0 )
    {
        pointer p = 0;
        if ( m_pools )
        {
            p = static_cast<pointer>( Pools_allocate_element( m_pools, n * sizeof( T ) ) );
        }
        else
        {
            p = std::allocator<T>::allocate( n, hint );
        }
        return p;
    }

    void deallocate( pointer p, size_type n )
    {
        if ( m_pools )
        {
            Pools_deallocate_element( m_pools, p );
        }
        else
        {
            return std::allocator<T>::deallocate( p, n );
        }
    }

    pools_allocator( Pools *pools_to_use ) throw() : std::allocator<T>(), m_pools( pools_to_use ) {}

    pools_allocator( const pools_allocator &a ) throw() : std::allocator<T>( a ), m_pools( a.m_pools ) {}

    template <class U>
    pools_allocator( const pools_allocator<U> &a ) throw()
        : std::allocator<T>( a ), m_pools( a.m_pools )
    {
    }

    ~pools_allocator() throw() {}

    Pools *m_pools;
};
}

#endif
