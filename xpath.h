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

  constexpr static auto attr_names =
   std::array<const char*, 9> 
  {
    "class"
    "id",
    "width",
    "height",
    "type",
    "data",
    "xmlns",
    "lang"
    "xml:lang"
  };

public:
  enum class type { not_initialized, dom, attribute };

  template<xpath::axis axis>
  using iterator = node_iterators::iterator<axis>;

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

#if 0
  int n_attrs() const
  {
    if (!dom->HasElementAttributes())
      return 0;

    CefDOMNode::AttributeMap attrs;
    dom->GetElementAttributes(attrs);
    return attrs.size();
  }

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
  //! as CefDOMNode::IsSame returns.. NB Iterators can be
  //! equal even if two iterators have different context
  //! nodes.
  bool operator==(const iterator_base& o) const noexcept
  {
    return (empty && o.empty)
      || (ovf == o.ovf 
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

#if 0
  operator pointer() 
  {
    //SCHECK(!empty);
    SCHECK(ovf == 0);
    return current;
  }

  operator const_pointer() const
  {
    //SCHECK(!empty); 
    SCHECK(ovf == 0);
    return current;
  }
#endif

protected:
  // Protecting ctrs makes this class "technical" only
  iterator_base() noexcept {}

  iterator_base(
    node context_node,
    node current_node,
    difference_type overflow = 0
  ) noexcept
    : context(context_node), 
      current(current_node),
      ovf(overflow),
      empty(false)
  {}

  // attribute
  iterator_base(node context_node, int attr_idx)
    : context(context_node), 
      current(context_node, attr_idx),
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
    : iterator_base(context_node, context_node)
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
    if (ovf < 0) {
      ovf_assert(false);
      ++ovf;
      assert(current->IsSame(context)
        || current->GetParent()->IsSame(context));
    }
    else if (const wrap next = current->GetNextSibling())
    {
      current = next;
      assert(current->GetParent()->IsSame(context));
    }
    else {
      ++ovf;
      ovf_assert(ovf == 1);
      assert(current->IsSame(context)
        || current->GetParent()->IsSame(context));
    }

    assert(current->GetParent()->IsSame(context));
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
    if (ovf > 0) {
      --ovf;
      ovf_assert(ovf == 0);
      assert(current->IsSame(context)
        || current->GetParent()->IsSame(context));
    }
    else if (const wrap prev = 
             current->GetPreviousSibling()
             )
    {
      current = prev;
//      std::cout << "1> " << current.tag_name() << std::endl;
      assert(current->GetParent()->IsSame(context));
    }
    else {
      ovf_assert(false);
      --ovf;
//      std::cout << "2> " << ovf << std::endl;
      assert(current->IsSame(context)
        || current->GetParent()->IsSame(context));
    }
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
        context_node->GetFirstChild().get() == nullptr
          ? +1 : 0
      )
  {}

  iterator(node context_node, end_t) noexcept
    : iterator_base(
        context_node, 
        context_node->GetLastChild().get() 
          ? node(context_node->GetLastChild())
          : context_node,
        +1
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
    if (ovf < 0) {
      ++ovf;
      ovf_assert(ovf == 0);
      //std::cout << "1O" << ovf << std::endl;
      return *this;
    }

    // go to the child first
    if (const wrap child = current->GetFirstChild())
    {
      //std::cout << "FC" << std::endl;
      current = child;
      return *this;
    }

    // now try the next sibling
    if (const wrap sibling = current->GetNextSibling())
    {
      //std::cout << "NS" << std::endl;
      current = sibling;
      return *this;
    }
    
    // now try the parent's next sibling
    while (!current->IsSame(context))
    {
      /*std::cout << "[context = " << context.tag_name() 
              << ", current = " << current.tag_name()
              << ']';*/
      if (const wrap parent = current->GetParent())
      {
        //std::cout << "^" << std::flush;
        current = parent;
        if (current->IsSame(context))
          break; // go to end()

        if (const wrap parent_sibling = 
            current->GetNextSibling())
        {
          //std::cout << "PNS" << node(parent_sibling).tag_name() << std::endl;
          current = parent_sibling;
          return *this;
        }
      } 
      else break;
    }

    // ok, we went to the end
    assert(current->IsSame(context));
    ++ovf;
    ovf_assert(ovf == 1);
    //std::cout << "2O" << ovf << std::endl;
    return *this = iterator(context, end_t());
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

  iterator(node context_node, node current_node) noexcept
    : iterator_base(context_node, current_node, 0)
  {}

  iterator(node context_node, end_t) noexcept
    : iterator_base(context_node, context_node, +1)
  {}

#if 0 // it can be used for axis::descendant_or_self
  //! get the last element of the iterator axis
  iterator axis_last()
  {
    auto last = iterator<axis::child>(context);
    decltype(last) begin;

    do {
      begin = last;
      last = --iterator<axis::child>(
        begin.current, 
        end_t()
      );
    } while (begin != last);

    return iterator<axis::descendant>(
      context, 
      last.current
    );
  }
#endif
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
    // while
    ++(current.attr_idx);
    std::cout << "attr_idx=" << current.attr_idx 
      << std::endl;
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
    --(current.attr_idx);
    std::cout << "attr_idx=" << current.attr_idx 
      << std::endl;
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
    : iterator_base(context_node, 0)
  {}

  iterator(node context_node, end_t) noexcept
    : iterator_base(context_node, context_node.attr_names.size())
  {}

  static_assert(node::attr_names.size() == 9, "FAIL");
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

inline std::shared_ptr<node::axis_<axis::self>> node::self() const
{
  return std::make_shared<axis_<xpath::axis::self>>(dom);
}

inline std::shared_ptr<node::axis_<axis::child>> node::child() const
{
  return std::make_shared<axis_<xpath::axis::child>>(dom);
}

inline std::shared_ptr<node::axis_<axis::descendant>> 
node::descendant() const
{
  return std::make_shared<axis_<xpath::axis::descendant>>(dom);
}

inline std::shared_ptr<node::axis_<axis::attribute>> 
node::attribute() const
{
  return std::make_shared<axis_<xpath::axis::attribute>>(dom);
}

}

//! prints <tag attrs...> of the node
std::ostream&
operator<< (std::ostream& out, CefRefPtr<CefDOMNode> dom);

// Must be in a namespace for Koeing lookup
namespace xpath {

inline std::ostream&
operator<< (std::ostream& out, const node& nd)
{
  switch(nd.the_type)
  {
    case node::type::dom:
      return out << (CefRefPtr<CefDOMNode>) nd.dom;
    case node::type::attribute:
      {
        const CefString name = nd.attr_names.at(nd.attr_idx);
        return out << '[' << name.ToString() << '=' 
          << nd.dom->GetElementAttribute(name).ToString() 
          << ']';
      }
    case node::type::not_initialized:
      return out << "(node not initialized)";
    default:
      THROW_NOT_IMPLEMENTED;
  }
}

//! prints matched tags
std::ostream&
operator<< (std::ostream& out, const select& sel);

}

#endif
