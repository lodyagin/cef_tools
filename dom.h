// -*-coding: mule-utf-8-unix; fill-column: 58; -*-
/**
 * @file
 * The dom node for usage from any thread.
 *
 * @author Sergei Lodyagin
 */

#ifndef OFFSCREEN_DOM_H
#define OFFSCREEN_DOM_H

#include <iostream>
#include "RHolder.h"
#include "Repository.h"
#include "SSingleton.h"
#include "xpath.h"

namespace renderer { namespace dom_visitor {
class query_base;
template<class Query>
class query;
}}

namespace shared {

class node_id_t : public std::vector<ptrdiff_t>
{
public:
  using vector = std::vector<ptrdiff_t>;

  node_id_t() {}

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

std::istream&
operator>> (std::istream& in, node_id_t& path);

class node_ptr;

class node_obj
{
  template<class Query>
  friend class ::renderer::dom_visitor::query;

public:
  using Par = ::renderer::dom_visitor::query_base;

  using iterator_base = 
    xpath::node_iterators::iterator_base<node_ptr>;

  std::string universal_id() const 
  { 
    return id;
  }

  bool IsElement() const
  {
    return type == xpath::node_type::node;
  }

  std::string GetElementTagName() const
  {
    return tag;
  }

  CefRect GetBoundingClientRect() const
  {
    return bounding_rect;
  }

protected:
  template<class It>
  node_obj(/*FIXME const*/ It& it)
    : id(it.path()),
      type(it->get_type()),
      tag(it->tag_name()),
      bounding_rect((*it)->GetBoundingClientRect())
  {
    auto as = it->attribute();
    for (std::pair<std::string, std::string> p : *as)
      attrs.insert(p);
  }

  const node_id_t id;
  const xpath::node_type type;
  const std::string tag;
  std::map<std::string, std::string> attrs;
  const CefRect bounding_rect;
};

std::ostream&
operator<<(std::ostream& out, const node_obj& nd);

class node_ptr : public curr::RHolder<node_obj> {};

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

  wrap() {}

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
  ) const = 0;

  virtual shared::node_id_t get_id(
    const curr::ObjectCreationInfo& oi
  ) const = 0;

  virtual shared::node_obj* create_next_derivation(
    const curr::ObjectCreationInfo& oi
  ) const = 0;

  shared::node_obj* create_derivation
    (const curr::ObjectCreationInfo& oi) const
  { THROW_NOT_IMPLEMENTED; }

  shared::node_obj* transform_object
    (const shared::node_obj*) const
  { THROW_NOT_IMPLEMENTED; }
};

template<class Query>
class query
  : public query_base, 
    public Query
{
public:
  using xpath_query = Query;
  using xpath_query_result = typename Query::result;

  node context;

  query(const Query& q) : Query(q) {}

  size_t n_objects(const curr::ObjectCreationInfo& oi)
    const override
  {
    result = this->execute(context);
    return result.size(&cur);
  }

  shared::node_id_t get_id(
    const curr::ObjectCreationInfo& oi
  ) const override
  {
    return cur.path();
  }

  shared::node_obj* create_next_derivation(
    const curr::ObjectCreationInfo& oi
  ) const override
  {
    assert((std::string) cur.path() == oi.objectId);
    auto obj = new shared::node_obj(
    /*FIXME*/ const_cast<typename Query::iterator&>
      (cur)
    );
    ++cur;
    return obj;
  }

protected:
  mutable xpath_query_result result;
  mutable typename Query::iterator cur;
};

template<
  class axis,
  template<class> class Test,
  class TestArg
>
query<
  ::xpath::step::query<
    wrap,
    Test<::xpath::step::prim_iterator_t<wrap, axis>>,
    axis, 
    true
  >
>
build_query(TestArg&& test_arg, bool f)
{
  return 
query<
  ::xpath::step::query<
    wrap,
    Test<::xpath::step::prim_iterator_t<wrap, axis>>,
    axis, 
    true
  >
>
  (::xpath::step::build_query
    <wrap, axis, Test, TestArg>
  (
    std::forward<TestArg>(test_arg),
    f
  ));
}

template<
  template<class> class Test,
  class TestArg,
  class NestedQuery
>
query<
  ::xpath::step::query<
    wrap, 
    Test<typename NestedQuery::iterator>,
    typename NestedQuery::xpath_query,
    false
  >
>
build_query(
  TestArg&& test_arg,
  NestedQuery&& nested_query
)
{
  return 
query<
  ::xpath::step::query<
    wrap, 
    Test<typename NestedQuery::iterator>,
    typename NestedQuery::xpath_query,
    false
  >
>
  (::xpath::step::build_query
    <wrap, Test, TestArg, typename NestedQuery::xpath_query>
  (
    std::forward<TestArg>(test_arg),
    std::forward<NestedQuery>(nested_query)
  ));
}

} // dom_visitor

class node_repository :
  public curr::SAutoSingleton<node_repository>,
  public curr::SparkRepository<
    shared::node_obj, 
    dom_visitor::query_base,
    std::map,
    shared::node_id_t
  >
{
public:
  using Spark = curr::SparkRepository<
    shared::node_obj, 
    dom_visitor::query_base,
    std::map,
    shared::node_id_t
  >;

  node_repository()
    : Spark("renderer::node_repository", 0)
  {
    this->complete_construction();
  }

  list_type query(
    int browser_id,
    const dom_visitor::query_base& q
  );
};

} // renderer

#endif

