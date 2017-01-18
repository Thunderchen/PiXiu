#ifndef PIXIU_CTRL_H
#define PIXIU_CTRL_H

#include "../data_struct/CritBitTree.h"
#include "../data_struct/SuffixTree.h"

struct PiXiuCtrl {
    CritBitTree cbt;
    SuffixTree st;

    int setitem(uint8_t *);

    bool contains(uint8_t *);

    PXSGen * getitem(uint8_t *);

    CBTGen * iter(uint8_t *);

    int delitem(uint8_t *);

    void free_prop(void);
};

#endif