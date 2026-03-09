#include "bitmask.h"
#include <stdio.h>

// replace gcc's __builtin_popcount for MSVC
#ifdef _MSC_VER
#include <intrin.h>
#define __builtin_popcountll __popcnt
#endif

#define IDXBITS 32

bitmask bitmask_new(int initial_size) {
  bitmask bm = {.m = 0, .n = 0, .a = 0};
  int max_idx = (initial_size / IDXBITS) + 1;
  while (max_idx >= kv_size(bm))
    kv_push(uint32_t, bm, 0);

  return bm;
}

void bitmask_set(bitmask *bm, int id) {
  int idx = id / IDXBITS;
  while (idx >= kv_size(*bm))
    kv_push(uint32_t, *bm, 0);

  uint32_t add_mask = 1U << (id % IDXBITS);
  kv_A(*bm, idx) |= add_mask;
}

bool bitmask_check(bitmask *bm, int id) {
  int idx = id / IDXBITS;
  if (idx >= kv_size(*bm))
    return false;

  uint32_t check_mask = 1U << (id % IDXBITS);
  return (kv_A(*bm, idx) & check_mask) != 0;
}

int bitmask_gaps(bitmask *bm, int until) {
  int gaps = 0, idx;
  int until_idx = (until / IDXBITS);

  until_idx = (until_idx > kv_size(*bm)) ? kv_size(*bm) : until_idx;
  for (idx = 0; idx < until_idx; idx++) {
    uint32_t target = kv_A(*bm, idx);
    gaps += __builtin_popcountll(~target);
  }
  if (until % IDXBITS) {
    uint32_t until_mask = (1 << (until % IDXBITS)) - 1;
    uint32_t target = kv_A(*bm, idx) | ~until_mask;
    gaps += __builtin_popcountll(~target);
  }

  return gaps;
}

void bitmask_reset(bitmask *bm) {
  for (int idx = 0; idx < kv_size(*bm); idx++)
    kv_A(*bm, idx) = 0;
}

void bitmask_free(bitmask *bm) {
  if (bm)
    kv_destroy(*bm);
}

void bitmask_print(bitmask *bm) {
  void *memory = (void *)bm->a;
  int extent = (bm->n) * sizeof(uint32_t);
  FILE *fp = stdout;
  char c = '|', e = '\n';
  char DIGITS_BIN[] = "01";

  char *offset = (char *)(memory);
  while (extent--) {
    unsigned bits = 0;
    while (bits < 8) {
      putc(DIGITS_BIN[(*offset >> bits++) & 1], fp);
    }
    if ((extent) && (c)) {
      putc(c, fp);
    }
    offset++;
  }
  if (e) {
    putc(e, fp);
  }
  return;
}
