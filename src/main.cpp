#include "common/gen.h"
#include "common/List.h"
#include "common/MemPool.h"
#include "common/Que.h"
#include "data_struct/CritBitTree.h"
#include <stdio.h>

void t_CritBitTree(void);

void t_gen(void);

void t_List(void);

void t_MemPool(void);

void t_PiXiuStr(void);

void t_Que(void);

int main() {
#ifndef NDEBUG
    t_CritBitTree();
    t_gen();
    t_List();
    t_MemPool();
    t_PiXiuStr();
    t_Que();

    printf("\nt_OK\n");
#endif
    return 0;
}

void t_CritBitTree(void) {
    assert(sizeof(CBTInner) == 24);
    CritBitTree cbt;
    char * output;
    auto chunk = PiXiuChunk_init();

    auto cbt_insert = [&](uint8_t src[]) {
        int len;
        for (len = 0; src[len] != '\0'; ++len);
        auto pxs = PiXiuStr_init_key(src, len);
        chunk->strs[chunk->used_num] = pxs;
        cbt.setitem(pxs, chunk, chunk->used_num);
        chunk->used_num++;
        assert(cbt.contains(pxs));
    };

    cbt_insert((uint8_t *) "ec$");
    cbt_insert((uint8_t *) "abec$");
    cbt_insert((uint8_t *) "ejjc$");
    cbt_insert((uint8_t *) "acd$");

    assert(!strcmp((output = cbt.repr()), "diff: 0, mask: 251\n"
            "    diff: 1, mask: 254\n"
            "        abec$\n"
            "        acd$\n"
            "    diff: 1, mask: 247\n"
            "        ec$\n"
            "        ejjc$\n"));
    free(output);

    auto prefix = PiXiuStr_init((uint8_t *) "e", 1);
    auto cbt_gen = cbt.iter(prefix);

    auto expect = "ec$ejjc$";
    auto i = 0;
    PXSGen * pxs_gen;
    uint8_t rv;
    while (cbt_gen->operator()(pxs_gen)) {
        while (pxs_gen->operator()(rv)) {
            if (char_visible(rv)) {
                assert(rv == expect[i]);
                i++;
            }
        }
        PXSGen_free(pxs_gen);
    }
    CBTGen_free(cbt_gen);
    PiXiuStr_free(prefix);

    auto cbt_delete = [&](uint8_t src[]) {
        int len;
        for (len = 0; src[len] != '\0'; ++len);
        auto pxs = PiXiuStr_init_key(src, len);
        cbt.delitem(pxs);
        assert(!cbt.contains(pxs));
        PiXiuStr_free(pxs);
    };

    cbt_delete((uint8_t *) "ejjc$");
    cbt_delete((uint8_t *) "abec$");

    assert(!strcmp((output = cbt.repr()), "diff: 0, mask: 251\n"
            "    acd$\n"
            "    ec$\n"));
    free(output);

    cbt_delete((uint8_t *) "ec$");
    cbt_delete((uint8_t *) "acd$");
    assert(!strcmp(cbt.repr(), "~"));

    cbt.free_prop();
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
    PiXiuStr_free(str);

    auto chunk_nf = (PiXiuChunk *) malloc(sizeof(PiXiuChunk));
    for (int i = 0; i < PXC_STR_NUM; ++i) {
        chunk_nf->strs[i] = (PiXiuStr *) malloc(1);
    }
    PiXiuChunk_free(chunk_nf);
    // --- PXS
#ifndef NDEBUG
#define PXS_STREAM(...) PiXiuStr_init_stream((PXSMsg) __VA_ARGS__)

#define set_expect(...) expect = (uint8_t[]) { __VA_ARGS__ }
#define assert_out(target) for(int i=0;i<target->len;++i) assert(target->data[i]==expect[i])
#define assert_pxs() assert_out(pxs)
#endif
    // init
    uint8_t * expect;

    uint8_t input[] = {1, PXS_UNIQUE, 2, PXS_UNIQUE, 4};
    auto pxs = PiXiuStr_init_key(input, 5);
    set_expect(1, 251, 251, 2, 251, 251, 4, 251, 0);
    assert_pxs();
    PiXiuStr_free(pxs);

    pxs = PiXiuStr_init(input, 5);
    assert(pxs->len == 7);
    assert_pxs();
    PiXiuStr_free(pxs);

    auto a = PiXiuStr_init(input, 2);
    auto b = PiXiuStr_init(input, 2);
    auto merge = a->concat(b);
    assert(merge->len == 6);
    set_expect(1, 251, 251, 1, 251, 251);
    assert_out(merge);
    PiXiuStr_free(a);
    PiXiuStr_free(b);
    PiXiuStr_free(merge);
    // stream init
    assert(sizeof(PXSRecordSmall) == 6);
    assert(sizeof(PXSRecordBig) == 8);

    PXS_STREAM({ .chunk_idx__cmd = PXS_STREAM_ON });
    for (int i = 0; i < 4; ++i) {
        PXS_STREAM({ .chunk_idx__cmd = PXS_STREAM_PASS, .val = 1 });
    }
    for (int i = 0; i < 6; ++i) {
        PXS_STREAM({ .chunk_idx__cmd = 2, .pxs_idx = i, .val = 3 });
    }
    PXS_STREAM({ .chunk_idx__cmd = PXS_STREAM_PASS, .val = 1 });
    for (int i = 0; i < 7; ++i) {
        PXS_STREAM({ .chunk_idx__cmd = 3, .pxs_idx = i, .val = 4 });
    }
    pxs = PXS_STREAM({ .chunk_idx__cmd = PXS_STREAM_OFF });

    set_expect(1, 1, 1, 1, 3, 3, 3, 3, 3, 3, 1, 251, 7, 3, 0, 7, 0);
    assert_pxs();
    PiXiuStr_free(pxs);

    PXS_STREAM({ .chunk_idx__cmd = PXS_STREAM_ON });
    for (int i = 0; i < 1; ++i) {
        PXS_STREAM({ .chunk_idx__cmd = PXS_STREAM_PASS, .val = 1 });
    }
    for (int i = 0; i < 255; ++i) {
        PXS_STREAM({ .chunk_idx__cmd = 2, .pxs_idx = i, .val = 3 });
    }
    PXS_STREAM({ .chunk_idx__cmd = PXS_STREAM_PASS, .val = 1 });
    for (int i = 0; i < 256; ++i) {
        PXS_STREAM({ .chunk_idx__cmd = 3, .pxs_idx = i, .val = 4 });
    }
    pxs = PXS_STREAM({ .chunk_idx__cmd = PXS_STREAM_OFF });

    set_expect(1, 251, 255, 2, 0, 255, 0, 1, 251, 1, 3, 0, 0, 1, 0, 0);
    assert_pxs();
    PiXiuStr_free(pxs);

    // parse
    assert(sizeof(PXSRecord) == sizeof(PXSRecordBig));
    PXS_STREAM({ .chunk_idx__cmd = PXS_STREAM_ON });
    PXS_STREAM({ .chunk_idx__cmd = PXS_STREAM_PASS, .val = 1 });
    PXS_STREAM({ .chunk_idx__cmd = PXS_STREAM_PASS, .val = PXS_UNIQUE });
    PXS_STREAM({ .chunk_idx__cmd = PXS_STREAM_PASS, .val = PXS_UNIQUE });
    PXS_STREAM({ .chunk_idx__cmd = PXS_STREAM_PASS, .val = 1 });
    PXS_STREAM({ .chunk_idx__cmd = 1, .pxs_idx = 0, .val = PXS_UNIQUE });
    PXS_STREAM({ .chunk_idx__cmd = 1, .pxs_idx = 1, .val = PXS_UNIQUE });
    PXS_STREAM({ .chunk_idx__cmd = PXS_STREAM_PASS, .val = 3 });
    for (int i = 0; i < 11; ++i) {
        PXS_STREAM({ .chunk_idx__cmd = 2, .pxs_idx = i, .val = 2 });
    }
    PXS_STREAM({ .chunk_idx__cmd = PXS_STREAM_PASS, .val = 3 });
    for (int i = 1; i < 257; ++i) {
        PXS_STREAM({ .chunk_idx__cmd = 3, .pxs_idx = i, .val = 6 });
    }
    pxs = PXS_STREAM({ .chunk_idx__cmd = PXS_STREAM_OFF });

    set_expect(1, 251, 251, 1, 251, 251, 3, 251, 11, 2, 0, 11, 0, 3, 251, 1, 3, 0, 1, 1, 1, 0);
    assert_pxs();

    PXS_STREAM({ .chunk_idx__cmd = PXS_STREAM_ON });
    for (int i = 0; i < 11; ++i) {
        PXS_STREAM({ .chunk_idx__cmd = PXS_STREAM_PASS, .val = 2 });
    }
    PXS_STREAM({ .chunk_idx__cmd = PXS_STREAM_PASS, .val = 8 });
    auto pxs_i2v2 = PXS_STREAM({ .chunk_idx__cmd = PXS_STREAM_OFF });

    PXS_STREAM({ .chunk_idx__cmd = PXS_STREAM_ON });
    PXS_STREAM({ .chunk_idx__cmd = PXS_STREAM_PASS, .val = 8 });
    for (int i = 0; i < 256; ++i) {
        PXS_STREAM({ .chunk_idx__cmd = PXS_STREAM_PASS, .val = 6 });
    }
    PXS_STREAM({ .chunk_idx__cmd = PXS_STREAM_PASS, .val = 8 });
    auto pxs_i3v6 = PXS_STREAM({ .chunk_idx__cmd = PXS_STREAM_OFF });

    chunk.strs[2] = pxs_i2v2;
    chunk.strs[3] = pxs_i3v6;
    auto gen = pxs->parse(1, 272, adrOf(chunk));

    uint8_t rv_arr[271];
    int i = 0;
    while (gen->operator()(rv_arr[i])) {
        i++;
    }

    i = 6;
    for (int j = 0; j < 11; ++j) {
        assert(rv_arr[i++] == 2);
    }
    assert(rv_arr[i++] == 3);
    for (int j = 0; j < 271 - 1 - 11 - 6; ++j) {
        assert(rv_arr[i++] == 6);
    }

    PXSGen_free(gen);
    PiXiuStr_free(pxs);
    PiXiuStr_free(pxs_i2v2);
    PiXiuStr_free(pxs_i3v6);

    // key equal
    PXS_STREAM({ .chunk_idx__cmd = PXS_STREAM_ON });
    PXS_STREAM({ .chunk_idx__cmd = PXS_STREAM_PASS, .val = 1 });
    PXS_STREAM({ .chunk_idx__cmd = PXS_STREAM_PASS, .val = PXS_UNIQUE });
    PXS_STREAM({ .chunk_idx__cmd = PXS_STREAM_PASS, .val = PXS_KEY });
    auto k1 = PXS_STREAM({ .chunk_idx__cmd = PXS_STREAM_OFF });

    PXS_STREAM({ .chunk_idx__cmd = PXS_STREAM_ON });
    PXS_STREAM({ .chunk_idx__cmd = PXS_STREAM_PASS, .val = 2 });
    auto k2 = PXS_STREAM({ .chunk_idx__cmd = PXS_STREAM_OFF });

    PXS_STREAM({ .chunk_idx__cmd = PXS_STREAM_ON });
    PXS_STREAM({ .chunk_idx__cmd = PXS_STREAM_PASS, .val = 1 });
    PXS_STREAM({ .chunk_idx__cmd = PXS_STREAM_PASS, .val = PXS_UNIQUE });
    PXS_STREAM({ .chunk_idx__cmd = PXS_STREAM_PASS, .val = PXS_KEY });
    PXS_STREAM({ .chunk_idx__cmd = PXS_STREAM_PASS, .val = 7 });
    auto k1_ = PXS_STREAM({ .chunk_idx__cmd = PXS_STREAM_OFF });

    assert(k1->key_eq(k1_, adrOf(chunk)));
    assert(!k1->key_eq(k2, adrOf(chunk)));
    PiXiuStr_free(k1);
    PiXiuStr_free(k2);
    PiXiuStr_free(k1_);
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

    memPool.free_prop();
    assert(memPool.curr_pool == NULL);
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
    int * expect;
    List_init(int, list);

    List_append(int, list, 3);
    List_append(int, list, 2);
    List_append(int, list, 1);
    expect = (int[]) {3, 2, 1};
    for (int i = 0; i < list_len; ++i) {
        assert(list[i] == expect[i]);
    }
    assert(list_len == 3 && list_capacity == 3);

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
    assert(list_len == 5 && list_capacity == 6);

    List_free(list);
}
