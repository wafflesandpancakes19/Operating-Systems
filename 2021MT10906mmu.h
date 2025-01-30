#ifndef MMU_H
#define MMU_H

#include <unistd.h>
#include <sys/mman.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>  
#include <stddef.h>    


#define align4(x) (((((x)-1) >> 2) << 2) + 4)

// ASSIGNMENT 3 | PART B 

/* The size of the block's metadata (added as a header in the memory block) */
#define BLOCK_SIZE (sizeof(struct mem_block))

/* Forward declaration of mem_block */
typedef struct mem_block *mem_ptr;

/* The mem_block structure */
// it exists as a DOUBLY LINKED LIST
struct mem_block {
    bool free;
    size_t size;
    mem_ptr next;
    mem_ptr prev;
    void *ptr;    
    char data[1]; // Flexible array member to store actual data
};

/* Doubly linked list to manage memory blocks */
typedef struct {
    mem_ptr head;  // Head of the list
} DoublyLinkedList;

/* Initialize the linked list */
void list_init(DoublyLinkedList *list) {
    list->head = NULL;
}

// the below function inserts a block after a particular block in a doubly linked list
void list_insert_after(DoublyLinkedList *list, mem_ptr block, mem_ptr new_block) {
    new_block->next = block->next;
    new_block->prev = block;
    new_block->ptr = new_block->data;
    block->next = new_block;

    if (new_block->next) {
        new_block->next->prev = new_block;
    }
}

// The following function coalesces two adjacent blocks after checking if their neighbours are occupied 
mem_ptr list_coalesce(mem_ptr block) {
    // If the next block is free, merge the two
    if (block->next && block->next->free) {
        // size is uupdated 
        block->size += BLOCK_SIZE + block->next->size;
        block->next = block->next->next;  // pointers are updated 

        if (block->next) {
            block->next->prev = block;
        }
    }
    block->free = true;
    return block;
}

mem_ptr list_find_free_block(DoublyLinkedList *list, mem_ptr *last, size_t size) {
    // the following function finds a suitable free block using the FIRST FIT STRATEGY
    // the first block that it encounters that is free and that has a size larger than the required size is allotted 
    mem_ptr b = list->head;
    //printf("Searching for a free block of size: %zu\n", size);    
    while (b) { // while one still has nodes to search
        if (b->free && (b->size >= size)) {
            //printf("Found suitable free block at: %p | Size: %zu\n", (void*)b, b->size);
            return b;  // Return the found block
        } else {
            *last = b;  // Update last pointer
            b = b->next; // Move to the next block
        }
    }
    return NULL; // indicates that no suitable block was found 
}

// initializing global list for memory blocks (both occupied and unoccupied)
DoublyLinkedList mem_list = {NULL};

void split_space(mem_ptr block, size_t size) {
    // function that splits a free block into 2 - an occupied block of the required size and the remaining block as a free block 
    size_t aligned_size = align4(size);  // Ensure alignment

    // Ensure there's enough space for a new block and alignment
    if (block->size >= aligned_size + BLOCK_SIZE + 4) { 
        // Calculate the starting address for the new block
        mem_ptr new_block = (mem_ptr)((char*)block->data + aligned_size);
        new_block = (mem_ptr)align4((size_t)new_block);  // Align the new block
        
        // update the size 
        new_block->size = block->size - aligned_size - BLOCK_SIZE;
        new_block->free = true;
        new_block->ptr = new_block->data;

        block->size = aligned_size;  // Adjust size of the original block
        block->free = false;  // Mark the original block as used
        // Insert the new block into the memory list
        list_insert_after(&mem_list, block, new_block);
    }
}

/* Extend the heap if no suitable block is found */
mem_ptr make_space(mem_ptr last, size_t size) {
    size_t aligned_size = align4(size);  // Ensure the requested size is aligned
    mem_ptr block;
    block = sbrk(0);  // Get the current program break
    if (sbrk(BLOCK_SIZE + aligned_size) == (void*)-1) {
        return NULL;  // indicates that sbrk has failed 
    }

    block->size = aligned_size;
    block->free = false;
    block->next = NULL;
    block->ptr = block->data;

    if (last) {
        last->next = block;
        block->prev = last;
    } else {
        block->prev = NULL;
    }

    return block;
}

void* my_malloc(size_t size) {
    // aims to emulate the standard malloc() function 
    mem_ptr block, last = NULL;
    size_t aligned_size = align4(size);
    // search for a free block on receiving request 
    if (mem_list.head) {
        last = mem_list.head;
        block = list_find_free_block(&mem_list, &last, aligned_size);
        if (block) {
            // If the block is found, check if it is too large and hence has to be split 
            if (block->size - aligned_size >= (BLOCK_SIZE + 4)) {
                split_space(block, aligned_size);
            }
            block->free = false;  // Mark block as used
        } else {
            // If no suitable block, the space has to be extended 
            block = make_space(last, aligned_size);
            if (!block) return NULL;  // If sbrk() fails
        }
    } else {
        // this is the base case, when there are no blocks in the memory
        block = make_space(NULL, aligned_size);
        if (!block) return NULL;  // If sbrk() fails
        mem_list.head = block;
    }
    return block->data;
}

void* my_calloc(size_t nelem, size_t size) {
    // emulates the standard calloc() function
    // does what malloc() does, then makes each of the entries in the list 0
    size_t *new_ptr;
    size_t total_size = nelem * size;
    new_ptr = my_malloc(total_size);
    if (new_ptr) {
        memset(new_ptr, 0, align4(total_size));
    }
    return new_ptr;
}

int is_addr_valid(void* p) {
    // checks if the address represented by the pointer 'p' is valid 
    mem_ptr block = mem_list.head;
    while (block) {
        if ((void*)block->ptr <= p && p < (void*)((char*)block->ptr + block->size)) {
            return 1; // Pointer is valid
        }
        block = block->next;
    }
    return 0; // Pointer is not valid
}

void my_free(void* ptr) {
    if (is_addr_valid(ptr)) {
        mem_ptr block = (mem_ptr)((char*)ptr - offsetof(struct mem_block, data));
        block->free = true;  // block is marked as free 

        // while adding to the list, check if it can be coalesced (merged) with neighbouring blocks 
        if (block->prev && block->prev->free) {
            //printf("Coalescing with previous block at: %p\n", (void*)block->prev);
            block = list_coalesce(block->prev);  // Merge with previous block
        }
        if (block->next && block->next->free) {
            //printf("Coalescing with next block at: %p\n", (void*)block->next);
            list_coalesce(block);  // Merge with next block if free
        }

        // Check if this block is the last one in the list (can safely free back to system)
        if (!block->next) {
            //printf("Block at: %p is the last block, freeing to the system.\n", (void*)block);
            if (block->prev) {
                block->prev->next = NULL;  // Remove from the list
            } else {
                mem_list.head = NULL;  // List is now empty
            }
            brk(block);  // Release memory back to the system if it's the last block
        }
    } else {
        printf("Pointer %p is not valid.\n", ptr);
    }
}

#endif  // MMU_H
