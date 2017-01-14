#ifndef SCAPEGOAT_TREE_H
#define SCAPEGOAT_TREE_H

#include "../common/List.h"
#include "../common/MemPool.h"
#include <functional>
#include <math.h>

template<typename T>
struct SGTNode {
    SGTNode<T> * small;
    SGTNode<T> * big;
    T * obj;
};

template<typename T>
struct ScapegoatTree {
    typedef SGTNode<T> SGTN;

    SGTN * root = NULL;
    int size = 0;

    void setitem(T * obj, MemPool * pool) {
        auto mk_node = [&]() {
            auto node = (SGTN *) pool->p_malloc(sizeof(SGTN));
            node->small = node->big = NULL;
            node->obj = obj;
            return node;
        };

        if (this->root == NULL) {
            this->root = mk_node();
            this->size++;
            return;
        }
        auto height = 0;
        List_init(SGTN *, path);

        auto cursor = this->root;
        while (true) {
            if (cursor->obj->operator==(obj)) {
                return;
            }

            height++;
            List_append(SGTN *, path, cursor);
            if (obj->operator<(cursor->obj)) {
                cursor = cursor->small;

                if (cursor == NULL) {
                    cursor = path[path_len - 1]->small = mk_node();
                    this->size++;
                    break;
                }
            } else {
                cursor = cursor->big;

                if (cursor == NULL) {
                    cursor = path[path_len - 1]->big = mk_node();
                    this->size++;
                    break;
                }
            }
        }

        if (height > log2(this->size)) {
            this->rebuild(this->find_scapegoat(path, path_len, cursor));
        }
        List_free(path);
    };

    T * getitem(T * obj) {

    };

    struct fsg_ret {
        SGTN * pa = NULL;
        SGTN * scapegoat = NULL;
        int size;
    };

    fsg_ret find_scapegoat(SGTN * path[], int path_len, SGTN * cursor) {
        fsg_ret ret{};
        auto size = 1;
        auto height = 1;

        while (path_len) {
            auto parent = path[path_len - 1];
            path_len--;

            SGTN * sibling;
            if (parent->small == cursor) {
                sibling = parent->big;
            } else {
                assert(parent->big == cursor);
                sibling = parent->small;
            }

            ret.size = size;
            height++;
            size += this->get_size(sibling) + 1;
            if (height > log2(size)) {
                cursor = parent;
            } else {
                ret.pa = parent;
                break;
            }
        }
        if (!path_len) { ret.size = size; }
        ret.scapegoat = cursor;
        return ret;
    }

    void rebuild(fsg_ret ret) {
        auto pa = ret.pa;
        auto scapegoat = ret.scapegoat;
        auto size = ret.size;
        assert(this->get_size(scapegoat) == size);

        SGTN * ordered_nodes[size];
        auto i = 0;
        std::function<void(SGTN *)> add = [&](SGTN * node) {
            if (node->small != NULL) {
                add(node->small);
            }
            ordered_nodes[i] = node;
            i++;
            if (node->big != NULL) {
                add(node->big);
            }
        };
        add(scapegoat);
        assert(i == size);

        auto pick_mid = [](int op, int ed) -> int {
            return (op + ed) / 2;
        };
        std::function<SGTN *(int, int)> build_tree = [&](int op, int ed) -> SGTN * {
            if (op > ed) { return NULL; }
            if (op == ed) {
                ordered_nodes[op]->small = ordered_nodes[op]->big = NULL;
                return ordered_nodes[op];
            }

            auto mi = pick_mid(op, ed);
            auto mi_node = ordered_nodes[mi];
            mi_node->small = build_tree(op, mi - 1);
            mi_node->big = build_tree(mi + 1, ed);
            return mi_node;
        };

        auto sub = build_tree(0, size - 1);
        if (pa == NULL) {
            this->root = sub;
        } else if (pa->small == scapegoat) {
            pa->small = sub;
        } else {
            assert(pa->big = scapegoat);
            pa->big = sub;
        }
    }

    int get_size(SGTN * node) {
        if (node == NULL) {
            return 0;
        }
        auto size_small = this->get_size(node->small);
        auto size_big = this->get_size(node->big);
        return size_small + size_big + 1;
    }
};

#endif