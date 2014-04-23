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
//namespace dom {

class node_ptr;

class node
{
  template<xpath::axis ax, class Test>
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

namespace xpath {

//! [14] Expr
namespace expr {

//! [1] LocationPath (both absolute and relative - it is
//! based on the context node passed to the constructor)
template<::xpath::axis ax, class Test>
class path // TODO multistep
{
public:
  using axis_t = 
    node::axis_t<ax, Test::template the_template>;

  template<class... Args>
  path(wrap ctx, Args&&... test_args) 
    : test(std::forward<Args>(test_args)...),
      context(ctx)
  {}

  size_t n_objects(const curr::ObjectCreationInfo& oi)
  {
    const auto n = axis_t(context).xsize(&cur);
    SCHECK(n >= 0);
    return (size_t) n;
  }

  shared::node* create_next_derivation(
    const curr::ObjectCreationInfo& oi
  )
  {
    assert(cur.get_ovf() == 0);
    return new shared::node(cur++);
  }

  shared::node* create_derivation
    (const curr::ObjectCreationInfo& oi) const
  { THROW_NOT_IMPLEMENTED; }

  shared::node* transform_object
    (const shared::node*) const
  { THROW_NOT_IMPLEMENTED; }

protected:
  Test test;
  //! the context node
  renderer::dom_visitor::node context;
  typename axis_t::xiterator cur;
};

} // expr

}

}}

#endif

