#include "common/gen.h"
#include "common/List.h"
#include "common/MemPool.h"
#include "common/Que.h"
#include "proj/PiXiuStr.h"
#include <stdio.h>

void t_gen(void);

void t_List(void);

void t_MemPool(void);

void t_PiXiuStr(void);

void t_Que(void);

int main() {
#ifndef NDEBUG
    t_gen();
    t_List();
    t_MemPool();
    t_PiXiuStr();
    t_Que();

    printf("\nt_OK\n");
#endif
    return 0;
}

void t_PiXiuStr(void) {
    // --- PXC
    assert(sizeof(PiXiuStr) == 2);
    PiXiuChunk chunk;
    auto str = (PiXiuStr *) malloc(sizeof(PiXiuStr));
    str->len = 17;

    assert(chunk.used_num++ == 0);
    chunk.strs[0] = str;
    assert(chunk.getitem(0)->len == 17);
    assert(!chunk.is_delitem(0));

    chunk.delitem(0);
    assert(chunk.is_delitem(0));
    assert(chunk.getitem(0)->len == 17);

    free(str);
    // --- PXS
    // init
    uint8_t ptrAs(expect);
    uint8_t input[] = {1, PXS_UNIQUE, 2, PXS_UNIQUE, 4};

    auto pxs = PiXiuStr_init_key(input, 5);
    expect = (uint8_t[]) {1, PXS_UNIQUE, PXS_UNIQUE, 2, PXS_UNIQUE, PXS_UNIQUE, 4,
                          PXS_UNIQUE, PXS_KEY};
    for (int i = 0; i < pxs->len; ++i) {
        assert(pxs->data[i] == expect[i]);
    }
    PiXiuStr_free(pxs);

    pxs = PiXiuStr_init(input, 5);
    assert(pxs->len == 7);
    for (int i = 0; i < pxs->len; ++i) {
        assert(pxs->data[i] == expect[i]);
    }
    PiXiuStr_free(pxs);

    auto a = PiXiuStr_init(input, 2);
    auto b = PiXiuStr_init(input, 2);
    auto merge = a->concat(b);
    expect = (uint8_t[]) {1, PXS_UNIQUE, PXS_UNIQUE, 1, PXS_UNIQUE, PXS_UNIQUE};
    assert(merge->len == 6);
    for (int i = 0; i < merge->len; ++i) {
        assert(merge->data[i] == expect[i]);
    }
    PiXiuStr_free(a);
    PiXiuStr_free(b);
    PiXiuStr_free(merge);
    // stream init
    assert(sizeof(PXSRecordSmall) == 6);
    assert(sizeof(PXSRecordBig) == 8);
}

void t_MemPool(void) {
    MemPool memPool;
    auto v = (void *) 1;

    for (int i = 0; i < POOL_BLOCK_NUM; ++i) {
        auto adr = (void **) memPool.p_malloc(sizeof(v));
        valIn(adr) = v;
    }

    assert(memPool.curr_pool != NULL && memPool.curr_pool->prev_pool == NULL);
    for (int i = 0; i < POOL_BLOCK_NUM; ++i) {
        assert(memPool.curr_pool->blocks[i] == v);
    }
    assert(memPool.used_num == POOL_BLOCK_NUM);

    for (int i = 0; i < POOL_BLOCK_NUM - 1; ++i) {
        auto adr = (void **) memPool.p_malloc(sizeof(v));
        valIn(adr) = v;
    }

    assert(memPool.curr_pool->prev_pool != NULL);
    for (int i = 0; i < POOL_BLOCK_NUM - 1; ++i) {
        assert(memPool.curr_pool->blocks[i] == v);
    }
    assert(memPool.used_num == POOL_BLOCK_NUM - 1);

    memPool.p_malloc(sizeof(v) * 2);
    assert(memPool.used_num == 2);

    v = (void *) 2;
    auto huge_chunk = (void **) memPool.p_malloc(POOL_BLOCK_SIZE * (POOL_BLOCK_NUM + 1));
    assert(memPool.used_num == 2);

    for (int i = 0; i < POOL_BLOCK_NUM + 1; ++i) {
        huge_chunk[i] = v;
    }
    for (int i = 0; i < POOL_BLOCK_NUM + 1; ++i) {
        auto val = memPool.curr_pool->prev_pool->blocks[i];
        assert(val == v);
    }

    memPool.free_all();
}

void t_Que(void) {
    int q[] = {0, 0, 0};
    int q_len = 3;
    int q_cursor = 0;

    Que_push(q, 1);
    Que_push(q, 2);
    Que_push(q, 3);
    Que_push(q, 4);
    Que_push(q, 5);

    assert(Que_get(q, 0) == 3);
    assert(Que_get(q, 1) == 4);
    assert(Que_get(q, 2) == 5);
}

$gen(range0_10) {
    // var
    int i;
    // constructor

    // body
    $emit(int)
            for (i = 0; i < 10; ++i) {
                $yield(i);
            }
    $stop;
};

void t_gen(void) {
    range0_10 gen;
    int rv;
    for (int i = 0; i < 10; ++i) {
        assert(gen(rv));
        assert(rv == i);
    }
    assert(!gen(rv));
}

void t_List(void) {
    int ptrAs(expect);
    List_init(int, list);

    List_append(int, list, 3);
    List_append(int, list, 2);
    List_append(int, list, 1);
    expect = (int[]) {3, 2, 1};
    for (int i = 0; i < list_len; ++i) {
        assert(list[i] == expect[i]);
    }
    assert(list_len == 3 && list_capacity == 4);

    List_del(int, list, 1);
    assert(list[0] == 3 && list[1] == 1);
    List_del(int, list, 1);
    assert(list[0] == 3);
    List_del(int, list, 0);
    assert(list_len == 0);

    List_insert(int, list, 0, 0);
    List_insert(int, list, 0, 1);
    List_insert(int, list, 1, 2);
    expect = (int[]) {1, 2, 0};
    for (int i = 0; i < list_len; ++i) {
        assert(list[i] == expect[i]);
    }
    assert(list_len == 3);

    List_insert(int, list, 3, 3);
    List_insert(int, list, 3, 4);
    expect = (int[]) {1, 2, 0, 4, 3};
    for (int i = 0; i < list_len; ++i) {
        assert(list[i] == expect[i]);
    }
    assert(list_len == 5 && list_capacity == 8);

    List_free(list);
}
