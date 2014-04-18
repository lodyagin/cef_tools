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

node::operator std::pair<std::string, std::string>() const
{
    SCHECK(the_type == type::attribute);
    const size_t n = n_attrs();
    SCHECK(n > 0);
    assert(attr_idx >= 0);
    assert((size_t)attr_idx < n);

    CefString name, value;
    dom->GetElementAttributeByIdx(
      attr_idx, 
      name,
      value
    );
    return std::make_pair(
      name.ToString(), 
      value.ToString()
    );
}

std::shared_ptr<node::axis_<axis::self>> node::self() const
{
  // SCHECK(the_type != type::not_initialized);
  // already checked in the axis_::axis_
  return std::make_shared<axis_<xpath::axis::self>>(dom);
}

std::shared_ptr<node::axis_<axis::child>> node::child() const
{
  return std::make_shared<axis_<xpath::axis::child>>(dom);
}

std::shared_ptr<node::axis_<axis::descendant>> 
node::descendant() const
{
  return std::make_shared<axis_<xpath::axis::descendant>>(dom);
}

std::shared_ptr<node::axis_<axis::attribute>> 
node::attribute() const
{
  return std::make_shared<axis_<xpath::axis::attribute>>(dom);
}

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
      auto attrs = *nd.attribute();
      for (std::pair<string, string> p : attrs)
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
      const pair<string, string> p(nd);
      return out << '{' << p.first << '=' << p.second << '}';
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
