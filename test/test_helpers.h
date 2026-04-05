#ifndef TEST_HELPERS_H
#define TEST_HELPERS_H

#include "ndef.h"

static inline void make_record(ndef_record_t *rec, uint8_t tnf,
                                const uint8_t *type, uint8_t type_len,
                                const uint8_t *payload, uint32_t payload_len)
{
    rec->tnf         = tnf;
    rec->type        = type;
    rec->type_len    = type_len;
    rec->id          = NULL;
    rec->id_len      = 0;
    rec->payload     = payload;
    rec->payload_len = payload_len;
}

#endif /* TEST_HELPERS_H */
