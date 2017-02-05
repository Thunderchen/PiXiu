#ifndef MEM_POOL_H
#define MEM_POOL_H

#include <stddef.h>

#define POOL_BLOCK_NUM 65535
#define POOL_BLOCK_SIZE sizeof(void *)

struct Pool {
    Pool * prev_pool;
    void * blocks[POOL_BLOCK_NUM];
};

struct MemPool {
    Pool * curr_pool = NULL;
    int used_num;
    int nth = 0;

    void * p_malloc(int);

    void free_prop(void);
};

#endif