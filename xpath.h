// -*-coding: mule-utf-8-unix; fill-column: 58; -*-
/**
 * @file
 * The xpath-like CefDOMNode finder.
 *
 * @author Sergei Lodyagin
 */

#ifndef OFFSCREEN_XPATH_H
#define OFFSCREEN_XPATH_H

//#include <iostream>
#include <iterator>
#include <algorithm>
#include <cctype>
#include <assert.h>
#include "include/cef_dom.h"
#include "SCheck.h"

namespace xpath 
{

class node;

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

class iterator_base
{
public:
  using difference_type = ptrdiff_t;
  using size_type = size_t;
  using value_type = xpath::node; //CefDOMNode;
  using pointer = wrap;
  using const_pointer = const wrap;
  using reference = xpath::node;
  using const_reference = const xpath::node;
  using iterator_category = std::input_iterator_tag;

  //! Two iterators are equal if points to the same node
  //! as CefDOMNode::IsSame returns or is_empty() is true
  //! for both. NB Iterators can be equal
  //! even if two iterators have different context nodes.
  bool operator==(const iterator_base& o) const noexcept
  {
    return (is_empty() && o.is_empty())
      || (end_ovf == o.end_ovf 
          && current->IsSame(o.current));
  }

  bool operator!=(const iterator_base& o) const noexcept
  {
    return !operator==(o);
  }

  reference operator* ()
  {
    assert(is_valid());
    SCHECK(!is_empty());
    SCHECK(end_ovf == 0);
    return current;
  }

  const_reference operator* () const
  {
    assert(is_valid());
    SCHECK(!is_empty());
    SCHECK(end_ovf == 0);
    return current;
  }

  value_type* operator->()
  {
    assert(is_valid());
    SCHECK(!is_empty());
    SCHECK(end_ovf == 0);
    return current.get();
  }

  const value_type* operator->() const
  {
    assert(is_valid());
    SCHECK(!is_empty());
    SCHECK(end_ovf == 0);
    return current.get();
  }

  operator pointer() 
  {
    assert(is_valid());
    SCHECK(end_ovf == 0);
    return current;
  }

  operator const pointer() const
  {
    assert(is_valid());
    SCHECK(end_ovf == 0);
    return current;
  }

protected:
  // Making this class "technical" only
  iterator_base(
    wrap context_node,
    wrap current_node,
    size_type after_end = 0
  ) noexcept
    : context(context_node), 
      current(current_node),
      end_ovf(after_end)
  {
    assert(is_valid());
  }

  //! It is true only for not
  //! initialized by a value
  bool is_empty() const noexcept
  { 
    assert(is_valid());
    return (bool) context;
  }
  
  // the context node is not used by some iterators (e.g.,
  // for a child axis) but keep all possible fields in the
  // base class for not making virtual desctuctor.
  wrap context;

  wrap current;

  // The end()+end_ovf iterator points to the same current
  // as end()-1.
  size_type end_ovf = 0;

  // for asserts only
  bool is_valid() const noexcept
  {
#if 1
    return (bool) context == (bool) current;
#else
    // null context -> null current
    return curr::implication(context, current);
#endif
  }
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
  iterator() noexcept
    : iterator_base(nullptr, nullptr)
  {}

  iterator& operator++() noexcept
  {
    assert(is_valid());
    ++end_ovf;
    assert(end_ovf);
    return *this;
  }

  iterator operator++(int) noexcept
  {
    iterator copy(*this);
    ++(*this);
    return copy;
  }

protected:
  explicit iterator(wrap context_node) 
    noexcept
    : iterator_base(context_node, context_node)
  {
    assert(context_node.get());
  }

  iterator(wrap context_node, end_t) noexcept
    : iterator_base(context_node, context_node, +1)
  {
    assert(context_node.get());
  }
};

// an xpath child axis
template<>
class iterator<axis::child> : public iterator_base
{
  friend class xpath::node;
  friend class iterator<axis::descendant>;

public:
  iterator() noexcept
    : iterator_base(nullptr, nullptr)
  {}

  iterator& operator++() noexcept
  {
    assert(is_valid());
    if (is_empty()) {
      ++end_ovf;
      return *this;
    }

    // NB not forward after the end()
    if (const wrap next = current->GetNextSibling())
      current = next;
    else {
      ++end_ovf;
      assert(end_ovf);
    }
    assert(current->GetParent() == context);
    return *this;
  }

  iterator operator++(int) noexcept
  {
    iterator copy(*this);
    ++(*this);
    return copy;
  }

protected:
  explicit iterator(wrap context_node) 
    noexcept
    : iterator_base(
        context_node, 
        context_node->GetFirstChild().get() 
          ? context_node->GetFirstChild() : context_node,
        context_node->GetFirstChild().get() == nullptr
      )
  {
    assert(context_node.get());
  }

  iterator(wrap context_node, end_t) noexcept
    : iterator_base(
        context_node, 
        context_node->GetLastChild().get() 
          ? context_node->GetLastChild() : context_node,
        +1
      )
  {
    assert(context_node.get());
  }
};

// an xpath descendant axis
template<>
class iterator<axis::descendant> : public iterator_base
{
  friend class xpath::node;

public:
  iterator() noexcept
    : iterator_base(nullptr, nullptr)
  {}

  iterator& operator++() noexcept
  {
    assert(is_valid());

    if (is_empty()) {
      ++end_ovf;
      return *this;
    }

    // go to the child first
    if (const wrap child = current->GetFirstChild())
    {
      current = child;
      return *this;
    }

    // now try the next sibling
    if (const wrap sibling = current->GetNextSibling())
    {
      current = sibling;
      return *this;
    }
    
    // now try the parent's next sibling
    if (*this != iterator<axis::self>(context)) {
      const wrap parent = current->GetParent();
      assert(parent);
      if (const auto parent_sibling = 
          parent->GetNextSibling()
          )
      {
        current = parent_sibling;
        return *this;
      }
    }

    // ok, we went to the end
    ++end_ovf;
    assert(end_ovf);
    return *this;
  }

  iterator operator++(int) noexcept
  {
    iterator copy(*this);
    ++(*this);
    return copy;
  }

protected:
  explicit iterator(wrap context_node) noexcept
    : iterator_base(
        context_node, 
        context_node->GetFirstChild().get() 
          ? context_node->GetFirstChild() : context_node,
        context_node->GetFirstChild().get() == nullptr
      )
  {
    assert(context_node.get());
  }

  iterator(wrap context_node, end_t) noexcept
    : iterator_base(
        context_node, 
        (pointer) iterator(context_node).axis_last(),
        +1
      )
  {
    assert(context_node.get());
  }

  //! get the last element of the iterator axis
  iterator axis_last()
  {
    auto ch = iterator<axis::child>(context);

    while (ch.end_ovf == 0) {
      ch = iterator<axis::child>((pointer) ch, end_t());
    }

    return iterator(ch.context);
  }
};

}

//! The context node for all expressions
class node : public wrap
{
public:
  template<xpath::axis axis>
  using iterator = node_iterators::iterator<axis>;

#if 0
  node(wrap dom_) : dom(dom_) 
  {
    SCHECK(dom.get());
  }
#else
  using wrap::wrap;
#endif

  template<xpath::axis ax>
  class axis
  {
  public:
    using iterator = node::iterator<ax>;

    explicit axis(node dom_)
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

  axis<xpath::axis::self> self() const
  {
    return axis<xpath::axis::self>(dom);
  }

  axis<xpath::axis::child> child() const
  {
    return axis<xpath::axis::child>(dom);
  }

  axis<xpath::axis::descendant> descendant() const
  {
    return axis<xpath::axis::descendant>(dom);
  }

  std::string tag_name() const
  {
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

//protected:
//  CefRefPtr<CefDOMNode> dom;
};

struct select
{
  select(
    const std::string& tag_,
    const CefRefPtr<CefDOMNode>& dom_
  );
      
  std::string tag;
  node context_node;
};

}

//! prints <tag attrs...> of the node
std::ostream&
operator<< (std::ostream& out, CefRefPtr<CefDOMNode> dom);

//! prints matched tags
std::ostream&
operator<< (std::ostream& out, const xpath::select& sel);

#endif
