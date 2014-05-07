// -*-coding: mule-utf-8-unix; fill-column: 58; -*-
/**
 * @file
 * IPC support for some complex types 
 *
 * @author Sergei Lodyagin
 */

#include "include/cef_base.h"

inline std::tuple<int, int, int, int> 
to_tuple(const CefRect& r)
{
  return std::make_tuple(r.x, r.y, r.width, r.height);
}

