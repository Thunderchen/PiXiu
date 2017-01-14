#ifndef SCAPEGOAT_TREE_H
#define SCAPEGOAT_TREE_H

#include "../common/List.h"
#include "../common/MemPool.h"
#include <math.h>
#include <stddef.h>
#include <stdlib.h>

template<typename T>
struct SGTNode {
    SGTNode<T> * small;
    SGTNode<T> * big;
    T * obj;
};

template<typename T, MemPool * pool>
struct ScapegoatTree {
    typedef SGTNode<T> SGTN;

    SGTN * root = NULL;
    int size = 0;

    void setitem(T * obj) {
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
            if (obj->operator<(cursor)) {
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
            this->rebuild(this->find_scapegoat(path, cursor));
        }
        List_free(path);
    };

    T * getitem(T * obj) {

    };


    struct fsg_ret {
        SGTN * pa = NULL;
        SGTN * scapegoat = NULL;
    };

    fsg_ret find_scapegoat(SGTN * path[], SGTN * cursor) {
        fsg_ret ret;

    }

    void rebuild(fsg_ret ret) {

    }

    int get_size() {
    }
};

#endif