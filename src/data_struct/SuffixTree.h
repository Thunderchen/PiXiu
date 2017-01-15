#ifndef SUFFIX_TREE_H
#define SUFFIX_TREE_H

#include "../proj/PiXiuStr.h"
#include "ScapegoatTree.h"

struct STNode {
    STNode * successor = NULL;
    ScapegoatTree<STNode> subs;

    uint16_t op;
    uint16_t ed;
    uint16_t chunk_idx;

    void set_sub(STNode *);

    STNode * get_sub(uint8_t);

    bool operator<(STNode *);

    bool operator==(STNode *);
};

#endif