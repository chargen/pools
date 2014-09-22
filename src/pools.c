
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

#include "pools.h"

int Pools_init( struct Pools *self,
                int const pool_item_sizes[POOLS_NUM_POOLS],
                int const pool_num_items[POOLS_NUM_POOLS],
                void *( *low_level_allocation_function )( int ),
                void ( *low_level_free_function )( void * ) )
{
    int r = -1;
    int i;
    self->low_level_allocation_function = low_level_allocation_function;
    self->low_level_free_function = low_level_free_function;
    self->diag_num_frees_from_heap = 0;
    self->diag_num_spills_handled = 0;
    self->diag_num_spills_to_heap = 0;

    for ( i = 0; i < POOLS_NUM_POOLS; ++i )
    {
        r = Pool_init(
            &self->pool[i], pool_num_items[i], pool_item_sizes[i], low_level_allocation_function, low_level_free_function );
        if ( r == -1 )
        {
            int n;
            for ( n = 0; n < i; ++n )
            {
                Pool_terminate( &self->pool[n] );
            }
            break;
        }
    }
    return r;
}

void Pools_terminate( struct Pools *self )
{
    int n;
    for ( n = 0; n < POOLS_NUM_POOLS; ++n )
    {
        Pool_terminate( &self->pool[n] );
    }
}

void *Pools_allocate_element( struct Pools *self, int size )
{
    void *r = 0;
    int i;
    for ( i = 0; i < POOLS_NUM_POOLS; ++i )
    {
        if ( size < self->pool[i].element_size )
        {
            r = Pool_allocate_element( &self->pool[i] );
            if ( r != 0 )
            {
                break;
            }
            else
            {
                ++self->diag_num_spills_handled;
            }
        }
    }
    if ( r == 0 )
    {
        ++self->diag_num_spills_to_heap;
        r = self->low_level_allocation_function( size );
    }
    return r;
}

void Pools_deallocate_element( struct Pools *self, void *p )
{
    int i;
    int deallocated = 0;
    if ( p )
    {
        for ( i = 0; i < POOLS_NUM_POOLS; ++i )
        {
            if ( Pool_is_address_in_pool( &self->pool[i], p ) )
            {
                Pool_deallocate_element( &self->pool[i], p );
                deallocated = 1;
                break;
            }
        }
        if ( !deallocated )
        {
            ++self->diag_num_frees_from_heap;
            self->low_level_free_function( p );
        }
    }
}

#if defined( stdout ) && !defined( POOL_DISABLE_DIAGNOSTICS )
void Pools_diagnostics( struct Pools *self, FILE *f, const char *prefix )
{
    int i;
    int total_items_still_allocated = 0;
    for ( i = 0; i < POOLS_NUM_POOLS; ++i )
    {
        char newprefix[128];
        sprintf( newprefix, "%s:%2d:[%6d]:", prefix, i, self->pool[i].element_size );
        Pool_diagnostics( &self->pool[i], f, newprefix );
        total_items_still_allocated += self->pool[i].total_allocated_items;
    }
    fprintf( f, "%s:summary:total_items_still_allocated :%d\n", prefix, total_items_still_allocated );
    fprintf( f, "%s:summary:diag_num_frees_from_heap    :%d\n", prefix, self->diag_num_frees_from_heap );
    fprintf( f, "%s:summary:diag_num_spills_handled     :%d\n", prefix, self->diag_num_spills_handled );
    fprintf( f, "%s:summary:diag_num_spills_to_heap     :%d\n\n", prefix, self->diag_num_spills_to_heap );
}

#endif
