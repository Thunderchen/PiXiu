#include "common/List.h"
#include <stdio.h>

void t_List(void);

void t_CritBitTree(void);

int main() {
#ifndef NDEBUG
    t_List();
    t_CritBitTree();

    printf("\nt_OK\n");
#endif
    return 0;
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

void t_CritBitTree(void) {
}