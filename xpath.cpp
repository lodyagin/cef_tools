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

namespace renderer {
namespace dom_visitor {
namespace xpath {

std::ostream&
operator<< (std::ostream& out, const node& nd)
{
  using namespace std;

  switch(nd.the_type)
  {
    case node::type::dom:
    {
      if (!nd->IsElement())
        return out << "(not_element)";

      // tag name
      out << '<' << nd.tag_name();

      // attributes
      for (pair<string, string> p : *nd.attribute())
        out << ' ' << p.first << "=\"" << p.second << '"';
      out << '>';

      // bounding rect
      const CefRect r = nd->GetBoundingClientRect();
      out << " [" << r.x << ", " << r.y << ", " << r.width
         << ", " << r.height << "]";

      return out;
    }
    case node::type::attribute:
    {
#if 1
      const pair<string, string> p(nd);
      return out << '{' << p.first << '=' << p.second << '}';
#else
      CefString name, value;
      nd->GetElementAttributeByIdx(
        nd.attr_idx, name, value
      );
      return out << '[' << name.ToString() << '=' 
        << value.ToString() << ']';
#endif
    }
    case node::type::not_initialized:
      return out << "(node not initialized)";
    default:
      THROW_NOT_IMPLEMENTED;
  }
}

select::select(
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
operator<< (std::ostream& out, const select& sel)
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

}}}
