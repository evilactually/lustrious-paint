#include <caf/all.hpp>
#include <vector>
#include <iostream>


#include <windows.h>


// actor that other actors can subscribe too if they have proper behaviours implemented
// This actor is updated by sending it update message
// It polls new input and sends events to subscribed actors

void test() {
  
}

using namespace caf;
using ls_handle_window_message_atom = atom_constant<atom("update")>;
using ls_subscribe_atom = atom_constant<atom("subscribe")>;

//-------------------------------------------------------------------------------
// @ LsInputSystem
//-------------------------------------------------------------------------------
// This actor recieves window messages and turns them into input events understood
// understood by the rest of the program
//-------------------------------------------------------------------------------
class LsInputSystem : public event_based_actor {
friend void test(const LsInputSystem&);
public:
  LsInputSystem(actor_config& cfg) : event_based_actor(cfg) { }

  behavior make_behavior() override {
    return behavior{
      [&](ls_handle_window_message_atom) {
        // poll events and dispatch them to listeners
        this->request(listeners[0], infinite, 42);
      },
      [&](ls_subscribe_atom, const actor& listener) {
        listeners.push_back(listener);
      }
    };
  }
protected:
  std::vector<actor> listeners;
};

void test(const LsInputSystem& inputSystem) {
  inputSystem.listeners[0];
}