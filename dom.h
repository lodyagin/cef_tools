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
#include "browser.h"

namespace renderer { namespace dom_visitor {
class query_base;
template<class Query>
class query;
}}

namespace shared {

//! Node identifier. It is 
//! xpath::child_path except the first (unknown) element
//! (it is absent here)
class node_id_t : public std::vector<ptrdiff_t>
{
public:
  using vector = std::vector<ptrdiff_t>;

  node_id_t() {}

  node_id_t(const ::xpath::child_path_t& p)
  {
    reserve(10);
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

  friend std::ostream&
  operator<<(std::ostream&, const node_obj&);

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
  struct result 
  {
    virtual ~result() {}
  };

  virtual ~query_base() 
  {
    LOG_TRACE(log, "dom_visitor::~query_base()");
  }

  virtual size_t n_objects(
    const curr::ObjectCreationInfo& oi
  ) = 0;

  virtual shared::node_id_t get_id(
    const curr::ObjectCreationInfo& oi
  ) const = 0;

  virtual shared::node_obj* create_next_derivation(
    const curr::ObjectCreationInfo& oi
  ) = 0;

  shared::node_obj* create_derivation
    (const curr::ObjectCreationInfo& oi) const
  { THROW_NOT_IMPLEMENTED; }

  shared::node_obj* transform_object
    (const shared::node_obj*) const
  { THROW_NOT_IMPLEMENTED; }

private:
  using log = curr::Logger<query_base>;
};

template<class Query>
class query
  : public query_base, 
    public Query
{
public:
  using xpath_query = Query;
  using xpath_query_result = typename Query::result;

  struct result : query_base::result, xpath_query_result {};

  node context;

  query(const Query& q) : Query(q) {}

  ~query()
  {
    LOG_TRACE(log, "dom_visitor::~query()");
  }

  size_t n_objects(const curr::ObjectCreationInfo& oi)
    override
  {
    LOG_TRACE(log, "n_objects");
    result = this->execute(context);
    return result.size(&cur);
  }

  shared::node_id_t get_id(
    const curr::ObjectCreationInfo& oi
  ) const override
  {
    LOG_TRACE(log, 
      "get_id " << cur.path()
      << " -> " << shared::node_id_t(cur.path())
    );
    return cur.path();
  }

  shared::node_obj* create_next_derivation(
    const curr::ObjectCreationInfo& oi
  ) override
  {
    LOG_TRACE(log, 
      "create_next_derivation "
      << "cur.path() == " << cur.path()
      << ", oi.objectId == " << oi.objectId
    );
//    assert((std::string) cur.path() == oi.objectId);
    auto obj = new shared::node_obj(
    /*FIXME*/ const_cast<typename Query::iterator&>
      (cur)
    );
    ++cur;
    return obj;
  }

  //! release all CefDOMNodes
  void release()
  {
    context = renderer::dom_visitor::node();
    result = xpath_query_result();
    cur = typename Query::iterator();
  }

protected:
  xpath_query_result result;
  typename Query::iterator cur;

private:
  using log = curr::Logger<query>;
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
protected:

template<class Query>
class DOMVisitor : public CefDOMVisitor
{
public:
  DOMVisitor(
    node_repository& rep,
    Query&& q
  ) 
    : node_rep(rep),
      query(std::move(q))
  {}

  // DOM is valid only inside this function
  // do not store DOM externally!
  void Visit(CefRefPtr<CefDOMDocument> d) override
  {
    query.context = renderer::dom_visitor::wrap
      (d->GetDocument());
    result_list = node_rep.create_several_objects(query);
    // reset the context to release the DOM
    query.release();
  }

  list_type get_result_list() const
  {
    return result_list;
  }

protected:
  node_repository& node_rep;
  Query query;
  list_type result_list;

private:
  IMPLEMENT_REFCOUNTING();
};

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

  template<class Query>
  list_type query(
    int browser_id,
    Query&& q
  )
  {
    CefRefPtr<CefDOMVisitor> visitor = 
      new DOMVisitor<Query>(*this, std::move(q));

    shared::browser_repository::instance()
      . get_object_by_id(browser_id) -> br
      -> GetMainFrame() -> VisitDOM
        (visitor); // takes ownership (ptr)
    return dynamic_cast<DOMVisitor<Query>*>
      (visitor.get())->get_result_list();
  }
};

} // renderer

#endif

