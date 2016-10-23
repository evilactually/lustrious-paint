//===============================================================================
// @ LsOptional.h
// 
// Variant type for C++.
//
//===============================================================================

#pragma once

#include <exception>

template<typename T>
class LsOptional {
private:
    bool m_empty;
  T m_value;
public:
  LsOptional() : m_value(), m_empty(true) {};
  LsOptional(T value) : m_value(value), m_empty(false) {};
  LsOptional<T>& operator=(T value)
  {
    m_value = value;
    m_empty = false;
    return *this;
  }
  // Automatic conversion to boolean to make it easy to use in "if" branches
  explicit operator bool() const { return !m_empty; }
  // Automatic conversion to value type for easy access to stored values, throws exceptions.
  operator T() const { 
    if (m_empty) {
      throw std::invalid_argument("Optional value is empty!");
    }
    return m_value;
  }
  static LsOptional<T> None() {
    return LsOptional<T>();
  }
};