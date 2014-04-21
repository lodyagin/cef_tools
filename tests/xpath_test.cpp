#include <iostream>
#include <iterator>
#include <string.h>
#include <atomic>
#include <functional>
#include <boost/filesystem.hpp>
#include "include/cef_command_line.h"
#include "include/cef_task.h"
#include "Logging.h"
#include "Event.h"
#include "xpath.h"
#include "offscreen.h"
#include "browser.h"
#include "gtest/gtest.h"

using namespace renderer::dom_visitor::xpath;
using namespace curr;

namespace {

using log = Logger<LOG::Root>;

const int browser_id = 1;

using fun_t = std::function<void(CefRefPtr<CefDOMNode>)>;

class DOMVisitor : public CefDOMVisitor
{
public:
  DOMVisitor(const fun_t& f) : fun(f) {}

  // DOM is valid only inside this function
  // do not store DOM externally!
  void Visit(CefRefPtr<CefDOMDocument> d) override
  {
    fun(d->GetDocument());
  }

protected:
  fun_t fun;

private:
  IMPLEMENT_REFCOUNTING();
};

void test_dom(const fun_t& fun)
{
  shared::browser_repository::instance()
    . get_object_by_id(browser_id) -> br
    -> GetMainFrame() -> VisitDOM(new DOMVisitor(fun));
}

}

TEST(XpathBasic, Wrap) {
  test_dom([](CefRefPtr<CefDOMNode> root)
  {
    using namespace renderer::dom_visitor;
    EXPECT_TRUE((bool) wrap(root));
    EXPECT_FALSE((bool) wrap(root->GetParent()));
  });
}

TEST(Xpath, SelfAxis) {
  test_dom([](CefRefPtr<CefDOMNode> r)
  {
    node root(r);
    const node::self_iterator begin = root.self()->begin();
    const node::self_iterator end = root.self()->end();
    
    EXPECT_EQ(begin, begin);
    EXPECT_EQ(end, end);
    EXPECT_NE(begin, end);
    node::self_iterator it = begin;
    ++it;
    EXPECT_NE(it, begin);
    EXPECT_EQ(it, end);
    EXPECT_NE(begin, end);
    node::self_iterator it2 = it++;
    EXPECT_NE(it2, begin);
    EXPECT_EQ(it2, end);
    EXPECT_NE(it, begin);
    EXPECT_NE(it, end);
    EXPECT_NE(begin, end);

#ifndef XPATH_OVF_ASSERT
    {
      auto it = begin;
      --it; ++it;
      EXPECT_EQ(it, begin);
      auto i2 = end;
      ++it2; --it2;
      EXPECT_EQ(it2, end);
    }
#endif
  });
}

TEST(Xpath, SiblingAxes) {
  test_dom([](CefRefPtr<CefDOMNode> r)
  {
    node root(r);

    // a root node has no siblings
    EXPECT_EQ(0, root.following_sibling()->size());
    EXPECT_EQ(0, root.preceding_sibling()->size());

    auto head = root.descendant()->begin() + 3; //FIXME 3
    EXPECT_EQ("head", head->tag_name());
    EXPECT_EQ(1, head->following_sibling()->size());
    EXPECT_EQ(0, head->preceding_sibling()->size());
    auto body = head->following_sibling()->begin();
    EXPECT_EQ("body", body->tag_name());
    EXPECT_EQ(0, head->following_sibling()->size());
    EXPECT_EQ(1, head->preceding_sibling()->size());
    auto meta = head->child()->begin();
    EXPECT_EQ("meta", meta->tag_name());
#if 1
    // out only tags
    std::copy_if(
      meta->following_sibling()->begin(), 
      meta->following_sibling()->end(), 
      std::ostream_iterator<node>(std::cout, "\n"),
      [](const node& n) { return n->IsElement(); }
    );
#endif
    //EXPECT_EQ(?, meta->following_sibling()->size());

    auto last_script = head->child()->end() - 1;
    EXPECT_EQ("script", last_script->tag_name();
    //EXPECT_EQ(?, last_script->preceding_sibling()->size());
  });
}

TEST(Xpath, ChildAxis) {
  test_dom([](CefRefPtr<CefDOMNode> r)
  {
    node root(r);
    node::child_iterator begin = root.child()->begin();
    node::child_iterator end = root.child()->end();
    EXPECT_EQ(begin, begin);
    EXPECT_EQ(end, end);
    EXPECT_NE(begin, end);

    node::child_iterator it = begin;
    ++it; ++it;
    begin = (*it).child()->begin();
    end = (*it).child()->end();
    
    EXPECT_EQ((*begin).tag_name(), "head");
    int cnt1 = 0;
    for (node::child_iterator it2 = begin; 
         it2 != end; 
         ++it2)
    {
      it = it2;
      ++cnt1;
    }
    EXPECT_EQ((*it).tag_name(), "body");
    int cnt2 = 0;
    for (node::child_iterator it3 = end; 
         it3 != begin; 
         --it3
         )
      ++cnt2;
    EXPECT_EQ(cnt1, cnt2);

    const node::child_iterator body = it;

    it = begin;
    begin = it->child()->begin();
    end = it->child()->end();

    const auto img = body
      ->child()->begin()
      ->child()->begin()
      ->child()->begin()
      ->child()->begin()
      ->child()->begin();
    EXPECT_EQ(img ->tag_name(), "img");

    // empty tag
    EXPECT_EQ(
      img->child()->begin(),
      img->child()->end()
    );

#if 0
    std::copy(
      body->child()->begin(), 
      body->child()->end(), 
      std::ostream_iterator<node>(std::cout, "\n")
    );
#endif
  });
}

TEST(Xpath, DescendantAxis) {
  test_dom([](CefRefPtr<CefDOMNode> r)
  {
    node::descendant_iterator begin = 
      node(r).descendant()->begin();
    // TODO EXPECT_EQ(*begin, node(r));
    node::descendant_iterator end =  
      node(r).descendant()->end();
    EXPECT_NE(begin, end);
    auto doctype = *begin;

#if 0
    std::copy(
      begin, 
      end, 
      std::ostream_iterator<node>(std::cout, "\n")
//      [](const node& n) { return n->IsElement(); }
    );
#endif

    const int n_tags = std::count_if(
      begin, 
      end, 
      [](const node& n) { return n->IsElement(); }
    );
    EXPECT_EQ(n_tags, 67);
    {
      ++begin; // at <!-- ...>
      const int n_tags2 = std::count_if(
        begin->descendant()->begin(),
        begin->descendant()->end(),
        [](const node& n) { return n->IsElement(); }
      );
      EXPECT_EQ(n_tags2, 0);
    }

    ++begin; ++begin;
    auto meta = begin; ++meta; // points to 1st meta tag
    {
      EXPECT_EQ(begin->tag_name(), "head");
      end = begin->descendant()->end();
      auto new_begin = begin->descendant()->begin();
      auto old_begin = begin;
      ++old_begin;
      EXPECT_EQ(old_begin, new_begin);
#if 0
      auto new_begin2 = new_begin;
      --new_begin2;
      EXPECT_EQ(new_begin2, begin);
#endif
      begin = new_begin;
  
#if 0
      // out only tags
      std::copy_if(
        begin, 
        end, 
        std::ostream_iterator<node>(std::cout, "\n"),
        [](const node& n) { return n->IsElement(); }
        );
#endif

      const int n_tags3 = std::count_if(
        begin, 
        end, 
        [](const node& n) { return n->IsElement(); }
        );
      EXPECT_EQ(n_tags3, 12);
    }
    
    // empty axis check
    // empty tag
    EXPECT_EQ(
      meta->descendant()->begin(),
      meta->descendant()->end()
    );
  });
}

TEST(Xpath, AttributeAxis) {
  test_dom([](CefRefPtr<CefDOMNode> r)
  {
    node root(r);

    auto i = root.descendant()->begin();
    ++i; ++i; 
    EXPECT_EQ(i->tag_name(), "html");
    const auto html = i;
    EXPECT_EQ(i->n_attrs(), 3);
    {
      const int n_attrs = std::count_if(
        i->attribute()->begin(), 
        i->attribute()->end(), 
        [](const node&){ return true; }
      );
      EXPECT_EQ(n_attrs, 3);
    }
    auto end = i->attribute()->end();
    --(i->attribute()->end());
    EXPECT_EQ(end, i->attribute()->end());
#if 0
    std::copy(
      i->attribute()->begin(), 
      i->attribute()->end(), 
      std::ostream_iterator<node>(std::cout, "; \n")
    );
    std::cout << std::endl;
#endif

    ++i;
    EXPECT_EQ(i->tag_name(), "head");
    const auto head = i;
    {
      EXPECT_EQ(i->n_attrs(), 0);
      const auto begin = i->attribute()->begin();
      const auto end = i->attribute()->end();
      EXPECT_EQ(begin, end);
    }

#ifndef XPATH_OVF_ASSERT
    {
      const auto begin = html->attribute()->begin();
      const auto end = html->attribute()->end();
      auto it = begin;
      --it; ++it;
      EXPECT_EQ(begin, it);
      auto it2 = end;
      ++it2; --it2;
      EXPECT_EQ(end, it2);
    }
    {
      const auto begin = head->attribute()->begin();
      const auto end = head->attribute()->end();
      auto it = begin;
      --it; ++it;
      EXPECT_EQ(begin, it);
      auto it2 = end;
      ++it2; --it2;
      EXPECT_EQ(end, it2);
    }
#endif
  });
}

std::atomic<int> test_result(13);

class test_runner : public CefTask
{
public:
  void Execute() override
  {
    test_result = RUN_ALL_TESTS();
  }

private:
  IMPLEMENT_REFCOUNTING();
};

int main(int argc, char* argv[])
{
  testing::InitGoogleTest(&argc, argv);

  char** argv2 = new char*[argc+2];
  int argc2;
  for (argc2 = 0; argc2 < argc; argc2++)
    argv2[argc2] = argv[argc2];
  SCHECK(argv2[argc2++] = strdup((
    std::string("--url=file://")
    + boost::filesystem::current_path().string()
    + "/data/CU3OX - DEMO Page.html").c_str()
  ));
  LOG_DEBUG(log, "--url=" << argv2[argc2-1]);
  SCHECK(argv2[argc2++] = strdup("--off-screen"));

  offscreen(
    argc2, 
    argv2,
    []()
    {
      CURR_WAIT_L(
        Logger<LOG::Root>::logger(),
        shared::browser_repository::instance()
          . get_object_by_id(1)
          -> is_dom_ready(),
        60001
      );

      CefPostTask(TID_RENDERER, new test_runner);
    }
  );
  return test_result;
}
