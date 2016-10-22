//===============================================================================
// @ LsTuple.h
// 
// Tuple typedefs
//
//===============================================================================

#pragma once

template <class T, int C>
class LsTuple
{
public:
  template <typename... T> 
  LsTuple(T... ts) : components{ts...} { }
  inline T& operator[]( unsigned int i ) {
    return components[i];
  }
private:
  T components[C];
};
