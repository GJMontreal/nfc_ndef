#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "st25dv.h"
#include "ndef.h"
#include "ndef_tlv.h"
#include "ndef_text.h"

int main(int argc, char *argv[])
{
    if (argc < 2) {
        fprintf(stderr, "usage: dump_parse <file.bin>\n");
        return 1;
    }

    FILE *f = fopen(argv[1], "rb");
    if (!f) { perror("fopen"); return 1; }

    uint8_t buf[8192];
    size_t  n = fread(buf, 1, sizeof(buf), f);
    fclose(f);

    printf("File: %s (%zu bytes)\n\n", argv[1], n);

    /* Validate CC */
    if (n < 4 || buf[0] != ST25DV_CC_MAGIC) {
        fprintf(stderr, "No valid Capability Container (magic=0x%02X)\n", buf[0]);
        return 1;
    }
    uint16_t mem_size = ST25DV_CC_MLEN_TO_BYTES(buf[2]);
    printf("CC: magic=0x%02X  ver=0x%02X  MLEN=0x%02X (%u bytes)  access=0x%02X\n\n",
           buf[0], buf[1], buf[2], mem_size, buf[3]);

    /* Unwrap TLV starting after the CC */
    const uint8_t *ndef_region    = buf + ST25DV_NDEF_START_ADDR;
    uint16_t       ndef_region_len = (uint16_t)(n - ST25DV_NDEF_START_ADDR);

    const uint8_t *ndef_msg = NULL;
    uint16_t       ndef_len = 0;

    int rc = ndef_tlv_unwrap(ndef_region, ndef_region_len, &ndef_msg, &ndef_len);
    if (rc != NDEF_OK) {
        fprintf(stderr, "TLV unwrap failed (%d)\n", rc);
        return 1;
    }
    printf("NDEF message: %u bytes\n\n", ndef_len);

    /* Parse records */
    ndef_record_t records[16];
    int nrec = ndef_parse(ndef_msg, ndef_len, records, 16);
    if (nrec < 0) {
        fprintf(stderr, "NDEF parse failed (%d)\n", nrec);
        return 1;
    }
    printf("%d record(s) found:\n", nrec);

    for (int i = 0; i < nrec; i++) {
        const ndef_record_t *r = &records[i];
        char type_str[32] = "<none>";
        if (r->type_len > 0 && r->type_len < sizeof(type_str) - 1) {
            memcpy(type_str, r->type, r->type_len);
            type_str[r->type_len] = '\0';
        }

        printf("\n  [%d] TNF=0x%X  type=\"%s\" (%u byte)  payload=%u bytes\n",
               i, r->tnf, type_str, r->type_len, r->payload_len);

        /* Attempt text record decode */
        if (r->tnf == NDEF_TNF_WELL_KNOWN && r->type_len == 1 &&
            r->type[0] == NDEF_TYPE_TEXT) {
            char lang[8], text[1024];
            if (ndef_text_decode(r->payload, r->payload_len,
                                 lang, sizeof(lang),
                                 text, sizeof(text)) == NDEF_OK) {
                printf("       lang=\"%s\"  text=%s\n", lang, text);
            }
        } else {
            /* Print raw payload as hex */
            printf("       payload: ");
            uint32_t show = r->payload_len < 64 ? r->payload_len : 64;
            for (uint32_t j = 0; j < show; j++)
                printf("%02X ", r->payload[j]);
            if (r->payload_len > 64) printf("...");
            printf("\n");
        }
    }

    printf("\n");
    return 0;
}
