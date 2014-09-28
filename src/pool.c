
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
               size_t num_elements,
               size_t element_size,
               void *( *low_level_allocation_function )( size_t ),
               void ( *low_level_free_function )( void * ) )
{
    int r = -1;
    /* one bit per element */
    size_t size_of_allocated_flags_in_bytes = ( num_elements + 7 ) / 8;
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
    else
    {
        r = 0;
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
    memset( self, 0, sizeof( *self ) );
}

void *Pool_allocate_element( struct Pool *self )
{
    void *r = 0;
    if ( self->num_elements > 0 )
    {
        ssize_t item = -1;
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
    }
    return r;
}

int Pool_deallocate_element( struct Pool *self, void *p )
{
    if ( self->num_elements > 0 )
    {
        ssize_t item = Pool_get_element_for_address( self, p );
        if ( item >= 0 )
        {
            Pool_mark_element_available( self, item );
            ++self->diag_num_frees;
        }
        return item;
    }
    else
    {
        return -1;
    }
}

int Pool_is_element_available( struct Pool *self, size_t element_num )
{
    int r = 0;
    if ( element_num < self->num_elements )
    {
        size_t element_div_8 = element_num / 8;
        size_t element_mod_8 = element_num & 7;
        unsigned char bit = ( 1 << element_mod_8 );
        unsigned char flags = self->allocated_flags[element_div_8];

        if ( ( flags & bit ) == 0 )
        {
            r = 1;
        }
    }
    else
    {
        POOL_ABORT( "is_element_available given invalid element_num" );
    }
    return r;
}

void Pool_mark_element_allocated( struct Pool *self, size_t element_num )
{
    size_t element_div_8 = element_num / 8;
    size_t element_mod_8 = element_num & 7;
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

void Pool_mark_element_available( struct Pool *self, size_t element_num )
{
    size_t element_div_8 = element_num / 8;
    size_t element_mod_8 = element_num & 7;
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

void *Pool_get_address_for_element( struct Pool *self, size_t element_num )
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
        int offset = (int)( pp - base );
        if ( ( offset % self->element_size ) == 0 )
        {
            r = 1;
        }
    }
    return r;
}

ssize_t Pool_get_element_for_address( struct Pool *self, void const *p )
{
    unsigned char *base = (unsigned char *)self->element_storage;
    unsigned char *top = base + self->element_storage_size;
    unsigned char const *pp = (unsigned char const *)p;
    ssize_t r = -1;
    if ( base <= pp && pp < top )
    {
        int offset = (int)( pp - base );
        if ( ( offset % self->element_size ) == 0 )
        {
            r = offset / self->element_size;
        }
    }
    return r;
}

ssize_t Pool_find_next_available_element( struct Pool *self )
{
    ssize_t r = -1;
    if ( self->element_storage_size > 0 )
    {
        if ( self->total_allocated_items < self->num_elements )
        {
            size_t pos = self->next_available_hint;
            for ( size_t i = 0; i < self->num_elements; ++i )
            {
                int item = ( pos + i ) % self->num_elements;
                if ( Pool_is_element_available( self, item ) )
                {
                    r = item;
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

void Pool_diagnostics( struct Pool *self, const char *prefix, int ( *print )( const char * ) )
{
    size_t actual_allocated_items = 0;
    size_t i;
    for ( i = 0; i < self->num_elements; ++i )
    {
        if ( !Pool_is_element_available( self, i ) )
        {
            actual_allocated_items++;
        }
    }
    char buf[128];
    sprintf( buf, "%selement_size                     : %zu", prefix, self->element_size );
    print( buf );
    sprintf( buf, "%snum_elements                     : %zu", prefix, self->num_elements );
    print( buf );
    sprintf( buf, "%stotal_allocated_items            : %zu", prefix, self->total_allocated_items );
    print( buf );
    sprintf( buf, "%sactual_allocated_items           : %zu", prefix, actual_allocated_items );
    print( buf );
    sprintf( buf, "%sdiag_multiple_allocation_errors  : %zu", prefix, self->diag_multiple_allocation_errors );
    print( buf );
    sprintf( buf, "%sdiag_multiple_deallocation_errors: %zu", prefix, self->diag_multiple_deallocation_errors );
    print( buf );
    sprintf( buf, "%sdiag_num_allocations             : %zu", prefix, self->diag_num_allocations );
    print( buf );
    sprintf( buf, "%sdiag_num_frees                   : %zu", prefix, self->diag_num_frees );
    print( buf );
    sprintf( buf, "%sdiag_num_spills                  : %zu", prefix, self->diag_num_spills );
    print( buf );
    print( "" );
}

#endif
