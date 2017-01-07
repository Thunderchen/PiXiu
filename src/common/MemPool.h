#include "style.h"
#include <stddef.h>

#define POOL_BLOCK_NUM 65535
#define POOL_BLOCK_SIZE (sizeof(void *))

struct Pool {
    Pool ptrAs(prev_pool);
    void * blocks[POOL_BLOCK_NUM];
};

struct MemPool {
    Pool ptrAs(curr_pool) = NULL;
    int used_num;

    // ---
    void * p_malloc(int size);

    void free_all(void);
};