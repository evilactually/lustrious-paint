#pragma once

#include <vector>
#include <memory>
#include <algorithm>

namespace lslib {

using namespace std;

class destructor
{
protected:
  vector<unique_ptr<destructor>> attached;
  void(*fn)();
  void prune() {
    auto is_empty = [](unique_ptr<destructor>& x) { 
      return x.get()->fn == 0;
    };
    attached.erase(std::remove_if( attached.begin(), attached.end(), is_empty), attached.end());
  }
public:
  destructor(void(*fn)()):fn(fn) {
    // if (!fn) {
    //   throw std::string("Function must not be NULL!");
    // }
  }
  bool operator==(const destructor& other) {
    return fn == other.fn;
  }
  destructor& operator=(destructor&& other) {
    fn = other.fn;
    other.fn = 0;
    return *this;
  }
  destructor(destructor&& other) {
    fn = other.fn;
    other.fn = 0;
  }
  ~destructor() {
    // Call child destructors before the parent destructor
    attached.clear();
    // Invoke the parent destructor
    if (fn) {
      fn();
    }
  }
  destructor& attach(destructor& other) {
    if (!fn) {
      throw std::string("Attaching to moved out destructor!");
    }
    if (!other.fn) {
      throw std::string("Attaching moved out destructor!");
    }
    attached.push_back(make_unique<destructor>(std::move(other)));
    return *attached[attached.size()-1];
  }
  destructor& attach_to(destructor& other) {
    return other.attach(*this);
  }
  destructor detach(destructor& other) {
    // Check if destructor is attached
    auto it = find_if(attached.begin(), attached.end(), [&](unique_ptr<destructor>& x) {
      return x.get()->fn == other.fn;
    });
    if (it == attached.end())
    {
      throw std::string("This destructor is not attached here!");
    }
    // Move out the value
    destructor other_owned(std::move(*(it->get())));
    // Remove moved out values
    prune();
    return other_owned;
  }
  destructor detach_from(destructor& other) {
    return other.detach(*this);
  }
};

}