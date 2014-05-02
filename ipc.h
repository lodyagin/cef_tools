// -*-coding: mule-utf-8-unix; fill-column: 58; -*-
/**
 * @file
 * CEF IPC wrapper
 *
 * @author Sergei Lodyagin
 */

#ifndef OFFSCREEN_IPC_H
#define OFFSCREEN_IPC_H

#include <tuple>
#include "include/cef_process_message.h"

namespace ipc {

//! Represents the Fun parameters list as args<Fun>::tuple.
template<class Fun>
struct args;

template<class... Args>
struct args<void(Args...)>
{
  using tuple = std::tuple<Args...>;
};

/*
  An IPC call must contain tag string and signature.
  One can get the tag string as
  curr::type<fun_obj<...>>::name():

  template<class... Args>
  struct fun_obj
  {
    fun_obj(Args... args_) : args(args_)... {}
    void operator();

    Args... args;
  };
*/

// 1.
// pars to call a functional object:
// fun_obj<args...>

// 2.
// to create an object
// ObjType, IdType, Par -> id

// 3.
// to call method on an object: 
// ObjType, IdType, Id, args::tuple

}

#endif

