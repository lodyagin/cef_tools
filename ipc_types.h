// -*-coding: mule-utf-8-unix; fill-column: 58; -*-
/**
 * @file
 * IPC support for some complex types 
 *
 * @author Sergei Lodyagin
 */

#include <tuple>
#include <type_traits>
#include "include/cef_base.h"
#include "types/meta.h"

template<class T>
auto to_tuple(T& r)
  -> curr::enable_fun_if<
       std::is_base_of<CefRect, T>,
       std::tuple<int&, int&, int&, int&>
     >
{
  return std::forward_as_tuple(r.x, r.y, r.width, r.height);
}

