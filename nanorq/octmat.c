#include "oblas.h"
#include "octmat.h"

void om_resize(octmat *v, unsigned rows, unsigned cols) {
  void *aligned = NULL;

  v->rows = rows;
  v->cols = cols;
  v->cols_al = ALIGNED_COLS(cols);
  aligned = (uint8_t *)oblas_alloc(v->rows, v->cols_al, OCTMAT_ALIGN);
  memset(aligned, 0, v->cols_al * rows);
  v->data = aligned;
}

void om_destroy(octmat *v) {
  v->rows = 0;
  v->cols = 0;
  v->cols_al = 0;
  oblas_free(v->data);
  v->data = NULL;
}

void om_print(octmat m, FILE *stream) {
  fprintf(stream, "dense [%ux%u]\n", (unsigned)m.rows, (unsigned)m.cols);
  fprintf(stream, "|     ");
  for (unsigned j = 0; j < m.cols; j++) {
    fprintf(stream, "| %03d ", j);
  }
  fprintf(stream, "|\n");
  for (unsigned i = 0; i < m.rows; i++) {
    fprintf(stream, "| %03d | %3d ", i, om_A(m, i, 0));
    for (unsigned j = 1; j < m.cols; j++) {
      fprintf(stream, "| %3d ", om_A(m, i, j));
    }
    fprintf(stream, "|\n");
  }
}
