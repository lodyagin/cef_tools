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

xpath::select::select(
  const std::string& tag_,
  const CefRefPtr<CefDOMNode>& dom_
) 
  : tag(tag_), context_node(dom_)
{
  std::transform(
    tag.begin(), tag.end(), tag.begin(), ::tolower
  );
}

std::ostream&
operator<< (std::ostream& out, CefRefPtr<CefDOMNode> dom)
{
  assert(dom.get());
  if (!dom->IsElement())
    return out << "(not_element)";

#if 0
  if (!dom->HasElementAttributes())
    return out << "(not_attributes)";
#endif

#if 0
  CefDOMNode::AttributeMap attrs;
  // it hang ups
  dom->GetElementAttributes(attrs);
#endif
  out << '<' << dom->GetElementTagName();
#if 0
  for (auto p : attrs)
    out << ' ' << p.first << "=\"" << p.second << '"';
  out << '>';
#else
  out << " type=\"" << dom->GetElementAttribute("TYPE")
      << "\">";
#endif

#if 1
  const CefRect r = dom->GetBoundingClientRect();
  out << " [" << r.x << ", " << r.y << ", " << r.width
      << ", " << r.height << "]";
#endif
  return out;
}

std::ostream&
operator<< (std::ostream& out, const xpath::select& sel)
{
  auto descendants = sel.context_node.descendant();
  std::copy_if(
    descendants->begin(),
    descendants->end(),
    std::ostream_iterator<CefRefPtr<CefDOMNode>>(
      out,
      "\n"
    ),
    [&sel](const xpath::node& node)
    {
      return node.tag_name() == sel.tag;
    }
  );
  return out;
}


