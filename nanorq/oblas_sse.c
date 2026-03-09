#include "oblas.h"

#ifdef OBLAS_SSE

#include <emmintrin.h> /* sse2 */
#include <tmmintrin.h> /* sse3 */

void oswaprow_sse(uint8_t *restrict a, size_t i, size_t j, size_t k) {
  if (i == j)
    return;
  octet *ap = a + (i * ALIGNED_COLS(k));
  octet *bp = a + (j * ALIGNED_COLS(k));

  __m128i *ap128 = (__m128i *)ap;
  __m128i *bp128 = (__m128i *)bp;
  for (size_t idx = 0; idx < ALIGNED_COLS(k); idx += OCTMAT_ALIGN) {
    __m128i atmp = _mm_loadu_si128((__m128i *)(ap128));
    __m128i btmp = _mm_loadu_si128((__m128i *)(bp128));
    _mm_storeu_si128(ap128++, btmp);
    _mm_storeu_si128(bp128++, atmp);
  }
}

void oaxpy_sse(uint8_t *restrict a, uint8_t *restrict b, size_t i, size_t j, size_t k, uint8_t u) {
  octet *ap = a + (i * ALIGNED_COLS(k));
  octet *bp = b + (j * ALIGNED_COLS(k));

  if (u == 0)
    return;

  if (u == 1){
    oaddrow_sse(a, b, i, j, k);
    return;
  }

  const __m128i mask = _mm_set1_epi8(0x0f);
  const __m128i urow_hi = _mm_loadu_si128((__m128i *)OCT_MUL_HI[u]);
  const __m128i urow_lo = _mm_loadu_si128((__m128i *)OCT_MUL_LO[u]);

  __m128i *ap128 = (__m128i *)ap;
  __m128i *bp128 = (__m128i *)bp;
  for (size_t idx = 0; idx < ALIGNED_COLS(k); idx += OCTMAT_ALIGN) {
    __m128i bx = _mm_loadu_si128(bp128++);
    __m128i lo = _mm_and_si128(bx, mask);
    bx = _mm_srli_epi64(bx, 4);
    __m128i hi = _mm_and_si128(bx, mask);
    lo = _mm_shuffle_epi8(urow_lo, lo);
    hi = _mm_shuffle_epi8(urow_hi, hi);

    _mm_storeu_si128(
        ap128, _mm_xor_si128(_mm_loadu_si128(ap128), _mm_xor_si128(lo, hi)));
    ap128++;
  }
}

void oaddrow_sse(uint8_t *restrict a, uint8_t *restrict b, size_t i, size_t j, size_t k) {
  octet *ap = a + (i * ALIGNED_COLS(k));
  octet *bp = b + (j * ALIGNED_COLS(k));

  __m128i *ap128 = (__m128i *)ap;
  __m128i *bp128 = (__m128i *)bp;
  for (size_t idx = 0; idx < ALIGNED_COLS(k); idx += OCTMAT_ALIGN) {
    _mm_storeu_si128(
        ap128, _mm_xor_si128(_mm_loadu_si128(ap128), _mm_loadu_si128(bp128)));
    ap128++;
    bp128++;
  }
}

void oscal_sse(uint8_t *restrict a, size_t i, size_t k, uint8_t u) {
  octet *ap = a + (i * ALIGNED_COLS(k));

  if (u < 2)
    return;

  const __m128i mask = _mm_set1_epi8(0x0f);
  const __m128i urow_hi = _mm_loadu_si128((__m128i *)OCT_MUL_HI[u]);
  const __m128i urow_lo = _mm_loadu_si128((__m128i *)OCT_MUL_LO[u]);

  __m128i *ap128 = (__m128i *)ap;
  for (size_t idx = 0; idx < ALIGNED_COLS(k); idx += OCTMAT_ALIGN) {
    __m128i ax = _mm_loadu_si128(ap128);
    __m128i lo = _mm_and_si128(ax, mask);
    ax = _mm_srli_epi64(ax, 4);
    __m128i hi = _mm_and_si128(ax, mask);
    lo = _mm_shuffle_epi8(urow_lo, lo);
    hi = _mm_shuffle_epi8(urow_hi, hi);

    _mm_storeu_si128(ap128++, _mm_xor_si128(lo, hi));
  }
}

void oaxpy_b32_sse(uint8_t *a, uint32_t *b, size_t i, size_t k, uint8_t u) {
  __m128i *ap128 = (__m128i *)(a + i * ALIGNED_COLS(k));
  __m128i scatter_hi =
      _mm_set_epi32(0x03030303, 0x03030303, 0x02020202, 0x02020202);
  __m128i scatter_lo =
      _mm_set_epi32(0x01010101, 0x01010101, 0x00000000, 0x00000000);
  __m128i cmpmask =
      _mm_set_epi32(0x80402010, 0x08040201, 0x80402010, 0x08040201);
  __m128i u128 = _mm_set1_epi8(u);

  for (size_t idx = 0, p = 0; idx < k;
       idx += 8 * sizeof(uint32_t), p++, ap128++) {
    __m128i bcast = _mm_set1_epi32(b[p]);
    __m128i bytes_hi = _mm_shuffle_epi8(bcast, scatter_hi);
    __m128i bytes_lo = _mm_shuffle_epi8(bcast, scatter_lo);

    bytes_hi = _mm_andnot_si128(bytes_hi, cmpmask);
    bytes_lo = _mm_andnot_si128(bytes_lo, cmpmask);
    bytes_hi =
        _mm_and_si128(_mm_cmpeq_epi8(bytes_hi, _mm_setzero_si128()), u128);
    bytes_lo =
        _mm_and_si128(_mm_cmpeq_epi8(bytes_lo, _mm_setzero_si128()), u128);
    _mm_storeu_si128(ap128, _mm_xor_si128(_mm_loadu_si128(ap128), bytes_lo));
    ap128++;
    _mm_storeu_si128(ap128, _mm_xor_si128(_mm_loadu_si128(ap128), bytes_hi));
  }
}

#endif
