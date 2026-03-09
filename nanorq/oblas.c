#include "oblas.h"
#include <errno.h>

// To check CPUID
#ifdef _MSC_VER
#include <intrin.h>
#else // gcc
#include <cpuid.h>
#endif

void *oblas_alloc(size_t nmemb, size_t size, size_t align) {
  void *aligned = NULL;
  size_t aligned_sz = ((size / align) + ((size % align) ? 1 : 0)) * align;

#ifdef _MSC_VER
  aligned = _aligned_malloc(nmemb * aligned_sz, align);
  if (aligned == NULL)
    exit(ENOMEM);
#else
  if (posix_memalign((void *)&aligned, align, nmemb * aligned_sz) != 0)
    exit(ENOMEM);
#endif
  return aligned;
}

void check_cpuid(struct cpu_func *cf) {
#ifdef _MSC_VER
  int cpu_info[4], max_id;

  cf->oswaprow = oswaprow;
  cf->oaxpy = oaxpy;
  cf->oscal = oscal;
  cf->oaxpy_b32 = oaxpy_b32;

  __cpuid(cpu_info, 0);
  max_id = cpu_info[0]; // EAX
  if (max_id >= 1) {
    __cpuid(cpu_info, 1); // CPUID 1
#ifdef OBLAS_SSE
    if (cpu_info[2] & (1 << 9)) { // 1<<9 in ECX = SSSE3
      cf->oswaprow = oswaprow_sse;
      cf->oaxpy = oaxpy_sse;
      cf->oscal = oscal_sse;
      cf->oaxpy_b32 = oaxpy_b32_sse;
    }
#endif
  }
  if (max_id >= 7) {
    __cpuidex(cpu_info, 7, 0); // CPUID 7 0
#ifdef OBLAS_AVX
    if (cpu_info[1] & (1 << 5)) { // 1<<5 in EBX = AVX2
      cf->oswaprow = oswaprow_avx;
      cf->oaxpy = oaxpy_avx;
      cf->oscal = oscal_avx;
      cf->oaxpy_b32 = oaxpy_b32_avx;
    }
#endif
  }

#else // gcc
  unsigned int max_id, eax, ebx, ecx, edx;

  cf->oswaprow = oswaprow;
  cf->oaxpy = oaxpy;
  cf->oscal = oscal;
  cf->oaxpy_b32 = oaxpy_b32;

  __cpuid(0, eax, ebx, ecx, edx);
  max_id = eax; // EAX
  if (max_id >= 1) {
    __cpuid(1, eax, ebx, ecx, edx); // CPUID 1
#ifdef OBLAS_SSE
    if (ecx & (1 << 9)) { // 1<<9 in ECX = SSSE3
      cf->oswaprow = oswaprow_sse;
      cf->oaxpy = oaxpy_sse;
      cf->oscal = oscal_sse;
      cf->oaxpy_b32 = oaxpy_b32_sse;
    }
#endif
  }
  if (max_id >= 7) {
    __cpuid_count(7, 0, eax, ebx, ecx, edx); // CPUID 7 0
#ifdef OBLAS_AVX
    if (ebx & (1 << 5)) { // 1<<5 in EBX = AVX2
      cf->oswaprow = oswaprow_avx;
      cf->oaxpy = oaxpy_avx;
      cf->oscal = oscal_avx;
      cf->oaxpy_b32 = oaxpy_b32_avx;
    }
#endif
  }
#endif
}
