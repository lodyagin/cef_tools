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

#if 0
std::tuple<int&, int&, int&, int&> to_tuple(CefRect& r)
{
  return std::forward_as_tuple(r.x, r.y, r.width, r.height);
}

std::tuple<int, int, int, int> to_tuple(const CefRect& r)
{
  return std::make_tuple(r.x, r.y, r.width, r.height);
}

#else
// returns tuple for both setting/getting CefRect vals
// @tparam T is either CefRect& (setter) or const CefRect& (getter)
template<class T>
auto to_tuple(T& r)
  -> curr::enable_fun_if<
       std::is_same<CefRect, T>,
       decltype(std::forward_as_tuple(r.x, r.y, r.width, r.height))
     >
{
  return std::forward_as_tuple(r.x, r.y, r.width, r.height);
}
#endif
