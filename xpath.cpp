// -*-coding: mule-utf-8-unix; fill-column: 58; -*-
/**
 * @file
 * The xpath-like CefDOMNode finder.
 *
 * @author Sergei Lodyagin
 */

#include <assert.h>
#include <algorithm>
#include <cctype>
#include "include/cef_dom.h"
#include "string_utils.h"
#include "xpath.h"

xpath::select::select(
  const std::string& tag_,
  const CefRefPtr<CefDOMNode>& dom_,
  int d
) 
  : tag(tag_), dom(dom_), depth(d)
{
  SCHECK(dom.get());
  std::transform(
    tag.begin(), tag.end(), tag.begin(), ::tolower
  );
}

#if 1
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
  return out;
}

std::ostream&
operator<< (std::ostream& out, const xpath::select& sel)
{
  SCHECK(sel.dom.get());
  if (!sel.dom->IsElement())
    return out;

  std::string tag_name = 
    sel.dom->GetElementTagName().ToString();
  std::transform(
    tag_name.begin(), tag_name.end(), tag_name.begin(),
    ::tolower
  );

//  std::cout << tag_name << ' ' << std::flush;

  if (tag_name == sel.tag)
    out << sel.dom;

  // recursion into childs
  for(auto node = sel.dom->GetFirstChild();
      node.get() != nullptr;
      node = node->GetNextSibling())
  {
    // TODO copy std::string - optimize it
    out << xpath::select(sel.tag, node, sel.depth + 1);
  }
  return out;
}
#endif


