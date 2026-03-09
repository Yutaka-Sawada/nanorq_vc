#ifndef BITMASK_H
#define BITMASK_H

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "kvec.h"

typedef kvec_t(uint32_t) bitmask;

bitmask bitmask_new(int initial);
void bitmask_set(bitmask *bm, int id);
bool bitmask_check(bitmask *bm, int id);
int bitmask_gaps(bitmask *bm, int until);
void bitmask_reset(bitmask *bm);
void bitmask_free(bitmask *bm);
void bitmask_print(bitmask *bm);

#endif
