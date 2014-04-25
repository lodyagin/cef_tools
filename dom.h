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
#include <chrono>
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
class node_id_t
{
public:
  using vector = std::vector<ptrdiff_t>;

  node_id_t() {}

  node_id_t(
    int browser_id_,
    const ::xpath::child_path_t& child_path
  ) 
    : browser_id(browser_id_)
  {
    path.reserve(10);
    auto bg = child_path.begin();
    std::copy(
      ++bg, child_path.end(), 
      std::back_inserter(path)
    );
  }

  bool operator<(const node_id_t& o) const
  {
    if (browser_id != o.browser_id)
      return browser_id < o.browser_id;
    else
      return path < o.path;
  }

  operator std::string() const
  {
    return curr::sformat(*this);
  }

  int browser_id = 0;
  vector path;
};

std::ostream&
operator<< (std::ostream& out, const node_id_t& path);

std::istream&
operator>> (std::istream& in, node_id_t& path);

} // shared

namespace renderer {

class node_ptr;
class node_repository;

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

  //! time intervals 
  using duration = std::chrono::duration<int, std::milli>;

  /* screenshots */

  //! Takes the node screenshot into png
  //! with the path fname.
  void take_screenshot(
    const std::string& fname,
    bool prepend_timestamp = true
  );

  //! Takes the screenshot after secs seconds. Creates the
  //! thread for the take_screenshot() call and returns
  //! immediately.
  void take_screenshot_delayed(
    const std::string& fname,
    duration delay,
    bool prepend_timestamp = true
  );

  /* util methods */

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

  shared::node_id_t get_id() const
  {
    return id;
  }

protected:
  template<class It>
  node_obj(int browser_id, /*FIXME const*/ It& it)
    : id(browser_id, it.path()),
      type(it->get_type()),
      tag(it->tag_name()),
      bounding_rect((*it)->GetBoundingClientRect())
  {
    auto as = it->attribute();
    for (std::pair<std::string, std::string> p : *as)
      attrs.insert(p);
  }

  const shared::node_id_t id;
  const xpath::node_type type;
  const std::string tag;
  std::map<std::string, std::string> attrs;
  const CefRect bounding_rect;

private:
  using log = curr::Logger<node_obj>;
};

std::ostream&
operator<<(std::ostream& out, const node_obj& nd);

class node_ptr : public curr::RHolder<node_obj> {};

} // renderer

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

  virtual renderer::node_obj* create_next_derivation(
    const curr::ObjectCreationInfo& oi
  ) = 0;

  renderer::node_obj* create_derivation
    (const curr::ObjectCreationInfo& oi) const
  { THROW_NOT_IMPLEMENTED; }

  renderer::node_obj* transform_object
    (const renderer::node_obj*) const
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
  ) const override;

  renderer::node_obj* create_next_derivation(
    const curr::ObjectCreationInfo& oi
  ) override;

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
    renderer::node_obj, 
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
    int browser_id_,
    Query&& q
  ) 
    : node_rep(rep),
      query(std::move(q)),
      browser_id(browser_id_)
  {
    SCHECK(browser_id > 0);
  }

  // DOM is valid only inside this function
  // do not store DOM externally!
  void Visit(CefRefPtr<CefDOMDocument> d) override
  {
    
    query.context = renderer::dom_visitor::wrap
      (d->GetDocument());
    result_list = node_rep.create_several_objects
      (browser_id, query);
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
  int browser_id;

private:
  IMPLEMENT_REFCOUNTING();
};

public:
  using Spark = curr::SparkRepository<
    renderer::node_obj, 
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
  list_type query(int browser_id, Query&& q)
  {
    CefRefPtr<CefDOMVisitor> visitor = 
      new DOMVisitor<Query>
        (*this, browser_id, std::move(q));

    shared::browser_repository::instance()
      . get_object_by_id(browser_id) -> br
      -> GetMainFrame() -> VisitDOM
        (visitor); // takes ownership (ptr)
    return dynamic_cast<DOMVisitor<Query>*>
      (visitor.get())->get_result_list();
  }

  //! Adds current_browser_id (thread protected) set
  list_type create_several_objects(
    int browser_id,
    dom_visitor::query_base& param
  );

  int get_current_browser_id() const
  {
    return current_browser_id;
  }

protected:
  int current_browser_id = 0;
};

namespace dom_visitor {

template<class Query>
shared::node_id_t query<Query>
//
::get_id(
  const curr::ObjectCreationInfo& oi
) const
{
  const auto* rep = 
    dynamic_cast<const renderer::node_repository*>
    (oi.repository);
  SCHECK(rep);

  const auto id = shared::node_id_t(
    rep->get_current_browser_id(), cur.path()
    );

  LOG_TRACE(log, "get_id " << cur.path() << " -> " << id);
  return id;
}

template<class Query>
renderer::node_obj* query<Query>
//
::create_next_derivation(
  const curr::ObjectCreationInfo& oi
)
{
  const auto* rep = dynamic_cast<const node_repository*>
    (oi.repository);
  SCHECK(rep);
  SCHECK(rep->get_current_browser_id() > 0);

  LOG_TRACE(log, 
            "create_next_derivation "
            << "cur.path() == " << cur.path()
            << ", oi.objectId == " << oi.objectId
    );
  auto obj = new renderer::node_obj(
    rep->get_current_browser_id(),
    /*FIXME*/ const_cast<typename Query::iterator&>
    (cur)
    );
  ++cur;
  return obj;
}

} // dom_visitor

} // shared

#endif

