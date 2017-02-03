#include "MemPool.h"
#include "style.h"
#include "util.h"
#include <assert.h>
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
    int num = size / POOL_BLOCK_SIZE;
    int remainder = size % POOL_BLOCK_SIZE;
    if (remainder != 0) { num++; }
    this->used_num += num;
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

void t_MemPool(void) {
    MemPool memPool;
    auto v = (void *) 1;

    // <常规申请>
    for (int i = 0; i < POOL_BLOCK_NUM; ++i) {
        auto adr = (void **) memPool.p_malloc(sizeof(v));
        valIn(adr) = v;
    }

    assert(memPool.curr_pool != NULL && memPool.curr_pool->prev_pool == NULL);
    for (int i = 0; i < POOL_BLOCK_NUM; ++i) {
        assert(memPool.curr_pool->blocks[i] == v);
    }
    assert(memPool.used_num == POOL_BLOCK_NUM);
    // </>

    // <自动扩容>
    memPool.p_malloc(sizeof(v) * 2);
    assert(memPool.used_num == 2);
    // </>

    // <特大区块>
    v = (void *) 2;
    auto huge_chunk = (void **) memPool.p_malloc(POOL_BLOCK_SIZE * (POOL_BLOCK_NUM + 1));
    assert(memPool.used_num == 2);

    for (int i = 0; i < POOL_BLOCK_NUM + 1; ++i) {
        huge_chunk[i] = v;
        auto val = memPool.curr_pool->prev_pool->blocks[i];
        assert(val == v);
    }
    // </>

    // <回收>
    memPool.free_prop();
    assert(memPool.curr_pool == NULL);
    // </>
    PRINT_FUNC;
}