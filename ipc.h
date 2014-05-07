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

// Builds a tuple of arguments upon a cef message
template<int idx, class... Args>
struct get;

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
    Arg0& a0,
    Args&... a
  )
    : par<idx, Arg0>(msg, a0), 
      pars<par<idx, Arg0>::next_idx, Args...>(msg, a...)
  {}

  //! constructs from a tuple
  pars(
    CefRefPtr<CefProcessMessage> msg, 
    std::tuple<Arg0, Args...>& tup
  )
    : pars<idx, Arg0, Args...>(
       msg, 
       typename curr::tuple::gens<
         sizeof...(Args) + 1
       >::type(),
       tup
      )
  {}

protected:
  //! constructs from a tuple
  template<class... TArgs, int... S>
  pars(
    CefRefPtr<CefProcessMessage> msg, 
    curr::tuple::seq<S...>,
    std::tuple<TArgs...>& tup
  )
    : pars<idx, Arg0, Args...>(msg, std::get<S>(tup)...)
  {}
};

template<int idx, class... Args>
struct par<idx, std::tuple<Args...>> : pars<idx, Args...>
{
  static constexpr int next_idx = pars<idx, Args...>::next_idx;

  par(
    CefRefPtr<CefProcessMessage> msg, 
    std::tuple<Args...>& tup
  )
    : pars<idx, Args...>(msg, tup)
  {}
};


template<int idx, class Arg>
class par<idx, Arg, decltype((void)to_tuple(Arg()))>
{
public:
  using tuple = decltype(to_tuple(Arg()));

  static constexpr int next_idx = par<idx, tuple>::next_idx;

  par(CefRefPtr<CefProcessMessage> msg, Arg& arg)
  {
    tuple tup;
    par<idx, tuple>(msg, tup);
    arg = curr::tuple::aggregate_construct<Arg>(tup);
  }
};

template<int idx>
struct par<idx, int>
{
  static constexpr int next_idx = idx + 1;

  par(CefRefPtr<CefProcessMessage> msg, int& arg)
  {
    arg = msg->GetArgumentList()->GetInt(idx);
  }
};

template<int idx>
struct par<idx, double>
{
  static constexpr int next_idx = idx + 1;

  par(CefRefPtr<CefProcessMessage> msg, double& arg)
  {
    arg = msg->GetArgumentList()->GetDouble(idx);
  }
};

// TODO if it lexical casts
template<int idx>
struct par<idx, std::string>
{
  static constexpr int next_idx = idx + 1;

  par(CefRefPtr<CefProcessMessage> msg, std::string& arg)
  {
    arg= msg->GetArgumentList()->GetString(idx).ToString();
  }
};

template<int idx>
struct get<idx>
{
  get(CefRefPtr<CefProcessMessage> msg) {}
  std::tuple<> get_pars() const { return std::make_tuple(); }
};

template<int idx, class Arg0, class... Args>
struct get<idx, Arg0, Args...> 
  : par<idx, Arg0>, get<par<idx, Arg0>::next_idx, Args...>
{
  get(CefRefPtr<CefProcessMessage> msg)
   : par<idx, Arg0>(msg, a), 
     get<par<idx, Arg0>::next_idx, Args...>(msg)
  {}

  std::tuple<Arg0, Args...> get_pars() const
  {
    return std::tuple_cat(
      std::make_tuple(a),
      get<par<idx, Arg0>::next_idx, Args...>::get_pars()
    );
  }

  Arg0 a;
};

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
      get<1, Args...>(msg).get_pars()
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



#if 0
//! Represents the Fun parameters list as args<Fun>::tuple.
template<class Fun>
struct args;

template<class... Args>
struct args<void(Args...)>
{
  using tuple = std::tuple<Args...>;
};
#endif

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

} // receiver
} // ipc

#endif

