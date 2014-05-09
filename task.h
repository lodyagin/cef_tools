#ifndef OFFSCREEN_TASK_H
#define OFFSCREEN_TASK_H

#include <tuple>
#include <type_traits>
#include <functional>
#include "include/cef_task.h"
#include "types/meta.h"

#define REQUIRE_UI_THREAD()   assert(CefCurrentlyOn(TID_UI));
#define REQUIRE_IO_THREAD()   assert(CefCurrentlyOn(TID_IO));
#define REQUIRE_FILE_THREAD() assert(CefCurrentlyOn(TID_FILE));
#define REQUIRE_RENDERER_THREAD() assert(CefCurrentlyOn(TID_RENDERER));

namespace task {

namespace task_ {

  template<class... Args>
  class task : public CefTask
  {
  public:
    using fun_t = std::function<
      void(
        typename std::add_lvalue_reference<Args>::type...
      )
    >;

    task(const fun_t& fun_, Args... args_) 
      : fun(fun_),
        args(std::forward<Args>(args_)...) 
    {}

    void Execute() override
    {
      // pre-c++14 method of converting tuple to args...
      Execute_(
        typename curr::tuple::gens<sizeof...(Args)>
          ::type()
      );
    }

  protected:
    template<int... S>
    void Execute_(curr::tuple::seq<S...>)
    {
      fun(std::get<S>(args)...);
    }

    fun_t fun;

    //NB copy values (remove_reference)
    std::tuple<
      typename std::remove_reference<Args>::type...
    > args; 

  private:
    IMPLEMENT_REFCOUNTING(load);
  };

}

template<class Fun, class... Args>
void exec(CefThreadId th_id, Fun fun, Args&&... args)
{
  std::cout << "CefPostTask:1" << std::endl;
  CefPostTask(
    th_id, 
    new task_::task<Args&&...>(
      fun,
      std::forward<Args>(args)...
    )
  );
}

}

#endif
