#pragma once

#include <exception>

template<typename T>
class Optional {
private:
    bool m_empty;
	T m_value;
public:
	Optional() : m_value(), m_empty(true) {};
	Optional(T value) : m_value(value), m_empty(false) {};
	Optional<T>& operator=(T value)
    {
     	m_value = value;
    	m_empty = false;
    	return *this;
    }
	explicit operator bool() const { return !m_empty; }
	operator T() const { 
    if (m_empty) {
      throw std::invalid_argument("Optional value is empty!");
    }
    return m_value;
  }
  static Optional<T> None() {
    return Optional<T>();
  }
};
