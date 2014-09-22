
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

#include "pool.h"

int Pool_init( struct Pool *self,
               int num_elements,
               int element_size,
               void *( *low_level_allocation_function )( int ),
               void ( *low_level_free_function )( void * ) )
{
    int r = -1;
    /* one bit per element */
    int size_of_allocated_flags_in_bytes = num_elements / 8 + 1;
    memset( self, 0, sizeof( *self ) );
    self->element_size = element_size;
    self->num_elements = num_elements;
    self->next_available_hint = 0;
    self->diag_num_allocations = 0;
    self->diag_num_frees = 0;
    self->diag_num_spills = 0;
    self->total_allocated_items = 0;
    self->diag_multiple_allocation_errors = 0;
    self->diag_multiple_deallocation_errors = 0;
    self->element_storage_size = num_elements * element_size;
    self->low_level_allocation_function = low_level_allocation_function;
    self->low_level_free_function = low_level_free_function;

    if ( self->element_storage_size > 0 )
    {
        self->allocated_flags = (unsigned char *)low_level_allocation_function( size_of_allocated_flags_in_bytes );
        if ( self->allocated_flags )
        {
            memset( self->allocated_flags, 0, size_of_allocated_flags_in_bytes );
            self->element_storage = (unsigned char *)low_level_allocation_function( self->element_storage_size );
            if ( self->element_storage )
            {
                memset( self->element_storage, 0, self->element_storage_size );
                r = 0;
            }
            else
            {
                low_level_free_function( self->allocated_flags );
            }
        }
    }
    return r;
}

void Pool_terminate( struct Pool *self )
{
    if ( self->element_storage )
    {
        self->low_level_free_function( self->element_storage );
    }
    if ( self->allocated_flags )
    {
        self->low_level_free_function( self->allocated_flags );
    }
}

void *Pool_allocate_element( struct Pool *self )
{
    void *r = 0;
    int item = -1;
    item = Pool_find_next_available_element( self );

    if ( item != -1 )
    {
        Pool_mark_element_allocated( self, item );
        r = Pool_get_address_for_element( self, item );
        ++self->diag_num_allocations;
    }
    else
    {
        ++self->diag_num_spills;
    }
    return r;
}

int Pool_deallocate_element( struct Pool *self, void *p )
{
    int item = Pool_get_element_for_address( self, p );
    if ( item >= 0 )
    {
        Pool_mark_element_available( self, item );
        ++self->diag_num_frees;
    }
    return item;
}

int Pool_is_element_available( struct Pool *self, int element_num )
{
    int r = 1;
    int element_div_8 = element_num / 8;
    int element_mod_8 = element_num & 7;
    unsigned char bit = ( 1 << element_mod_8 );

    if ( ( self->allocated_flags[element_div_8] & bit ) != 0 )
    {
        r = 0;
    }
    return r;
}

void Pool_mark_element_allocated( struct Pool *self, int element_num )
{
    int element_div_8 = element_num / 8;
    int element_mod_8 = element_num & 7;
    unsigned char bit = ( 1 << element_mod_8 );
    unsigned char flags = self->allocated_flags[element_div_8];

    if ( ( flags & bit ) == bit )
    {
        self->diag_multiple_allocation_errors++;
        POOL_ABORT( "Multiple allocation" );
    }
    else
    {
        self->allocated_flags[element_div_8] = flags | bit;
        ++self->total_allocated_items;
        self->next_available_hint = ( self->next_available_hint + 1 ) % self->num_elements;
    }
}

void Pool_mark_element_available( struct Pool *self, int element_num )
{
    int element_div_8 = element_num / 8;
    int element_mod_8 = element_num & 7;
    unsigned char bit = ( 1 << element_mod_8 );
    unsigned char mask_bit = ~bit;
    unsigned char flags = self->allocated_flags[element_div_8];

    if ( ( flags & bit ) == 0 )
    {
        self->diag_multiple_deallocation_errors++;
        POOL_ABORT( "Multiple deallocation" );
    }
    else
    {
        self->allocated_flags[element_div_8] = flags & mask_bit;
        --self->total_allocated_items;
        self->next_available_hint = element_num;
    }
}

void *Pool_get_address_for_element( struct Pool *self, int element_num )
{
    unsigned char *base = (unsigned char *)self->element_storage;
    void *r = 0;
    if ( element_num >= self->num_elements )
    {
        POOL_ABORT( "get_address_for_element out of range" );
    }
    r = base + ( element_num * self->element_size );
    return r;
}

int Pool_is_address_in_pool( struct Pool *self, void const *p )
{
    unsigned char *base = (unsigned char *)self->element_storage;
    unsigned char *top = base + self->element_storage_size;
    unsigned char const *pp = (unsigned char const *)p;
    int r = 0;

    if ( base <= pp && pp < top )
    {
        int offset = pp - base;
        if ( ( offset % self->element_size ) == 0 )
        {
            r = 1;
        }
    }
    return r;
}

int Pool_get_element_for_address( struct Pool *self, void const *p )
{
    unsigned char *base = (unsigned char *)self->element_storage;
    unsigned char *top = base + self->element_storage_size;
    unsigned char const *pp = (unsigned char const *)p;
    int r = -1;
    if ( base <= pp && pp < top )
    {
        int offset = pp - base;
        if ( ( offset % self->element_size ) == 0 )
        {
            r = offset / self->element_size;
        }
    }
    return r;
}

int Pool_find_next_available_element( struct Pool *self )
{
    int r = -1;
    if ( self->element_storage_size > 0 )
    {
        if ( self->total_allocated_items < self->num_elements )
        {
            int pos = self->next_available_hint;
            for ( int i = 0; i < self->num_elements; ++i )
            {
                int item = ( pos + i ) % self->num_elements;
                if ( Pool_is_element_available( self, item ) )
                {
                    r = pos;
                    self->next_available_hint = item;
                    break;
                }
            }
        }
        else
        {
            r = -1;
        }
    }
    return r;
}

#if defined( stdout ) && !defined( POOL_DISABLE_DIAGNOSTICS )

void Pool_diagnostics( struct Pool *self, FILE *f, const char *prefix )
{
    fprintf( f, "%selement_size                     : %d\n", prefix, self->element_size );
    fprintf( f, "%snum_elements                     : %d\n", prefix, self->num_elements );
    fprintf( f, "%stotal_allocated_items            : %d\n", prefix, self->total_allocated_items );
    fprintf( f, "%sdiag_multiple_allocation_errors  : %d\n", prefix, self->diag_multiple_allocation_errors );
    fprintf( f, "%sdiag_multiple_deallocation_errors: %d\n", prefix, self->diag_multiple_deallocation_errors );
    fprintf( f, "%sdiag_num_allocations             : %d\n", prefix, self->diag_num_allocations );
    fprintf( f, "%sdiag_num_frees                   : %d\n", prefix, self->diag_num_frees );
    fprintf( f, "%sdiag_num_spills                  : %d\n\n", prefix, self->diag_num_spills );
}

#endif
