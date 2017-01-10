#include "CritBitTree.h"
#include <stddef.h>

CBTInter * CBTInter_init(void) {
    auto ret = (CBTInter *) malloc(offsetof(CBTInter, mask) + sizeof(CBTInter::mask));
    return ret;
}

void CBTInter_free(CBTInter * inter) {
    free(inter);
}