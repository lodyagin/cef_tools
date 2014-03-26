// -*-coding: mule-utf-8-unix; fill-column: 58; -*-
/**
 * @file
 * The xpath-like CefDOMNode finder.
 *
 * @author Sergei Lodyagin
 */

#include <iostream>
#include "SCheck.h"

namespace xpath {

struct select
{
  select(const std::string& tag_,
         const CefRefPtr<CefDOMNode>& dom_);
      
  std::string tag;
  CefRefPtr<CefDOMNode> dom;
};

}

//! prints <tag attrs...> of the node
std::ostream&
operator<< (std::ostream& out, CefRefPtr<CefDOMNode> dom);

//! prints matched tags
std::ostream&
operator<< (std::ostream& out, const xpath::select& sel);
