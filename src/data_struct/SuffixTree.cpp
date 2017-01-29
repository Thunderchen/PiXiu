#include "../proj/PiXiuStr.h"
#include "SuffixTree.h"
#include <string>

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

#define SET_AB_CHAR \
if (this->from > this->to) { \
    a_char = (uint8_t) this->chunk_idx; \
} else { \
    a_char = Glob_Ctx->getitem(this->chunk_idx)->data[this->from]; \
} \
if (another->from > another->to) { \
    b_char = (uint8_t) another->chunk_idx; \
} else { \
    b_char = Glob_Ctx->getitem(another->chunk_idx)->data[another->from]; \
}

bool STNode::operator<(STNode * another) {
    uint8_t a_char, b_char;
    SET_AB_CHAR;
    return a_char < b_char;
}

bool STNode::operator==(STNode * another) {
    uint8_t a_char, b_char;
    SET_AB_CHAR;
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

STNode * STNode_p_init(void) {
    auto ret = (STNode *) Glob_Pool->p_malloc(sizeof(STNode));
    ret->subs.root = NULL;
    ret->subs.size = 0;
    return ret;
}

void SuffixTree::init_prop() {
    assert(this->local_chunk.used_num == 0);
    assert(this->local_pool.curr_pool == NULL);
    memset(this->local_chunk.strs, 0, sizeof(PiXiuStr *) * PXC_STR_NUM);

    Glob_Ctx = adrOf(this->local_chunk);
    Glob_Pool = adrOf(this->local_pool);

    this->root = STNode_p_init();
    this->root->successor = this->root;
    this->remainder = this->counter = 0;

    this->act_node = this->root;
    this->act_chunk_idx = this->act_direct = this->act_offset = 0;

    this->cbt_chunk = (PiXiuChunk *) malloc(sizeof(PiXiuChunk));
    this->cbt_chunk->used_num = 0;
}

void SuffixTree::free_prop() {
    this->local_chunk.free_prop();
    this->local_pool.free_prop();
}

void SuffixTree::reset() {
    this->remainder = this->counter = 0;
    this->act_node = this->root;
    this->act_chunk_idx = this->act_direct = this->act_offset = 0;
}

char * SuffixTree::repr() {
    List_init(char, output);

    auto print_node = [&](STNode * node) {
        if (node == this->act_node) {
            List_append(char, output, '*';)
        }
        auto pxs = Glob_Ctx->getitem(node->chunk_idx);
        for (int i = node->from; i < node->to; ++i) {
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
            for (uint8_t i = 0; i < UINT8_MAX; ++i) {
                if ((sub_node = node->get_sub(i)) != NULL) {
                    print_tree(sub_node, lv);
                }
            }
        }
    };

    print_tree(this->root, 0);
    List_append(char, output, '\0');
    return output;
}

#define MSG_NO_COMPRESS PiXiuStr_init_stream((PXSMsg) {.chunk_idx__cmd=PXS_STREAM_PASS, .val=msg_char})
#define MSG_COMPRESS(c_i, p_i) \
PiXiuStr_init_stream((PXSMsg) { \
    .chunk_idx__cmd=c_i, \
    .pxs_idx=p_i, \
    .val=msg_char \
})

static void s_insert_char(SuffixTree * self, uint16_t chunk_idx, uint8_t msg_char) {
    self->remainder++;
    auto curr_pxs = Glob_Ctx->getitem(chunk_idx);

    auto case_root = [&](bool send_msg) {
        auto edge_node = self->root->get_sub(msg_char);
        if (edge_node == NULL) {
            auto leaf_node = STNode_p_init();
            leaf_node->successor = self->root;

            leaf_node->chunk_idx = chunk_idx;
            leaf_node->from = self->counter;
            leaf_node->to = curr_pxs->len;
            self->root->set_sub(leaf_node);
            leaf_node->parent = self->root;
            self->remainder--;
            if (send_msg) { MSG_NO_COMPRESS; }
        } else {
            self->act_chunk_idx = edge_node->chunk_idx;
            self->act_direct = edge_node->from;
            self->act_offset++;
            assert(self->act_offset == 1);
            if (send_msg) { MSG_COMPRESS(edge_node->chunk_idx, edge_node->from); }
        }
    };

    if (self->act_node->is_root() && self->act_offset == 0) {
        case_root(true);
    } else {
        auto edge_pxs = Glob_Ctx->getitem(self->act_chunk_idx);
        auto edge_node = self->act_node->get_sub(edge_pxs->data[self->act_direct]);

        STNode * next_edge_node;
        if (edge_node->from + self->act_offset == edge_node->to
            && (next_edge_node = edge_node->get_sub(msg_char))) {
            self->act_node = edge_node;
            self->act_chunk_idx = next_edge_node->chunk_idx;
            self->act_direct = next_edge_node->from;
            self->act_offset = 1;
            MSG_COMPRESS(next_edge_node->chunk_idx, next_edge_node->from);
        } else if (edge_node->from + self->act_offset < edge_node->to
                   && msg_char == edge_pxs->data[edge_node->from + self->act_offset]) {
            MSG_COMPRESS(edge_node->chunk_idx, edge_node->from + self->act_offset);
            self->act_offset++;
        } else {
            MSG_NO_COMPRESS;
            STNode * prev_inner_node = NULL;

            auto split_grow = [&]() {
                auto leaf_node = STNode_p_init();
                leaf_node->successor = self->root;

                leaf_node->chunk_idx = chunk_idx;
                leaf_node->from = self->counter;
                leaf_node->to = curr_pxs->len;
                self->remainder--;

                if ((edge_node->is_leaf() || edge_node->to - edge_node->from > 1)
                    && edge_node->from + self->act_offset != edge_node->to) {
                    auto inner_node = STNode_p_init();
                    inner_node->successor = self->root;

                    if (prev_inner_node != NULL) {
                        prev_inner_node->successor = inner_node;
                    }
                    prev_inner_node = inner_node;

                    inner_node->chunk_idx = edge_node->chunk_idx;
                    inner_node->from = edge_node->from;
                    inner_node->to = inner_node->from + self->act_offset;
                    inner_node->parent = edge_node->parent;
                    inner_node->parent->set_sub(inner_node);

                    edge_node->from = inner_node->to;
                    edge_node->parent = inner_node;

                    inner_node->set_sub(edge_node);
                    inner_node->set_sub(leaf_node);
                    leaf_node->parent = inner_node;
                } else {
                    if (prev_inner_node != NULL) {
                        prev_inner_node->successor = edge_node;
                    }
                    prev_inner_node = edge_node;

                    edge_node->set_sub(leaf_node);
                    leaf_node->parent = edge_node;
                }
            };

            auto overflow_fix = [&]() {
                auto end = self->counter;
                auto begin = end - self->act_offset;
                edge_node = self->act_node->get_sub(curr_pxs->data[self->counter - self->act_offset]);
                edge_pxs = Glob_Ctx->getitem(edge_node->chunk_idx);

                int supply;
                while (end - begin > (supply = edge_node->to - edge_node->from)) {
                    self->act_node = edge_node;
                    begin += supply;
                    self->act_offset -= supply;

                    edge_node = self->act_node->get_sub(curr_pxs->data[begin]);
                    edge_pxs = Glob_Ctx->getitem(edge_node->chunk_idx);
                    self->act_direct = edge_node->from;
                }
            };

            while (self->remainder > 0) {
                split_grow();
                if (!self->act_node->is_inner()) {
                    self->act_offset--;
                    self->act_direct++;

                    if (self->act_offset > 0) {
                        overflow_fix();
                    } else {
                        case_root(false);
                        break;
                    }
                } else {
                    self->act_node = self->act_node->successor;
                    overflow_fix();
                }

                if (edge_node->from + self->act_offset == edge_node->to
                    && (next_edge_node = edge_node->get_sub(msg_char))) {
                    self->act_node = edge_node;
                    self->act_chunk_idx = next_edge_node->chunk_idx;
                    self->act_direct = next_edge_node->from;
                    self->act_offset = 1;

                    if (prev_inner_node) {
                        prev_inner_node->successor = self->act_node;
                    }
                    break;
                } else if (edge_node->from + self->act_offset < edge_node->to
                           && msg_char == edge_pxs->data[edge_node->from + self->act_offset]) {
                    self->act_offset++;
                    break;
                }
            }
        }
    }
    self->counter++;
}

SuffixTree::s_ret SuffixTree::setitem(PiXiuStr * src) {
    auto idx = this->local_chunk.used_num;
    assert(idx == this->cbt_chunk->used_num);

    this->local_chunk.strs[idx] = src;
    PiXiuStr_init_stream((PXSMsg) {.chunk_idx__cmd=PXS_STREAM_ON});
    for (int i = 0; i < src->len; ++i) {
        s_insert_char(this, idx, src->data[i]);
    }
    this->cbt_chunk->strs[idx] = PiXiuStr_init_stream((PXSMsg) {.chunk_idx__cmd=PXS_STREAM_OFF});

    this->local_chunk.used_num++;
    this->cbt_chunk->used_num++;
    this->reset();
    return s_ret{this->cbt_chunk, idx};
}

void t_SuffixTree(void) {
    using namespace std;

    auto contains = [](SuffixTree * self, uint8_t src[], int begin, int end) -> bool {
        auto edge_node = self->root->get_sub(src[begin]);
        if (edge_node == NULL) {
            return false;
        }

        while (true) {
            auto edge_pxs = Glob_Ctx->getitem(edge_node->chunk_idx);
            for (int i = edge_node->from; i < edge_node->to; ++i) {
                if (edge_pxs->data[i] != src[begin]) {
                    return false;
                } else {
                    begin++;
                    if (begin == end) {
                        return true;
                    }
                }
            }
            edge_node = edge_node->get_sub(src[begin]);
        }
    };

    SuffixTree st;
    st.init_prop();

    string alphabet[] = {"A", "B", "C", "D", "E"};
    for (int i = 0; i < 100; ++i) {
        string sample;
        auto len = rand() % 20 + 1;
        for (int j = 0; j < len; ++j) {
            sample += alphabet[rand() % lenOf(alphabet)];
        }
        sample += '.';

        auto end = (int) sample.size();
        auto src = (uint8_t *) sample.c_str();
        auto res = st.setitem(PiXiuStr_init(src, end));
        for (int begin = 0; begin < end; ++begin) {
            assert(contains(adrOf(st), src, begin, end));
        }

        auto str = res.cbt_chunk->getitem(res.idx)->parse(0, PXSG_MAX_TO, res.cbt_chunk)->consume_repr();
        assert(!strcmp(str, sample.c_str()));
        free(str);
    }

    free(st.repr());
    PiXiuChunk_free(st.cbt_chunk);
    st.free_prop();
    PRINT_FUNC;
}