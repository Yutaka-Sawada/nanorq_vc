#include "oblas.h"

void oswaprow(uint8_t *restrict a, size_t i, size_t j, size_t k) {
  if (i == j)
    return;
  octet *ap = a + (i * ALIGNED_COLS(k));
  octet *bp = a + (j * ALIGNED_COLS(k));

  for (size_t idx = 0; idx < k; idx++) {
    OCTET_SWAP(ap[idx], bp[idx]);
  }
}

void oaxpy(uint8_t *restrict a, uint8_t *restrict b, size_t i, size_t j,
           size_t k, uint8_t u) {
  octet *ap = a + (i * ALIGNED_COLS(k));
  octet *bp = b + (j * ALIGNED_COLS(k));

  if (u == 0)
    return;

  if (u == 1) {
    oaddrow(a, b, i, j, k);
    return;
  }

  const octet *urow_hi = OCT_MUL_HI[u];
  const octet *urow_lo = OCT_MUL_LO[u];
  for (size_t idx = 0; idx < k; idx++) {
    octet b_lo = bp[idx] & 0x0f;
    octet b_hi = (bp[idx] & 0xf0) >> 4;
    ap[idx] ^= urow_hi[b_hi] ^ urow_lo[b_lo];
  }
}

void oaddrow(uint8_t *restrict a, uint8_t *restrict b, size_t i, size_t j,
             size_t k) {
  octet *ap = a + (i * ALIGNED_COLS(k));
  octet *bp = b + (j * ALIGNED_COLS(k));

  for (size_t idx = 0; idx < k; idx++) {
    ap[idx] ^= bp[idx];
  }
}

void oscal(uint8_t *restrict a, size_t i, size_t k, uint8_t u) {
  octet *ap = a + (i * ALIGNED_COLS(k));

  if (u < 2)
    return;

  const octet *urow_hi = OCT_MUL_HI[u];
  const octet *urow_lo = OCT_MUL_LO[u];
  for (size_t idx = 0; idx < k; idx++) {
    octet a_lo = ap[idx] & 0x0f;
    octet a_hi = (ap[idx] & 0xf0) >> 4;
    ap[idx] = urow_hi[a_hi] ^ urow_lo[a_lo];
  }
}

void oaxpy_b32(uint8_t *a, uint32_t *b, size_t i, size_t k, uint8_t u) {
  octet *ap = a + (i * ALIGNED_COLS(k));
  for (size_t idx = 0, p = 0; idx < k; idx += 8 * sizeof(uint32_t), p++) {
    uint32_t tmp = b[p];
    while (tmp > 0) {
      int tz = __builtin_ctz(tmp);
      tmp = tmp & (tmp - 1);
      ap[tz + idx] ^= u;
    }
  }
}
