#ifndef pools_h
#define pools_h

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

#define POOLS_MAX_POOLS ( 16 )

struct Pools
{
    /**
     * @brief num_pools The current number of pools
     */
    size_t num_pools;

    /**
     * @brief pool The array of pools.
     */
    struct Pool pool[POOLS_MAX_POOLS];

    /**
     * @brief low_level_allocation_function the pointer to the system's low level allocation function
     */
    void *( *low_level_allocation_function )( size_t );

    /**
     * @brief low_level_free_function The pointer to the system's low level free dunction
     */
    void ( *low_level_free_function )( void * );

    /**
     * @brief diag_num_spills_handled Diagnostics counter of the number of spills that happened but were handled by another Pool
     */
    size_t diag_num_spills_handled;

    /**
     * @brief diag_num_spills_to_heap Diagnostics counter of the number of spills that happened that had to be handled by the
     * system heap
     */
    size_t diag_num_spills_to_heap;

    /**
     * @brief diag_num_frees_from_heap Diagnostics counter of the number of frees of objects that were allocated on the heap
     * because of a spill
     */
    size_t diag_num_frees_from_heap;

    /**
     * @brief name The name of this collection of Pools
     */
    const char *name;
};

/**
 * @brief Pools_init                    Initialize a Pools structure, a set of POOLS_MAX_POOLS pools
 * @param self                          Pointer to Pools struct to init
 * @param name                          Pointer to string of name of this collection of pools
 * @param low_level_allocation_function Pointer to low level memory allocation function
 * @param low_level_free_function       Pointer to low level memory free function
 * @return                              -1 on error, 0 on success
 */
int Pools_init( struct Pools *self,
                const char *name,
                void *( *low_level_allocation_function )( size_t ),
                void ( *low_level_free_function )( void * ) );

/**
 * @brief Pools_add                     Add a pool to a set of Pools
 * @param self                          Pointer to Pools struct to add a pool to
 * @param element_size                  The size of the element for this new pool
 * @param num_elements                  The number of elements for this new pool
 * @return                              -1 on error, 0 on success
 */
int Pools_add( struct Pools *self, size_t element_size, size_t number_of_elements );

/**
 * @brief Pools_terminate           Terminate a Pools and deallocate low level buffers used by all
 *                                  pools except the spills onto the heap.
 * @param self                      Pointer to the Pools to terminate
 */
void Pools_terminate( struct Pools *self );

/**
 * @brief Pools_allocate_element    Attempt to allocate space for an object from the best Pool, or use the heap if none are
 * available.
 * @param self                      Pointer to Pools struct
 * @param size                      Size of the item to allocate
 * @return                          pointer to allocated item, or 0 on error
 */
void *Pools_allocate_element( struct Pools *self, size_t size );

/**
 * @brief Pools_deallocate_element  Find the pool that a pointer was allocated from and do the appropriate thing to de-allocate
 * it.
 * @param self                      Pointer to Pools struct
 * @param p                         Pointer to allocated item
 */
void Pools_deallocate_element( struct Pools *self, void *p );

#if defined( stdout ) && !defined( POOL_DISABLE_DIAGNOSTICS )
/**
 * @brief Pools_diagnostics          Print pool diagnostics counters
 * @param self                      Pointer to Pool struct to diagnose
 * @param prefix                    Pointer to cstring which will be put in front of each line outputted
 * @param print                     Pointer to function to be called for each line of text
 */
void Pools_diagnostics( struct Pools *self, const char *prefix, int ( *print )( const char * ) );
#endif

#endif
