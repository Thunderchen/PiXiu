#include "../common/Que.h"
#include "CritBitTree.h"

int CritBitTree::setitem(PiXiuStr * src, PiXiuChunk * ctx, int idx) {

}

char * CritBitTree::repr(void) {

}

tuple<void *, void *, void *> CritBitTree::find_best_match(PiXiuStr * src) {

}

CBTInner * CBTInner_init(void) {
    return (CBTInner *) malloc(offsetof(CBTInner, mask) + sizeof(CBTInner::mask));
}

void CBTInner_free(CBTInner * inner) {
    free(inner);
}