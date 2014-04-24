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
operator<< (std::ostream& out, const node_id_t& path)
{
  for (const auto k : path)
    out << '/' << k;
  return out;
}

std::istream&
operator>> (std::istream& in, node_id_t& path)
{
  path.reserve(10);
  while(in) {
    SCHECK(in.get() == '/');
    node_id_t::value_type l;
    in >> l;
    path.push_back(l);
  }
  return in;
}

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


}


