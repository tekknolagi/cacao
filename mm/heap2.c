#include <stddef.h>
#include <unistd.h>		/* getpagesize, mmap, ... */
#include <sys/mman.h>
#ifndef MAP_FAILED
#define MAP_FAILED ((void*) -1)
#endif
#include <sys/types.h>
#include <stdio.h>
#include "../asmpart.h"
#include "../callargs.h"
#include "../threads/thread.h"
#include "../threads/locks.h"
#include <assert.h>

#include "mm.h"

#define PAGESIZE_MINUS_ONE	(getpagesize() - 1)

#undef ALIGN
#undef OFFSET

#define false 0
#define true 1

#include "allocator.h" /* rev. 1 allocator */
#include "bitmap2.h"   /* rev. 2 bitmap management */

bool collectverbose;

void gc_call (void);

/* --- macros */

#define align_size(size)	((size) & ~((1 << ALIGN) - 1))
#define MAP_ADDRESS			(void*) 0x10000000

#define VERBOSITY_MESSAGE	1
#define VERBOSITY_DEBUG		2
#define VERBOSITY_MISTRUST	3
#define VERBOSITY_TRACE		4
#define VERBOSITY_PARANOIA	5
#define VERBOSITY_LIFESPAN	6

//#define VERBOSITY			VERBOSITY_MESSAGE
//#define VERBOSITY			VERBOSITY_PARANOIA
#define VERBOSITY			VERBOSITY_LIFESPAN

/* --- file-wide variables */

static void*	heap_base = NULL;
static SIZE		heap_size = 0;
static void*	heap_top = NULL;
static void*	heap_limit = NULL;
static void*	heap_next_collection = NULL;

static bitmap_t* start_bitmap = NULL;
static BITBLOCK* start_bits = NULL;
static bitmap_t* reference_bitmap = NULL;
static BITBLOCK* reference_bits = NULL;
static bitmap_t* mark_bitmap = NULL;
static BITBLOCK* mark_bits = NULL;

static void**	stackbottom = NULL;

typedef struct address_list_node {
	void* address;
	struct address_list_node* next;
} address_list_node;

static address_list_node* references = NULL;
static address_list_node* finalizers = NULL;

/* --- implementation */

void 
heap_init (SIZE size, 
		   SIZE startsize, /* when should we collect for the first time ? */
		   void **in_stackbottom)
{
#if VERBOSITY == VERBOSITY_MESSAGE
	fprintf(stderr, "The heap is verbose.\n");
#elif VERBOSITY == VERBOSITY_DEBUG
	fprintf(stderr, "The heap is in debug mode.\n");
#elif VERBOSITY == VERBOSITY_MISTRUST
	fprintf(stderr, "The heap is mistrusting us.\n");
#elif VERBOSITY == VERBOSITY_TRACE
	fprintf(stderr, "The heap is outputting trace information.\n");
#elif VERBOSITY == VERBOSITY_PARANOIA
	fprintf(stderr, "The heap is paranoid.\n");
#elif VERBOSITY == VERBOSITY_LIFESPAN
	fprintf(stderr, "The heap is collecting lifespan information.\n");
#endif

	/* 1. Initialise the freelists & reset the allocator's state */
	allocator_init(); 

	/* 2. Allocate at least (alignment!) size bytes of memory for the heap */
	heap_size = align_size(size + ((1 << ALIGN) - 1));

#if 1
	/* For now, we'll try to map in the stack at a fixed address...
	   ...this will change soon. -- phil. */
	heap_base = (void*) mmap (MAP_ADDRESS, 
							  ((size_t)heap_size + PAGESIZE_MINUS_ONE) & ~PAGESIZE_MINUS_ONE,
							  PROT_READ | PROT_WRITE, 
							  MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED, 
							  -1, 
							  (off_t) 0);
#else
	heap_base = malloc(heap_size);
#endif

	if (heap_base == MAP_FAILED) {
		/* unable to allocate the requested amount of memory */
		fprintf(stderr, "The queen, mylord, is dead!\n");
		exit(-1);
	}

	/* 3. Allocate the bitmaps */
	start_bitmap = bitmap_allocate(heap_base, heap_size);
	reference_bitmap = bitmap_allocate(heap_base, heap_size);
	mark_bitmap = bitmap_allocate(heap_base, heap_size);

	start_bits = start_bitmap->bitmap;
    reference_bits = reference_bitmap->bitmap;
    mark_bits = mark_bitmap->bitmap;

	/* 4. Mark the first free-area as an object-start */
	bitmap_setbit(start_bits, heap_base);

	/* 5. Initialise the heap's state (heap_top, etc.) */
	stackbottom = in_stackbottom; /* copy the stackbottom */

	heap_top = heap_base; /* the current end of the heap (just behind the last allocated object) */
	heap_limit = heap_base + heap_size; /* points just behind the last accessible block of the heap */

	/* 6. calculate a useful first collection limit */
	/* This is extremly primitive at this point... 
	   we should replace it with something more useful -- phil. */
	heap_next_collection = heap_base + (heap_size / 8);

	/* 7. Init the global reference lists & finalizer addresses */
	references = NULL;
	finalizers = NULL;
}

void 
heap_close (void)
{
	/* 1. Clean up on the heap... finalize all remaining objects */
#if 0
	address_list_node* curr = finalizers;
	while (curr) {
		address_list_node* prev = curr;

		if (bitmap_testbit(start_bits, curr->address))
			asm_calljavamethod(((java_objectheader*)(curr->address))->vftbl->class->finalizer, curr->address, NULL, NULL, NULL);

		curr = curr->next;
		free(prev);
	}
#endif

	/* 2. Release the bitmaps */
	bitmap_release(start_bitmap);
	bitmap_release(reference_bitmap);
	bitmap_release(mark_bitmap);

	/* 3. Release the memory allocated to the heap */
	if (heap_base)
		munmap(heap_base, heap_size);
}

static
inline
void
heap_add_address_to_address_list(address_list_node** list, void* address)
{
	/* Note: address lists are kept sorted to simplify finalization */

	address_list_node* new_node = malloc(sizeof(address_list_node));
	new_node->address = address;
	new_node->next = NULL;

	while (*list && (*list)->next) {
#if VERBOSITY >= VERBOSITY_PARANOIA
		if ((*list)->address == address)
			fprintf(stderr,
					"Attempt to add a duplicate adress to an adress list.\n");
#endif

		if ((*list)->next->address < address)
			list = &(*list)->next;
		else {
			new_node->next = *list;
			*list = new_node;
			return;
		}			
	}

	new_node->next = *list;
	*list = new_node;
}

static
inline
void
heap_add_finalizer_for_object_at(void* addr)
{
	/* Finalizers seem to be very rare... for this reason, I keep a linked
	   list of object addresses, which have a finalizer attached. This list
	   is kept in ascending order according to the order garbage is freed.
	   This list is currently kept separate from the heap, but should be 
	   moved onto it, but some JIT-marker code to handle these special
	   objects will need to be added first. -- phil. */

	heap_add_address_to_address_list(&finalizers, addr);
}

void* 
heap_allocate (SIZE		  in_length, 
			   bool 	  references, 
			   methodinfo *finalizer)
{
	SIZE	length = align_size(in_length + ((1 << ALIGN) - 1)); 
	void*	free_chunk = NULL;

#if VERBOSITY >= VERBOSITY_MISTRUST && 0
	/* check for misaligned in_length parameter */
	if (length != in_length)
		fprintf(stderr,
				"heap2.c: heap_allocate was passed unaligned in_length parameter: %ld, \n         aligned to %ld. (mistrust)\n",
				in_length, length);
#endif

	// fprintf(stderr, "heap_allocate: 0x%lx (%ld) requested, 0x%lx (%ld) aligned\n", in_length, in_length, length, length);

	if (finalizer)
		fprintf(stderr, "finalizer detected\n");

#if VERBOSITY >= VERBOSITY_LIFESPAN 
	/* perform garbage collection to collect data for lifespan analysis */
	if (heap_top > heap_base)
		gc_call();
#endif

	intsDisable();

 retry:
	/* 1. attempt to get a free block with size >= length from the freelists */
	free_chunk = allocator_alloc(length);

	/* 2. if unsuccessful, try alternative allocation strategies */
	if (!free_chunk) {
		/* 2.a if the collection threshold would be exceeded, collect the heap */
		if (heap_top + length > heap_next_collection) {
			/* 2.a.1. collect if the next_collection threshold would be exceeded */
			gc_call();

			/* 2.a.2. we just ran a collection, recheck the freelists */
			free_chunk = allocator_alloc(length);
			if (free_chunk)
				goto success;

			/* 2.a.3. we can't satisfy the request from the freelists, check
			          against the heap_limit whether growing the heap is possible */
			if (heap_top + length > heap_limit)
				goto failure;
		}

		/* 2.b. grow the heap */
		free_chunk = heap_top;
		heap_top += length;
	}

 success:
	/* 3.a. mark all necessary bits, store the finalizer & return the newly allocate block */
#if 0
	/* I don't mark the object-start anymore, as it always is at the beginning of a free-block,
	   which already is marked (Note: The first free-block gets marked in heap_init). -- phil. */
  	bitmap_setbit(start_bits, free_chunk); /* mark the new object */
#endif
	bitmap_setbit(start_bits, free_chunk + length); /* mark the freespace behind the new object */
	if (references)
		bitmap_setbit(reference_bits, free_chunk);
	else 
		bitmap_clearbit(reference_bits, free_chunk);

#if 1
	/* store a hint, that there's a finalizer */
	if (finalizer)
		heap_add_finalizer_for_object_at(free_chunk);
#else
#   warning "finalizers disabled."
#endif

#if VERBOSITY >= VERBOSITY_TRACE
	fprintf(stderr, "heap_allocate: returning free block of 0x%lx bytes at 0x%lx\n", length, free_chunk);
#endif
#if VERBOSITY >= VERBOSITY_PARANOIA && 0
	fprintf(stderr, "heap_allocate: reporting heap state variables:\n");
	fprintf(stderr, "\theap top              0x%lx\n", heap_top);
	fprintf(stderr, "\theap size             0x%lx (%ld)\n", heap_size, heap_size);
	fprintf(stderr, "\theap_limit            0x%lx\n", heap_limit);
	fprintf(stderr, "\theap next collection  0x%lx\n", heap_next_collection);
#endif

	return free_chunk;
	
 failure:
	/* 3.b. failure to allocate enough memory... fail gracefully */
#if VERBOSITY >= VERBOSITY_MESSAGE
	fprintf(stderr, 
			"heap2.c: heap_allocate was unable to allocate 0x%lx bytes on the VM heap.\n",
			length);
#endif

#if VERBOSITY >= VERBOSITY_TRACE
	fprintf(stderr, "heap_allocate: returning NULL\n");
#endif

	return NULL;

	intsRestore();
}

void 
heap_addreference (void **reflocation)
{
	/* I currently use a separate linked list (as in the original code) to hold
	   the global reference locations, but I'll change this to allocate these
	   in blocks on the heap; we'll have to add JIT-Marker code for those Java
	   objects then. -- phil. */

	heap_add_address_to_address_list(&references, reflocation);
}

static
inline
void gc_finalize (void)
{
	/* This will have to be slightly rewritten as soon the JIT-marked heap-based lists are used. -- phil. */

	address_list_node*  curr = finalizers;
	address_list_node*  prev;

#if 0
	while (curr) {
		if (!bitmap_testbit(mark_bits, curr->address)) {
			/* I need a way to save derefs. Any suggestions? -- phil. */
			asm_calljavamethod(((java_objectheader*)curr->address)->vftbl->class->finalizer, 
							   curr->address, NULL, NULL, NULL);

			prev->next = curr->next;			
			free(curr);
		} else {
			prev = curr;
			curr = curr->next;
		}
	}
#else
	while (curr) {
		if (curr->address) {
			if (!bitmap_testbit(mark_bits, curr->address)) {
				fprintf(stderr, "executing finalizer\n");
				asm_calljavamethod(((java_objectheader*)curr->address)->vftbl->class->finalizer, 
								   curr->address, NULL, NULL, NULL);
				curr->address = 0;
			}
		}

		curr = curr->next;
	}
#endif
}

static 
inline 
void gc_reclaim (void)
{
	void* free_start;
	void* free_end = heap_base;
	BITBLOCK* temp_bits;
	bitmap_t* temp_bitmap;

	/* 1. reset the freelists */
	allocator_reset();

	/* 2. reclaim unmarked objects */
#if 0 && SIZE_FROM_CLASSINFO
	free_start = bitmap_find_next_combination_set_unset(start_bitmap,
														mark_bitmap,
														free_end);
	while (free_start < heap_top) {
		if (!bitmap_testbit(start_bits, free_start) || 
			bitmap_testbit(mark_bits, free_start)) {
			fprintf(stderr, "gc_reclaim: inconsistent bit info for 0x%lx\n", free_start);
		}

		free_end = free_start;
		while((free_end < heap_top) && 
			  (!bitmap_testbit(mark_bits, free_end)) {
			free_end += 
		}



bitmap_find_next_setbit(mark_bitmap, free_start + 8); /* FIXME: constant used */

			//			fprintf(stderr, "%lx -- %lx\n", free_start, free_end);
			
			if (free_end < heap_top) {
				allocator_free(free_start, free_end - free_start);

				//				fprintf(stderr, "gc_reclaim: freed 0x%lx bytes at 0x%lx\n", free_end - free_start, free_start);

#if !defined(JIT_MARKER_SUPPORT)
				/* might make trouble with JIT-Marker support. The Marker for unused blocks 
				   might be called, leading to a bad dereference. -- phil. */
				bitmap_setbit(mark_bits, free_start);
#endif
			}
		} else {
			//			fprintf(stderr, "hmmm... found freeable area of 0 bytes at heaptop ?!\n");
			free_end = heap_top;
		}			
	}
#else
	while (free_end < heap_top) {
		free_start = bitmap_find_next_combination_set_unset(start_bitmap,
															mark_bitmap,
															free_end);

		if (!bitmap_testbit(start_bits, free_start) || 
			bitmap_testbit(mark_bits, free_start))
			fprintf(stderr, "gc_reclaim: inconsistent bit info for 0x%lx\n", free_start);

		if (free_start < heap_top) {
			free_end   = bitmap_find_next_setbit(mark_bitmap, free_start + 8); /* FIXME: constant used */

			//			fprintf(stderr, "%lx -- %lx\n", free_start, free_end);
			
			if (free_end < heap_top) {
				allocator_free(free_start, free_end - free_start);

				//				fprintf(stderr, "gc_reclaim: freed 0x%lx bytes at 0x%lx\n", free_end - free_start, free_start);

#if !defined(JIT_MARKER_SUPPORT)
				/* might make trouble with JIT-Marker support. The Marker for unused blocks 
				   might be called, leading to a bad dereference. -- phil. */
				bitmap_setbit(mark_bits, free_start);
#endif
			}
		} else {
			//			fprintf(stderr, "hmmm... found freeable area of 0 bytes at heaptop ?!\n");
			free_end = heap_top;
		}			
	}
#endif

	/* 3.1. swap mark & start bitmaps */
	temp_bits = mark_bits;
	mark_bits = start_bits;
	start_bits = temp_bits;

	temp_bitmap = mark_bitmap;
	mark_bitmap = start_bitmap;
	start_bitmap = temp_bitmap;

	/* 3.2. mask reference bitmap */
#warning "bitmap masking unimplemented --- references will not be cleared"

	/* 3.3. update heap_top */
	if (free_start < heap_top)
		heap_top = free_start;

	if (heap_top < heap_limit)
		bitmap_setbit(start_bits, heap_top);

	/* 4. adjust the collection threshold */
	heap_next_collection = heap_top + (heap_limit - heap_top) / 8;
	if (heap_next_collection > heap_limit)
		heap_next_collection = heap_limit;

#if VERBOSITY >= VERBOSITY_PARANOIA && 0
	fprintf(stderr, "gc_reclaim: reporting heap state variables:\n");
	fprintf(stderr, "\theap top              0x%lx\n", heap_top);
	fprintf(stderr, "\theap size             0x%lx (%ld)\n", heap_size, heap_size);
	fprintf(stderr, "\theap_limit            0x%lx\n", heap_limit);
	fprintf(stderr, "\theap next collection  0x%lx\n", heap_next_collection);

	allocator_dump();
#endif
}

static
inline 
void 
gc_mark_object_at (void** addr)
{
#if 0
	//	fprintf(stderr, "gc_mark_object_at: called for address 0x%lx\n", addr);

	if (!((void*)addr >= heap_base && 
		  (void*)addr < heap_top)) {
		//		fprintf(stderr, "not an address on the heap.\n");
	} else {
		if (!bitmap_testbit(start_bits, (void*)addr)) {
			//			fprintf(stderr, "not an object.\n");
		}
		else {
			if (bitmap_testbit(mark_bits, (void*)addr)) {
				//				fprintf(stderr, "gc_mark_object_at: called for address 0x%lx: ", addr);
				//				fprintf(stderr, "already marked.\n");
			}
		}
	}
#endif

	/* 1. is the addr aligned, on the heap && the start of an object */
	if (!((long)addr & ((1 << ALIGN) - 1)) &&
		(void*)addr >= heap_base && 
		(void*)addr < heap_top && 
		bitmap_testbit(start_bits, (void*)addr) &&
		!bitmap_testbit(mark_bits, (void*)addr)) {
		bitmap_setbit(mark_bits, (void*)addr); 

		//		fprintf(stderr, "gc_mark_object_at: called for address 0x%lx: ", addr);
		//		fprintf(stderr, "marking object.\n");

#ifdef JIT_MARKER_SUPPORT
		/* 1.a. invoke the JIT-marker method */
   		asm_calljavamethod(addr->vftbl->class->marker, addr, NULL, NULL, NULL);
#else
		/* 1.b. mark the references contained */
		if (bitmap_testbit(reference_bits, addr)) {
			void** end;
#ifdef SIZE_FROM_CLASSINFO
			void** old_end;

			if (((java_objectheader*)addr)->vftbl == class_array->vftbl) {
				end = (void**)((long)addr + (long)((java_arrayheader*)addr)->alignedsize); 
//				fprintf(stderr, "size found for array at 0x%lx: 0x%lx\n", addr, ((java_arrayheader*)addr)->alignedsize);
			}
			else {
				end = (void**)((long)addr + (long)((java_objectheader*)addr)->vftbl->class->alignedsize);							   
//				fprintf(stderr, "size found for 0x%lx: 0x%lx\n", addr, ((java_objectheader*)addr)->vftbl->class->alignedsize);
			}

			old_end = (void**)bitmap_find_next_setbit(start_bitmap, (void*)addr + 8);
			if (end != old_end) {
				fprintf(stderr, "inconsistent object length for object at 0x%lx:", addr);
				fprintf(stderr, " old = 0x%lx ; new = 0x%lx\n", old_end, end);
			}
#else
			end = (void**)bitmap_find_next_setbit(start_bitmap, addr + 8); /* points just behind the object */
#endif

			while (addr < end)
				gc_mark_object_at(*(addr++));
		}
#endif	
	}

	return;
}

static
inline 
void gc_mark_references (void)
{
	address_list_node* curr = references;

	//	fprintf(stderr, "gc_mark_references\n");

	while (curr) {
		gc_mark_object_at(*((void**)(curr->address)));
		curr = curr->next;
	}
}

static
inline
void 
markreferences(void** start, void** end)
{
	while (start < end)
		gc_mark_object_at(*(start++));
}

static
inline 
void gc_mark_stack (void)
{
	void *dummy;

#ifdef USE_THREADS 
    thread *aThread;
	
	if (currentThread == NULL) {
		void **top_of_stack = &dummy;
		
		if (top_of_stack > stackbottom)
			markreferences(stackbottom, top_of_stack);
		else
			markreferences(top_of_stack, stackbottom);
	}
	else {
		for (aThread = liveThreads; aThread != 0;
			 aThread = CONTEXT(aThread).nextlive) {
			mark_object_at((void*)aThread);
			if (aThread == currentThread) {
				void **top_of_stack = &dummy;
				
				if (top_of_stack > (void**)CONTEXT(aThread).stackEnd)
					markreferences((void**)CONTEXT(aThread).stackEnd, top_of_stack);
				else 	
					markreferences(top_of_stack, (void**)CONTEXT(aThread).stackEnd);
			}
			else {
				if (CONTEXT(aThread).usedStackTop > CONTEXT(aThread).stackEnd)
					markreferences((void**)CONTEXT(aThread).stackEnd,
								   (void**)CONTEXT(aThread).usedStackTop);
				else 	
					markreferences((void**)CONTEXT(aThread).usedStackTop,
								   (void**)CONTEXT(aThread).stackEnd);
			}
	    }

		markreferences((void**)&threadQhead[0],
					   (void**)&threadQhead[MAX_THREAD_PRIO]);
	}
#else
    void **top_of_stack = &dummy;

	//	fprintf(stderr, "gc_mark_stack\n");
	
    if (top_of_stack > stackbottom) {
		//		fprintf(stderr, "stack growing upward\n");
        markreferences(stackbottom, top_of_stack);
	} else {
		//		fprintf(stderr, "stack growing downward\n");
        markreferences(top_of_stack, stackbottom);
	}
#endif
}


static 
void gc_run (void)
{
	static int armageddon_is_near = 0;

	if (armageddon_is_near) {
		/* armageddon_is_here! */
		fprintf(stderr, "Oops, seems like there's a slight problem here: gc_run() called while still running?!\n");
		return;
	}

	armageddon_is_near = true;
	heap_next_collection = heap_limit;  /* try to avoid finalizer-induced collections */

	bitmap_clear(mark_bitmap);

	asm_dumpregistersandcall(gc_mark_stack);
	gc_mark_references();
	gc_finalize();
	gc_reclaim();

	armageddon_is_near = false;
}


/************************* Function: gc_init **********************************

  Initializes anything that must be initialized to call the gc on the right
  stack.

******************************************************************************/

void
gc_init (void)
{
}

/************************** Function: gc_call ********************************

  Calls the garbage collector. The garbage collector should always be called
  using this function since it ensures that enough stack space is available.

******************************************************************************/

void
gc_call (void)
{
#ifdef USE_THREADS
	assert(blockInts == 0);

	intsDisable();
	if (currentThread == NULL || currentThread == mainThread)
		gc_run();
	else
		asm_switchstackandcall(CONTEXT(mainThread).usedStackTop, gc_run);
	intsRestore();
#else
	gc_run();
#endif
}



/*
 * These are local overrides for various environment variables in Emacs.
 * Please do not remove this and leave it at the end of the file, where
 * Emacs will automagically detect them.
 * ---------------------------------------------------------------------
 * Local variables:
 * mode: c
 * indent-tabs-mode: t
 * c-basic-offset: 4
 * tab-width: 4
 * End:
 */
