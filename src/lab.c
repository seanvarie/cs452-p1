#include "lab.h"
#include <sys/mman.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

#define BLOCK_METADATA_SIZE sizeof(struct avail)
//#define BLOCK_METADATA_SIZE sizeof(short int) * 2 //because of struct re-ordering, this may or may not work depending on the system/compiler being used

void buddy_init(struct buddy_pool *pool, size_t size){
    if(0 == size){
        size = UINT64_C(1) << DEFAULT_K;
    }

    pool->kval_m = btok(size);
    pool->numbytes = UINT64_C(1) << pool->kval_m;

    pool->base=mmap(NULL, pool->numbytes, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if(MAP_FAILED == pool->base){
        perror("buddy: could not allocate memory pool");
    }

    for(unsigned int i = 0; i < pool->kval_m; i++){
        pool->avail[i].next = &pool->avail[i];
        pool->avail[i].prev = &pool->avail[i];
        pool->avail[i].kval = i;
        pool->avail[i].tag = BLOCK_UNUSED;
    }

    pool->avail[pool->kval_m].next = pool->base;
    pool->avail[pool->kval_m].prev = pool->base;
    pool->avail[pool->kval_m].kval = pool->kval_m;
    pool->avail[pool->kval_m].tag = BLOCK_UNUSED;

    ((struct avail *)pool->base)->next = &pool->avail[pool->kval_m];
    ((struct avail *)pool->base)->prev = &pool->avail[pool->kval_m];
    ((struct avail *)pool->base)->kval = pool->kval_m;
    ((struct avail *)pool->base)->tag = BLOCK_AVAIL;

}

void buddy_destroy(struct buddy_pool *pool){
    int status = munmap(pool->base, pool->numbytes);
    if (-1 == status){
        perror("buddy: destroy failed!");
    }
}

size_t btok(size_t bytes){
    size_t retVal = 0;
    bytes--;
    while (bytes > 0){
        bytes >>= 1;
        retVal++;
    }
    return retVal;
}

struct avail *buddy_calc(struct buddy_pool *pool, struct avail *buddy){
    return (struct avail *)((((void*)buddy-pool->base) ^ (UINT64_C(1)<<buddy->kval))+pool->base);
}

void *buddy_malloc(struct buddy_pool *pool, size_t size){
    size += BLOCK_METADATA_SIZE;

    unsigned short int kval = btok(size);

    kval = kval < SMALLEST_K ? SMALLEST_K : kval;//take max of kval and SMALLEST_K

    if(kval > pool->kval_m){
        errno = ENOMEM;
        return NULL;
    }

    struct avail * smallestBlock;
    struct avail * smallestBlockBuddy;
    unsigned int smallestK;//k value of the smallest (available) block that is big enough for this malloc
    for(smallestK = kval; smallestK <= pool->kval_m; smallestK++){
        if(BLOCK_AVAIL == pool->avail[smallestK].next->tag){
            break;
        }
    }

    if(smallestK > pool->kval_m){//if there is not a large enough block (if we never broke out of the for loop above)
        errno = ENOMEM;
        return NULL;
    }

    smallestBlock = pool->avail[smallestK].next;
    smallestBlockBuddy = buddy_calc(pool, smallestBlock);
    for(;smallestK > kval; smallestK--){
        smallestBlock->kval--;
        smallestBlockBuddy = buddy_calc(pool, smallestBlock);
        smallestBlock->tag = BLOCK_RESERVED;

        smallestBlockBuddy->kval = smallestBlock->kval;
        smallestBlockBuddy->tag = BLOCK_AVAIL;
        //insert smallestBlockBuddy into its linked list (just after the dummy head)
        smallestBlockBuddy->next = pool->avail[smallestBlockBuddy->kval].next;
        smallestBlockBuddy->prev = &pool->avail[smallestBlockBuddy->kval];
        smallestBlockBuddy->prev->next = smallestBlockBuddy;
        smallestBlockBuddy->next->prev = smallestBlockBuddy;
    }

    smallestBlock->tag = BLOCK_RESERVED;
    //remove reserved block from it's list
    smallestBlock->prev->next = smallestBlock->next;
    smallestBlock->next->prev = smallestBlock->prev;

    return ((void *)smallestBlock + BLOCK_METADATA_SIZE);
}

void buddy_free(struct buddy_pool *pool, void *ptr){
    struct avail * freeBlock = (struct avail *)(ptr - BLOCK_METADATA_SIZE);

    struct avail * buddy = buddy_calc(pool, freeBlock);//if freeBlock->kval >= pool->kval_m this will likely be a pointer to a memory location outise the region claimed with mmap and thus should not be accessed or modified

    while(freeBlock->kval < pool->kval_m && BLOCK_AVAIL == buddy->tag){//This will not result in accessing memory outside the region claimed with mmap only because of short circuiting
        //remove buddy from it's list
        buddy->prev->next = buddy->next;
        buddy->next->prev = buddy->prev;

        //set free block to the first block out of it and it's buddy
        freeBlock = freeBlock > buddy ? buddy : freeBlock;

        freeBlock->kval++;
        buddy = buddy_calc(pool, freeBlock);
    }

    freeBlock->tag = BLOCK_AVAIL;
    //insert freeBlock into its linked list (just after the dummy head)
    freeBlock->prev = &pool->avail[freeBlock->kval];
    freeBlock->next = pool->avail[freeBlock->kval].next;
    freeBlock->prev->next = freeBlock;
    freeBlock->next->prev = freeBlock;
}

/**
 * returns the smallest of the two inputs
 */
size_t minimum_size(size_t param1, size_t param2){
    return param1 < param2 ? param2 : param2;
}

void *buddy_realloc(struct buddy_pool *pool, void *ptr, size_t size){
    if(NULL == ptr){
        return buddy_malloc(pool, size);
    }
    if(0 == size){
        buddy_free(pool, ptr);
        return ptr;
    }
    struct avail * reallocBlock = (struct avail *)(ptr - BLOCK_METADATA_SIZE);

    if((UINT64_C(1) << reallocBlock->kval) >= (size + BLOCK_METADATA_SIZE)){
        return ptr;
    }
    else{
        void * retVal = buddy_malloc(pool, size);
        if(NULL == retVal){
            errno = ENOMEM;
            return NULL;
        }
        memcpy(retVal, ptr, minimum_size(size, UINT64_C(1) << reallocBlock->kval));
        buddy_free(pool, ptr);
        return retVal;
    }
}