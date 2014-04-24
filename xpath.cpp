// -*-coding: mule-utf-8-unix; fill-column: 58; -*-
/**
 * @file
 * The xpath-like CefDOMNode finder.
 *
 * @author Sergei Lodyagin
 */

#include <iterator>
#include <assert.h>
#include <algorithm>
#include <cctype>
#include "include/cef_dom.h"
#include "string_utils.h"
#include "xpath.h"

namespace xpath {

std::ostream&
operator<< (std::ostream& out, const child_path_t& path)
{
  if (path.size() == 0)
    out << "?empty?";
  else if (path.size() == 1)
    out << '.';
  else {
    auto start = path.begin(); 
    ++start;
    auto last = path.end();
    --last;
    for (auto it = start; it != path.end(); ++it)
    {
      out << *it;
      if (it != last)
        out << '/';
    }
  }
  return out;
}

}

namespace renderer {
namespace dom_visitor {
namespace xpath {

}}}
