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
#include "SSingleton.h"
#include "xpath.h"

/*
namespace renderer { namespace dom_visitor { 
namespace xpath { namespace expr {

template<axis ax, class Test>
class path;

}}}}
*/

namespace shared {

class node_id_t : public std::vector<ptrdiff_t>
{
public:
  using vector = std::vector<ptrdiff_t>;

  node_id_t(const ::xpath::child_path_t& p)
    : vector(p.size() - 1)
  {
    auto bg = p.begin();
    std::copy(
      ++bg, p.end(), 
      std::back_inserter(*this)
    );
  }

  operator std::string() const
  {
    return curr::sformat(*this);
  }
};

std::ostream&
operator<< (std::ostream& out, const node_id_t& path);

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

  const node_id_t id;
};

}

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

class query_base
{
public:
  virtual ~query_base() {}
  
  virtual size_t n_objects(
    const curr::ObjectCreationInfo& oi
  ) = 0;

  virtual shared::node_id_t get_id(
    const curr::ObjectCreationInfo& oi
  ) const = 0;

  virtual shared::node* create_next_derivation(
    const curr::ObjectCreationInfo& oi
  ) = 0;

  shared::node* create_derivation
    (const curr::ObjectCreationInfo& oi) const
  { THROW_NOT_IMPLEMENTED; }

  shared::node* transform_object
    (const shared::node*) const
  { THROW_NOT_IMPLEMENTED; }
};

template<class Query>
class query : public query_base, public Query
{
public:
  query(const Query& q) : Query(q) {}

  size_t n_objects(const curr::ObjectCreationInfo& oi)
  {
    return ::xpath::step::size(*this, &cur);
  }

  shared::node_id_t get_id(
    const curr::ObjectCreationInfo& oi
  ) const override
  {
    return cur.path();
  }

  shared::node* create_next_derivation(
    const curr::ObjectCreationInfo& oi
  )
  {
    assert(cur.path() == oi.objectId);
    return new shared::node(cur++);
  }

protected:
  mutable typename Query::iterator cur;
};

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

} // renderer

class node_repository :
  public curr::SAutoSingleton<node_repository>,
  public curr::SparkRepository<
    shared::node, 
    dom_visitor::query_base,
    std::map,
    shared::node_id_t
  >
{
};

} // dom_visitor

namespace shared {

}

#endif

