#ifndef SUFFIX_TREE_H
#define SUFFIX_TREE_H

#include "../proj/PiXiuStr.h"
#include "ScapegoatTree.h"

struct STNode {
    STNode * successor = NULL;
    ScapegoatTree<STNode> subs;

    uint16_t from;
    uint16_t to;
    uint16_t chunk_idx;

    void set_sub(STNode *);

    STNode * get_sub(uint8_t);

    bool operator<(STNode *);

    bool operator==(STNode *);

    bool is_root(void);

    bool is_inner(void);

    bool is_leaf(void);
};

struct SuffixTree {
    STNode * root;
    int counter;
    int remainder;

    STNode * act_node;
    uint16_t act_chunk_idx;
    uint16_t act_direct;
    uint16_t act_offset;

    MemPool local_pool;
    PiXiuChunk local_chunk;
    PiXiuChunk * cbt_chunk;

    struct s_ret {
        PiXiuChunk * cbt_chunk;
        uint16_t idx;
    };

    s_ret setitem(PiXiuStr *);

    char * repr(void);

    void init_prop(void);

    void free_prop(void);

    void reset(void);
};

STNode * STNode_init(void);

#endif