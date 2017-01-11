#include "../proj/PiXiuStr.h"
#include <tuple>

using std::tuple;

struct CBTInner {
    void * crit_node[2];
    uint16_t chunk_idx[2];

    uint16_t diff_at;
    uint8_t mask;
};

struct CritBitTree {
    void * root = NULL;
    uint16_t chunk_idx;

    int setitem(PiXiuStr *, PiXiuChunk *, int);

    char * repr(void);

private:
    tuple<void *, void *, void *> find_best_match(PiXiuStr *);
};

CBTInner * CBTInner_init(void);

void CBTInner_free(CBTInner *);