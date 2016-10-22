//===============================================================================
// @ LsBccLattice.h
// 
// Tuple typedefs
//
//===============================================================================

#pragma once

template <class T, int C>
class tuple_t
{
public:
  template <typename... T> 
  tuple_t(T... ts) : components{ts...} { }
  inline T& operator[]( unsigned int i ) {
    return components[i];
  }
private:
  T components[C];
};

// typedef int64_t int64x2_t[2];
// typedef int32_t int32x2_t[2];
// typedef int16_t int16x2_t[2];
// typedef int8_t  int8x2_t[2];

// typedef int64_t int64x3_t[3];
// typedef int32_t int32x3_t[3];
// typedef int16_t int16x3_t[3];
// typedef int8_t  int8x3_t[3];

// typedef int64_t uint64x2_t[2];
// typedef int32_t uint32x2_t[2];
// typedef int16_t uint16x2_t[2];
// typedef int8_t  uint8x2_t[2];

// typedef int64_t uint64x3_t[3];
// typedef int32_t uint32x3_t[3];
// typedef int16_t uint16x3_t[3];
// typedef int8_t  uint8x3_t[3];

// typedef float  floatx2_t[2];
// typedef double doublex2_t[2];

// typedef float  floatx3_t[3];
// typedef double doublex3_t[3];

