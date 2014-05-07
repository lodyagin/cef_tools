// -*-coding: mule-utf-8-unix; fill-column: 58; -*-
/**
 * @file
 * CEF IPC wrapper
 *
 * @author Sergei Lodyagin
 */

#ifndef OFFSCREEN_IPC_H
#define OFFSCREEN_IPC_H

#include <string>
#include <tuple>
#include <unordered_map>
#include "include/cef_process_message.h"
#include "types/meta.h"
#include "Repository.h"

namespace ipc {
namespace receiver {

//! an entries (the descendants) are built upon a fun_obj
//! signature and allow to call the fun_obj
class entry_base
{
public:
  struct Par 
  {
    PAR_DEFAULT_ABSTRACT(base);
    virtual std::string get_id() const = 0;
  };

  virtual void call(CefRefPtr<CefProcessMessage> msg) = 0;
};

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
  pars(CefRefPtr<CefProcessMessage> msg) {}
};

template<int idx, class Arg0, class... Args>
class pars<idx, Arg0, Args...> : public pars<idx, Args...>
{
public:
  pars(
    CefRefPtr<CefProcessMessage> msg, 
    Arg0& a0,
    Args&... a
  )
    : par(msg, a0), pars<idx, Args...>(msg, a)
  {}

  //! constructs from a tuple
  template<class... TArgs>
  pars(
    CefRefPtr<CefProcessMessage> msg, 
    std::tuple<TArgs...>& tup
  )
    : pars<idx, Arg0, Args...>(
       msg, 
       typename gens<std::tuple_size<Tuple>::value>::type(),
       tup
      )
  {}

protected:
  //! constructs from a tuple
  template<class... TArgs, int... S>
  pars(
    CefRefPtr<CefProcessMessage> msg, 
    seq<S...>,
    std::tuple<TArgs...>& tup
  )
    : pars<idx, Arg0, Args...>(msg, std::get<S>(tup)...)
  {}
};

template<int idx, class... Args>
struct par<idx, std::tuple<Args...>> : pars<idx, Args...>
{
  par(
    CefRefPtr<CefProcessMessage> msg, 
    std::tuple<Args...>& tup
  )
    : pars(msg, tup)
  {}
};


template<int idx, class Arg>
class par<idx, Arg, decltype(Arg::to_tuple())>
{
public:
  using tuple = decltype(Arg::to_tuple());

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
  par(CefRefPtr<CefProcessMessage> msg, int& arg)
  {
    arg = msg->GetArgumentList()->GetInt(idx);
  }
};

template<int idx>
struct par<idx, double>
{
  par(CefRefPtr<CefProcessMessage> msg, double& arg)
  {
    arg = msg->GetArgumentList()->GetDouble(idx);
  }
};

template<int idx>
struct par<idx, std::string>
{
  par(CefRefPtr<CefProcessMessage> msg, std::string& arg)
  {
    arg= msg->GetArgumentList()->GetString(idx).ToString();
  }
};



template<int idx>
struct get<idx>
{
  get(CefRefPtr<CefProcessMessage> msg) {}
  std::tuple<> get_pars() const { return std::tuple<>(); }
};

template<int idx, class Arg0, class... Args>
struct get<idx, Arg0, Args...> 
  : par<idx, Arg0>, get<idx + 1, Args...>
{
  get(CefRefPtr<CefProcessMessage> msg)
   : par(a), get/*<idx + 1, Args...>*/(msg)
  {}

  std::tuple<Arg0, Args...> get_pars() const
  {
    return std::tuple_cat(
      std::tuple<Arg0>(a),
      get<idx + 1, Args...>::get_pars()
    );
  }

  Arg0 a;
};

template<class FunObj>
class entry;

template<class Fun, class Args...>
class entry<Fun<Args...>> : public entry_base
{
public:
  struct Par : entry_base::Par 
  {
   
  };

  void call(CefRefPtr<CefProcessMessage> msg) override
  {
    assert(type<Fun<Args...>>::name() == msg->GetName());
    curr::tuple::call<Fun<Args...>>(get<1, Args...>(msg));
  }
};

class repository final
  : public Repository<
    entry_base, 
    entry_base::Par, 
    std::unordered_map,
    std::string
   >,
   public SAutoSingleton<repository>
{
public:
  using ParentRepository = Repository<
    entry_base, 
    entry_base::Par, 
    std::unordered_map,
    std::string
  >;

  repository() 
   : ParentRepository(type<repository>::name(), 0)
  { 
    this->complete_construction();
  }
  
  ~repository()
  {
    this->destroy();
  }

  void call(CefRefPtr<CefProcessMessage> msg);
};

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

