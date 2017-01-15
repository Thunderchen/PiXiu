#include "SuffixTree.h"

#define AB_CHAR() \
a_char = Glob_Ctx->getitem(this->chunk_idx)->data[0]; \
if (another->from > another->to) { \
    b_char = (uint8_t) another->chunk_idx; \
} else { \
    b_char = Glob_Ctx->getitem(another->chunk_idx)->data[0]; \
}

static MemPool * Glob_Pool;
static PiXiuChunk * Glob_Ctx;

void STNode::set_sub(STNode * node) {
    this->subs.setitem(node, Glob_Pool);
}

STNode * STNode::get_sub(uint8_t key) {
    STNode cmp{.from=1, .to=0};
    cmp.chunk_idx = key;
    return this->subs.getitem(adrOf(cmp));
}

bool STNode::operator<(STNode * another) {
    uint8_t a_char, b_char;
    AB_CHAR();
    return a_char < b_char;
}

bool STNode::operator==(STNode * another) {
    uint8_t a_char, b_char;
    AB_CHAR();
    return a_char == b_char;
}

bool STNode::is_root() {
    return this->successor == this;
}

bool STNode::is_inner() {
    return !this->is_root() && this->subs.root != NULL;
}

bool STNode::is_leaf() {
    return !this->is_root() && this->subs.root == NULL;
}