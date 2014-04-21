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

//! [begin, end) overflow asswertiong
//#define XPART_OVF_ASSERT

#ifndef XPATH_OVF_ASSERT
#  define ovf_assert(x)
#else
#  define ovf_assert(x) assert(x)
#endif

namespace xpath 
{

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
  constexpr static value_type uninitialized()
  {
    return xpath::uninitialized<value_type>(0);
  }

  child_path_t(node_difference_type idx)
  {
    push_back(idx);
  }
};

//! The namespace for internal usage by the node class only
namespace node_iterators {

//! The end iterator marker for constructor
struct end_t {};

template<class NodePtr>
class iterator_base;

template<class NodePtr, xpath::axis axis> 
class iterator;

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
  using iterator = node_iterators::iterator<NodePtr, axis>;

  using self_iterator = iterator<axis::self>;
  using child_iterator = iterator<axis::child>;
  using descendant_iterator = iterator<axis::descendant>;
  using attribute_iterator = iterator<axis::attribute>;
  using following_sibling_iterator = 
    iterator<axis::following_sibling>;


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

  template<xpath::axis ax>
  class axis_;

  std::shared_ptr<axis_<xpath::axis::self>> self() const;

  std::shared_ptr<axis_<xpath::axis::child>> child() const;

  std::shared_ptr<axis_<xpath::axis::descendant>> 
  descendant() const;

  std::shared_ptr<axis_<xpath::axis::attribute>> 
  attribute() const;

  std::shared_ptr<axis_<xpath::axis::following_sibling>> 
  following_sibling() const;

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

  NodePtr dom;

  //! depth to context_node if it is a result of xpath
  //! expression or -1
  //int depth = -1;

  type the_type = type::not_valid;

  //! the attribute sequence number
  int attr_idx = 0; 
};

namespace node_iterators {

//! All axis iterators types are interoperable 
//! (can be safely reinterpret_cast-ed).
template<class NodePtr>
class iterator_base
{
public:
  using difference_type = xpath::node_difference_type;
  using size_type = size_t;
  using value_type = xpath::node<NodePtr>;
  using pointer = xpath::node<NodePtr>*;
  using const_pointer = const xpath::node<NodePtr>*;
  using reference = xpath::node<NodePtr>;
  using const_reference = const xpath::node<NodePtr>;
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

protected:
  // Protecting ctrs makes this class "technical" only
  iterator_base() noexcept {}

  iterator_base(
    const node<NodePtr>& context_node,
    const node<NodePtr>& current_node,
    difference_type overflow,
    difference_type child_idx
  ) noexcept
    : context(context_node), 
      current(current_node),
      child_path(child_idx),
      ovf(overflow),
      empty(false)
  {}

  //! for attribute axis only
  iterator_base(
    const node<NodePtr>& context_node,
    difference_type overflow,
    int attr_idx,
    difference_type child_idx
  ) noexcept
    : context(context_node), 
      current(context_node, attr_idx),
      child_path(child_idx),
      ovf(overflow),
      empty(false)
  {}

  bool go_first_child()
  {
    if (current.go_first_child()) {
      child_path.push_back(0);
      return true;
    }
    return false;
  }

  bool go_last_child()
  {
    if (current.go_last_child()) {
      child_path.push_back(current_child_idx());
      return true;
    }
    return false;
  }

  bool go_next_sibling()
  {
    return current.go_next_sibling()
      && (++(child_path.back()), true);
  }

  bool go_prev_sibling()
  {
    return current.go_prev_sibling()
      && (--(child_path.back()), true);
  }

  //! reloads itself with the parent if any or
  //! returns false 
  bool go_parent()
  {
    if (current.go_parent()) {
      SCHECK(child_path.size() >= 2);
      child_path.pop_back();
      return true;
    }
    else {
      assert(child_path.size() == 1);
      return false;
    }
  }

  bool go_context_first_child()
  {
    current = context;
    // empty current path
    child_path = child_path(child_path::uninitialized());
    return go_first_child();
  }

  bool go_context_last_child()
  {
    current = context;
    // empty current path
    child_path = child_path(child_path::uninitialized());
    return go_last_child();
  }

  bool go_context()
  {
    current = context;
    // empty current path
    child_path = child_path(child_path::uninitialized());
    return true;
  }

  //! The context node is not used by some iterators (e.g.,
  //! for a child axis) but keep all possible fields in the
  //! base class for not making virtual desctuctor.
  node<NodePtr> context;

  node<NodePtr> current;

  //! it 
//  difference_type child_idx = uninitialized(child_idx);

  //! stores child indexes when go down. child index is
  //! the 0-based number in the child list,
  //! set by go_xxx methods
  child_path_t child_path;

  //! The end()+N (N>=0) iterator points to the same
  //! this->current as end()-1.
  difference_type ovf = 0;

  //! true if context and current are not initialized
  bool empty = true;
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
  friend class xpath::node<NodePtr>;
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
  explicit iterator(const node<NodePtr>& context_node) noexcept
    : iterator_base<NodePtr>(context_node, context_node, 0)
  {}

  iterator(const node<NodePtr>& context_node, end_t) noexcept
    : iterator_base<NodePtr>(context_node, context_node, +1)
  {}
};

// an xpath child axis
template<class NodePtr>
class iterator<NodePtr, axis::child> 
  : public iterator_base<NodePtr>
{
  friend class xpath::node<NodePtr>;
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
      if (!go_next_sibling()) {
        const bool res = go_context_first_child(); //cycled
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
      if (!go_prev_sibling()) {
        const bool res = go_context_last_child(); // cycled
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
  explicit iterator(const node<NodePtr>& context_node) noexcept
    : iterator_base<NodePtr>(
        context_node, 
        context_node->GetFirstChild().get()
          ? node<NodePtr>(context_node->GetFirstChild())
          : context_node,
        0
      )
  {}

  iterator(const node<NodePtr>& context_node, end_t) noexcept
    : iterator_base<NodePtr>(
        context_node, 
        context_node->GetFirstChild().get()
          ? node<NodePtr>(context_node->GetFirstChild())
          : context_node,
        // begin == end for an empty axis
        context_node->GetFirstChild().get() ? +1 : 0
      )
  {}
};

// an xpath following_sibling axis
template<class NodePtr>
class iterator<NodePtr, axis::following_sibling> 
  : public iterator_base<NodePtr>
{
  friend class xpath::node<NodePtr>;

public:
  iterator() noexcept {}

  iterator& operator++() noexcept
  {
    if (!go_next_sibling())
    {
      // cycle to the context node with ovf
      go_context();
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
    if (!go_prev_sibling())
    {
      // cycle to the context with ovf
      go_context();
      ++(this->ovf);
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
        context_node,
        0,
        child_path::uninitialized()
      )
  {}

  iterator(const node<NodePtr>& context_node, end_t) 
    noexcept
    : iterator(context_node)
  {
    auto tmp = *this;
    ovf = tmp.go_next_sibling();
  }
};

// an xpath descendant axis
template<class NodePtr>
class iterator<NodePtr, axis::descendant> 
  : public iterator_base<NodePtr>
{
  friend class xpath::node<NodePtr>;

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
    if (go_first_child())
    {
      LOG_TRACE(log, "FC");
    }
    // now try the next sibling
    else if (go_next_sibling())
    {
      LOG_TRACE(log, "NS");
    }
    else {
      // now try the parents' next sibling
      while (!this->current->IsSame(this->context))
      {
        LOG_TRACE(log, 
          "[context = " << this->context.tag_name() 
          << ", this->current = " << this->current.tag_name()
          << ']');
        if (go_parent())
        {
          LOG_TRACE(log, "^");
          if (this->current->IsSame(this->context))
            break; // go to end()

          if (go_next_sibling())
          {
            LOG_TRACE(log, 
              "PNS" << this->current.tag_name()
            );
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
  explicit iterator(const node<NodePtr>& context_node) noexcept
    : iterator_base<NodePtr>(
        context_node, 
        context_node->GetFirstChild().get()
          ? node<NodePtr>(context_node->GetFirstChild())
          : context_node,
        context_node->GetFirstChild().get() == nullptr
      )
  {}

  iterator(const node<NodePtr>& context_node, end_t) noexcept
    : iterator_base<NodePtr>(
        context_node, 
        context_node,
        // begin == end for an empty axis
        context_node->GetFirstChild().get() ? 0 : +1
      )
  {}

private:
  typedef curr::Logger<iterator<NodePtr, axis::descendant>> log;
};

// an xpath attribute axis
template<class NodePtr>
class iterator<NodePtr, axis::attribute> 
  : public iterator_base<NodePtr>
{
  friend class xpath::node<NodePtr>;

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
  explicit iterator(const node<NodePtr>& context_node) noexcept
    : iterator_base<NodePtr>(context_node, 0, 0)
  {}

  iterator(const node<NodePtr>& context_node, end_t) noexcept
    : iterator_base<NodePtr>(
        context_node, 
        // if no attrs begin == end
        context_node.n_attrs() == 0 ? 0 : +1,
        0 //it is cycled
      )
  {}
};

}

#if 0
struct select
{
  select(
    const std::string& tag_,
    const CefRefPtr<CefDOMNode>& dom_
  );
      
  std::string tag;
  node<NodePtr> context_node;
};
#endif

// the implementation

template<class NodePtr>
template<xpath::axis ax>
class node<NodePtr>::axis_
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

  size_type size() const
  {
    const difference_type dist = end() - begin();
    SCHECK(dist >= 0);
    static_assert(
      sizeof(size_type) >= sizeof(difference_type),
      "bad typedefs"
    );
    return dist;
  }

protected:
  node dom;
};

template<class NodePtr>
std::shared_ptr<node<NodePtr>::axis_<axis::self>> 
node<NodePtr>::self() const
{
  return std::make_shared<axis_<xpath::axis::self>>(dom);
}

template<class NodePtr>
std::shared_ptr<node<NodePtr>::axis_<axis::child>> 
node<NodePtr>::child() const
{
  return std::make_shared<axis_<xpath::axis::child>>(dom);
}

template<class NodePtr>
std::shared_ptr<node<NodePtr>::axis_<axis::descendant>> 
node<NodePtr>::descendant() const
{
  return std::make_shared<axis_<xpath::axis::descendant>>
    (dom);
}

template<class NodePtr>
std::shared_ptr<node<NodePtr>::axis_<axis::attribute>> 
node<NodePtr>::attribute() const
{
  return std::make_shared<axis_<xpath::axis::attribute>>
    (dom);
}

template<class NodePtr>
std::shared_ptr<node<NodePtr>
::axis_<axis::following_sibling>> node<NodePtr>
//
::following_sibling() const
{
  return std::make_shared
    <axis_<xpath::axis::following_sibling>> (dom);
}

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

/*
//! prints matched tags
std::ostream&
operator<< (std::ostream& out, const select& sel);
*/

}

// for usage in renderer thread (process) only
namespace renderer {

// for usage only inside the CefDOMVisitor::Visit
namespace dom_visitor {

//! Adds conversion to bool for CefRefPtr
class wrap : public CefRefPtr<CefDOMNode>
{
public:
  wrap(const CefRefPtr& o) : CefRefPtr(o) {}

/*
  wrap& operator=(CefRefPtr o)
  {
    swap(o);
    return *this;
  }
*/

  using CefRefPtr::CefRefPtr;

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
