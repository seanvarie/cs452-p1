#include <assert.h>
#include <stdlib.h>
#include <time.h>
#ifdef __APPLE__
#include <sys/errno.h>
#else
#include <errno.h>
#endif
#include "harness/unity.h"
#include "../src/lab.h"
#include <string.h>

void setUp(void) {
  // set stuff up here
}

void tearDown(void) {
  // clean stuff up here
}

/**
 * Check the pool to ensure it is full.
 */
void check_buddy_pool_full(struct buddy_pool *pool)
{
  //A full pool should have all values 0-(kval-1) as empty
  for (size_t i = 0; i < pool->kval_m; i++)
    {
      assert(pool->avail[i].next == &pool->avail[i]);
      assert(pool->avail[i].prev == &pool->avail[i]);
      assert(pool->avail[i].tag == BLOCK_UNUSED);
      assert(pool->avail[i].kval == i);
    }

  //The avail array at kval should have the base block
  assert(pool->avail[pool->kval_m].next->tag == BLOCK_AVAIL);
  assert(pool->avail[pool->kval_m].next->next == &pool->avail[pool->kval_m]);
  assert(pool->avail[pool->kval_m].prev->prev == &pool->avail[pool->kval_m]);

  //Check to make sure the base address points to the starting pool
  //If this fails either buddy_init is wrong or we have corrupted the
  //buddy_pool struct.
  assert(pool->avail[pool->kval_m].next == pool->base);
}

/**
 * Check the pool to ensure it is empty.
 */
void check_buddy_pool_empty(struct buddy_pool *pool)
{
  void * mallocResult = buddy_malloc(pool, 1);
  if(NULL != mallocResult){
    buddy_free(pool, mallocResult);
    assert(false);
  }
  //An empty pool should have all values 0-(kval) as empty
  for (size_t i = 0; i <= pool->kval_m; i++)
    {
      assert(pool->avail[i].next == &pool->avail[i]);
      assert(pool->avail[i].prev == &pool->avail[i]);
      assert(pool->avail[i].tag == BLOCK_UNUSED);
      assert(pool->avail[i].kval == i);
    }
}

/**
 * Test allocating 1 byte to make sure we split the blocks all the way down
 * to MIN_K size. Then free the block and ensure we end up with a full
 * memory pool again
 */
void test_buddy_malloc_one_byte(void)
{
  fprintf(stderr, "->Test allocating and freeing 1 byte\n");
  struct buddy_pool pool;
  int kval = MIN_K;
  size_t size = UINT64_C(1) << kval;
  buddy_init(&pool, size);
  void *mem = buddy_malloc(&pool, 1);
  //Make sure correct kval was allocated
  buddy_free(&pool, mem);
  check_buddy_pool_full(&pool);
  buddy_destroy(&pool);
}

/**
 * Tests the allocation of one massive block that should consume the entire memory
 * pool and makes sure that after the pool is empty we correctly fail subsequent calls.
 */
void test_buddy_malloc_one_large(void)
{
  fprintf(stderr, "->Testing size that will consume entire memory pool\n");
  struct buddy_pool pool;
  size_t bytes = UINT64_C(1) << MIN_K;
  buddy_init(&pool, bytes);

  //Ask for an exact K value to be allocated. This test makes assumptions on
  //the internal details of buddy_init.
  size_t ask = bytes - sizeof(struct avail);
  void *mem = buddy_malloc(&pool, ask);
  assert(mem != NULL);

  //Move the pointer back and make sure we got what we expected
  struct avail *tmp = (struct avail *)mem - 1;
  assert(tmp->kval == MIN_K);
  assert(tmp->tag == BLOCK_RESERVED);
  check_buddy_pool_empty(&pool);

  //Verify that a call on an empty tool fails as expected and errno is set to ENOMEM.
  void *fail = buddy_malloc(&pool, 5);
  assert(fail == NULL);
  assert(errno = ENOMEM);

  //Free the memory and then check to make sure everything is OK
  buddy_free(&pool, mem);
  check_buddy_pool_full(&pool);
  buddy_destroy(&pool);
}

/**
 * Tests the allocation of one massive block that is too big for the memory pool and 
 * makes sure that the call fails correctly.
 */
void test_buddy_malloc_one_too_large(void)
{
  fprintf(stderr, "->Test attempting allocating too large a block\n");
  struct buddy_pool pool;
  int kval = MIN_K;
  size_t size = UINT64_C(1) << kval;
  buddy_init(&pool, size);
  
  //Ask for larger than an exact K value to be allocated. This test makes assumptions on
  //the internal details of buddy_init.
  size_t ask = size - sizeof(struct avail) + 1;
  void *mem = buddy_malloc(&pool, ask);
  assert(mem == NULL);
  assert(errno = ENOMEM);

  //Make sure correct kval was allocated
  check_buddy_pool_full(&pool);
  buddy_destroy(&pool);
}

/**
 * Tests the allocation of one massive block that is too big for the memory pool after 
 * a 1 byte malloc and makes sure that the call fails correctly.
 */
void test_buddy_malloc_one_byte_and_one_too_large(void)
{
  fprintf(stderr, "->Test allocating 1 byte, attempting allocating too large a block and freeing the 1 byte\n");
  struct buddy_pool pool;
  int kval = MIN_K;
  size_t size = UINT64_C(1) << kval;
  buddy_init(&pool, size);

  void *mem = buddy_malloc(&pool, 1);
  assert(mem != NULL);
  
  //Ask for larger than an exact K value to be allocated. This test makes assumptions on
  //the internal details of buddy_init.
  size_t ask = size - sizeof(struct avail) + 1;
  void *fail = buddy_malloc(&pool, ask);
  assert(fail == NULL);
  assert(errno = ENOMEM);

  //Make sure correct kval was allocated
  buddy_free(&pool, mem);
  check_buddy_pool_full(&pool);
  buddy_destroy(&pool);
}

/**
 * Test allocating a few blocks of memory. Then free the blocks and 
 * ensure we end up with a full memory pool again
 */
void test_multiple_buddy_malloc(void)
{
  fprintf(stderr, "->Test allocating and freeing a few blocks of memory\n");
  struct buddy_pool pool;
  int kval = MIN_K;
  size_t size = UINT64_C(1) << kval;
  buddy_init(&pool, size);
  void *mem1 = buddy_malloc(&pool, 1);
  assert(NULL != mem1);
  void *mem2 = buddy_malloc(&pool, 9);
  assert(NULL != mem2);
  void *mem3 = buddy_malloc(&pool, 2);
  assert(NULL != mem3);
  void *mem4 = buddy_malloc(&pool, 13);
  assert(NULL != mem4);

  assert(mem1 != mem2);
  assert(mem1 != mem3);
  assert(mem1 != mem4);
  assert(mem2 != mem3);
  assert(mem2 != mem4);
  assert(mem3 != mem4);
  //Make sure correct kval was allocated
  buddy_free(&pool, mem1);
  buddy_free(&pool, mem2);
  buddy_free(&pool, mem3);
  buddy_free(&pool, mem4);
  check_buddy_pool_full(&pool);
  buddy_destroy(&pool);
}

/**
 * Tests which buddy_mallocs 2 enough space and stores the string "my 
 * data", then reallocs and the new pointer has the same data.Then free 
 * the block and ensure we end up with a full memory pool again
 */
void test_buddy_realloc_data(void)
{
  fprintf(stderr, "->Test data is retained from allocating and reallocating\n");
  char testString[8] = "my data";
  struct buddy_pool pool;
  int kval = DEFAULT_K;
  size_t size = UINT64_C(1) << kval;
  buddy_init(&pool, size);
  void *mem1 = buddy_malloc(&pool, 20);
  strcpy(mem1, testString);
  void *mem2 = buddy_realloc(&pool, mem1, 130);
  assert(NULL != mem2);
  assert(0 == strcmp(testString, (char *)mem2));
  //Make sure correct kval was allocated
  buddy_free(&pool, mem2);
  check_buddy_pool_full(&pool);
  buddy_destroy(&pool);
}

/**
 * Test which buddy_mallocs 2 bytes, then reallocs one byte on that 
 * pointer. Test then verifies that the return pointer of the realloc 
 * is the same as the input. Then free the block and ensure we end up 
 * with a full memory pool again
 */
void test_buddy_realloc_2_to_1_bytes(void)
{
  fprintf(stderr, "->Test allocating 2 bytes and reallocating as 1\n");
  struct buddy_pool pool;
  int kval = MIN_K;
  size_t size = UINT64_C(1) << kval;
  buddy_init(&pool, size);
  void *mem1 = buddy_malloc(&pool, 2);
  void *mem2 = buddy_realloc(&pool, mem1, 1);
  assert(mem1 == mem2);
  //Make sure correct kval was allocated
  buddy_free(&pool, mem2);
  check_buddy_pool_full(&pool);
  buddy_destroy(&pool);
}

/**
 * Test which buddy_mallocs one byte, then reallocs 2 bytes on that 
 * pointer. Test then verifies that the return pointer of the realloc 
 * is the same as the input. Then free the block and ensure we end up 
 * with a full memory pool again
 */
void test_buddy_realloc_1_to_2_bytes(void)
{
  fprintf(stderr, "->Test allocating 1 byte and reallocating as 2\n");
  struct buddy_pool pool;
  int kval = MIN_K;
  size_t size = UINT64_C(1) << kval;
  buddy_init(&pool, size);
  void *mem1 = buddy_malloc(&pool, 1);
  void *mem2 = buddy_realloc(&pool, mem1, 2);
  assert(mem1 == mem2);
  //Make sure correct kval was allocated
  buddy_free(&pool, mem2);
  check_buddy_pool_full(&pool);
  buddy_destroy(&pool);
}

/**
 * Test allocating a few blocks of memory. Then reallocating some of 
 * those blocks and freeing the blocks and ensure we end up with a 
 * full memory pool again
 */
void test_multiple_buddy_malloc_and_realloc(void)
{
  fprintf(stderr, "->Test allocating, reallocating and freeing a few blocks of memory\n");
  struct buddy_pool pool;
  int kval = MIN_K;
  size_t size = UINT64_C(1) << kval;
  buddy_init(&pool, size);
  void *mem1 = buddy_malloc(&pool, 1);
  assert(NULL != mem1);
  void *mem2 = buddy_malloc(&pool, 9);
  assert(NULL != mem2);
  void *mem3 = buddy_malloc(&pool, 2);
  assert(NULL != mem3);
  mem2 = buddy_realloc(&pool, mem2, 3);
  assert(NULL != mem2);
  mem3 = buddy_realloc(&pool, mem3, 100);
  assert(NULL != mem3);
  void *mem4 = buddy_malloc(&pool, 13);
  assert(NULL != mem4);

  assert(mem1 != mem2);
  assert(mem1 != mem3);
  assert(mem1 != mem4);
  assert(mem2 != mem3);
  assert(mem2 != mem4);
  assert(mem3 != mem4);

  //Make sure correct kval was allocated
  buddy_free(&pool, mem1);
  buddy_free(&pool, mem2);
  buddy_free(&pool, mem3);
  buddy_free(&pool, mem4);
  check_buddy_pool_full(&pool);
  buddy_destroy(&pool);
}

/**
 * Tests to make sure that the struct buddy_pool is correct and all fields
 * have been properly set kval_m, avail[kval_m], and base pointer after a
 * call to init
 */
void test_buddy_init(void)
{
  fprintf(stderr, "->Testing buddy init\n");
  //Loop through all kval MIN_k-DEFAULT_K and make sure we get the correct amount allocated.
  //We will check all the pointer offsets to ensure the pool is all configured correctly
  for (size_t i = MIN_K; i <= DEFAULT_K; i++)
    {
      size_t size = UINT64_C(1) << i;
      struct buddy_pool pool;
      buddy_init(&pool, size);
      check_buddy_pool_full(&pool);
      buddy_destroy(&pool);
    }
}


int main(void) {
  time_t t;
  unsigned seed = (unsigned)time(&t);
  fprintf(stderr, "Random seed:%d\n", seed);
  srand(seed);
  printf("Running memory tests.\n");

  UNITY_BEGIN();
  RUN_TEST(test_buddy_init);
  RUN_TEST(test_buddy_malloc_one_byte);
  RUN_TEST(test_buddy_malloc_one_large);
  RUN_TEST(test_buddy_malloc_one_too_large);
  RUN_TEST(test_buddy_malloc_one_byte_and_one_too_large);
  RUN_TEST(test_multiple_buddy_malloc);
  RUN_TEST(test_buddy_realloc_data);
  RUN_TEST(test_buddy_realloc_2_to_1_bytes);
  RUN_TEST(test_buddy_realloc_1_to_2_bytes);
  RUN_TEST(test_multiple_buddy_malloc_and_realloc);
return UNITY_END();
}