#include "../common/List.h"
#include "../common/util.h"
#include "PiXiuStr.h"
#include <assert.h>

static PiXiuStr * escape_unique(uint8_t[], int, bool);

PiXiuStr * PiXiuStr_init_key(uint8_t src[], int src_len) {
    return escape_unique(src, src_len, true);
};

PiXiuStr * PiXiuStr_init(uint8_t src[], int src_len) {
    return escape_unique(src, src_len, false);
};

PiXiuStr * PiXiuStr_init_stream(PXSMsg msg) {
    static int list_len;
    static int list_capacity;
    static uint8_t ptrAs(list);

    static int compress_len;
    static int compress_idx;
    static int compress_to;

    auto chunk_idx = msg.chunk_idx__cmd;
    auto pxs_idx = msg.pxs_idx;
    auto msg_char = msg.val;
    PiXiuStr ptrAs(ret);

    auto try_explode = [&]() {
        if (compress_len > 0) {
            if (compress_len > sizeof(PXSRecordSmall)) {
                list_len -= compress_len;

                if (compress_len > UINT8_MAX) {
                    auto record = (PXSRecordBig *) adrOf(list[list_len]);
                    record->head = PXS_UNIQUE;
                    record->sign = PXS_COMPRESS;
                    record->idx = (uint16_t) compress_idx;
                    record->to = (uint16_t) compress_to;
                    record->from = (uint16_t) (compress_to - compress_len);

                    list_len += sizeof(PXSRecordBig);
                } else {
                    auto record = (PXSRecordSmall *) adrOf(list[list_len]);
                    record->head = PXS_UNIQUE;
                    record->len = (uint8_t) compress_len;
                    record->idx = (uint16_t) compress_idx;
                    record->to = (uint16_t) compress_to;

                    list_len += sizeof(PXSRecordSmall);
                }
            }
            compress_len = 0;
        }
    };

    auto set_record = [&]() {
        compress_idx = chunk_idx;
        compress_to = pxs_idx + 1;
        compress_len++;
    };

    ret = NULL;
    switch (chunk_idx) {
        case PXS_STREAM_ON:
            list_len = 0;
            list_capacity = 2;
            list = (uint8_t *) malloc((size_t) list_capacity);

            compress_len = 0;
            break;

        case PXS_STREAM_PASS:
            try_explode();
            List_append(uint8_t, list, msg_char);
            break;

        case PXS_STREAM_OFF:
            try_explode();
            ret = (PiXiuStr *) malloc(sizeof(PiXiuStr) + list_len);
            ret->len = (uint16_t) list_len;
            assert(list_len <= UINT16_MAX);
            memcpy(ret->data, list, (size_t) list_len);
            free(list);
            break;

        default:
            if (compress_len == 0) {
                set_record();
            } else {
                compress_to++;
                compress_len++;
            }
            List_append(uint8_t, list, msg_char);
            break;
    }

    return ret;
};

PiXiuStr * PiXiuStr::concat(PiXiuStr * another) {
    auto ret = (PiXiuStr *) malloc(sizeof(PiXiuStr) + this->len + another->len);
    ret->len = this->len + another->len;
    assert(ret->len <= UINT16_MAX);
    memcpy(ret->data, this->data, this->len);
    memcpy(adrOf(ret->data[this->len]), another->data, another->len);
    return ret;
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
    assert(adr != NULL);
    return adr_de_spec(adr);
}

void PiXiuStr_free(PiXiuStr * pxs) {
    free(pxs);
};

static PiXiuStr * escape_unique(uint8_t src[], int src_len, bool is_key) {
    List_init(int, occur_list);

    for (int i = 0; i < src_len; ++i) {
        if (src[i] == PXS_UNIQUE) {
            List_append(int, occur_list, i);
        }
    }

    int len = src_len + occur_list_len + (is_key ? 2 : 0);
    assert(len <= UINT16_MAX);
    auto pxs = (PiXiuStr *) malloc(sizeof(PiXiuStr) + len);
    pxs->len = (uint16_t) len;

    int src_cursor, ret_cursor;
    src_cursor = ret_cursor = 0;
    for (int i = 0; i < occur_list_len; ++i) {
        int cpy_len = occur_list[i] - src_cursor;
        if (cpy_len > 0) {
            memcpy(adrOf(pxs->data[ret_cursor]), adrOf(src[src_cursor]), (size_t) cpy_len);
            src_cursor += cpy_len;
            ret_cursor += cpy_len;
        }
        pxs->data[ret_cursor] = PXS_UNIQUE;
        ret_cursor++;
        pxs->data[ret_cursor] = PXS_UNIQUE;
        ret_cursor++;
        src_cursor++;
    }

    int cpy_len = src_len - src_cursor;
    if (cpy_len > 0) {
        memcpy(adrOf(pxs->data[ret_cursor]), adrOf(src[src_cursor]), (size_t) cpy_len);
        ret_cursor += cpy_len;
    }
    if (is_key) {
        pxs->data[ret_cursor] = PXS_UNIQUE;
        ret_cursor++;
        pxs->data[ret_cursor] = PXS_KEY;
    }

    List_free(occur_list);
    return pxs;
}