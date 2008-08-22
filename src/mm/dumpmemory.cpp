/* src/mm/dumpmemory.cpp - dump memory management

   Copyright (C) 1996-2005, 2006, 2007, 2008
   CACAOVM - Verein zur Foerderung der freien virtuellen Maschine CACAO
   Copyright (C) 2008 Theobroma Systems Ltd.

   This file is part of CACAO.

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2, or (at
   your option) any later version.

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
   02110-1301, USA.

*/


#include "config.h"

#include <stdint.h>

#include "mm/dumpmemory.hpp"
#include "mm/memory.h"

#include "threads/thread.hpp"

#include "vm/options.h"
#include "vm/os.hpp"

#if defined(ENABLE_STATISTICS)
# include "vm/statistics.h"
#endif

#include "vm/vm.hpp"


/*******************************************************************************

  This structure is used for dump memory allocation if cacao
  runs without threads.

*******************************************************************************/

#if !defined(ENABLE_THREADS)
static dumpinfo_t _no_threads_dumpinfo;
#endif


/* dump_check_canaries *********************************************************

   Check canaries in dump memory.

   IN:
      di...........dumpinfo_t * of the dump area to check
	  bottomsize...dump size down to which the dump area should be checked
	               (specify 0 to check the whole dump area)

   ERROR HANDLING:
      If any canary has been changed, this function aborts the VM with
	  an error message.

*******************************************************************************/

#if defined(ENABLE_MEMCHECK)
static void dump_check_canaries(dumpinfo_t *di, s4 bottomsize)
{
	dump_allocation_t *da;
	uint8_t           *pm;
	int                i, j;

	/* iterate over all dump memory allocations above bottomsize */

	da = di->allocations;

	while (da && da->used >= bottomsize) {
		/* check canaries */

		pm = ((uint8_t *) da->mem) - MEMORY_CANARY_SIZE;

		for (i = 0; i < MEMORY_CANARY_SIZE; ++i) {
			if (pm[i] != i + MEMORY_CANARY_FIRST_BYTE) {
				fprintf(stderr, "canary bytes:");

				for (j = 0; j < MEMORY_CANARY_SIZE; ++j)
					fprintf(stderr, " %02x", pm[j]);

				fprintf(stderr,"\n");

				vm_abort("error: dump memory bottom canary killed: "
						 "%p (%d bytes allocated at %p)\n",
						 pm + i, da->size, da->mem);
			}
		}

		pm = ((uint8_t *) da->mem) + da->size;

		for (i = 0; i < MEMORY_CANARY_SIZE; ++i) {
			if (pm[i] != i + MEMORY_CANARY_FIRST_BYTE) {
				fprintf(stderr, "canary bytes:");

				for (j = 0; j < MEMORY_CANARY_SIZE; ++j)
					fprintf(stderr, " %02x", pm[j]);

				fprintf(stderr, "\n");

				vm_abort("error: dump memory top canary killed: "
						 "%p (%d bytes allocated at %p)\n",
						 pm + i, da->size, da->mem);
			}
		}

		da = da->next;
	}
}
#endif /* defined(ENABLE_MEMCHECK) */


/**
 * Allocate a new thread-local dump memory structure.
 */
DumpMemory::DumpMemory() : _size(0), _used(0)
{
}


/**
 * Stupid realloc implementation for dump memory.  Avoid, if possible.
 */
void* DumpMemory::reallocate(void* src, size_t len1, size_t len2)
{
	void* dst = allocate(len2);

	(void) os::memcpy(dst, src, len1);

#if defined(ENABLE_MEMCHECK)
	// Destroy the source.
	(void) os::memset(src, MEMORY_CLEAR_BYTE, len1);
#endif

	return dst;
}


/**
 * Add the given dump area to the area list.
 *
 * @param dm Pointer to dump area.
 */
void DumpMemory::add_area(DumpMemoryArea* dma)
{
	_areas.push_back(dma);

	// Increase the size count of the dump memory.
	_size += dma->get_size();

#if defined(ENABLE_STATISTICS)
	if (opt_stat) {
		if (_size > (size_t) maxdumpsize)
			maxdumpsize = _size;
	}
#endif
}


/**
 * Remove the given dump area from the area list.
 *
 * @param dm Pointer to dump area.
 */
void DumpMemory::remove_area(DumpMemoryArea* dma)
{
	// Sanity check.
	assert(_areas.back() == dma);

	// Remove the last area from the list.  The check above guarantees
	// we are removing the correct area.
	_areas.pop_back();

	// Decrease the size and used count.
	_size -= dma->get_size();
	_used -= dma->get_used();
}


/**
 * Allocate a new dump memory area.
 *
 * @ param size Required memory size.
 */
DumpMemoryArea::DumpMemoryArea(size_t size) : _size(0), _used(0)
{
	// Get the DumpMemory object of the current thread.
	DumpMemory* dm = DumpMemory::get_current();

	// Add this area to the areas list.
	dm->add_area(this);
}


/**
 * Release all dump memory blocks in the current dump area.
 */
DumpMemoryArea::~DumpMemoryArea()
{
	// Get the DumpMemory object of the current thread.
	DumpMemory* dm = DumpMemory::get_current();

#if defined(ENABLE_MEMCHECK)
	{
		dump_allocation_t *da, *next;

		/* check canaries */

		dump_check_canaries(di, size);

		/* iterate over all dump memory allocations about to be released */

		da = di->allocations;

		while ((da != NULL) && (da->used >= size)) {
			next = da->next;

			/* invalidate the freed memory */

			(void) os_memset(da->mem, MEMORY_CLEAR_BYTE, da->size);

			FREE(da, dump_allocation_t);

			da = next;
		}
		di->allocations = da;
	}
#endif /* defined(ENABLE_MEMCHECK) */


	// Free all memory blocks.
	for (std::vector<DumpMemoryBlock*>::iterator it = _blocks.begin(); it != _blocks.end(); it++) {
		// Call the destructor of the current block.
		delete *it;
	}

	// Remove this area for the area list.
	dm->remove_area(this);
}


/**
 * Allocate a dump memory block for the current dump memory area.
 *
 * @param size Required memory size.
 *
 * @return Pointer to the newly allocated block.
 */
DumpMemoryBlock* DumpMemoryArea::allocate_new_block(size_t size)
{
	DumpMemoryBlock* dmb = new DumpMemoryBlock(size);
	_blocks.push_back(dmb);

#if defined(ENABLE_STATISTICS)
	if (opt_stat) {
		DumpMemory* dm = DumpMemory::get_current();
		dm->add_size(dmb->get_size());

		if (dm->get_size() > (size_t) maxdumpsize)
			maxdumpsize = dm->get_size();
	}
#endif

	return dmb;
}


/**
 * Allocate a memory block for the current dump memory block.
 *
 * @param size Required memory size.
 */
DumpMemoryBlock::DumpMemoryBlock(size_t size) : _size(0), _used(0), _block(0)
{
	// If requested size is greater than the default, make the new
	// memory block as big as the requested size.  Otherwise use the
	// default size.
	_size = (size > DEFAULT_SIZE) ? size : DEFAULT_SIZE;

	// Allocate a memory block.
	_block = memory_checked_alloc(_size);

#if defined(ENABLE_STATISTICS)
	// The amount of globally allocated dump memory (thread safe).
	if (opt_stat)
		globalallocateddumpsize += _size;
#endif
}


/**
 * Release the memory block for the dump memory block.
 *
 * @param size Required memory size.
 */
DumpMemoryBlock::~DumpMemoryBlock()
{
	// Release the memory block.
	mem_free(_block, /* XXX */ 1);

#if defined(ENABLE_STATISTICS)
	// The amount of globally allocated dump memory (thread safe).
	if (opt_stat)
		globalallocateddumpsize -= _size;
#endif
}


// Legacy C interface.

extern "C" {
	void* DumpMemory_allocate(size_t size) { return DumpMemory::allocate(size); }
	void* DumpMemory_reallocate(void* src, size_t len1, size_t len2) { return DumpMemory::reallocate(src, len1, len2); }
}


/*
 * These are local overrides for various environment variables in Emacs.
 * Please do not remove this and leave it at the end of the file, where
 * Emacs will automagically detect them.
 * ---------------------------------------------------------------------
 * Local variables:
 * mode: c++
 * indent-tabs-mode: t
 * c-basic-offset: 4
 * tab-width: 4
 * End:
 * vim:noexpandtab:sw=4:ts=4:
 */
