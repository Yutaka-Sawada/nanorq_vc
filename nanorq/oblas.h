#ifndef OCTET_BLAS_H
#define OCTET_BLAS_H

#include <stdint.h>

/*
If you want to use faster functions, define one or two below.
Because I cannot test, OBLAS_AVX512 and OBLAS_NEON are removed.
#define OBLAS_SSE
#define OBLAS_AVX
*/
#define OBLAS_SSE
#define OBLAS_AVX

#include "octmat.h"
#include "octtables.h"

#define OCTET_MUL(u, v) OCT_EXP[OCT_LOG[u] + OCT_LOG[v]]
#define OCTET_DIV(u, v) OCT_EXP[OCT_LOG[u] - OCT_LOG[v] + 255]
#define OCTET_SWAP(u, v)                                                       \
  do {                                                                         \
    uint8_t __tmp = (u);                                                       \
    (u) = (v);                                                                 \
    (v) = __tmp;                                                               \
  } while (0)

#define ALIGNED_COLS(k)                                                        \
  (((k) / OCTMAT_ALIGN) + (((k) % OCTMAT_ALIGN) ? 1 : 0)) * OCTMAT_ALIGN

typedef uint8_t octet;

// replace gcc's __builtin_ctz for MSVC
#ifdef _MSC_VER
#include <immintrin.h>
#define __builtin_ctz _tzcnt_u32
#endif

void *oblas_alloc(size_t nmemb, size_t size, size_t align);

#ifdef _MSC_VER
#define oblas_free(p) _aligned_free(p);
#else
#define oblas_free(p) free(p);
#endif

// functions in oblas_classic.c
void oswaprow(uint8_t *a, size_t i, size_t j, size_t k);
void oaxpy(uint8_t *a, uint8_t *b, size_t i, size_t j, size_t k, uint8_t u);
void oaddrow(uint8_t *a, uint8_t *b, size_t i, size_t j, size_t k);
void oscal(uint8_t *a, size_t i, size_t k, uint8_t u);
void oaxpy_b32(uint8_t *a, uint32_t *b, size_t i, size_t k, uint8_t u);

#ifdef OBLAS_SSE // functions in oblas_sse.c
void oswaprow_sse(uint8_t *a, size_t i, size_t j, size_t k);
void oaxpy_sse(uint8_t *a, uint8_t *b, size_t i, size_t j, size_t k, uint8_t u);
void oaddrow_sse(uint8_t *a, uint8_t *b, size_t i, size_t j, size_t k);
void oscal_sse(uint8_t *a, size_t i, size_t k, uint8_t u);
void oaxpy_b32_sse(uint8_t *a, uint32_t *b, size_t i, size_t k, uint8_t u);
#endif

#ifdef OBLAS_AVX // functions in oblas_avx.c
void oswaprow_avx(uint8_t *a, size_t i, size_t j, size_t k);
void oaxpy_avx(uint8_t *a, uint8_t *b, size_t i, size_t j, size_t k, uint8_t u);
void oaddrow_avx(uint8_t *a, uint8_t *b, size_t i, size_t j, size_t k);
void oscal_avx(uint8_t *a, size_t i, size_t k, uint8_t u);
void oaxpy_b32_avx(uint8_t *a, uint32_t *b, size_t i, size_t k, uint8_t u);
#endif

// CPU specific functions
void check_cpuid(void);

typedef void (* CPU_OSWAPROW) (uint8_t *a, size_t i, size_t j, size_t k);
CPU_OSWAPROW cpu_oswaprow;

typedef void (* CPU_OAXPY) (uint8_t *a, uint8_t *b, size_t i, size_t j, size_t k, uint8_t u);
CPU_OAXPY cpu_oaxpy;

typedef void (* CPU_OSCAL) (uint8_t *a, size_t i, size_t k, uint8_t u);
CPU_OSCAL cpu_oscal;

typedef void (* CPU_OAXPY_B32) (uint8_t *a, uint32_t *b, size_t i, size_t k, uint8_t u);
CPU_OAXPY_B32 cpu_oaxpy_b32;

#endif
