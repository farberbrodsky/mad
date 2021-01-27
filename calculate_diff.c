#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>

typedef u_int8_t u8;
typedef u_int16_t u16;
typedef u_int32_t u32;
typedef unsigned long ul;

const u32 HASH_LEN = 6;
const u32 COPY_LEN = 5;
const u32 MAX_COPY_LEN = 127;
const u32 MAX_ADD_LEN = 127;
const u16 b = 256;
const u16 r = 40487;
const u16 b_power = (b * (b * (b * (b * (b % r) % r) % r) % r) % r); // b^HASH_LEN-1 % r

void calculate_diff(char *source, char *target, FILE *output) {
    ul source_len = strlen(source);
    ul target_len = strlen(target);
    #define ST(i) (i < source_len ? (source[i]) : (target[i - target_len]))
    ul combined_len = source_len + target_len;

    if (combined_len < HASH_LEN) {
        // the result is just an ADD of all the string
        u8 add_op = target_len;
        fwrite(&add_op, sizeof(u8), 1, output);
        fwrite(target, sizeof(char), target_len, output);
        return;
    }

    u16 *hashes = malloc((combined_len - HASH_LEN + 1) * sizeof(u16));
    // calculate initial hash
    u16 hash = 0;
    for (u32 i = 0; i < HASH_LEN; ++i) {
        hash = (hash * b + source[i]) % r;
    }
    hashes[0] = hash;
    bool add_mode = false; // to only put the ADD when you stop adding
    u32 add_mode_since = 0;
    for (u32 i = 1; i < combined_len - HASH_LEN; ++i) {
        hash = ((hash - ST(i - 1) * b_power) * b) + ST(i + HASH_LEN);
        hashes[i] = hash;
        // search for longest string with the same hash
        u32 longest_match = 0;
        u32 longest_match_length = 0;
        for (u32 j = 0; j < i; ++j) {
            if (hashes[j] == hashes[i]) {
                // check match length
                u32 k = j;
                for (; (k - j) < MAX_COPY_LEN && k < combined_len; ++k) {
                    if (ST(k) != ST(j)) {
                        break;
                    }
                }
                u32 match_length = k - j;
                if (match_length > longest_match_length) {
                    longest_match = j;
                    longest_match_length = match_length;
                }
            }
        }
        if (longest_match_length > COPY_LEN) {
            if (add_mode) {
                // add everything before this, in 127 byte chunks
                for (u32 from = add_mode_since; from < i; from += 128) {
                    u8 add_len = i - from;
                    fwrite(&add_len, sizeof(u8), 1, output);
                    if (from < source_len) {
                        fwrite(source + from, sizeof(char), source_len - from, output);
                    }
                    if (i >= source_len) {
                        fwrite(target + (from - source_len), sizeof(char), i - (from - source_len), output);
                    }
                }
                add_mode = false;
            }
            // COPY is better than ADD
            u8 copy_op = (u8)longest_match_length | 0b10000000;
            fwrite(&copy_op, sizeof(u8), 1, output);
            fwrite(&longest_match, sizeof(u16), 1, output);
            i = longest_match;
        } else {
            // start an ADD
            if (!add_mode) {
                add_mode = true;
                add_mode_since = i;
            }
        }
    }
    // free all the stuff
    free(hashes);
}
