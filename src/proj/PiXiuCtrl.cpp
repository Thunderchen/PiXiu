#include "PiXiuCtrl.h"

int PiXiuCtrl::setitem(uint8_t * k, int k_len, uint8_t * v, int v_len) {
#ifndef NDEBUG
    auto counter = 0;
    for (int i = 0; i < k_len; ++i) {
        if (k[i] == PXS_UNIQUE) {
            counter++;
        }
    }
    for (int i = 0; i < v_len; ++i) {
        if (v[i] == PXS_UNIQUE) {
            counter++;
        }
    }
    assert(counter + k_len + v_len + 2 <= UINT16_MAX);
#endif

    auto pxs_k = PiXiuStr_init_key(k, k_len);
    auto pxs_v = PiXiuStr_init(v, v_len);
    auto pxs = pxs_k->concat(pxs_v);
    PiXiuStr_free(pxs_k);
    PiXiuStr_free(pxs_v);

    auto product = this->st.setitem(pxs);
    return this->cbt.setitem(pxs, product.cbt_chunk, product.idx);
}

#define WRAPPER(name, action) \
assert(name ## _len + 2 <= UINT16_MAX); \
auto pxs = PiXiuStr_init_key(name, name ## _len); \
auto ret = action(pxs); \
PiXiuStr_free(pxs); \
return ret;

bool PiXiuCtrl::contains(uint8_t * k, int k_len) {
    WRAPPER(k, this->cbt.contains);
}

PXSGen * PiXiuCtrl::getitem(uint8_t * k, int k_len) {
    WRAPPER(k, this->cbt.getitem);
}

CBTGen * PiXiuCtrl::iter(uint8_t * prefix, int prefix_len) {
    WRAPPER(prefix, this->cbt.iter);
}

int PiXiuCtrl::delitem(uint8_t * k, int k_len) {

}

void PiXiuCtrl::free_prop() {

}