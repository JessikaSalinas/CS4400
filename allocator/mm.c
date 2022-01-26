/*
 * mm-naive.c - The least memory-efficient malloc package.
 * 
 * In this naive approach, a block is allocated by allocating a
 * new page as needed.  A block is pure payload. There are no headers or
 * footers.  Blocks are never coalesced or reused.
 *
 * Name: Jessika Jimenez
 * UID: u0864868
 * 
 * NOTE TO STUDENTS: Replace this header comment with your own header
 * comment that gives a high level description of your solution.
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "mm.h"
#include "memlib.h"

/* always use 16-byte alignment */
#define ALIGNMENT 16

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~(ALIGNMENT-1))

/* rounds up to the nearest multiple of mem_pagesize() */
#define PAGE_ALIGN(size) (((size) + (mem_pagesize()-1)) & ~(mem_pagesize()-1))


/* --- My vaiables + macros --- */


typedef size_t block_header;
typedef	size_t block_footer;
#define WSIZE 8
#define DSIZE 16
#define PAGE_OVERHEAD 32
#define OVERHEAD (sizeof(block_header) + sizeof(block_footer))
#define THRESHOLD (4096*60)

// Lists node next/pervious
typedef	struct node_list{
  struct node_list* next;
  struct node_list* prev;
}node_list;

struct node_list* list_hdr;
size_t init_mapped;

// Get/set a value, given a header pointer
#define GET(ptr) (*(size_t *)(ptr))
#define SET(ptr, val) (*(size_t *)(ptr) = (val));

// Get the header/footer pointer, given a payload pointer 
#define HDR_PTR(ptr) ((char *)(ptr) - sizeof(block_header))
#define FTR_PTR(ptr) ((char *)(ptr) + GET_SIZE(HDR_PTR(ptr)) - OVERHEAD)

// Pack together a size and alloc bit
#define PACK(size, alloc) ((size) | (alloc))

// Get size/alloc, given a header pointer
#define GET_SIZE(ptr) (GET(ptr) & ~0xF)
#define GET_ALLOC(ptr) (GET(ptr) & 0x1)

// Get next/previous pointer, given a block pointer
#define NEXT_BLK_PTR(ptr) ((char *)(ptr) + GET_SIZE(HDR_PTR(ptr)))
#define PREV_BLK_PTR(ptr) ((char *)(ptr) - GET_SIZE((char *)(ptr) - OVERHEAD))
#define NEXT_FREE(ptr) (*(void **)(ptr + WSIZE))
#define PREV_FREE(ptr) (*(void **)(ptr))

// Declared helper functions
static void add_node(void* ptr);
static void remove_node(void* ptr);
static void set_alloc(void* ptr, size_t size);
static void* extend(size_t size);
static void* find_blk(size_t size);
static void* coalesce(void* ptr);


/* --- My helper functions --- */


// Adds node into node list
static void add_node(void* ptr) {
  if (ptr == NULL)
    return;

  node_list* node = (node_list*)ptr;
  node->next = list_hdr;

  if (list_hdr != NULL)
    list_hdr->prev = node;

  node->prev = NULL;
  list_hdr = node;
}

// Removes node from node list
static void remove_node(void* ptr) {
  if (ptr == NULL)
    return;

  node_list* node = (node_list*)ptr;

  if (node->prev == NULL) {
    if (node->next == NULL)
      list_hdr = NULL; 

    else {
      list_hdr = node->next;
      node->next->prev = NULL;
    }
  }
  else {
    if (node->next == NULL)
      node->prev->next = NULL;

    else {
      node->prev->next = node->next;
      node->next->prev = node->prev;
    }
  }
}

// Allocates a set block
static void set_alloc(void* ptr, size_t size) {
  if (ptr == NULL)
    return NULL;

  size_t init_size = GET_SIZE(HDR_PTR(ptr));
  size_t diff = init_size - size;

  remove_node(ptr);
  
  if (diff > PAGE_OVERHEAD) {
    SET(HDR_PTR(ptr), PACK(size, 1));
    SET(FTR_PTR(ptr), PACK(size, 1));
    SET(HDR_PTR(NEXT_BLK_PTR(ptr)), PACK(diff, 0));
    SET(FTR_PTR(NEXT_BLK_PTR(ptr)), PACK(diff, 0));
    add_node(NEXT_BLK_PTR(ptr));
  }
  else {
     SET(HDR_PTR(ptr), PACK(init_size, 1));
     SET(FTR_PTR(ptr), PACK(init_size, 1));
  }
}

// Extends memory & initializes new mem chunk
static void* extend(size_t size) {
  size_t size_cpy;
  size_t init_size = PAGE_ALIGN((2 * init_mapped) + PAGE_OVERHEAD);
  size_t pg_size = PAGE_ALIGN(size + PAGE_OVERHEAD);
  
  if (pg_size > init_size) {
    size_cpy = pg_size;
    init_mapped = size_cpy;
  }
  else if (init_size <= THRESHOLD) {
    size_cpy = init_size;
    init_mapped = size_cpy;
  }
  else 
    size = init_mapped;

  size_cpy = PAGE_ALIGN(size_cpy + PAGE_OVERHEAD);
  void* ptr = mem_map(size_cpy);

  SET(ptr, 0);
  ptr += 8;

  SET(ptr, PACK(16, 1));
  ptr += 8;

  SET(ptr, PACK(16, 1));
  ptr += 8;

  SET(ptr, PACK(size_cpy - PAGE_OVERHEAD, 0));
  ptr += 8;

  SET(FTR_PTR(ptr), PACK(size_cpy - PAGE_OVERHEAD, 0));
  SET((FTR_PTR(ptr) + 0x8), PACK(0, 1));

  add_node(ptr);
  return ptr;
}

// Checks for free block to allocate 
static void* find_blk(size_t size) {
  struct node_list* node = list_hdr;

  while (node) {
    if (GET_SIZE(HDR_PTR(node)) >= size)
      return (void*)node;
    else
      node = node->next;
  }
  return NULL;
}

// Coalesce free block & returns pointer to new block
static void* coalesce(void* ptr) {
  if (ptr == NULL)
    return NULL;
  
  size_t next_alloc = GET_ALLOC(HDR_PTR(NEXT_BLK_PTR(ptr)));
  size_t prev_alloc = GET_ALLOC(HDR_PTR(PREV_BLK_PTR(ptr)));
  size_t size = GET_SIZE(HDR_PTR(ptr));
  
  if(prev_alloc && next_alloc)
    add_node(ptr);

  else if (!prev_alloc && next_alloc) {
    size += GET_SIZE(HDR_PTR(PREV_BLK_PTR(ptr)));
    SET(HDR_PTR(PREV_BLK_PTR(ptr)), PACK(size, 0));
    SET(FTR_PTR(ptr), PACK(size, 0));
    ptr = PREV_BLK_PTR(ptr);
  }

  else if (prev_alloc && !next_alloc) {
    remove_node(NEXT_BLK_PTR(ptr));
    size += GET_SIZE(HDR_PTR(NEXT_BLK_PTR(ptr)));
    SET(HDR_PTR(ptr), PACK(size, 0));
    SET(FTR_PTR(ptr), PACK(size, 0));
    add_node(ptr);
  }

  else {
    remove_node(NEXT_BLK_PTR(ptr));
    size += GET_SIZE(HDR_PTR(PREV_BLK_PTR(ptr))) + GET_SIZE(HDR_PTR(NEXT_BLK_PTR(ptr)));
    SET(HDR_PTR(PREV_BLK_PTR(ptr)), PACK(size, 0));
    SET(FTR_PTR(NEXT_BLK_PTR(ptr)), PACK(size, 0));
    ptr = PREV_BLK_PTR(ptr);
  }
  
  return ptr;
}


/* --- Given functions to complete --- */


/* 
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
  list_hdr = NULL;  
  return 0;
}


/* 
 * mm_malloc - Allocate a block by using bytes from current_avail,
 *     grabbing a new page if necessary.
 */
void *mm_malloc(size_t size)
{
  if (size == 0)
    return NULL;

  size_t new_size = ALIGN(size + OVERHEAD);
  void *ptr = NULL;
  
  if ((ptr = find_blk(new_size)) != NULL) 
    set_alloc(ptr, new_size);

  else {
    if ((ptr = extend(new_size)) != NULL) {
      ptr = list_hdr;
      set_alloc(ptr, new_size);
    }
  }
  
  return ptr;
}


/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr)
{
  if (ptr == NULL)
    return;
  
  void* new_ptr;
  size_t size = GET_SIZE(HDR_PTR(ptr));

  SET(HDR_PTR(ptr), PACK(size, 0));
  SET(FTR_PTR(ptr), PACK(size, 0));
  new_ptr = coalesce(ptr);

  if (((GET_SIZE(new_ptr - 24)) == OVERHEAD) && ((GET_SIZE(FTR_PTR(new_ptr) + 8) == 0))) {

    if (GET_SIZE(HDR_PTR(new_ptr)) >= (10*4096)) {
      remove_node(new_ptr);
      size_t mmap_size = GET_SIZE(HDR_PTR(new_ptr)) + PAGE_OVERHEAD;
      mem_unmap(new_ptr - PAGE_OVERHEAD, mmap_size);
    }
  }
}
