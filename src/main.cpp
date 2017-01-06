#include "common/List.h"
#include <stdio.h>

void t_List(void);

int main() {
#ifndef NDEBUG
    t_List();
    printf("\nt_OK\n");
#endif
    return 0;
}

void t_List(void) {
    int ptrAs(expect);
    List_init(int, list_nf);

    List_append(int, list_nf, 3);
    List_append(int, list_nf, 2);
    List_append(int, list_nf, 1);
    expect = (int[]) {3, 2, 1};
    for (int i = 0; i < list_nf_len; ++i) {
        assert(list_nf[i] == expect[i]);
    }
    assert(list_nf_len == 3 && list_nf_capacity == 4);

    List_del(int, list_nf, 1);
    assert(list_nf[0] == 3 && list_nf[1] == 1);
    List_del(int, list_nf, 1);
    assert(list_nf[0] == 3);
    List_del(int, list_nf, 0);
    assert(list_nf_len == 0);

    List_insert(int, list_nf, 0, 0);
    List_insert(int, list_nf, 0, 1);
    List_insert(int, list_nf, 1, 2);
    expect = (int[]) {1, 2, 0};
    for (int i = 0; i < list_nf_len; ++i) {
        assert(list_nf[i] == expect[i]);
    }
    assert(list_nf_len == 3);

    List_insert(int, list_nf, 3, 3);
    List_insert(int, list_nf, 3, 4);
    expect = (int[]) {1, 2, 0, 4, 3};
    for (int i = 0; i < list_nf_len; ++i) {
        assert(list_nf[i] == expect[i]);
    }
    assert(list_nf_len == 5 && list_nf_capacity == 8);

    List_free(list_nf);
}