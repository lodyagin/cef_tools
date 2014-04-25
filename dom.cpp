// -*-coding: mule-utf-8-unix; fill-column: 58; -*-
/**
 * @file
 * The dom node for usage from any thread.
 *
 * @author Sergei Lodyagin
 */

#include "Repository.hpp"
#include "SSingleton.hpp"
#include "dom.h"
#include "browser.h"

namespace shared {

std::ostream&
operator<< (std::ostream& out, const node_id_t& id)
{
  out << id.browser_id << ':';
  for (const auto k : id.path)
    out << '/' << k;
  return out;
}

std::istream&
operator>> (std::istream& in, node_id_t& id)
{
  in >> id.browser_id;
  SCHECK(in.get() == ':');

  id.path.reserve(10);
  while(in) {
    SCHECK(in.get() == '/');
    node_id_t::vector::value_type l;
    in >> l;
    id.path.push_back(l);
  }
  return in;
}

} // shared

namespace renderer {

std::ostream&
operator<<(std::ostream& out, const node_obj& nd)
{
  using namespace std;

  switch(nd.type)
  {
    case xpath::node_type::node:
    {
      // tag name
      out << '<' << nd.tag;

      // attributes
      for (const auto p : nd.attrs)
        out << ' ' << p.first << "=\"" << p.second << '"';
      out << '>';

      // bounding rect
      const CefRect& r = nd.bounding_rect;
      out << " [" << r.x << ", " << r.y << ", " << r.width
         << ", " << r.height << "]";

      return out;
    }

    default:
      THROW_NOT_IMPLEMENTED;
  }
}

node_repository::list_type node_repository
//
::create_several_objects(
  int browser_id,
  dom_visitor::query_base& param
)
{
  SCHECK(browser_id > 0);
  RLOCK(this->objectsM);
  current_browser_id = browser_id;
  return Spark::create_several_objects(param);
}


} // renderer


