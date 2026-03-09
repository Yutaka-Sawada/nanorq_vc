#ifndef OCTMAT_H
#define OCTMAT_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef OBLAS_AVX
#define OCTMAT_ALIGN 32
#endif

#ifndef OCTMAT_ALIGN
#define OCTMAT_ALIGN 16
#endif

typedef struct {
  uint8_t *data;
  unsigned rows;
  unsigned cols;
  unsigned cols_al;
  struct cpu_func *cf;
} octmat;

#define OM_INITIAL                                                             \
  { .rows = 0, .cols = 0, .cols_al = 0, .data = 0 }

#define om_R(v, x) ((v).data + ((x) * (v).cols_al))
#define om_P(v) om_R(v, 0)
#define om_A(v, x, y) (om_R(v, x)[(y)])

void om_resize(octmat *v, unsigned rows, unsigned cols);
void om_destroy(octmat *v);
void om_print(octmat m, FILE *stream);

#endif
