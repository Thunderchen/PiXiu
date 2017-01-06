#include "../common/style.h"
#include <stdint.h>

// interface
struct CBTStr {
    // --- //
    int len(void);

    uint8_t getitem(int);
};

struct CBTInner { // assert size
    void ptrAs(crit_0);
    void ptrAs(crit_1);

    uint16_t diff_at;
    uint8_t mask;

    uint8_t extra_ui8_0[2]; // memory padding
    uint8_t extra_ui8_1[3];
};

struct CritBitTree {
    void ptrAs(root);

    // --- //
    int setitem(CBTStr *);

    CBTStr * getitem(CBTStr *);

    int delitem(CBTStr *);

    void repr(uint8_t *&, int &, int &);

    // todo: iter
};