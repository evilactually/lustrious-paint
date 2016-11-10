#include <caf/all.hpp>
#include <vector>
#include <windows.h>

using namespace caf;
using ls_handle_window_message_atom = atom_constant<atom("update")>;
using ls_subscribe_atom = atom_constant<atom("subscribe")>;

// using input_system_actor = typed_actor<replies_to<ls_handle_window_message_atom, HWND, UINT, WPARAM, LPARAM>::with<LRESULT>,
//                                        replies_to<int>::with<void>>;

using input_system_actor = typed_actor<replies_to<ls_handle_window_message_atom, HWND, UINT, WPARAM, LPARAM>::with<LRESULT>,
                                       replies_to<ls_subscribe_atom, strong_actor_ptr>::with<void>>;

//-------------------------------------------------------------------------------
// @ LsInputSystem
//-------------------------------------------------------------------------------
// This actor recieves window messages and turns them into input events
// understood by the rest of the program
//-------------------------------------------------------------------------------
class LsInputSystem : public input_system_actor::base {
friend void test(const LsInputSystem&);
public:
  LsInputSystem(actor_config& cfg) : input_system_actor::base(cfg) { }

  input_system_actor::behavior_type make_behavior() override {
    return {
      [](ls_handle_window_message_atom, HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) -> LRESULT {
        return 1;
      },
      [](ls_subscribe_atom, strong_actor_ptr listener) {
        return;
      }


      // [](ls_subscribe_atom, const actor& listener) {
      //   return;
      // }

      // [](ls_handle_window_message_atom, HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
      //   // poll events and dispatch them to listeners
      //   //this->request(listeners[0], infinite, 42);
      //   //return static_cast<LRESULT>(0);
      //   return;
      // },
      // [](ls_subscribe_atom, const actor& listener) {
      //   //listeners.push_back(listener);
      //   return;
      // }
    };
  }
protected:
  std::vector<actor> listeners;
};


LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
  return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

// using calculator_actor = typed_actor<replies_to<add_atom, int, int>::with<int>,
//                                      replies_to<sub_atom, int, int>::with<int>>;

// Ls


// class Ls : public event_based_actor {
// friend void test(const LsInputSystem&);
// public:
//   LsInputSystem(actor_config& cfg) : event_based_actor(cfg) { }

//   behavior make_behavior() override {
//     return behavior{
//       [&](ls_handle_window_message_atom) {
//         // poll events and dispatch them to listeners
//         this->request(listeners[0], infinite, 42);
//       },
//       [&](ls_subscribe_atom, const actor& listener) {
//         listeners.push_back(listener);
//       }
//     };
//   }
// protected:
//   std::vector<actor> listeners;
// };

int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {

}

// Design
// A window is an actor it created from initial information and having a stack of handlers representing it's WinProc
// Compose WinProc out of a stack of handler actors
// Each handler chooses what messages it's going to handle and which to pass down the stack by returning Optional<LRESULT>
// 

// Basic Design
// WinProc recieves a message
// Passes it down a stack of actors
// One of the actors in the stack is actor that transforms raw input into input events, i.e key presses, 
// mouse moves that are platform independent and easy to use