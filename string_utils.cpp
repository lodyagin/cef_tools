// -*-coding: mule-utf-8-unix; fill-column: 58; -*-
/**
 * @file
 * CefString utils.
 *
 * @author Sergei Lodyagin
 */

#include "string_utils.h"

std::ostream&
operator<< (std::ostream& out, const CefString& s)
{
  return out << s.ToString();
}
