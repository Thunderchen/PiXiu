#include "SuffixTree.h"

static MemPool * Glob_Pool = NULL;
static PiXiuChunk * Glob_Ctx = NULL;

void STNode::set_sub(STNode * node) {
    this->subs.setitem(node, Glob_Pool);
}

STNode * STNode::get_sub(uint8_t key) {
    STNode cmp{.from=1, .to=0};
    cmp.chunk_idx = key;
    return this->subs.getitem(adrOf(cmp));
}

#define SET_AB_CHAR() \
a_char = Glob_Ctx->getitem(this->chunk_idx)->data[0]; \
if (another->from > another->to) { \
    b_char = (uint8_t) another->chunk_idx; \
} else { \
    b_char = Glob_Ctx->getitem(another->chunk_idx)->data[0]; \
}

bool STNode::operator<(STNode * another) {
    uint8_t a_char, b_char;
    SET_AB_CHAR();
    return a_char < b_char;
}

bool STNode::operator==(STNode * another) {
    uint8_t a_char, b_char;
    SET_AB_CHAR();
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

STNode * STNode_init(void) {
    assert(Glob_Pool != NULL);
    auto ret = (STNode *) Glob_Pool->p_malloc(sizeof(STNode));
    ret->successor = NULL;
    ret->subs.root = NULL;
    ret->subs.size = 0;
    return ret;
}

void SuffixTree::init_prop() {
    assert(this->local_chunk.used_num == 0);
    assert(this->local_pool.curr_pool == NULL);

    Glob_Ctx = adrOf(this->local_chunk);
    Glob_Pool = adrOf(this->local_pool);

    this->root = STNode_init();
    this->root->successor = this->root;
    this->remainder = this->counter = 0;

    this->act_node = this->root;
    this->act_chunk_idx = this->act_direct = this->act_offset = 0;

    this->cbt_chunk = (PiXiuChunk *) this->local_pool.p_malloc(sizeof(PiXiuChunk));
    this->cbt_chunk->used_num = 0;
}

void SuffixTree::free_prop() {
    this->local_chunk.free_prop();
    this->local_pool.free_prop();
}

void SuffixTree::reset() {
    this->remainder = 0;
    this->counter = 0;
    this->act_node = this->root;
    this->act_chunk_idx = 0;
    this->act_direct = 0;
    this->act_offset = 0;
}

SuffixTree::s_ret SuffixTree::setitem(PiXiuStr *) {

}

char * SuffixTree::repr() {
    List_init(char, output);

    std::function<void(STNode *)> print_node = [&](STNode * node) {
        auto pxs = this->local_chunk.getitem(node->chunk_idx);
        for (int i = 0; i < pxs->len; ++i) {
            if (char_visible(pxs->data[i])) {
                List_append(char, output, pxs->data[i]);
            }
        }
    };

    std::function<void(STNode *, int)> print_tree = [&](STNode * node, int lv) {
        if (lv > 0) {
            for (int i = 0; i < (lv - 1) * 2; ++i) {
                List_append(char, output, ' ');
            }
            for (int i = 0; i < 2; ++i) {
                List_append(char, output, '-');
            }
            print_node(node);
        } else {
            List_append(char, output, '#');
        }
        List_append(char, output, '\n');

        if (node->subs.root != NULL) {
            lv++;
            STNode * sub_node;
            for (uint8_t i = 0; i <= UINT8_MAX && (sub_node = node->get_sub(i)) != NULL; ++i) {
                print_tree(sub_node, lv);
            }
        }
    };

    List_append(char, output, '\0');
    return output;
}