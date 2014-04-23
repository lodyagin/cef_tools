// -*-coding: mule-utf-8-unix; fill-column: 58; -*-
/**
 * @file
 * The xpath-like CefDOMNode finder.
 *
 * @author Sergei Lodyagin
 */

//TODO end_t iterator is begin() - 1 with ovf reset

#ifndef OFFSCREEN_XPATH_H
#define OFFSCREEN_XPATH_H

#include <iostream>
#include <iterator>
#include <limits>
#include <algorithm>
#include <cctype>
#include <assert.h>
#include <list>
#include <utility>
#include "include/cef_dom.h"
#include "SCheck.h"
#include "SCommon.h"

//! [begin, end) overflow on iterator movement assertion
//! (it will always raise an exception on data access
//! anyway)
//#define XPART_OVF_ASSERT

#ifndef XPATH_OVF_ASSERT
#  define ovf_assert(x)
#else
#  define ovf_assert(x) assert(x)
#endif

namespace xpath 
{

//! A cycled iterator is that one which has
//! end().ovf_equal(begin()) == true
struct cycled_iterator_tag {};

struct cycled_bidirectional_iterator_tag 
  : std::bidirectional_iterator_tag,
    cycled_iterator_tag
{};

struct cycled_random_access_iterator_tag
  : std::random_access_iterator_tag,
    cycled_iterator_tag
{};

//! An xpath expression over Iterator. 
//! This version always returns `result'.
//! @tparam Iterator is node_iterators::iterator, not
//! xpath::iterator (the last will be created from this
//! expression) 
template<class Iterator, bool result>
class constant_expression
{
public:
/*
  template<class I, bool R>
  using the_templ = constant_expression<I, R>;
*/
  virtual bool operator()(Iterator it) const
  {
    return result;
  }
};

template<class It>
using true_expression = constant_expression<It, true>;

template<class It>
using false_expression = constant_expression<It, false>;

//! The special error value to mark uninitialized data.
template<class Int>
constexpr static Int uninitialized(Int)
{
  return std::numeric_limits<Int>::min();
}

//! xpath axes
enum class axis {
  ancestor,
  ancestor_or_self,
  attribute,
  child,
  descendant,
  descendant_or_self,
  following,
  following_sibling,
  namespace_axis,
  parent,
  preceding,
  preceding_sibling,
  self
};

template<class NodePtr>
class node;

using node_difference_type = ptrdiff_t;

class child_path_t : public std::list<node_difference_type>
{
public:
  using std::list<node_difference_type>::list;

  child_path_t(node_difference_type idx)
  {
    push_back(idx);
  }

  constexpr static value_type uninitialized()
  {
    return xpath::uninitialized<value_type>(0);
  }

  operator std::string() const
  {
    return curr::sformat(*this);
  }
};

std::ostream&
operator<< (std::ostream& out, const child_path_t& path);

//! The namespace for internal usage by the node class only
namespace node_iterators {

//! The end iterator marker for constructor
struct end_t {};

template<class NodePtr>
class iterator_base;

template<class NodePtr, xpath::axis axis> 
class iterator;

template<class Iterator>
class random_access_adapter;

}

template<class NodePtr>
std::ostream&
operator<< (std::ostream& out, const node<NodePtr>& nd);

//! The context node for all expressions. It's based on
//! underlying NodePtr (like CefDOMNode).
template<class NodePtr>
class node
{
  friend class node_iterators::iterator_base<NodePtr>;
  friend class node_iterators::iterator
    <NodePtr, axis::attribute>;

  friend std::ostream&
  operator<<<NodePtr> (
    std::ostream& out, 
    const node<NodePtr>& nd
  );

public:
  enum class type { not_valid, dom, attribute };

  template<xpath::axis axis>
  using iterator = node_iterators::random_access_adapter<
    node_iterators::iterator<NodePtr, axis>
  >;

  using self_iterator = iterator<axis::self>;
  using child_iterator = iterator<axis::child>;
  using descendant_iterator = iterator<axis::descendant>;
  using attribute_iterator = iterator<axis::attribute>;
  using following_sibling_iterator = 
    iterator<axis::following_sibling>;
  using preceding_sibling_iterator = 
    iterator<axis::preceding_sibling>;


  node(const node& o) = default;

  node(NodePtr dom_) : dom(dom_), the_type(type::dom)
  {
    SCHECK(dom);
  }

  node(NodePtr dom_, int attr) 
    : dom(dom_), the_type(type::attribute), attr_idx(attr)
  {
    SCHECK(dom);
  }

  node& operator=(const node& o) = default;

  template<
    xpath::axis ax, 
    template<class> class Expr = xpath::false_expression
  >
  class axis_;

#define XPATH_INTERNAL_AXIS_DEF(ax) \
  std::shared_ptr<                                      \
    axis_<xpath::axis::ax/*, xpath::false_expression*/>   \
  > ax() const;                                         \
                                                        \
  template<template<class> class Expr, class... Args>   \
  std::shared_ptr<axis_<xpath::axis::ax, Expr>>         \
  ax(Args&&... expr_args) const;

#define XPATH_INTERNAL_AXIS_DECL(ax) \
  template<class NodePtr>                                 \
  std::shared_ptr<node<NodePtr>::axis_<axis::ax>>         \
  node<NodePtr>::ax() const                               \
  {                                                       \
    return std::make_shared<axis_<xpath::axis::ax>>(dom); \
  }                                                       \
                                                          \
  template<class NodePtr>                                 \
  template<template<class> class Expr, class... Args>   \
  std::shared_ptr<                                        \
    node<NodePtr>::axis_<axis::ax, Expr>                  \
  >                                                       \
  node<NodePtr>::ax(Args&&... expr_args) const            \
  {                                                       \
    return std::make_shared<axis_<xpath::axis::ax>>       \
      (dom, std::forward<Args>(expr_args)...);            \
  }

  XPATH_INTERNAL_AXIS_DEF(self);
  XPATH_INTERNAL_AXIS_DEF(child);
  XPATH_INTERNAL_AXIS_DEF(descendant);
  XPATH_INTERNAL_AXIS_DEF(attribute);
  XPATH_INTERNAL_AXIS_DEF(following_sibling);
  XPATH_INTERNAL_AXIS_DEF(preceding_sibling);

  std::string tag_name() const
  {
    SCHECK(the_type != type::attribute);

    if (!dom->IsElement())
      return std::string();

    std::string res = dom->GetElementTagName().ToString();
    std::transform(
      res.begin(), 
      res.end(), 
      res.begin(),
      ::tolower
    );
    return res;
  }

  int n_attrs() const
  {
    if (!dom->IsElement())
      return 0;

    return dom->GetNumberOfElementAttributes();
  }

  /* attributes accessors */
    
  operator std::pair<std::string, std::string>() const
  {
    SCHECK(the_type == type::attribute);
    const int n = n_attrs();
    SCHECK(n > 0);
    assert(attr_idx >= 0);
    assert(attr_idx < n);

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

#if 0
  std::string name() const
  {
    SCHECK(the_type == type::attribute);

    CefString name, value;
    dom->GetElementAttributeByIdx(
      attr_idx, 
      name,
      value
    );
    return name.ToString();
  }
#endif

#if 0
  bool operator==(CefRefPtr<CefDOMNode> o) const
  {
    return (empty && o.get() == nullptr)
      || dom->IsSame(o);
  }

  bool operator!=(CefRefPtr<CefDOMNode> o) const
  {
    return !operator==(o);
  }
#endif

  NodePtr operator->() 
  { 
    assert(is_valid());
    return dom; 
  }

  const NodePtr operator->() const 
  { 
    assert(is_valid());
    return dom; 
  }

  operator NodePtr()
  {
    assert(is_valid());
    return dom;
  }

  operator const NodePtr() const
  {
    assert(is_valid());
    return dom;
  }

  //! Access the attribute value by name
  //! @return the attribute value or empty string
  std::string operator[](const std::string& name) const
  {
    check_load_attributes();
    const auto p = attr_map.find(name);
    return p != attr_map.end() ? p->second : std::string();
  }

  //! reloads itself with the first child if any or
  //! returns false 
  bool go_first_child() noexcept
  {
    if (const NodePtr first = dom->GetFirstChild()) {
      dom = first;
      return true;
    }
    else return false;
  }

  //! reloads itself with the last child if any or
  //! returns false 
  bool go_last_child() noexcept
  {
    if (const NodePtr last = dom->GetLastChild()) {
      dom = last;
      return true;
    }
    else return false;
  }

  //! reloads itself with the next sibling if any or
  //! returns false 
  bool go_next_sibling() noexcept
  {
    if (const NodePtr next = dom->GetNextSibling()) {
      dom = next;
      return true;
    }
    else return false;
  }

  //! reloads itself with the previous sibling if any or
  //! returns false 
  bool go_prev_sibling() noexcept
  {
    if (const NodePtr prev = dom->GetPreviousSibling()) {
      dom = prev;
      return true;
    }
    else return false;
  }

  //! reloads itself with the parent if any or
  //! returns false 
  bool go_parent() noexcept
  {
    if (const NodePtr parent = dom->GetParent()) {
      dom = parent;
      return true;
    }
    else return false;
  }

protected:
  bool is_valid() const
  {
    return the_type != type::not_valid
      && dom.get() != nullptr;
  }

  //! Loads all attributes into attr_map if it was empty
  //! only
  void check_load_attributes() const
  {
    if (!attr_map.empty())
      return;

    const int n = n_attrs();
    
    CefString name, value;
    for (int i = 0; i < n; i++) {
      dom->GetElementAttributeByIdx(i, name, value);
      auto p = attr_map.emplace(
        name.ToString(), 
        value.ToString()
      );
      if (!p.second) {
        LOG_WARN(log, "the " << tag_name() 
          << " tag have repeated attributes");
      }
    }
  }

  NodePtr dom;

  //! depth to context_node if it is a result of xpath
  //! expression or -1
  //int depth = -1;

  type the_type = type::not_valid;

  //! the attribute sequence number
  int attr_idx = 0; 

  //! name-values pairs for all attributes
  mutable std::map<std::string, std::string> attr_map;

private:
  using log = curr::Logger<node<NodePtr>>;
};

namespace node_iterators {

//! All axis iterators types are interoperable 
//! (can be safely reinterpret_cast-ed).
template<class NodePtr>
class iterator_base
{
public:
  using node_ptr_type = NodePtr;

  using difference_type = xpath::node_difference_type;
  using size_type = size_t;
  using value_type = xpath::node<NodePtr>;
  using pointer = xpath::node<NodePtr>*;
  using const_pointer = const xpath::node<NodePtr>*;
  using reference = xpath::node<NodePtr>;
  using const_reference = const xpath::node<NodePtr>;
  using iterator_category =
    cycled_bidirectional_iterator_tag;

  static_assert(
    sizeof(size_type) == sizeof(difference_type),
    "bad typedefs"
  );

  //! Two iterators are equal if both are empty or points
  //! to the same node
  //! as CefDOMNode::IsSame returns and their attr_idx are
  //! equal. NB Iterators can be
  //! equal even if two iterators have different context
  //! nodes or ever iterator types.
  bool operator==(const iterator_base& o) const noexcept
  {
    return (empty && o.empty)
      || (ovf == o.ovf 
          && current.attr_idx == o.current.attr_idx
          && current->IsSame(o.current));
  }

  bool operator!=(const iterator_base& o) const noexcept
  {
    return !operator==(o);
  }

  reference operator* ()
  {
    //todo switch node type, check attr range
    SCHECK(!empty);
    SCHECK(ovf == 0);
    return current;
  }

  const_reference operator* () const
  {
    SCHECK(!empty);
    SCHECK(ovf == 0);
    return current;
  }

  pointer operator->()
  {
    SCHECK(!empty);
    SCHECK(ovf == 0);
    return &current;
  }

  const_pointer operator->() const
  {
    SCHECK(!empty);
    SCHECK(ovf == 0);
    return &current;
  }

  child_path_t path() const
  {
    return child_path;
  }

  difference_type get_ovf() const
  {
    return ovf;
  }

protected:
  // Protecting ctrs makes this class "technical" only
  iterator_base() noexcept {}

  iterator_base(
    const node<NodePtr>& context_node,
    const node<NodePtr>& current_node,
    difference_type overflow,
    const child_path_t& child_path_
  ) noexcept
    : context(context_node), 
      current(current_node),
      child_path(child_path_),
      ovf(overflow),
      empty(false)
  {}

  //! for attribute axis only
  iterator_base(
    const node<NodePtr>& context_node,
    difference_type overflow,
    int attr_idx,
    const child_path_t& child_path_
  ) noexcept
    : context(context_node), 
      current(context_node, attr_idx),
      child_path(child_path_),
      ovf(overflow),
      empty(false)
  {}

  bool go_first_child()
  {
    if (current.go_first_child()) {
      child_path.push_back(0);
      LOG_TRACE(log, "FC: " << child_path);
      return true;
    }
    return false;
  }

  bool go_last_child()
  {
    if (current.go_last_child()) {
      child_path.push_back(
        current.preceding_sibling()->size()
      );
      LOG_TRACE(log, "LC: " << child_path);
      return true;
    }
    return false;
  }

  bool go_next_sibling()
  {
    if (current.go_next_sibling()) {
      ++(child_path.back());
      LOG_TRACE(log, "NS: " << child_path);
      return true;
    }
    return false;
  }

  bool go_prev_sibling()
  {
    if (current.go_prev_sibling()) {
      --(child_path.back());
      LOG_TRACE(log, "PS: " << child_path);
      return true;
    }
    return false;
  }

  //! reloads itself with the parent if any or
  //! returns false 
  bool go_parent()
  {
    if (current.go_parent()) {
      SCHECK(child_path.size() >= 1);
      child_path.pop_back();
      LOG_TRACE(log, "PAR^: " << child_path);
      return true;
    }
    else {
      assert(child_path.size() == 1);
      LOG_TRACE(log, "PAR-: " << child_path);
      return false;
    }
  }

  bool go_context_first_child()
  {
    current = context;
    // empty current path
    child_path = child_path_t::uninitialized();
    LOG_TRACE(log, "CTX!(fc)");
    return go_first_child();
  }

  bool go_context_last_child()
  {
    current = context;
    // empty current path
    child_path = child_path_t::uninitialized();
    LOG_TRACE(log, "CTX!(lc)");
    return go_last_child();
  }

  bool go_context()
  {
    current = context;
    // empty current path
    child_path = child_path_t::uninitialized();
    LOG_TRACE(log, "CTX!");
    return true;
  }

  //! Two iterators are ovf_equal if they points to the
  //! same node and/or attribute despite of ovf values.
  bool ovf_equal(const iterator_base& o) const noexcept
  {
    assert(!empty);
    assert(!o.empty);
    assert(context->IsSame(o.context));
    return current.attr_idx == o.current.attr_idx
      && current->IsSame(o.current);
  }

  //! The context node is not used by some iterators (e.g.,
  //! for a child axis) but keep all possible fields in the
  //! base class for not making virtual desctuctor.
  node<NodePtr> context;

  node<NodePtr> current;

  //! stores child indexes when go down. child index is
  //! the 0-based number in the child list,
  //! set by go_xxx methods
  child_path_t child_path;

  //! The end()+N (N>=0) iterator points to the same
  //! this->current as end()-1.
  difference_type ovf = 0;

  //! true if context and current are not initialized
  bool empty = true;

private:
  using log = curr::Logger<iterator_base>;
};

template<class NodePtr, xpath::axis axis>
class iterator
{
};

// an xpath self axis
template<class NodePtr>
class iterator<NodePtr, axis::self> 
  : public iterator_base<NodePtr>
{
  friend class iterator<NodePtr, axis::descendant>;

public:
  iterator() noexcept {}

  iterator& operator++() noexcept
  {
    ++(this->ovf);
    ovf_assert(this->ovf == 1);
    return *this;
  }

  iterator operator++(int) noexcept
  {
    iterator copy(*this);
    ++(*this);
    return copy;
  }

  iterator& operator--() noexcept
  {
    --(this->ovf);
    ovf_assert(this->ovf == 0);
    return *this;
  }

  iterator operator--(int) noexcept
  {
    iterator copy(*this);
    --(*this);
    return copy;
  }

protected:
  explicit iterator(const node<NodePtr>& context_node) 
    noexcept
    : iterator_base<NodePtr>(
        context_node, 
        context_node, 
        0,
        child_path_t::uninitialized()
      )
  {}

  iterator(const node<NodePtr>& context_node, end_t) 
    noexcept
    : iterator_base<NodePtr>(
        context_node, 
        context_node, 
        +1,
        child_path_t::uninitialized()
      )
  {}
};

// an xpath child axis
template<class NodePtr>
class iterator<NodePtr, axis::child> 
  : public iterator_base<NodePtr>
{
  friend class iterator<NodePtr, axis::descendant>;

public:
  iterator() noexcept {}

  iterator& operator++() noexcept
  {
    if (this->current->IsSame(this->context)) 
      // empty child axis
      ++(this->ovf);
    else {
      assert(
        this->current->GetParent()->IsSame(this->context)
      );
      if (!this->go_next_sibling()) {
        //cycled
        const bool res = this->go_context_first_child(); 
        assert(res);
        ++(this->ovf);
      }
    }
    ovf_assert(this->ovf);
    return *this;
  }

  iterator operator++(int) noexcept
  {
    iterator copy(*this);
    ++(*this);
    return copy;
  }

  iterator& operator--() noexcept
  {
    if (this->current->IsSame(this->context)) 
      // empty child axis
      --(this->ovf);
    else {
      assert(
        this->current->GetParent()->IsSame(this->context)
      );
      if (!this->go_prev_sibling()) {
        // cycled
        const bool res = this->go_context_last_child(); 
        assert(res);
        --(this->ovf);
      }
    }
    ovf_assert(this->ovf);
    return *this;
  }

  iterator operator--(int) noexcept
  {
    iterator copy(*this);
    --(*this);
    return copy;
  }

protected:
  explicit iterator(const node<NodePtr>& context_node) 
    noexcept
    : iterator_base<NodePtr>(
        context_node, 
        context_node->GetFirstChild().get()
          ? node<NodePtr>(context_node->GetFirstChild())
          : context_node,
        0,
        context_node->GetFirstChild().get() 
          ? 0 : child_path_t::uninitialized()
      )
  {}

  iterator(const node<NodePtr>& context_node, end_t) 
    noexcept
    : iterator_base<NodePtr>(
        context_node, 
        context_node->GetFirstChild().get()
          ? node<NodePtr>(context_node->GetFirstChild())
          : context_node,
        // begin == end for an empty axis
        context_node->GetFirstChild().get() ? +1 : 0,
        context_node->GetFirstChild().get() 
          ? 0 : child_path_t::uninitialized()
      )
  {}
};

// an xpath following_sibling axis
template<class NodePtr>
class iterator<NodePtr, axis::following_sibling> 
  : public iterator_base<NodePtr>
{
public:
  iterator() noexcept {}

  iterator& operator++() noexcept
  {
    if (!this->go_next_sibling())
    {
      // cycle to the context node + 1 with ovf
      this->go_context();
      this->go_next_sibling();
      ++(this->ovf);
    }
    ovf_assert(this->ovf);
    return *this;
  }

  iterator operator++(int) noexcept
  {
    iterator copy(*this);
    ++(*this);
    return copy;
  }

  iterator& operator--() noexcept
  {
    // it must be always after the context node
    SCHECK(this->go_prev_sibling());
    if (this->current->IsSame(this->context)) {
      // go to the last sibling
      while(this->go_next_sibling()) {
        LOG_TRACE(log, 
          "following_sibling: go_next_sibling()"
        );
      }
      --(this->ovf);
    }
    ovf_assert(this->ovf);
    return *this;
  }

  iterator operator--(int) noexcept
  {
    iterator copy(*this);
    --(*this);
    return copy;
  }

protected:
  explicit iterator(const node<NodePtr>& context_node) 
    noexcept
    : iterator_base<NodePtr>(
        context_node, 
        context_node->GetNextSibling().get() 
          ? node<NodePtr>(context_node->GetNextSibling())
          : context_node,
        0,
        child_path_t::uninitialized()
      )
  {}

  iterator(const node<NodePtr>& context_node, end_t) 
    noexcept
    : iterator_base<NodePtr>(
        context_node, 
        context_node->GetNextSibling().get() 
          ? node<NodePtr>(context_node->GetNextSibling())
          : context_node,
        context_node->GetNextSibling().get() ? +1 : 0,
        child_path_t::uninitialized()
      )
  {}

private:
  using log = curr::Logger<iterator>;
};

// an xpath preceding_sibling axis. NB It is reversed axis.
template<class NodePtr>
class iterator<NodePtr, axis::preceding_sibling> 
  : public iterator_base<NodePtr>
{
public:
  iterator() noexcept {}

  iterator& operator++() noexcept
  {
    if (!this->go_prev_sibling())
    {
      // cycle to the context node - 1 with ovf
      this->go_context();
      this->go_prev_sibling();
      ++(this->ovf);
    }
    ovf_assert(this->ovf);
    return *this;
  }

  iterator operator++(int) noexcept
  {
    iterator copy(*this);
    ++(*this);
    return copy;
  }

  iterator& operator--() noexcept
  {
    // it must be always before the context node
    SCHECK(this->go_next_sibling());
    if (this->current->IsSame(this->context)) {
      // go to the first sibling
      while(this->go_prev_sibling()) 
        ;
      --(this->ovf);
    }
    ovf_assert(this->ovf);
    return *this;
  }

  iterator operator--(int) noexcept
  {
    iterator copy(*this);
    --(*this);
    return copy;
  }

protected:
  explicit iterator(const node<NodePtr>& context_node) 
    noexcept
    : iterator_base<NodePtr>(
        context_node, 
        context_node->GetPreviousSibling().get() 
          ? node<NodePtr>(
              context_node->GetPreviousSibling()
            )
          : context_node,
        0,
        child_path_t::uninitialized()
      )
  {}

  iterator(const node<NodePtr>& context_node, end_t) 
    noexcept
    : iterator_base<NodePtr>(
        context_node, 
        context_node->GetPreviousSibling().get() 
          ? node<NodePtr>(
              context_node->GetPreviousSibling()
            )
          : context_node,
        context_node->GetPreviousSibling().get() ? +1 : 0,
        child_path_t::uninitialized()
      )
  {}
};

// an xpath descendant axis
template<class NodePtr>
class iterator<NodePtr, axis::descendant> 
  : public iterator_base<NodePtr>
{
public:
  iterator() noexcept {}

  iterator& operator++() noexcept
  {
    if (this->current->IsSame(this->context)) {
      ++(this->ovf);
      ovf_assert(this->ovf);
      LOG_TRACE(log, "O" << this->ovf);
    }

    // go to the child first
    if (this->go_first_child())
      ;
    // now try the next sibling
    else if (this->go_next_sibling())
      ;
    else {
      // now try the parents' next sibling
      while (!this->current->IsSame(this->context))
      {
        LOG_TRACE(log, 
          "[context = " << this->context.tag_name() 
          << ", this->current = " 
          << this->current.tag_name()
          << ']');
        if (this->go_parent())
        {
          if (this->current->IsSame(this->context))
            break; // go to end()

          if (this->go_next_sibling())
            break;
        } 
        else break;
      }
      if (this->current->IsSame(this->context)) {
        this->go_first_child(); //cycled
        ++(this->ovf);
      }
    }
    ovf_assert(this->ovf);
    return *this;
  }

  iterator operator++(int) noexcept
  {
    iterator copy(*this);
    ++(*this);
    return copy;
  }

  iterator& operator--()
  {
    THROW_NOT_IMPLEMENTED;
  }

  iterator operator--(int)
  {
    iterator copy(*this);
    --(*this);
    return copy;
  }

protected:
  explicit iterator(const node<NodePtr>& context_node) 
    noexcept
    : iterator_base<NodePtr>(
        context_node, 
        context_node->GetFirstChild().get()
          ? node<NodePtr>(context_node->GetFirstChild())
          : context_node,
        0,
        context_node->GetFirstChild().get() 
          ? child_path_t{child_path_t::uninitialized(), 0}
          : child_path_t{child_path_t::uninitialized()}
      )
  {}

  iterator(const node<NodePtr>& context_node, end_t) 
    noexcept
    : iterator_base<NodePtr>(
        context_node, 
        context_node->GetFirstChild().get()
          ? node<NodePtr>(context_node->GetFirstChild())
          : context_node,
        // begin == end for an empty axis
        context_node->GetFirstChild().get() ? +1 : 0,
        context_node->GetFirstChild().get() 
          ? child_path_t{child_path_t::uninitialized(), 0}
          : child_path_t{child_path_t::uninitialized()}
      )
  {}

private:
  typedef curr::Logger<iterator<NodePtr, axis::descendant>>
    log;
};

// an xpath attribute axis
template<class NodePtr>
class iterator<NodePtr, axis::attribute> 
  : public iterator_base<NodePtr>
{
public:
  iterator() noexcept {}

  iterator& operator++() noexcept
  {
    const int n = this->current.n_attrs();
    int& idx = this->current.attr_idx;
    if (++idx >= n)
    {
      idx = 0;
      ++(this->ovf);
    }
    ovf_assert(this->ovf);
    return *this;
  }

  iterator operator++(int) noexcept
  {
    iterator copy(*this);
    ++(*this);
    return copy;
  }

  iterator& operator--() noexcept
  {
    const int n = this->current.n_attrs();
    const int m = (n == 0) ? 0 : n - 1;
    int& idx = this->current.attr_idx;
    if (--idx < 0)
    {
      idx = m;
      --(this->ovf);
    }
    ovf_assert(this->ovf);
    return *this;
  }

  iterator operator--(int) noexcept
  {
    iterator copy(*this);
    --(*this);
    return copy;
  }

protected:
  explicit iterator(const node<NodePtr>& context_node) 
    noexcept
    : iterator_base<NodePtr>(
        context_node, 
        0, 
        0,
        child_path_t::uninitialized()
        )
  {}

  iterator(const node<NodePtr>& context_node, end_t) 
    noexcept
    : iterator_base<NodePtr>(
        context_node, 
        // if no attrs begin == end
        context_node.n_attrs() == 0 ? 0 : +1,
        0, //it is cycled
        child_path_t::uninitialized()
      )
  {}
};

template<class It>
typename random_access_adapter<It>::difference_type
operator-(
  const random_access_adapter<It>& a,
  const random_access_adapter<It>& b
);

// select cycled/uncycled tag based on the Iterator
// argument

template<class BaseTag, class Cycled = void>
struct random_access_adapter_iterator_category 
  : std::random_access_iterator_tag
{};

template<class BaseTag>
struct random_access_adapter_iterator_category<
  BaseTag,
  typename std::enable_if<
    std::is_base_of<cycled_iterator_tag, BaseTag>::value
  >::type
>
  : cycled_random_access_iterator_tag
{};

//! Makes a RandomAccessIterator 
//! from a BidirectionalIterator
template<class Iterator>
class random_access_adapter : public Iterator
{
  friend class xpath::node
    <typename Iterator::node_ptr_type>;

  friend 
  typename random_access_adapter<Iterator>::difference_type
  operator-<Iterator>(
    const random_access_adapter<Iterator>& a,
    const random_access_adapter<Iterator>& b
  );

public:
  using typename Iterator::difference_type;
  using typename Iterator::size_type;
  using typename Iterator::value_type;
  using typename Iterator::pointer;
  using typename Iterator::const_pointer;
  using typename Iterator::reference;
  using typename Iterator::const_reference;
  using iterator_category = 
    node_iterators::random_access_adapter_iterator_category
      <typename Iterator::iterator_category>;

  using Iterator::Iterator;

  random_access_adapter& operator++()
  {
    Iterator::operator++();
    return *this;
  }

  random_access_adapter operator++(int)
  {
    random_access_adapter copy(*this);
    ++(*this);
    return copy;
  }

  random_access_adapter& operator--()
  {
    Iterator::operator--();
    return *this;
  }

  random_access_adapter operator--(int)
  {
    random_access_adapter copy(*this);
    --(*this);
    return copy;
  }

  random_access_adapter& operator+=(difference_type n)
  {
    std::advance((Iterator&)*this, n);
    return *this;
  }

  random_access_adapter& operator-=(difference_type n)
  {
    return operator+=(-n);
  }

  random_access_adapter operator+(difference_type n) const
  {
    random_access_adapter copy(*this);
    return copy += n;
  }

  random_access_adapter operator-(difference_type n) const
  {
    random_access_adapter copy(*this);
    return copy -= n;
  }
};

template<class It>
typename random_access_adapter<It>::value_type
operator+(
  typename random_access_adapter<It>::difference_type n,
  typename random_access_adapter<It>::value_type it
)
{
  return it + n;
}

template<class It>
typename random_access_adapter<It>::difference_type
operator-(
  const random_access_adapter<It>& a,
  const random_access_adapter<It>& b
)
{
  // TODO specialized exceptions
  SCHECK(!a.empty);
  SCHECK(!b.empty);
  SCHECK(a.context->IsSame(b.context));

  typename It::difference_type cnt = 0;
  random_access_adapter<It> x = a;
  const auto old_ovf = x.ovf;
  while (!x.ovf_equal(b)) {
    if (x.ovf - old_ovf > 1) {
      // if context and iterator type is the same they
      // must be ovf_equal after finite number of steps
      THROW_PROGRAM_ERROR;
    }
    assert(x.ovf >= old_ovf);
    ++x;
    ++cnt;
  }
  //LOG_TRACE(log, "cnt = " << cnt);
  const auto ovf = x.ovf - old_ovf;
  //LOG_TRACE(log, "ovf = " << ovf);
  if (x.ovf == b.ovf) 
    return - cnt;
  else {
    // calculate a full cycle
    typename It::difference_type cnt2 = 0;

    do {
      ++x;
      ++cnt2;
    } while (!x.ovf_equal(a));

    const auto cycle = cnt + cnt2;
    return - (cnt + cycle * (b.ovf - a.ovf - ovf));
  }
}

}

//! The selective iterator. Selects only steps satisfied
//! to an expression.
//! @tparam iteration_lim limits number of steps used for
//! match search to prevent an infinite loop.
template<
  class It, 
  template<class> class Expr
>
class iterator  
{
public:
  using expr_t = Expr<It>;

  using iterator_category = 
    typename It::iterator_category; // NB
  using value_type = typename It::value_type;
  using difference_type = typename It::difference_type;
  using pointer = typename It::pointer;
  using reference = typename It::reference;

  static_assert(
    std::is_base_of<
      cycled_iterator_tag, 
      iterator_category
    >::value,
    "It parameter of xpath::iterator "
    "must be a cycled iterator "
    "(because of empty_iterval logic)"
  );

  //! constructs over the iterator, end limit and expression
  explicit iterator(It x, const expr_t& expr_) 
    : current(x), 
      expr(expr_),
      empty_interval(skip_unmatched())
  {
    assert(expr(current));
  }

  //! constructs over the iterator, end limit and expression
  explicit iterator(It x, expr_t&& expr_) 
    : current(x),
      expr(std::move(expr_)),
      empty_interval(skip_unmatched())
  {
    assert(expr(current));
  }

  It base() const
  {
    return current;
  }

  reference operator*() const
  {
    SCHECK(!empty_interval);
    assert(expr(current));
    return *current;
  }

  pointer operator->() const
  {
    return &(operator*());
  }

  iterator& operator++()
  {
    assert(expr(current));
    next_matched();
    return *this;
  }

  iterator operator++(int) &
  {
    iterator copy(*this);
    ++(*this);
    return copy;
  }

  iterator operator++(int) &&
  {
    return *this;
  }

  iterator& operator+=(difference_type n)
  {
    if (n > 0)
      while (n--) ++(*this);
    else 
      while (n++) --(*this);
    return *this;
  }

  iterator operator+(difference_type n) const &
  {
    iterator copy(*this);
    return copy += n;
  }

  iterator operator+(difference_type n) &&
  {
    return operator+=(n);
  }

  iterator& operator--()
  {
    assert(expr(current));
    prev_matched();
    return *this;
  }

  iterator operator--(int) &
  {
    iterator copy(*this);
    --(*this);
    return copy;
  }

  iterator operator--(int) &&
  {
    return *this;
  }

  iterator& operator-=(difference_type n)
  {
    return operator+=(-n);
  }

  iterator operator-(difference_type n) const &
  {
    iterator copy(*this);
    return copy -= n;
  }

  iterator operator-(difference_type n) &&
  {
    return operator-=(n);
  }

  reference operator[](difference_type n) const
  {
    return *(*this + n);
  }

protected:
  //! If current is not matched with expr forwards to the
  //! first matched (with possible one overflow). If no
  //! matches return false.
  bool skip_unmatched()
  {
    const auto max_ovf = current.get_ovf() + 1;
    while(!expr(current)) {
      ++current;
      if (current.get_ovf() > max_ovf)
        return false;
    }
    return true;
  }

  //! if current is not matched with expr backward to the
  //! last matched.
  void skip_unmatched_backward()
  {
    assert(!empty_interval);
    const auto min_ovf = current.get_ovf() - 1;
    while(!expr(current)) {

      --current;

      if (current.get_ovf() < min_ovf)
        THROW_PROGRAM_ERROR; 
        // must not be called when empty_iterval == true
    }
  }

  //! Unconditionally forwards to the
  //! next iterator position matched with expr. 
  //! NB do not check current.ovf, the
  //! result needs to be comparend with axis_::xend().
  void next_matched()
  {
    assert(!empty_interval);
    ++current;
    skip_unmatched();
  }

  //! Unconditionally backwards to the
  //! previous iterator position matched with expr. 
  //! NB do not check current.ovf, the
  //! result needs to be comparend with axis_::xend().
  void prev_matched()
  {
    assert(!empty_interval);
    --current;
    skip_unmatched_backward();
  }

  It current;

  expr_t expr;

  //! begin = end, the current value is inaccessible
  bool empty_interval;
};

template<class I, template<class> class Expr>
bool operator==(
  const iterator<I, Expr>& lhs, 
  const iterator<I, Expr>& rhs
)
{
  return lhs.empty_iterval == rhs.empty_interval
    && lhs.base() == rhs.base();
}

template<class I, template<class> class Expr>
bool operator!=(
  const iterator<I, Expr>& lhs, 
  const iterator<I, Expr>& rhs
)
{
  return ! operator==(lhs, rhs);
}

template<class NodePtr>
template<
  xpath::axis ax, 
  template<class> class Expr // = xpath::false_expression
>
class node<NodePtr>::axis_
{
public:
  using iterator = node::iterator<ax>;
  using xiterator = xpath::iterator<iterator, Expr>;
  using expr_t = Expr<iterator>;

  explicit axis_(node dom_) 
    : dom(dom_),
      expr(xpath::false_expression<iterator>())
  {}

  template<class... Args>
  explicit axis_(node dom_, Args&&... expr_args)
    : dom(dom_),
      expr(std::forward<Args>(expr_args)...)
  {}

  iterator begin()
  {
    return iterator(dom);
  }

  iterator end()
  {
    return iterator(
      dom, 
      node_iterators::end_t()
      );
  }

  iterator xbegin()
  {
    return xiterator(begin(), expr);
  }

  iterator xend()
  {
    return xiterator(end(), expr);
  }

  //! the number of nodes in the axis
  typename iterator::size_type size() //const
  {
    const auto dist = end() - begin();
    SCHECK(dist >= 0);
    return dist;
  }

  //! the number of only expr matched nodes
  typename iterator::size_type xsize() //const
  {
    const auto dist = xend() - xbegin();
    SCHECK(dist >= 0);
    return dist;
  }

protected:
  const node dom;
  const expr_t expr;
};

XPATH_INTERNAL_AXIS_DECL(self);
XPATH_INTERNAL_AXIS_DECL(child);
XPATH_INTERNAL_AXIS_DECL(descendant);
XPATH_INTERNAL_AXIS_DECL(attribute);
XPATH_INTERNAL_AXIS_DECL(following_sibling);
XPATH_INTERNAL_AXIS_DECL(preceding_sibling);

// Must be in the namespace for Koeing lookup
template<class NodePtr>
std::ostream&
operator<< (std::ostream& out, const node<NodePtr>& nd)
{
  using namespace std;

  switch(nd.the_type)
  {
    case node<NodePtr>::type::dom:
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

    case node<NodePtr>::type::attribute:
    {
      const pair<string, string> p(nd);
      return out << '{' << p.first 
                 << '=' << p.second << '}';
    }

    case node<NodePtr>::type::not_valid:
      return out << "(node is not valid)";

    default:
      THROW_NOT_IMPLEMENTED;
  }
}

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

namespace xpath {

using node = ::xpath::node<wrap>;

}

}}

#endif
