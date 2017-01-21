#include "../proj/PiXiuStr.h"
#include "SuffixTree.h"

static MemPool * Glob_Pool = NULL;
static PiXiuChunk * Glob_Ctx = NULL;

void STNode::set_sub(STNode * node) {
    assert(Glob_Pool != NULL);
    this->subs.setitem(node, Glob_Pool);
}

STNode * STNode::get_sub(uint8_t key) {
    STNode cmp{.from=1, .to=0};
    cmp.chunk_idx = key;
    return this->subs.getitem(adrOf(cmp));
}

#define SET_AB_CHAR \
assert(Glob_Ctx != NULL); \
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

STNode * STNode_p_init(void) {
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
    this->remainder = 0;
    this->counter = 0;
    this->act_node = this->root;
    this->act_chunk_idx = 0;
    this->act_direct = 0;
    this->act_offset = 0;
}

char * SuffixTree::repr() {
    List_init(char, output);

    std::function<void(STNode *)> print_node = [&](STNode * node) {
        auto pxs = this->local_chunk.getitem(node->chunk_idx);
        if (node == this->act_node) {
            List_append(char, output, '*';)
        }
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
#define MSG_COMPRESS(c_idx, p_idx) \
PiXiuStr_init_stream((PXSMsg) { \
    .chunk_idx__cmd=c_idx, \
    .pxs_idx=p_idx, \
    .val=msg_char \
})

static void s_case_root(SuffixTree * self, uint16_t chunk_idx, uint8_t msg_char, bool send_msg = true) {
    auto collapse_node = self->root->get_sub(msg_char);
    if (collapse_node == NULL) { // 无法坍缩, 新建叶结点
        auto leaf_node = STNode_p_init();
        leaf_node->chunk_idx = chunk_idx;
        leaf_node->from = self->counter;
        leaf_node->to = Glob_Ctx->getitem(chunk_idx)->len;
        self->root->set_sub(leaf_node);
        self->remainder--;
        if (send_msg) { MSG_NO_COMPRESS; }
    } else { // 开始坍缩
        self->act_chunk_idx = collapse_node->chunk_idx;
        self->act_direct = collapse_node->from;
        self->act_offset++;
        if (send_msg) { MSG_COMPRESS(collapse_node->chunk_idx, collapse_node->from); }
    }
}

static void s_overflow_fix(SuffixTree * self, uint16_t chunk_idx, uint16_t remainder) {
    auto temp_uchar = Glob_Ctx->getitem(self->act_chunk_idx)->data[self->act_direct];
    auto collapse_node = self->act_node->get_sub(temp_uchar);

    // counter - remainder + 1 = collapse_node.op
    auto supply = collapse_node->to - collapse_node->from;
    if (self->act_offset > supply) {
        self->act_node = collapse_node;
        remainder -= supply;
        temp_uchar = Glob_Ctx->getitem(chunk_idx)->data[self->counter - remainder + 1];

        auto next_collapse_node = collapse_node->get_sub(temp_uchar);
        self->act_chunk_idx = next_collapse_node->chunk_idx;
        self->act_direct = next_collapse_node->from;
        self->act_offset -= supply;
        return s_overflow_fix(self, chunk_idx, remainder);
    }
}

static void s_split_grow(SuffixTree * self, uint16_t chunk_idx, STNode * collapse_node) {
    if (collapse_node->to - collapse_node->from > 1 &&
        collapse_node->from + self->act_offset != collapse_node->to) {
        auto inherit_node = STNode_p_init();
        inherit_node->chunk_idx = collapse_node->chunk_idx;
        inherit_node->from = collapse_node->from + self->act_offset;
        inherit_node->to = collapse_node->to;
        inherit_node->successor = collapse_node->successor;
        inherit_node->subs = collapse_node->subs;

        // 原坍缩点成为 inner_node
        collapse_node->to = inherit_node->from;
        collapse_node->subs.root = NULL;
        collapse_node->subs.size = 0;
        collapse_node->set_sub(inherit_node);
    }

    // 新叶节点记录 uchar
    auto leaf_node = STNode_p_init();
    leaf_node->chunk_idx = chunk_idx;
    leaf_node->from = self->counter;
    leaf_node->to = Glob_Ctx->getitem(chunk_idx)->len;
    collapse_node->set_sub(leaf_node);
    self->remainder--;
}

static void s_insert_char(SuffixTree * self, uint16_t chunk_idx, uint8_t msg_char) {
    self->remainder++;
    uint8_t temp_uchar;

    if (self->act_node->is_root() && self->act_offset == 0) {
        s_case_root(self, chunk_idx, msg_char);
    } else { // 已坍缩
        temp_uchar = Glob_Ctx->getitem(self->act_chunk_idx)->data[self->act_direct];
        auto collapse_node = self->act_node->get_sub(temp_uchar);
        assert(Glob_Ctx->getitem(collapse_node->chunk_idx)->data[collapse_node->from] == temp_uchar);

        // edge 扩大?
        if (collapse_node->from + self->act_offset == collapse_node->to) {
            auto next_collapse_node = collapse_node->get_sub(msg_char);
            if (next_collapse_node != NULL) { // YES
                self->act_node = collapse_node; // 推移 act_node
                self->act_chunk_idx = next_collapse_node->chunk_idx;
                self->act_direct = next_collapse_node->from;
                self->act_offset = 1;
                MSG_COMPRESS(next_collapse_node->chunk_idx, next_collapse_node->from);
                goto end;
            } else { // NO
                goto explode;
            }
        }

        temp_uchar = Glob_Ctx->getitem(collapse_node->chunk_idx)->data[collapse_node->from + self->act_offset];
        if (temp_uchar == msg_char) { // YES
            MSG_COMPRESS(collapse_node->chunk_idx, collapse_node->from + self->act_offset);
            self->act_offset++;
        } else { // NO
            explode: // 炸开累积后缀
            MSG_NO_COMPRESS;
            while (self->remainder > 0) {
                s_split_grow(self, chunk_idx, collapse_node);

                if (!self->act_node->is_inner()) {
                    // 状态转移
                    self->act_offset--;
                    self->act_direct++;

                    if (self->act_offset > 0) {
                        s_overflow_fix(self, chunk_idx, self->remainder);
                        temp_uchar = Glob_Ctx->getitem(self->act_chunk_idx)->data[self->act_direct];
                        auto next_collapse_node = self->act_node->get_sub(temp_uchar);
                        collapse_node->successor = next_collapse_node;
                        collapse_node = next_collapse_node;
                    } else { // 后缀已用完, 回到 case_root
                        collapse_node->successor = self->root;
                        s_case_root(self, chunk_idx, msg_char, false);
                        break;
                    }
                } else { // 需要使用 suffix link
                    self->act_node = self->act_node->successor;
                    s_overflow_fix(self, chunk_idx, self->remainder);

                    temp_uchar = Glob_Ctx->getitem(self->act_chunk_idx)->data[self->act_direct];
                    auto next_collapse_node = self->act_node->get_sub(temp_uchar);
                    collapse_node->successor = next_collapse_node;
                    collapse_node = next_collapse_node;
                }

                if (collapse_node->from + self->act_offset == collapse_node->to) {
                    auto next_collapse_node = collapse_node->get_sub(msg_char);
                    if (next_collapse_node != NULL) {
                        self->act_node = collapse_node;
                        self->act_chunk_idx = next_collapse_node->chunk_idx;
                        self->act_direct = next_collapse_node->from;
                        self->act_offset = 1;
                        break;
                    }
                } else {
                    assert(collapse_node->from + self->act_offset < collapse_node->to);
                    if (Glob_Ctx->getitem(collapse_node->chunk_idx)
                                ->data[collapse_node->from + self->act_offset] == msg_char) {
                        break;
                    }
                }
            }
        }
    }

    end:
    self->counter++;
}

SuffixTree::s_ret SuffixTree::setitem(PiXiuStr * src) {
    assert(Glob_Ctx != NULL && Glob_Pool != NULL);
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
    SuffixTree st;
    st.init_prop();
    char * expect;

    auto insert = [&](uint8_t src[]) {
        int len;
        for (len = 0; src[len] != '\0'; ++len);
        auto pxs = PiXiuStr_init(src, len);
        st.setitem(pxs);
        assert(!strcmp((char *) src,
                       st.cbt_chunk
                               ->getitem(st.cbt_chunk->used_num - 1)
                               ->parse(0, PXSG_MAX_TO, st.cbt_chunk)
                               ->consume_repr()));
    };

    expect = (char *) "#\n"
            "--ARY\n"
            "--I\n"
            "  --ONARY\n"
            "  --PPI\n"
            "  --SSI\n"
            "    --ONARY\n"
            "    --PPI\n"
            "    --SSIPPI\n"
            "--MISSI\n"
            "  --ONARY\n"
            "  --SSIPPI\n"
            "--NARY\n"
            "--ONARY\n"
            "--P\n"
            "  --I\n"
            "  --PI\n"
            "--RY\n"
            "--S\n"
            "  --I\n"
            "    --ONARY\n"
            "    --PPI\n"
            "    --SSIPPI\n"
            "  --SI\n"
            "    --ONARY\n"
            "    --PPI\n"
            "    --SSIPPI\n"
            "--Y\n";
    insert((uint8_t *) "MISSISSIPPI");
    insert((uint8_t *) "MISSIONARY");
    assert(!strcmp(st.repr(), expect));

    expect = (char *) "#\n"
            "--ARY\n"
            "--B\n"
            "  --BI\n"
            "  --I\n"
            "--I\n"
            "  --BBI\n"
            "  --ONARY\n"
            "  --PPI\n"
            "  --SSI\n"
            "    --BBI\n"
            "    --ONARY\n"
            "    --PPI\n"
            "    --SSI\n"
            "      --BBI\n"
            "      --PPI\n"
            "--M\n"
            "  --ISSI\n"
            "    --ONARY\n"
            "    --SSI\n"
            "      --BBI\n"
            "      --PPI\n"
            "  --MMMMMMMMMM\n"
            "--NARY\n"
            "--ONARY\n"
            "--P\n"
            "  --I\n"
            "  --PI\n"
            "--RY\n"
            "--S\n"
            "  --I\n"
            "    --BBI\n"
            "    --ONARY\n"
            "    --PPI\n"
            "    --SSI\n"
            "      --BBI\n"
            "      --PPI\n"
            "  --SI\n"
            "    --BBI\n"
            "    --ONARY\n"
            "    --PPI\n"
            "    --SSI\n"
            "      --BBI\n"
            "      --PPI\n"
            "--Y\n";
    insert((uint8_t *) "MISSISSIBBI");
    insert((uint8_t *) "MMMMMMMMMMM");
    assert(!strcmp(st.repr(), expect));
    assert(st.cbt_chunk->getitem(2)->len == 9);
    assert(st.cbt_chunk->getitem(3)->len == 8);

    uint8_t temp[259];
    for (int i = 0; i < lenOf(temp) - 1; ++i) {
        temp[i] = 'A';
    }
    temp[lenOf(temp) - 1] = '\0';

    insert(temp);
    assert(st.cbt_chunk->getitem(4)->len == 10);
    st.free_prop();

    st.init_prop();
    expect = (char *) "#\n"
            "--ADCCDB\n"
            "--BDDADCCDB\n"
            "--C\n"
            "  --CDB\n"
            "  --D\n"
            "    --B\n"
            "    --DDBDDADCCDB\n"
            "--D\n"
            "  --ADCCDB\n"
            "  --BDDADCCDB\n"
            "  --CCDB\n"
            "  --D\n"
            "    --ADCCDB\n"
            "    --BDDADCCDB\n"
            "    --DBDDADCCDB\n";
    insert((uint8_t *) "CDDDBDDADCCDB");
    assert(!strcmp(st.repr(), expect));
    st.free_prop();

    st.init_prop();
    insert((uint8_t *) "DACBCBDDDADBADADBD");
}