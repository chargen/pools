
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

#include <stdlib.h>
#include "pools.h"

struct Pools my_pools;

void *my_low_level_allocation( int sz )
{
    return malloc( (size_t)sz );
}

void my_low_level_free( void *p )
{
    free( p );
}

#define EXERCISE_POOL_COUNT ( 32768 )
void exercise_pool()
{
    void *ptrs[EXERCISE_POOL_COUNT];
    int max_ptrs = EXERCISE_POOL_COUNT;
    int i;

    for ( i = 0; i < max_ptrs; ++i )
    {
        int sz = random() % 7000;
        ptrs[i] = Pools_allocate_element( &my_pools, sz );
    }

#if !defined( POOL_DISABLE_DIAGNOSTICS )
    Pools_diagnostics( &my_pools, stdout, "allocated" );
#endif

    for ( i = 0; i < max_ptrs; ++i )
    {
        Pools_deallocate_element( &my_pools, ptrs[i] );
    }

#if !defined( POOL_DISABLE_DIAGNOSTICS )
    Pools_diagnostics( &my_pools, stdout, "freed    " );
#endif
}

int main()
{
    int r = 255;
    if ( Pools_init( &my_pools, my_low_level_allocation, my_low_level_free ) == 0 )
    {
        r = 0;
        if ( Pools_add( &my_pools, 64, 1024 ) )
        {
            POOL_ABORT( "alloc" );
        }
        if ( Pools_add( &my_pools, 128, 1024 ) )
        {
            POOL_ABORT( "alloc" );
        }
        if ( Pools_add( &my_pools, 256, 1024 ) )
        {
            POOL_ABORT( "alloc" );
        }
        if ( Pools_add( &my_pools, 512, 1024 ) )
        {
            POOL_ABORT( "alloc" );
        }
        if ( Pools_add( &my_pools, 1024, 1024 ) )
        {
            POOL_ABORT( "alloc" );
        }
        if ( Pools_add( &my_pools, 2048, 1024 ) )
        {
            POOL_ABORT( "alloc" );
        }
        if ( Pools_add( &my_pools, 4096, 1024 ) )
        {
            POOL_ABORT( "alloc" );
        }
        if ( Pools_add( &my_pools, 8192, 1024 ) )
        {
            POOL_ABORT( "alloc" );
        }
        exercise_pool();

        Pools_terminate( &my_pools );
    }
    return r;
}
