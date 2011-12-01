// ======================================================================== //
// Copyright 2009-2011 Intel Corporation                                    //
//                                                                          //
// Licensed under the Apache License, Version 2.0 (the "License");          //
// you may not use this file except in compliance with the License.         //
// You may obtain a copy of the License at                                  //
//                                                                          //
//     http://www.apache.org/licenses/LICENSE-2.0                           //
//                                                                          //
// Unless required by applicable law or agreed to in writing, software      //
// distributed under the License is distributed on an "AS IS" BASIS,        //
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. //
// See the License for the specific language governing permissions and      //
// limitations under the License.                                           //
// ======================================================================== //

#ifndef __PF_INTRINSICS_H__
#define __PF_INTRINSICS_H__

#include "sys/platform.hpp"
#include <xmmintrin.h>
#include <emmintrin.h>

#if defined(__MSVC__)

#include <intrin.h>

#define PF_COMPILER_WRITE_BARRIER       _WriteBarrier()
#define PF_COMPILER_READ_WRITE_BARRIER  _ReadWriteBarrier()

#if _MSC_VER >= 1400
#pragma intrinsic(_ReadBarrier)
#define PF_COMPILER_READ_BARRIER        _ReadBarrier()
#else
#define PF_COMPILER_READ_BARRIER        _ReadWriteBarrier()
#endif /* _MSC_VER >= 1400 */

INLINE int __bsf(int v) {
  unsigned long r = 0; _BitScanForward(&r,v); return r;
}

INLINE int __bsr(int v) {
  unsigned long r = 0; _BitScanReverse(&r,v); return r;
}

INLINE int __btc(int v, int i) {
  long r = v; _bittestandcomplement(&r,i); return r;
}

INLINE int __bts(int v, int i) {
  long r = v; _bittestandset(&r,i); return r;
}

INLINE int __btr(int v, int i) {
  long r = v; _bittestandreset(&r,i); return r;
}

INLINE void memoryFence(void) { _mm_mfence(); }

#if defined(__X86_64__) && !defined(__INTEL_COMPILER)

INLINE size_t __bsf(size_t v) {
  unsigned long r = 0; _BitScanForward64(&r,v); return r;
}

INLINE size_t __bsr(size_t v) {
  unsigned long r = 0; _BitScanReverse64(&r,v); return r;
}

INLINE size_t __btc(size_t v, size_t i) {
  __int64 r = v; _bittestandcomplement64(&r,i); return r;
}

INLINE size_t __bts(size_t v, size_t i) {
  __int64 r = v; _bittestandset64(&r,i); return r;
}

INLINE size_t __btr(size_t v, size_t i) {
  __int64 r = v; _bittestandreset64(&r,i); return r;
}

#endif /* defined(__X86_64__) && !defined(__INTEL_COMPILER) */

typedef int32 atomic32_t;

INLINE int32 atomic_add(volatile int32* m, const int32 v) {
  return _InterlockedExchangeAdd((volatile long*)m,v);
}

INLINE int32 atomic_cmpxchg(volatile int32* m, const int32 v, const int32 c) {
  return _InterlockedCompareExchange((volatile long*)m,v,c);
}

#if defined(__X86_64__)

typedef int64 atomic_t;

INLINE int64 atomic_add(volatile int64* m, const int64 v) {
  return _InterlockedExchangeAdd64(m,v);
}

INLINE int64 atomic_cmpxchg(volatile int64* m, const int64 v, const int64 c) {
  return _InterlockedCompareExchange64(m,v,c);
}

#else

typedef int32 atomic_t;

#endif /* defined(__X86_64__) */

#else

INLINE unsigned int __popcnt(unsigned int in) {
  int r = 0; asm ("popcnt %1,%0" : "=r"(r) : "r"(in)); return r;
}

INLINE int __bsf(int v) {
  int r = 0; asm ("bsf %1,%0" : "=r"(r) : "r"(v)); return r;
}

INLINE int __bsr(int v) {
  int r = 0; asm ("bsr %1,%0" : "=r"(r) : "r"(v)); return r;
}

INLINE int __btc(int v, int i) {
  int r = 0; asm ("btc %1,%0" : "=r"(r) : "r"(i), "0"(v) : "flags"); return r;
}

INLINE int __bts(int v, int i) {
  int r = 0; asm ("bts %1,%0" : "=r"(r) : "r"(i), "0"(v) : "flags"); return r;
}

INLINE int __btr(int v, int i) {
  int r = 0; asm ("btr %1,%0" : "=r"(r) : "r"(i), "0"(v) : "flags"); return r;
}

INLINE size_t __bsf(size_t v) {
  size_t r = 0; asm ("bsf %1,%0" : "=r"(r) : "r"(v)); return r;
}

INLINE size_t __bsr(size_t v) {
  size_t r = 0; asm ("bsr %1,%0" : "=r"(r) : "r"(v)); return r;
}

INLINE size_t __btc(size_t v, size_t i) {
  size_t r = 0; asm ("btc %1,%0" : "=r"(r) : "r"(i), "0"(v) : "flags"); return r;
}

INLINE size_t __bts(size_t v, size_t i) {
  size_t r = 0; asm ("bts %1,%0" : "=r"(r) : "r"(i), "0"(v) : "flags"); return r;
}

INLINE size_t __btr(size_t v, size_t i) {
  size_t r = 0; asm ("btr %1,%0" : "=r"(r) : "r"(i), "0"(v) : "flags"); return r;
}

INLINE void memoryFence(void) { _mm_mfence(); }

typedef int32 atomic32_t;

INLINE int32 atomic_add(int32 volatile* value, int32 input)
{  asm volatile("lock xadd %0,%1" : "+r" (input), "+m" (*value) : "r" (input), "m" (*value)); return input; }

INLINE int32 atomic_cmpxchg(int32 volatile* value, const int32 input, int32 comparand)
{  asm volatile("lock cmpxchg %2,%0" : "=m" (*value), "=a" (comparand) : "r" (input), "m" (*value), "a" (comparand) : "flags"); return comparand; }

#if defined(__X86_64__)

  typedef int64 atomic_t;

  INLINE int64 atomic_add(int64 volatile* value, int64 input)
  {  asm volatile("lock xaddq %0,%1" : "+r" (input), "+m" (*value) : "r" (input), "m" (*value));  return input;  }

  INLINE int64 atomic_cmpxchg(int64 volatile* value, const int64 input, int64 comparand)
  {  asm volatile("lock cmpxchgq %2,%0" : "+m" (*value), "+a" (comparand) : "r" (input), "m" (*value), "r" (comparand) : "flags"); return comparand;  }

#else

  typedef int32 atomic_t;

#endif /* defined(__X86_64__) */

#define PF_COMPILER_READ_WRITE_BARRIER    asm volatile("" ::: "memory");
#define PF_COMPILER_WRITE_BARRIER         PF_COMPILER_READ_WRITE_BARRIER
#define PF_COMPILER_READ_BARRIER          PF_COMPILER_READ_WRITE_BARRIER

#endif /* __MSVC__ */

template <typename T>
INLINE T __load_acquire(volatile T *ptr)
{
  PF_COMPILER_READ_WRITE_BARRIER;
  T x = *ptr; // for x86, load == load_acquire
  PF_COMPILER_READ_WRITE_BARRIER;
  return x;
}

template <typename T>
INLINE void __store_release(volatile T *ptr, T x)
{
  PF_COMPILER_READ_WRITE_BARRIER;
  *ptr = x; // for x86, store == store_release
  PF_COMPILER_READ_WRITE_BARRIER;
}
#endif /* __PF_INTRINSICS_H__ */

