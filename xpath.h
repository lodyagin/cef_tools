// -*-coding: mule-utf-8-unix; fill-column: 58; -*-
/**
 * @file
 * The xpath-like finder based on a generic node
 * with CefDOMNode-like interface.
 *
 * @author Sergei Lodyagin
 */

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

//! xpath axes
namespace axis {
struct  ancestor {};
struct  ancestor_or_self {};
struct  attribute {};
struct  child {};
struct  descendant {};
struct  descendant_or_self {};
struct  following {};
struct  following_sibling {};
struct  namespace_axis {};
struct  parent {};
struct  preceding {};
struct  preceding_sibling {};
struct  self {};
}

//! [38] (xpath) NodeType
enum class node_type { 
  comment, 
  text, 
  processing_instruction,
  node,
  other_type,
  unknown_type
};

//! [7] NodeTest
namespace test {

//! An xpath test over Iterator. 
//! This version always returns `result'.
//! @tparam Iterator is node_iterators::iterator, not
//! xpath::iterator (the last will be created from this
//! test) 
template<class Iterator>
class constant
{
public:
  template<class I>
  using the_template = constant<I>;

  constant(bool res = false) : result(res) {}
  
  bool operator()(Iterator it) const
  {
    return result;
  }

protected:
  bool result;
};

//! a NodeType test
template<class It>
class node_type
{
public:
  template<class I>
  using the_template = node_type<I>;

  using type = xpath::node_type;

  node_type() : the_type(node_type::unknown_type) {}
  node_type(type a_type) : the_type(a_type) {}

  bool operator()(It it) const
  {
    LOG_TRACE(log, "xpath::test::node_type: "
      << (int)it->get_type() << " vs " << (int)the_type
    );
    return it->get_type() == the_type;
  }

protected:
  type the_type;

private:
  using log = curr::Logger<node_type>;
};

//! [37] NameTest
template<class It>
class name
{
public:
  template<class I>
  using the_template = name<I>;

  name() : inited(false) {}

  name(const std::string& nm) 
    : inited(true),
      the_name(nm) 
  {}

  name(std::string&& nm) 
    : inited(true),
      the_name(std::move(nm)) 
  {}

  bool operator()(It it) const
  {
    LOG_TRACE(log, "xpath::test::name: "
      << it->tag_name() << " vs " << the_name
    );
    return it->tag_name() == the_name;
  }

protected:
  bool inited;
  std::string the_name;

private:
  using log = curr::Logger<name>;
};

//! Test with a callback function
template<class It>
class fun
{
public:
  template<class I>
  using the_template = fun<I>;

  using generic_iterator =
    typename It::value_type::generic_iterator;

  using function_t = std::function<
    bool(const generic_iterator&)
  >;

  fun() 
    : fun([](const generic_iterator&){ return false; })
  {}

  fun(const function_t& f) : function(f) {}
    
  bool operator()(const It& it) const
  {
    return function(it.base());
  }

protected:
  function_t function;
};

} // test

//! The special error value to mark uninitialized data.
template<class Int>
constexpr static Int uninitialized(Int)
{
  return std::numeric_limits<Int>::min();
}

template<class NodePtr>
class node;

using node_difference_type = ptrdiff_t;

class child_path_t : public std::list<node_difference_type>
{
public:
  using std::list<node_difference_type>::list;

  child_path_t() {}

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

//! The namespace for internal usage by xpath
namespace node_iterators {

//! The end iterator marker for constructor
struct end_t {};

template<class NodePtr>
class iterator_base;

template<class NodePtr, class axis> 
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
  using type = xpath::node_type;

  using generic_iterator = node_iterators::iterator_base
    <NodePtr>;

  //! internal type
  enum class itype { not_valid, dom, attribute };

  template<class axis>
  using iterator = node_iterators::iterator<NodePtr, axis>;

#define XPATH_INTERNAL_ITERATOR_ALIAS(ax) \
  using ax ## _iterator =                       \
    node_iterators::random_access_adapter<      \
      iterator<axis::ax>                        \
    >;

  XPATH_INTERNAL_ITERATOR_ALIAS(self);
  XPATH_INTERNAL_ITERATOR_ALIAS(child);
  XPATH_INTERNAL_ITERATOR_ALIAS(descendant);
  XPATH_INTERNAL_ITERATOR_ALIAS(attribute);
  XPATH_INTERNAL_ITERATOR_ALIAS(following_sibling);
  XPATH_INTERNAL_ITERATOR_ALIAS(preceding_sibling);

  node() {}

  node(const node& o) = default;

  node(NodePtr dom_) : dom(dom_), the_type(itype::dom)
  {
    SCHECK(dom);
  }

  node(NodePtr dom_, int attr) 
    : dom(dom_), the_type(itype::attribute), attr_idx(attr)
  {
    SCHECK(dom);
  }

  node& operator=(const node& o) = default;

  template<
    class ax, 
    template<class> class Test = xpath::test::constant
  >
  class axis_t;

#define XPATH_INTERNAL_AXIS_DEF(ax) \
  std::shared_ptr<                                      \
    axis_t<xpath::axis::ax>                              \
  > ax() const;                                         \
                                                        \
  template<template<class> class Test, class... Args>   \
  std::shared_ptr<axis_t<xpath::axis::ax, Test>>         \
  ax(Args&&... test_args) const;

#define XPATH_INTERNAL_AXIS_DECL(ax) \
  template<class NodePtr>                                 \
  std::shared_ptr<node<NodePtr>::axis_t<axis::ax>>         \
  node<NodePtr>::ax() const                               \
  {                                                       \
    return std::make_shared<axis_t<xpath::axis::ax>>(dom); \
  }                                                       \
                                                          \
  template<class NodePtr>                                 \
  template<template<class> class Test, class... Args>   \
  std::shared_ptr<                                        \
    node<NodePtr>::axis_t<axis::ax, Test>                  \
  >                                                       \
  node<NodePtr>::ax(Args&&... test_args) const            \
  {                                                       \
    return std::make_shared<axis_t<xpath::axis::ax, Test>> \
      (dom, std::forward<Args>(test_args)...);            \
  }

  XPATH_INTERNAL_AXIS_DEF(self);
  XPATH_INTERNAL_AXIS_DEF(child);
  XPATH_INTERNAL_AXIS_DEF(descendant);
  XPATH_INTERNAL_AXIS_DEF(attribute);
  XPATH_INTERNAL_AXIS_DEF(following_sibling);
  XPATH_INTERNAL_AXIS_DEF(preceding_sibling);

  type get_type() const
  {
    SCHECK(dom);
    switch(dom->GetType())
    {
      case DOM_NODE_TYPE_COMMENT:
        return type::comment;
      case DOM_NODE_TYPE_TEXT:
        return type::text;
      case DOM_NODE_TYPE_PROCESSING_INSTRUCTIONS:
        return type::processing_instruction;
      case DOM_NODE_TYPE_ELEMENT:
        return type::node;
      default:
        return type::other_type;
    }
  }

  std::string tag_name() const
  {
    SCHECK(the_type != itype::attribute);

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
    SCHECK(the_type == itype::attribute);
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
    SCHECK(the_type == itype::attribute);

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

  bool is_valid() const
  {
    return the_type != itype::not_valid
      && dom.get() != nullptr;
  }

protected:
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

  itype the_type = itype::not_valid;

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
//    SCHECK(ovf == 0);
    return current;
  }

  const_reference operator* () const
  {
    SCHECK(!empty);
//    SCHECK(ovf == 0);
    return current;
  }

  pointer operator->()
  {
    SCHECK(!empty);
//    SCHECK(ovf == 0);
    return &current;
  }

  const_pointer operator->() const
  {
    SCHECK(!empty);
//    SCHECK(ovf == 0);
    return &current;
  }

  child_path_t path() const
  {
    return child_path;
  }

  const iterator_base& base() const
  {
    return *this;
  }

  bool is_empty() const {
    return empty;
  }

  bool is_same_context(const iterator_base& o) const
  {
    return context->IsSame(o.context);
  }

  difference_type& get_ovf()
  {
    return ovf;
  }

  difference_type get_ovf() const
  {
    return ovf;
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

  //! The context node is not used by some iterators (e.g.,
  //! for a child axis) but keep all possible fields in the
  //! base class for not making virtual desctuctor.
  node<NodePtr> context;

  node<NodePtr> current;

  //! stores child indexes when go down. child index is
  //! the 0-based number in the child list,
  //! set by go_xxx methods
  child_path_t child_path;

  //! The end()+N iterator points to the same
  //! this->current as begin()+N+cycle_size
  difference_type ovf = 0;

  //! true if context and current are not initialized
  bool empty = true;

private:
  using log = curr::Logger<iterator_base>;
};

template<class NodePtr, class axis>
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
//  SCHECK(!a.is_empty());
//  SCHECK(!b.is_empty());
  SCHECK(a.is_same_context(b));

  typename It::difference_type cnt = 0;
  random_access_adapter<It> x = a;
  const auto old_ovf = x.get_ovf();
  while (!x.ovf_equal(b)) {
    if (x.get_ovf() - old_ovf > 1) {
      // if context and iterator type is the same they
      // must be ovf_equal after finite number of steps
      THROW_PROGRAM_ERROR;
    }
    assert(x.get_ovf() >= old_ovf);
    ++x;
    ++cnt;
  }
  //LOG_TRACE(log, "cnt = " << cnt);
  const auto ovf = x.get_ovf() - old_ovf;
  //LOG_TRACE(log, "ovf = " << ovf);
  if (x.get_ovf() == b.get_ovf()) 
    return - cnt;
  else {
    // calculate a full cycle
    typename It::difference_type cnt2 = 0;

    do {
      ++x;
      ++cnt2;
    } while (!x.ovf_equal(a));

    const auto cycle = cnt + cnt2;
    return - (cnt + cycle * (b.get_ovf() - a.get_ovf() - ovf));
  }
}

}

//! The selective iterator. Selects only steps satisfied
//! to a  test.
template<
  class It, 
  template<class> class Test
>
class iterator  
{
public:
  using test_t = Test<It>;

  using node_ptr_type = typename It::node_ptr_type;
  using value_type = typename It::value_type;
  using difference_type = typename It::difference_type;
  using pointer = typename It::pointer;
  using reference = typename It::reference;
  using size_type = typename It::size_type;
  using const_pointer = typename It::const_pointer;
  using const_reference = typename It::const_reference;

  static_assert(
    std::is_base_of<
      cycled_iterator_tag, 
      typename It::iterator_category
    >::value,
    "It parameter of xpath::iterator "
    "must be a cycled iterator "
    "(because of empty_interval logic)"
  );
  
  // NB hides random access from It
  using iterator_category = 
    cycled_bidirectional_iterator_tag;

  iterator() : empty_interval(true) {}

  //! constructs over the iterator, end limit and test
  explicit iterator(It x, const test_t& test_) 
    : current(x), 
      test(test_),
      empty_interval(!skip_unmatched())
  {
    assert(empty_interval || test(current));
  }

  //! constructs over the iterator, end limit and test
  explicit iterator(It x, test_t&& test_) 
    : current(x),
      test(std::move(test_)),
      empty_interval(!skip_unmatched())
  {
    assert(empty_interval || test(current));
  }

  //! constructs over an end iterator, end limit and test
  //! an end() iterator must be passed as x!
  explicit iterator(
    It x, 
    const test_t& test_, 
    node_iterators::end_t
  ) 
    : iterator(x, test_)
  {
    if (empty_interval && !current.is_empty()) 
      --current.get_ovf();
  }

  //! constructs over an end iterator, end limit and test
  //! an end() iterator must be passed as x!
  explicit iterator(
    It x, 
    test_t&& test_, 
    node_iterators::end_t
  ) 
    : iterator(x, std::move(test_))
  {
    if (empty_interval && !current.is_empty()) 
      --current.get_ovf();
  }

  child_path_t path() const
  {
    return current.path();
  }

  const It& base() const
  {
    return current;
  }

  // forward calls to current

  bool is_empty() const {
    return empty_interval; // NB
  }

  bool is_same_context(const iterator& o) const
  {
    return current.is_same_context(o.current);
  }

  difference_type& get_ovf()
  {
    return current.get_ovf();
  }

  difference_type get_ovf() const
  {
    return current.get_ovf();
  }

  bool ovf_equal(const iterator& o) const
  {
    return current.ovf_equal(o.current);
  }

  reference operator*()
  {
    SCHECK(!empty_interval);
    assert(test(current));
    return *current;
  }

  pointer operator->()
  {
    SCHECK(!empty_interval);
    assert(test(current));
    return current.operator->();
  }

  iterator& operator++()
  {
    assert(empty_interval || test(current));
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

#if 0
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
#endif

  iterator& operator--()
  {
    assert(test(current));
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

  bool operator==(const iterator& o) const
  {
    return (empty_interval && o.empty_interval)
      || (base() == o.base());
  }

  bool operator!=(const iterator& o) const
  {
    return !operator==(o);
  }

#if 0
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
#endif

protected:
  //! If current is not matched with test forwards to the
  //! first matched (with possible one overflow). If no
  //! matches return false.
  bool skip_unmatched()
  {
    if (current.is_empty())
      return false;

    const auto max_ovf = current.get_ovf() + 1;
    while(!test(current)) {
      ++current;
      if (current.get_ovf() > max_ovf)
      {
        --(current.get_ovf());
        return false;
      }
    }
    return true;
  }

  //! if current is not matched with test backward to the
  //! last matched.
  void skip_unmatched_backward()
  {
    assert(!empty_interval);
    const auto min_ovf = current.get_ovf() - 1;
    while(!test(current)) {

      --current;

      if (current.get_ovf() < min_ovf)
        THROW_PROGRAM_ERROR; 
        // must not be called when empty_iterval == true
    }
  }

  //! Unconditionally forwards to the
  //! next iterator position matched with test. 
  //! NB do not check current.ovf, the
  //! result needs to be comparend with axis_t::xend().
  void next_matched()
  {
    //assert(!empty_interval);
    ++current;
    skip_unmatched();
  }

  //! Unconditionally backwards to the
  //! previous iterator position matched with test. 
  //! NB do not check current.ovf, the
  //! result needs to be comparend with axis_t::xend().
  void prev_matched()
  {
    assert(!empty_interval);
    --current;
    skip_unmatched_backward();
  }

  It current;

  test_t test;

  //! begin = end, the current value is inaccessible
  bool empty_interval;
};

//! [4] Step
namespace step {

template<class NodePtr, class axis>
using prim_iterator_t =
  node_iterators::random_access_adapter<
    node_iterators::iterator<NodePtr, axis>
  >;

template<class BaseIt, class Test>
using pred_iterator_t =
  node_iterators::random_access_adapter<
    xpath::iterator<BaseIt, Test::template the_template>
  >;

template<class NodePtr, class axis, class Test>
using step1_iterator_t = pred_iterator_t<
  prim_iterator_t<NodePtr, axis>,
  Test
>;

template<
  class NodePtr, 
  class axis, 
  class... Predicates
>
struct iterator
{
};

template<
  class NodePtr, 
  class axis, 
  class Test
>
struct iterator<NodePtr, axis, Test>
  : step1_iterator_t<NodePtr, axis, Test>
{
  using prim_iterator = prim_iterator_t<NodePtr, axis>;
  using step1_iterator = 
    step1_iterator_t<NodePtr, axis, Test>;

  iterator() {}

  iterator(
    const node<NodePtr>& context, 
    const Test& test
  )
    : step1_iterator(
        prim_iterator(context), 
        test
      )
  {}

  iterator(
    node_iterators::end_t end,
    const node<NodePtr>& context, 
    const Test& test
  )
    : step1_iterator(
        prim_iterator(context, end), 
        test,
        end
      )
  {}
};

template<
  class NodePtr, 
  class axis, 
  class Predicate0,
  class... Predicates // the last is Test
>
struct iterator
  <NodePtr, axis, Predicate0, Predicates...>
    : pred_iterator_t<
        iterator<NodePtr, axis, Predicates...>,
        Predicate0
      >
{
  using nested_iterator = iterator
    <NodePtr, axis, Predicates...>;
  using pred_iterator = pred_iterator_t<
    iterator<NodePtr, axis, Predicates...>,
    Predicate0
  >;

  iterator(
    const node<NodePtr>& context, 
    Predicate0&& p0,
    Predicates&&... ps
  )
    : pred_iterator(
        nested_iterator(
          context, 
          std::forward<Predicates>(ps)...
        ),
        p0
      )
  {}

  iterator(
    node_iterators::end_t end,
    const node<NodePtr>& context, 
    Predicate0&& p0,
    Predicates&&... ps
  )
    : pred_iterator(
        nested_iterator(
          end,
          context, 
          std::forward<Predicates>(ps)...
        ),
        p0,
        end
      )
  {}
};

//! Returns the number of nodes
//! If pointer args are not null returns begin() and end()
//! values also.
template<class Query>
typename Query::iterator::size_type size(
  typename Query::result& qr,
  typename Query::iterator* bg_ = nullptr,
  typename Query::iterator* nd_ = nullptr
)
{
    const auto bg = qr.begin();
    const auto nd = qr.end();

    if (bg_) *bg_ = bg;
    if (nd_) *nd_ = bg;

    if (bg.is_empty())
      return 0;

    const auto dist = nd - bg;
    SCHECK(dist >= 0);
    return dist;
}

//! An xpath query. 
template<
  class NodePtr, 
  class Expr,
  class Prev,
  bool is_primary = true
>
class query
{
};

template<class NodePtr, class Expr, class axis>
class query<NodePtr, Expr, axis, true>
{
public:
  using iterator = step::iterator<NodePtr, axis, Expr>;

  struct result
  {
    using query_type = query<NodePtr, Expr, axis, true>;

    result() {}

    result(const node<NodePtr>& ctx, const Expr& tst) 
      : context(ctx), test(tst)
    {
      SCHECK(context.is_valid());
    }

    iterator begin()
    {
      SCHECK(context.is_valid());
      return iterator(context, test);
    }

    iterator end()
    {
      SCHECK(context.is_valid());
      return iterator(
        node_iterators::end_t(),
        context,
        test
      );
    }

    typename iterator::size_type size(
      iterator* bg = nullptr,
      iterator* nd = nullptr
    )
    {
      return step::size<query_type>(*this, bg, nd);
    }

    node<NodePtr> context;
    Expr test;
  };

  query(Expr&& tst) 
    : test(std::forward<Expr>(tst))
  {}

  result execute(const node<NodePtr>& ctx) const
  {
    return result(ctx, test);
  }

protected:
  const Expr test;
};


template<
  class NodePtr, 
  class Expr,
  class NestedQuery
>
class query<NodePtr, Expr, NestedQuery, false>
  : public NestedQuery
{
public:
  using iterator = pred_iterator_t<
    typename NestedQuery::iterator,
    Expr
  >;
  using nested_query = NestedQuery;
  using nested_result = typename NestedQuery::result;

  struct result : nested_result
  {
    using query_type = 
      query<NodePtr, Expr, NestedQuery, false>;

    result() {}

    result(
      //const node<NodePtr>& ctx, 
      const Expr& tst,
      nested_result&& nested
    ) 
      : nested_result(std::move(nested)),
        //context(ctx), 
        test(tst)
    {}

    iterator begin()
    {
//      SCHECK(context.is_valid());
      return iterator(nested_result::begin(), test);
    }

    iterator end()
    {
//      SCHECK(context.is_valid());
      return iterator(
        nested_result::end(), 
        test,
        node_iterators::end_t()
      );
    }

    typename iterator::size_type size(
      iterator* bg = nullptr,
      iterator* nd = nullptr
    )
    {
      return step::size<query_type>(*this, bg, nd);
    }

//    node<NodePtr> context;
    Expr test;
  };

  query(Expr&& e, NestedQuery&& nq) 
    : NestedQuery(std::forward<NestedQuery>(nq)),
      test(std::forward<Expr>(e))
  {}

  result execute(const node<NodePtr>& ctx) const
  {
    return result(test, NestedQuery::execute(ctx));
  }

protected:
  const Expr test;
};

template<
  class NodePtr,
  class axis,
  template<class> class Test,
  class TestArg
>
query<
    NodePtr, 
    Test<prim_iterator_t<NodePtr, axis>>,
    axis, 
    true
>
build_query(
  TestArg&& test_arg,
  bool
)
{
  return query<
    NodePtr, 
    Test<prim_iterator_t<NodePtr, axis>>,
    axis, 
    true
  > 
  (
    Test<prim_iterator_t<NodePtr, axis>>
      (std::forward<TestArg>(test_arg))
  );
}

//! a nested query
template<
  class NodePtr,
  template<class> class Test,
  class TestArg,
  class NestedQuery
>
query<
    NodePtr, 
    Test<typename NestedQuery::iterator>,
    NestedQuery,
    false
>
build_query(
  TestArg&& test_arg,
  NestedQuery&& nested_query
)
{
  return query<
    NodePtr, 
    Test<typename NestedQuery::iterator>,
    NestedQuery,
    false
  > 
  (
    Test<typename NestedQuery::iterator>(
      std::forward<TestArg>(test_arg)
    ), 
    std::forward<NestedQuery>(nested_query)
  );
}

} // step

// TODO refactor with step::query
template<class NodePtr>
template<
  class ax, 
  template<class> class Test
>
class node<NodePtr>::axis_t
{
public:
  using iterator = node_iterators::random_access_adapter<
    node::iterator<ax>
  >;

  // NB Test can use random access iterator internally
  using xiterator = node_iterators::random_access_adapter<
    xpath::iterator<iterator, Test>
  >;

  using test_t = Test<iterator>;

  explicit axis_t(const node& dom_) 
    : dom(dom_),
      test(false)
  {}

  template<class... Args>
  explicit axis_t(const node& dom_, Args&&... test_args)
    : dom(dom_),
      test(std::forward<Args>(test_args)...)
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

  xiterator xbegin()
  {
    return xiterator(begin(), test);
  }

  xiterator xend()
  {
    return xiterator(end(), test, node_iterators::end_t());
  }

  //! the number of nodes in the axis
  typename iterator::size_type size() //const
  {
    const auto dist = end() - begin();
    SCHECK(dist >= 0);
    return dist;
  }

  //! Returns the number of only test matched nodes
  //! If args are not null returns xbegin() and xend()
  //! values also.
  typename iterator::size_type xsize(
    xiterator* xbg_ = nullptr,
    xiterator* xnd_ = nullptr
  )
  {
    const auto xbg = xbegin();
    const auto xnd = xend();

    if (xbg_) *xbg_ = xbg;
    if (xnd_) *xnd_ = xbg;

    if (xbg.is_empty())
      return 0;

    const auto dist = xnd - xbg;
    SCHECK(dist >= 0);
    return dist;
  }

protected:
  const node dom;
  const test_t test;
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
    case node<NodePtr>::itype::dom:
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

    case node<NodePtr>::itype::attribute:
    {
      const pair<string, string> p(nd);
      return out << '{' << p.first 
                 << '=' << p.second << '}';
    }

    case node<NodePtr>::itype::not_valid:
      return out << "(node is not valid)";

    default:
      THROW_NOT_IMPLEMENTED;
  }
}

}

#endif
