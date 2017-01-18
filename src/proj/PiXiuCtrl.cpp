#include "PiXiuCtrl.h"

int PiXiuCtrl::setitem(uint8_t * k, int k_len, uint8_t * v, int v_len) {
    auto repeat_counter = 0;

    for (int i = 0; i < k_len; ++i) {
        if (k[i] == PXS_UNIQUE) {
            repeat_counter++;
        }
    }
    for (int i = 0; i < v_len; ++i) {
        if (v[i] == PXS_UNIQUE) {
            repeat_counter++;
        }
    }
    if (repeat_counter + k_len + v_len + 2 > UINT16_MAX) {
        return PX_CTRL_OVERFLOW;
    }

    auto pxs_k = PiXiuStr_init_key(k, k_len);
    auto pxs_v = PiXiuStr_init(v, v_len);
    auto pxs = pxs_k->concat(pxs_v);
    PiXiuStr_free(pxs_k);
    PiXiuStr_free(pxs_v);

    auto product = this->st.setitem(pxs);
    return this->cbt.setitem(pxs, product.cbt_chunk, product.idx);
}

bool PiXiuCtrl::contains(uint8_t * k, int k_len) {
    if (k_len + 2 > UINT16_MAX) {
        return PX_CTRL_OVERFLOW;
    }
    auto pxs = PiXiuStr_init_key(k, k_len);
    auto ret = this->cbt.contains(pxs);
    PiXiuStr_free(pxs);
    return ret;
}

PXSGen * PiXiuCtrl::getitem(uint8_t * k, int k_len) {

}

CBTGen * PiXiuCtrl::iter(uint8_t * prefix, int prefix_len) {

}

int PiXiuCtrl::delitem(uint8_t * k, int k_len) {

}

void PiXiuCtrl::free_prop() {

}