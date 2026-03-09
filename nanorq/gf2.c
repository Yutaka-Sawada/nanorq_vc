#include "gf2.h"
#include "oblas.h"
#include <stdlib.h>
#include <string.h>

gf2mat *gf2mat_new(int rows, int cols) {
  gf2mat *gf2 = calloc(1, sizeof(gf2mat));
  gf2->rows = rows;
  gf2->cols = cols;
  gf2->stride = ((cols / gf2wsz) + ((cols % gf2wsz) ? 1 : 0));
  gf2->bits = oblas_alloc(rows, gf2->stride * sizeof(gf2word), sizeof(void *));
  memset(gf2->bits, 0, rows * gf2->stride * sizeof(gf2word));
  return gf2;
}

void gf2mat_free(gf2mat *gf2) {
  if (gf2 && gf2->bits)
    oblas_free(gf2->bits);
  if (gf2)
    free(gf2);
}

void gf2mat_print(gf2mat *gf2, FILE *stream) {
  fprintf(stream, "gf2 [%ux%u]\n", (unsigned)gf2->rows, (unsigned)gf2->cols);
  fprintf(stream, "|     ");
  for (int j = 0; j < gf2->cols; j++) {
    fprintf(stream, "| %03d ", j);
  }
  fprintf(stream, "|\n");
  for (int i = 0; i < gf2->rows; i++) {
    fprintf(stream, "| %03d | %3d ", i, gf2mat_get(gf2, i, 0));
    for (int j = 1; j < gf2->cols; j++) {
      fprintf(stream, "| %3d ", gf2mat_get(gf2, i, j));
    }
    fprintf(stream, "|\n");
  }
}

int gf2mat_get(gf2mat *gf2, int i, int j) {
  if (i >= gf2->rows || j >= gf2->cols)
    return 0;
  gf2word *a = gf2->bits + i * gf2->stride;
  div_t p = div(j, gf2wsz);
  gf2word mask = 1 << p.rem;
  return !!(a[p.quot] & mask);
}

void gf2mat_set(gf2mat *gf2, int i, int j, uint8_t b) {
  if (i >= gf2->rows || j >= gf2->cols)
    return;
  gf2word *a = gf2->bits + i * gf2->stride;
  div_t p = div(j, gf2wsz);
  gf2word mask = 1 << p.rem;
  a[p.quot] = (b) ? (a[p.quot] | mask) : (a[p.quot] & ~mask);
}

void gf2mat_xor(gf2mat *a, gf2mat *b, int i, int j) {
  gf2word *ap = a->bits + i * a->stride;
  gf2word *bp = b->bits + j * b->stride;
  int stride = a->stride;
  for (int idx = 0; idx < stride; idx++) {
    ap[idx] ^= bp[idx];
  }
}

void gf2mat_fill(gf2mat *gf2, int i, uint8_t *dst) {
  gf2word *a = gf2->bits + i * gf2->stride;
  int stride = gf2->stride;
  for (int idx = 0; idx < stride; idx++) {
    gf2word tmp = a[idx];
    while (tmp > 0) {
      int tz = __builtin_ctz(tmp);
      tmp = tmp & (tmp - 1);
      dst[tz + idx * gf2wsz] = 1;
    }
  }
}
