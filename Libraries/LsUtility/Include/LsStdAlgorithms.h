//===============================================================================
// @ StdAlgorithms.h
// 
// Extensions to standard library algorithms
//
//===============================================================================

//-------------------------------------------------------------------------------
//-- Dependencies ---------------------------------------------------------------
//-------------------------------------------------------------------------------

#include <algorithm>
#include "LsOptional.h"

//-------------------------------------------------------------------------------
//-- Functions ------------------------------------------------------------------
//-------------------------------------------------------------------------------

template< class InputIt, class UnaryPredicate>
LsOptional<int> find_index_if( InputIt first, InputIt last, UnaryPredicate q) {
  auto found_iter = find_if(first, last, q);
  if ( found_iter != last )
  {
    return LsOptional<int>(found_iter - first);
  }
  else 
  {
    return LsOptional<int>::None();
  }
}

template< class InputIt, class T>
LsOptional<int> find_index( InputIt first, InputIt last, const T& value) {
  return find_index_if( first, 
                        last,
                        [&](const LsVector3& some_node) { return some_node == value; });
}