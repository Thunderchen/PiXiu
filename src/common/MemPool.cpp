#include "MemPool.h"
#include "style.h"
#include <math.h>
#include <stdlib.h>

void * MemPool::p_malloc(int size) {
    if (this->curr_pool == NULL) {
        this->curr_pool = (Pool *) malloc(sizeof(Pool));
        this->curr_pool->prev_pool = NULL;
        this->used_num = 0;
    }

    if (size > POOL_BLOCK_SIZE * POOL_BLOCK_NUM) {
        auto pool = (Pool *) malloc(sizeof(Pool::prev_pool) + size);
        pool->prev_pool = this->curr_pool->prev_pool;
        this->curr_pool->prev_pool = pool;
        return adrOf(pool->blocks);
    }

    if (size > POOL_BLOCK_SIZE * (POOL_BLOCK_NUM - this->used_num)) {
        auto pool = (Pool *) malloc(sizeof(Pool));
        pool->prev_pool = this->curr_pool;
        this->curr_pool = pool;
        this->used_num = 0;
    }

    auto ret = adrOf(this->curr_pool->blocks[this->used_num]);
    this->used_num += ceil(size / POOL_BLOCK_SIZE);
    return ret;
}

void MemPool::free_prop() {
    Pool * cursor = this->curr_pool;
    while (cursor != NULL) {
        auto next_cursor = cursor->prev_pool;
        free(cursor);
        cursor = next_cursor;
    }
    this->curr_pool = cursor;
}
