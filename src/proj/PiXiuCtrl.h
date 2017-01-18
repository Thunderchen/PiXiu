#ifndef PIXIU_CTRL_H
#define PIXIU_CTRL_H

#include "../data_struct/CritBitTree.h"
#include "../data_struct/SuffixTree.h"

#define PX_CTRL_OVERFLOW -1

struct PiXiuCtrl {
    CritBitTree cbt;
    SuffixTree st;

    int setitem(uint8_t *, int, uint8_t *, int);

    bool contains(uint8_t *, int);

    PXSGen * getitem(uint8_t *, int);

    CBTGen * iter(uint8_t *, int);

    int delitem(uint8_t *, int);

    void free_prop(void);
};

#endif