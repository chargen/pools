#ifndef pool_h
#define pool_h

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
#include <string.h>
#include <stdio.h>

#if !defined( POOL_NO_ABORT_ON_ERROR ) && !defined( POOL_ABORT )
#define POOL_ABORT( msg )                                                                                                      \
    do                                                                                                                         \
    {                                                                                                                          \
        fprintf( stderr, "%s", msg );                                                                                          \
        abort();                                                                                                               \
    } while ( 0 )
#else
#define POOL_ABORT( msg )                                                                                                      \
    do                                                                                                                         \
    {                                                                                                                          \
    } while ( 0 )
#endif

struct Pool
{
    /**
     * @brief num_elements The number of elements in this pool
     */
    int num_elements;

    /**
     * @brief element_size The size in bytes of each element
     */
    int element_size;

    /**
     * @brief next_available_hint The best guess of the next available element
     */
    int next_available_hint;

    /**
     * @brief element_storage_size The total size in bytes of the element_storage buffer
     */
    int element_storage_size;

    /**
     * @brief total_allocated_items The total number of items that are currently allocated
     */
    int total_allocated_items;

    /**
     * @brief allocated_flags The storage for the bit map of allocated/deallocated flags. One bit per element
     */
    unsigned char *allocated_flags;

    /**
     * @brief element_storage The storage for all of the element
     */
    unsigned char *element_storage;

    /**
     * @brief diag_num_allocations Diagnostics counter for the number of allocations
     */
    int diag_num_allocations;

    /**
     * @brief diag_num_frees Diagnostics counter for the number of frees
     */
    int diag_num_frees;

    /**
     * @brief diag_num_spills Diagnostics counter for the number of spills : allocations that had to be pushed to a larger pool
     */
    int diag_num_spills;

    /**
     * @brief diag_multiple_allocation_errors Diagnostics counter for the number of times an element was allocated more than
     * once at a time
     */
    int diag_multiple_allocation_errors;

    /**
     * @brief diag_multiple_deallocation_errors Diagnostics counter for the number of times an element was deallocated more than
     * once at a time
     */
    int diag_multiple_deallocation_errors;

    /**
     * @brief low_level_allocation_function the pointer to the system's low level allocation function
     */
    void *( *low_level_allocation_function )( int );

    /**
     * @brief low_level_free_function The pointer to the system's low level free dunction
     */
    void ( *low_level_free_function )( void * );
};

/**
 * @brief Pool_init                     Initialize a Pool
 * @param self                          Pointer to Pool struct to initialize
 * @param num_elements                  The number of elements to allocate. May be 0 to disable Pool.
 * @param element_size                  The size of each element in bytes.May be 0 to disable Pool.
 * @param low_level_allocation_function Pointer to low level memory allocation function
 * @param low_level_free_function       Pointer to low level memory free function
 * @return                              -1 on error, 0 on success
 */
int Pool_init( struct Pool *self,
               int num_elements,
               int element_size,
               void *( *low_level_allocation_function )( int ),
               void ( *low_level_free_function )( void * ) );

/**
 * @brief Pool_terminate            Terminate a Pool and deallocate low level buffers
 * @param self                      Pointer to the Pool to terminate
 */
void Pool_terminate( struct Pool *self );

/**
 * @brief Pool_allocate_element     Allocate one element from the pool
 * @param self                      The pool to allocate from
 * @return                          0 on failure or pointer to allocated element
 */
void *Pool_allocate_element( struct Pool *self );

/**
 * @brief Pool_deallocate_element   Deallocate one element from the pool
 * @param self                      The pool to deallocate from
 * @param p                         The pointer to deallocate
 * @return                          -1 if the item is not allocated from this pool, or the item index if positive
 */
int Pool_deallocate_element( struct Pool *self, void *p );

/**
 * @brief Pool_is_element_available Check to see if a specific element index is available
 * @param self                      The Pool to use
 * @param element_num               The element index to check
 * @return                          1 if the element is available, 0 otherwise.
 */
int Pool_is_element_available( struct Pool *self, int element_num );

/**
 * @brief Pool_mark_element_allocated   Mark the specified element as allocated
 * @param self                          The Pool to use
 * @param element_num                   The element index to mark as allocated
 */
void Pool_mark_element_allocated( struct Pool *self, int element_num );

/**
 * @brief Pool_mark_element_available   Mark the specified element as available
 * @param self                          The Pool to use
 * @param element_num                   The element index to mark as available
 */
void Pool_mark_element_available( struct Pool *self, int element_num );

/**
 * @brief Pool_get_address_for_element  Calculate the memory address of a specific element
 * @param self                          The Pool to use
 * @param element_num                   The element index to use
 * @return                              Pointer to the element, or 0 if the element_num is out of range
 */
void *Pool_get_address_for_element( struct Pool *self, int element_num );

/**
 * @brief Pool_is_address_in_pool       Calculate if the specified address points to an element in this Pool
 * @param self                          The Pool to use
 * @param p                             The pointer to check
 * @return                              1 if the address points to the beginning of an element inside this Pool, 0 otherwise
 */
int Pool_is_address_in_pool( struct Pool *self, void const *p );

/**
 * @brief Pool_get_element_for_address  Calculate the element number given a pointer
 * @param self                          The Pool to use
 * @param p                             The pointer to check
 * @return                              The element number, or -1 if the pointer is not pointing to the beginning of an element
 * in this Pool
 */
int Pool_get_element_for_address( struct Pool *self, void const *p );
int Pool_find_next_available_element( struct Pool *self );

#if defined( stdout ) && !defined( POOL_DISABLE_DIAGNOSTICS )
/**
 * @brief Pool_diagnostics          Print pool diagnostics counters
 * @param self                      Pointer to Pool struct to diagnose
 * @param f                         Pointer to FILE to print to
 * @param prefix                    Pointer to cstring which will be put in front of each line outputted
 */
void Pool_diagnostics( struct Pool *self, FILE *f, const char *prefix );
#endif

#endif
