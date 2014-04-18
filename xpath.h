// -*-coding: mule-utf-8-unix; fill-column: 58; -*-
/**
 * @file
 * The xpath-like CefDOMNode finder.
 *
 * @author Sergei Lodyagin
 */

#ifndef OFFSCREEN_XPATH_H
#define OFFSCREEN_XPATH_H

#include <iostream>
#include <iterator>
#include <algorithm>
#include <cctype>
#include <assert.h>
#include "include/cef_dom.h"
#include "SCheck.h"

// [begin, end) overflow asswertiong
#if 1
#  define ovf_assert(x)
#else
#  define ovf_assert(x) assert(x)
#endif

// for usage in renderer thread (process) only
namespace renderer {

// for usage only inside the CefDOMVisitor::Visit
namespace dom_visitor {

namespace xpath 
{

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

//! The namespace for internal usage by the node class only
namespace node_iterators {

//! Adds conversion to bool for CefRefPtr
class wrap : public CefRefPtr<CefDOMNode>
{
public:
  wrap(const CefRefPtr& o)
    : CefRefPtr(o)
  {}

  using CefRefPtr<CefDOMNode>::CefRefPtr;

  operator bool() const
  {
    return get();
  }
};

//! The end iterator marker for constructor
struct end_t {};

class iterator_base;

template<xpath::axis axis> class iterator;

}

//! The context node for all expressions
class node
{
  friend class node_iterators::iterator_base;
  friend class node_iterators::iterator<axis::attribute>;

  friend std::ostream&
  operator<< (std::ostream& out, const node& nd);

public:
  enum class type { not_initialized, dom, attribute };

  template<xpath::axis axis>
  using iterator = node_iterators::iterator<axis>;

  node(const node& o) = default;

  node(node_iterators::wrap dom_) 
    : dom(dom_), the_type(type::dom)
  {
    SCHECK(dom);
    assert((dom.get() == nullptr) == 
      (the_type == type::not_initialized));
  }

  node(CefRefPtr<CefDOMNode> dom_) 
    : node(node_iterators::wrap(dom_))
  {}

  node(node_iterators::wrap dom_, int attr) 
    : dom(dom_), the_type(type::attribute), attr_idx(attr)
  {
    SCHECK(dom);
    assert((dom.get() == nullptr) == 
      (the_type == type::not_initialized));
  }

  node& operator=(const node& o) = default;

  template<xpath::axis ax>
  class axis_;

  std::shared_ptr<axis_<xpath::axis::self>> self() const;

  std::shared_ptr<axis_<xpath::axis::child>> child() const;

  std::shared_ptr<axis_<xpath::axis::descendant>> 
  descendant() const;

  std::shared_ptr<axis_<xpath::axis::attribute>> 
  attribute() const;

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

  size_t n_attrs() const
  {
    if (!dom->IsElement())
      return 0;

    return dom->GetNumberOfElementAttributes();
  }

  /* attributes accessors */
    
  operator std::pair<std::string, std::string>() const;

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

  node_iterators::wrap operator->() 
  { 
    SCHECK(the_type == type::dom);
    return dom; 
  }

  const node_iterators::wrap operator->() const 
  { 
    SCHECK(the_type == type::dom);
    return dom; 
  }

  operator node_iterators::wrap()
  {
    SCHECK(the_type != type::not_initialized);
    return dom;
  }

  operator const node_iterators::wrap() const
  {
    SCHECK(the_type != type::not_initialized);
    return dom;
  }

protected:
  //! It is protected 
  //! thus it's impossible declare "no node"
  node() noexcept : dom(nullptr) {}

  node_iterators::wrap dom;

  //! depth to context_node if it is a result of xpath
  //! expression or -1
  //int depth = -1;

  type the_type = type::not_initialized;

  //! the attribute sequence number
  int attr_idx = 0;
};

using self_iterator = node::iterator<axis::self>;
using child_iterator = node::iterator<axis::child>;
using descendant_iterator = 
  node::iterator<axis::descendant>;

namespace node_iterators {

//! All axis iterators types are interoperable 
//! (can be safely reinterpret_cast-ed).
class iterator_base
{
public:
  using difference_type = ptrdiff_t;
  using size_type = size_t;
  using value_type = xpath::node;
  using pointer = xpath::node*;
  using const_pointer = const xpath::node*;
  using reference = xpath::node;
  using const_reference = const xpath::node;
  using iterator_category = std::input_iterator_tag;

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
          && ((wrap)current)->IsSame(o.current));
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

protected:
  // Protecting ctrs makes this class "technical" only
  iterator_base() noexcept {}

  iterator_base(
    node context_node,
    node current_node,
    difference_type overflow
  ) noexcept
    : context(context_node), 
      current(current_node),
      ovf(overflow),
      empty(false)
  {}

  //! for attribute axis only
  iterator_base(
    node context_node,
    difference_type overflow,
    int attr_idx
  ) noexcept
    : context(context_node), 
      current(context_node, attr_idx),
      ovf(overflow),
      empty(false)
  {}

  // the context node is not used by some iterators (e.g.,
  // for a child axis) but keep all possible fields in the
  // base class for not making virtual desctuctor.
  node context;

  node current;

  // The end()+N (N>=0) iterator points to the same
  // this->current as end()-1.
  difference_type ovf = 0;

  //! true if context and current are not initialized
  bool empty = true;
};

template<xpath::axis axis>
class iterator
{
};

// an xpath self axis
template<>
class iterator<axis::self> : public iterator_base
{
  friend class xpath::node;
  friend class iterator<axis::descendant>;

public:
  iterator() noexcept {}

  iterator& operator++() noexcept
  {
    ++ovf;
    ovf_assert(ovf == 1);
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
    --ovf;
    ovf_assert(ovf == 0);
    return *this;
  }

  iterator operator--(int) noexcept
  {
    iterator copy(*this);
    --(*this);
    return copy;
  }

protected:
  explicit iterator(node context_node) noexcept
    : iterator_base(context_node, context_node, 0)
  {}

  iterator(node context_node, end_t) noexcept
    : iterator_base(context_node, context_node, +1)
  {}
};

// an xpath child axis
template<>
class iterator<axis::child> : public iterator_base
{
  friend class xpath::node;
  friend class iterator<axis::descendant>;

public:
  iterator() noexcept {}

  iterator& operator++() noexcept
  {
    if (current->IsSame(context)) // empty child axis
      ++ovf;
    else {
      assert(current->GetParent()->IsSame(context));
      if (const wrap next = current->GetNextSibling())
        current = next;
      else {
        current = context->GetFirstChild(); // cycled
        ++ovf;
      }
    }
    ovf_assert(ovf);
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
    if (current->IsSame(context)) // empty child axis
      --ovf;
    else {
      assert(current->GetParent()->IsSame(context));
      if (const wrap prev = current->GetPreviousSibling())
        current = prev;
      else {
        current = context->GetLastChild(); // cycled
        --ovf;
      }
    }
    ovf_assert(ovf);
    return *this;
  }

  iterator operator--(int) noexcept
  {
    iterator copy(*this);
    --(*this);
    return copy;
  }

protected:
  explicit iterator(node context_node) noexcept
    : iterator_base(
        context_node, 
        context_node->GetFirstChild().get()
          ? node(context_node->GetFirstChild())
          : context_node,
        0
      )
  {}

  iterator(node context_node, end_t) noexcept
    : iterator_base(
        context_node, 
        context_node->GetFirstChild().get()
          ? node(context_node->GetFirstChild())
          : context_node,
        // begin == end for an empty axis
        context_node->GetFirstChild().get() ? +1 : 0
      )
  {}
};

// an xpath descendant axis
template<>
class iterator<axis::descendant> : public iterator_base
{
  friend class xpath::node;

public:
  iterator() noexcept {}

  iterator& operator++() noexcept
  {
    if (current->IsSame(context)) {
      ++ovf;
      ovf_assert(ovf);
      LOG_TRACE(log, "O" << ovf);
    }

    // go to the child first
    if (const wrap child = current->GetFirstChild())
    {
      LOG_TRACE(log, "FC");
      current = child;
    }
    // now try the next sibling
    else if (const wrap sibling =current->GetNextSibling())
    {
      LOG_TRACE(log, "NS");
      current = sibling;
    }
    else {
      // now try the parents' next sibling
      while (!current->IsSame(context))
      {
        LOG_TRACE(log, "[context = " << context.tag_name() 
                  << ", current = " << current.tag_name()
                  << ']');
        if (const wrap parent = current->GetParent())
        {
          LOG_TRACE(log, "^");
          current = parent;
          if (current->IsSame(context))
            break; // go to end()

          if (const wrap parent_sibling = 
              current->GetNextSibling())
          {
            LOG_TRACE(log, 
              "PNS" << node(parent_sibling).tag_name());
            current = parent_sibling;
            break;
          }
        } 
        else break;
      }
    }

    return *this;
  }

  iterator operator++(int) noexcept
  {
    iterator copy(*this);
    ++(*this);
    return copy;
  }

protected:
  explicit iterator(node context_node) noexcept
    : iterator_base(
        context_node, 
        context_node->GetFirstChild().get()
          ? node(context_node->GetFirstChild())
          : context_node,
        context_node->GetFirstChild().get() == nullptr
      )
  {}

  iterator(node context_node, end_t) noexcept
    : iterator_base(
        context_node, 
        context_node,
        // begin == end for an empty axis
        context_node->GetFirstChild().get() ? 0 : +1
      )
  {}

private:
  typedef curr::Logger<iterator<axis::descendant>> log;
};

// an xpath attribute axis
template<>
class iterator<axis::attribute> : public iterator_base
{
  friend class xpath::node;

public:
  iterator() noexcept {}

  iterator& operator++() noexcept
  {
    const size_t n = current.n_attrs();
    if (n == 0 || (++(current.attr_idx) %= n) == 0)
      if (ovf >= 0) ++ovf;
    if (ovf < 0) ++ovf;
    ovf_assert(ovf);
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
    const size_t n = current.n_attrs();
    if (n == 0 || (--(current.attr_idx) %= n) == 0)
      if (ovf <= 0) --ovf;
    if (ovf > 0) --ovf;
    ovf_assert(ovf);
    return *this;
  }

  iterator operator--(int) noexcept
  {
    iterator copy(*this);
    --(*this);
    return copy;
  }

protected:
  explicit iterator(node context_node) noexcept
    : iterator_base(context_node, 0, 0)
  {}

  iterator(node context_node, end_t) noexcept
    : iterator_base(
        context_node, 
        // if no attrs begin == end
        context_node.n_attrs() == 0 ? 0 : +1,
        0 //it is cycled
      )
  {}
};

}

struct select
{
  select(
    const std::string& tag_,
    const CefRefPtr<CefDOMNode>& dom_
  );
      
  std::string tag;
  node context_node;
};

// the implementation

template<xpath::axis ax>
class node::axis_
{
public:
  using iterator = node::iterator<ax>;

  explicit axis_(node dom_)
    : dom(dom_)
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

protected:
  node dom;
};

// Must be in the namespace for Koeing lookup
std::ostream&
operator<< (std::ostream& out, const node& nd);

//! prints matched tags
std::ostream&
operator<< (std::ostream& out, const select& sel);

}}}

#endif
