//===============================================================================
// @ LsTuple.h
// 
// Tuple typedefs
//
//
//
//-------------------------------------------------------------------------------
//-- Notes ----------------------------------------------------------------------
//-------------------------------------------------------------------------------
//
// explicit LsTuple(T... ts) : components{ts...} { }
//
// This is explicit because otherwise this code is ambiguous:
//
// 1. NodeEdgeIterator iter = GetNodeEdgeIterator(node);
// 2. LsBCCEdge e = iter; <- Do we call LsTuple constructor
//                           and make a tuple of iterators
//                           or NodeEdgeIterator user conversion operator?
//
// By marking it "explicit" we disallow using this constructor in implicit 
// conversions. Since we are rarely interested in single tuples this is not
// a problem.
//===============================================================================

#pragma once

#include <algorithm>

template <class T, int C>
class LsTuple
{
public:
  LsTuple(std::initializer_list<T> l) {
    std::copy(l.begin(), l.end(), &components[0]);
  }
  template <typename... T> 
  explicit LsTuple(T... ts) : components{ts...} { }
  inline T& operator[]( unsigned int i ) {
    return components[i];
  }
  inline const T& operator[]( unsigned int i ) const {
    return components[i];
  }
private:
  T components[C];
};
