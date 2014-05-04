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

