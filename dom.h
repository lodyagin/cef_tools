// -*-coding: mule-utf-8-unix; fill-column: 58; -*-
/**
 * @file
 * The dom node for usage from any thread.
 *
 * @author Sergei Lodyagin
 */

#ifndef OFFSCREEN_DOM_H
#define OFFSCREEN_DOM_H

#include "Repository.h"
#include "xpath.h"

/*
namespace renderer { namespace dom_visitor { 
namespace xpath { namespace expr {

template<axis ax, class Test>
class path;

}}}}
*/

namespace shared {

class node_ptr;

class node
{
  template<class axis, class Test>
  friend class path;

public:
  using iterator_base = 
    xpath::node_iterators::iterator_base<node_ptr>;

protected:
  node(const iterator_base& it);
};

}//}

// for usage in renderer thread (process) only
namespace renderer {

// for usage only inside the CefDOMVisitor::Visit
namespace dom_visitor {

//! Adds conversion to bool for CefRefPtr
class wrap : public CefRefPtr<CefDOMNode>
{
public:
  using CefRefPtr::CefRefPtr;

  wrap(const CefRefPtr& o) : CefRefPtr(o) {}

  operator bool() const
  {
    return get();
  }
};

using node = ::xpath::node<wrap>;



template<
  class axis,
  template<class> class Test,
  class TestArg
>
::xpath::step::query<
    wrap,
    Test<::xpath::step::prim_iterator_t<wrap, axis>>,
    axis, 
    true
>
build_query(const node& ctx, TestArg&& test_arg, bool f)
{
  return ::xpath::step::build_query
    <wrap, axis, Test, TestArg>
  (
    ctx, 
    std::forward<TestArg>(test_arg),
    f
  );
}

template<
  template<class> class Test,
  class TestArg,
  class NestedQuery
>
::xpath::step::query<
    wrap, 
    Test<typename NestedQuery::iterator>,
    NestedQuery,
    false
>
build_query(
  TestArg&& test_arg,
  NestedQuery&& nested_query
)
{
  return ::xpath::step::build_query
    <wrap, Test, TestArg, NestedQuery>
  (
    std::forward<TestArg>(test_arg),
    std::forward<NestedQuery>(nested_query)
  );
}

}}

#endif

