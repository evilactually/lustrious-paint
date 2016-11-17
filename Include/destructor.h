namespace lslib {

using namespace std;

class destructor
{
public:
  int value;
  void(*fn)();
  vector<unique_ptr<destructor>> attached;
  std::list<destructor> _attached;
  static bool is_valid(const destructor& x) { return x.value != 0; }
  static bool is_invalid(const destructor& x) { return !is_valid(x); }
  //destructor(void(*fn)()):fn(fn) { }
  destructor(int value):value(value),fn(0) { }
  bool operator==(const destructor& other) {
    //return fn == other.fn;
    return value == other.value;
  }
  destructor& operator=(destructor&& other) {
    //fn = other.fn;
    //other.fn = 0;
    value = other.value;
    other.value = 0;
    return *this;
  }
  destructor(destructor&& other) {
    value = other.value;
    //fn = other.fn;
    other.value = 0;
    //other.fn = 0;
  }
  ~destructor() {
    // Call child destructors before the parent destructor
    attached.clear();
    // Invoke the parent destructor
    std::cout << "~" << value << std::endl;
    // if (fn) {
    //   fn();
    // }
  }
  void prune() {
    auto is_empty = [](unique_ptr<destructor>& x) { 
      return x.get()->value == 0;
    };
    attached.erase(std::remove_if( attached.begin(), attached.end(), is_empty), attached.end());
  }
  destructor& attach(destructor& other) {
    if (!value) {
      throw std::string("Attaching to moved out destructor!");
    }
    if (!other.value) {
      throw std::string("Attaching moved out destructor!");
    }
    attached.push_back(make_unique<destructor>(std::move(other)));
    return *attached[attached.size()-1];
  }
  destructor& attach_to(destructor& other) {
    return other.attach(*this);
    //return *other.attached[other.attached.size()-1];
  }
  destructor detach(destructor& other) {
    // Check if destructor is attached
    auto it = find_if(attached.begin(), attached.end(), [&](unique_ptr<destructor>& x) {
      return x.get()->value == other.value;
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
};

}