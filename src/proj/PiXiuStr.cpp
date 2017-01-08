#include "../common/util.h"
#include "PiXiuStr.h"
#include <assert.h>

PiXiuStr * PiXiuStr_init_key(uint8_t src[], int src_len) {

};

PiXiuStr * PiXiuStr_init(uint8_t src[], int src_len) {

};

PiXiuStr * PiXiuStr_init_stream(uint8_t msg_char, int chunk_idx, int offset) {

};

PiXiuStr * PiXiuStr::concat(PiXiuStr * another) {

}

Generator PiXiuStr::parse(int op, int ed, PiXiuChunk * ctx) {

}

void PiXiuChunk::delitem(int idx) {
    auto adr = this->strs[idx];
    assert(adr != NULL && !adr_is_spec(adr));
    this->strs[idx] = adr_mk_spec(adr);
}

bool PiXiuChunk::is_delitem(int idx) {
    auto adr = this->strs[idx];
    assert(adr != NULL);
    return adr_is_spec(adr);
}

PiXiuStr * PiXiuChunk::getitem(int idx) {
    auto adr = this->strs[idx];
    return adr_de_spec(adr);
}