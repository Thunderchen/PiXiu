#include "../proj/PiXiuStr.h"

struct CBTInner {
    void * crit_node[2];
    uint16_t chunk_idx[2];

    uint16_t diff_at;
    uint8_t mask;
};

struct CritBitTree {
    void * root = NULL;
    uint16_t chunk_idx;

    int setitem(PiXiuStr *, PiXiuChunk *, uint16_t);

    char * repr(void);

private:
    struct fbm_ret {
        void * grand;
        void * pa;
        void * crit_node;
    };

    fbm_ret find_best_match(PiXiuStr *);
};

CBTInner * CBTInner_init(void);

void CBTInner_free(CBTInner *);