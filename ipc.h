// -*-coding: mule-utf-8-unix; fill-column: 58; -*-
/**
 * @file
 * CEF IPC wrapper
 *
 * @author Sergei Lodyagin
 */

#ifndef OFFSCREEN_IPC_H
#define OFFSCREEN_IPC_H

#include <iostream>
#include <string>
#include <tuple>
#include <unordered_map>
#include "include/cef_process_message.h"
#include "types/meta.h"
#include "Repository.h"
#include "SCommon.h"
#include "SSingleton.h"
#include "ipc_types.h"

namespace ipc {

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

namespace par_ {

template<int idx, class Arg, class = void>
struct par;

template<int idx, class... Args>
struct pars;

template<int idx>
struct pars<idx>
{
  static constexpr int next_idx = idx; // no data here

  pars(CefRefPtr<CefProcessMessage> msg) {}

  //! constructs from a tuple
  pars(
    CefRefPtr<CefProcessMessage> msg, 
    std::tuple<>& tup
  )
  {}
};

template<int idx, class Arg0, class... Args>
class pars<idx, Arg0, Args...> 
  : public par<idx, Arg0>,
    public pars<par<idx, Arg0>::next_idx, Args...>
{
public:
  static constexpr int next_idx = 
    pars<par<idx, Arg0>::next_idx, Args...>::next_idx;
  
  pars(
    CefRefPtr<CefProcessMessage> msg, 
    Arg0 a0,
    Args... a
  )
    : par<idx, Arg0>(msg, a0), 
      pars<par<idx, Arg0>::next_idx, Args...>(msg, a...)
  {}

  //! constructs from a tuple
  pars(
    CefRefPtr<CefProcessMessage> msg, 
    std::tuple<Arg0, Args...>&& tup
  )
    : pars<idx, Arg0, Args...>(
       msg, 
       typename curr::tuple::gens<
         sizeof...(Args) + 1
       >::type(),
       std::move(tup)
      )
  {}

protected:
  //! constructs from a tuple
  template<class... TArgs, int... S>
  pars(
    CefRefPtr<CefProcessMessage> msg, 
    curr::tuple::seq<S...>,
    std::tuple<TArgs...>&& tup
  )
    : pars<idx, Arg0, Args...>(msg, std::get<S>(tup)...)
  {}
};

template<int idx, class... Args>
struct par<idx, std::tuple<Args...>&&> 
  : pars<idx, Args...>
{
  static constexpr int next_idx = 
    pars<idx, Args...>::next_idx;

  par(
    CefRefPtr<CefProcessMessage> msg, 
    std::tuple<Args...>&& tup
  )
    : pars<idx, Args...>(msg, std::move(tup))
  {}
};

#if 0
template<int idx, class Arg>
class par<
  idx, 
  Arg&, 
  decltype((void)to_tuple(std::remove_reference<Arg>()))
>
{
public:
  using tuple = 
    decltype(to_tuple(std::remove_reference<Arg>()));

  static constexpr int next_idx = 
    par<idx, tuple>::next_idx;

  par(CefRefPtr<CefProcessMessage> msg, Arg& arg)
  {
    tuple tup;
    par<idx, tuple>(msg, tup);
    arg = curr::tuple::aggregate_construct<Arg>(tup);
  }
};
#else
template<int idx, class Arg>
class par<
  idx, 
  Arg, 
  decltype((void)to_tuple(std::declval<Arg>()))
> 
  : par<idx, decltype(to_tuple(std::declval<Arg>()))&&>
{
public:
  using parent_par = 
    par<idx, decltype(to_tuple(std::declval<Arg>()))&&>;
  //using tuple = decltype(to_tuple(std::declval<Arg>()));

  static constexpr int next_idx = parent_par::next_idx;

#if 1
  par(CefRefPtr<CefProcessMessage> msg, Arg& arg)
    : parent_par(msg, to_tuple(arg))
  {}
#else
  par(CefRefPtr<CefProcessMessage> msg, Arg& arg)
  {
    par<idx, tuple&>(msg, to_tuple(arg));
  }
#endif
};
#endif

template<int idx>
struct par<idx, int&>
{
  static constexpr int next_idx = idx + 1;

  par(CefRefPtr<CefProcessMessage> msg, int& arg)
  {
    arg = msg->GetArgumentList()->GetInt(idx);
  }
};

template<int idx>
struct par<idx, double&>
{
  static constexpr int next_idx = idx + 1;

  par(CefRefPtr<CefProcessMessage> msg, double& arg)
  {
    arg = msg->GetArgumentList()->GetDouble(idx);
  }
};

// TODO if it lexical casts
template<int idx>
struct par<idx, std::string&>
{
  static constexpr int next_idx = idx + 1;

  par(CefRefPtr<CefProcessMessage> msg, std::string& arg)
  {
    arg= msg->GetArgumentList()->GetString(idx).ToString();
  }
};

template<int idx>
struct par<idx, const int&>
{
  static constexpr int next_idx = idx + 1;

  par(CefRefPtr<CefProcessMessage> msg, const int& arg)
  {
    msg->GetArgumentList()->SetInt(idx, arg);
  }
};

template<int idx>
struct par<idx, const double&>
{
  static constexpr int next_idx = idx + 1;

  par(CefRefPtr<CefProcessMessage> msg, const double& arg)
  {
    msg->GetArgumentList()->SetDouble(idx, arg);
  }
};

template<int idx>
struct par<idx, const std::string&>
{
  static constexpr int next_idx = idx + 1;

  par(
    CefRefPtr<CefProcessMessage> msg, 
    const std::string& arg
  )
  {
    msg->GetArgumentList()->SetString(idx, arg);
  }
};

} // par_

namespace receiver {

//! an entries (the descendants) are built upon a fun_obj
//! signature and allow to call the fun_obj
class entry_base
{
public:
  struct Par 
  {
    PAR_DEFAULT_ABSTRACT(entry_base);
    virtual std::string get_id(
      const curr::ObjectCreationInfo& info
    ) const = 0;
  };

  virtual ~entry_base() {}

  virtual void call(CefRefPtr<CefProcessMessage> msg) = 0;

  virtual std::string universal_id() const = 0;
};

std::ostream&
operator<< (std::ostream& out, const entry_base& entr);

namespace receiver_ {

// Builds a tuple of arguments upon a cef message
template<int idx, class... Args>
struct get;

//} // receiver_
//} // receiver

//namespace receiver {
//namespace receiver_ {

template<int idx>
struct get<idx>
{
  get(CefRefPtr<CefProcessMessage> msg) {}

  std::tuple<> get_pars() const 
  { 
    return std::make_tuple(); 
  }
};

template<int idx, class Arg0, class... Args>
struct get<idx, Arg0, Args...> 
  : par_::par<idx, Arg0&>, 
    get<par_::par<idx, Arg0&>::next_idx, Args...>
{
  get(CefRefPtr<CefProcessMessage> msg)
   : par_::par<idx, Arg0&>(msg, a), 
     get<par_::par<idx, Arg0&>::next_idx, Args...>(msg)
  {}

  std::tuple<Arg0, Args...> get_pars() const
  {
    return std::tuple_cat(
      std::make_tuple(a),
      get<par_::par<idx, Arg0&>::next_idx, Args...>
        ::get_pars()
    );
  }

  Arg0 a;
};

} // receiver_

template<class FunObj>
class entry;

template<template <class...> class Fun, class... Args>
class entry<Fun<Args...>> : public entry_base
{
public:
  struct Par : entry_base::Par 
  {
    std::string get_id(
      const curr::ObjectCreationInfo& info
    ) const override
    {
      return curr::type<Fun<Args...>>::name();
    }

    entry_base* create_derivation(
      const curr::ObjectCreationInfo& info
    ) const override
    {
      return new entry<Fun<Args...>>;
    }
  };

  void call(CefRefPtr<CefProcessMessage> msg) override
  {
    assert(
      curr::type<Fun<Args...>>::name() == 
        msg->GetName().ToString()
    );
    curr::tuple::call<Fun<Args...>>(
      receiver_::get<1, Args...>(msg).get_pars()
    );
  }

  std::string universal_id() const override
  {
    return curr::type<Fun<Args...>>::name();
  }
};

class repository final
  : public curr::Repository<
    entry_base, 
    entry_base::Par, 
    std::unordered_map,
    std::string
   >,
   public curr::SAutoSingleton<repository>
{
public:
  using ParentRepository = curr::Repository<
    entry_base, 
    entry_base::Par, 
    std::unordered_map,
    std::string
  >;

  repository() 
   : ParentRepository(curr::type<repository>::name(), 0)
  { 
    this->complete_construction();
  }
  
  // @returns true if msg is processed
  bool call(CefRefPtr<CefProcessMessage> msg);

  template<class FunObj>
  void reg()
  {
    this->create_object(typename entry<FunObj>::Par());
  }
};

} // receiver

namespace sender {

namespace sender_ {

template<int idx, class... Args>
struct put;

#if 0
template<int idx, class Arg, class = void>
struct par;

template<int idx>
struct par<idx, int>
{
  static constexpr int next_idx = idx + 1;

  par(CefRefPtr<CefProcessMessage> msg, int arg)
  {
    msg->GetArgumentList()->SetInt(idx, arg);
  }
};

template<int idx>
struct par<idx, double>
{
  static constexpr int next_idx = idx + 1;

  par(CefRefPtr<CefProcessMessage> msg, double arg)
  {
    msg->GetArgumentList()->SetDouble(idx, arg);
  }
};

template<int idx>
struct par<idx, std::string>
{
  static constexpr int next_idx = idx + 1;

  par(
    CefRefPtr<CefProcessMessage> msg, 
    const std::string& arg
  )
  {
    msg->GetArgumentList()->SetString(idx, arg);
  }
};

template<int idx, class... Args>
struct par<idx, std::tuple<Args...>> : pars<idx, Args...>
{
  static constexpr int next_idx = 
    pars<idx, Args...>::next_idx;

  par(
    CefRefPtr<CefProcessMessage> msg, 
    const std::tuple<Args...>& tup
  )
    : pars<idx, Args...>(msg, tup)
  {}
};


// struct/class 
template<int idx, class Struct>
struct par<
  idx,
  Struct,
  std::enable_if<std::is_class<Struct>::value>::type
>
  : par<idx, decltype(to_tuple(Struct()))>
{
  static constexpr int next_idx = idx 
    + par<idx, decltype(to_tuple(Struct()))>::next_id;

  par(
    CefRefPtr<CefProcessMessage> msg, 
    const Struct& arg
  )
    : par(msg, to_tuple(arg))
  {}
};
#endif

template<int idx>
struct put<idx>
{
  put(CefRefPtr<CefProcessMessage> msg) {}
};

template<int idx, class Arg0, class... Args>
struct put<idx, Arg0, Args...>
  : par_::par<idx, Arg0>,
    put<par_::par<idx, Arg0>::next_idx, Args...>
{
  put(CefRefPtr<CefProcessMessage> msg, Arg0 a0, Args... a)
    : par_::par<idx, Arg0>(msg, a0),
      put<par_::par<idx, Arg0>::next_idx, Args...>
        (msg, a...)
  {}
};

} // sender_

template<template<class...> class Fun, class... Args>
void send(Args&&... args)
{
  CefRefPtr<CefProcessMessage> msg = 
    CefProcessMessage::Create(
      curr::type<Fun<Args...>>::name()
    );
  sender_::put<1, Args&&...>(msg, std::forward<Args>(args)...);
}

} // sender
} // ipc

#endif

