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

using namespace xpath;
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
  using namespace node_iterators;
  test_dom([](CefRefPtr<CefDOMNode> root)
  {
    EXPECT_TRUE((bool) wrap(root));
    EXPECT_FALSE((bool) wrap(root->GetParent()));
  });
}

TEST(Xpath, SelfAxis) {
  test_dom([](CefRefPtr<CefDOMNode> r)
  {
    node root(r);
    self_iterator begin = root.self()->begin();
    self_iterator end = root.self()->end();
    
    EXPECT_EQ(begin, begin);
    EXPECT_EQ(end, end);
    EXPECT_NE(begin, end);
    self_iterator it = begin;
    ++it;
    EXPECT_NE(it, begin);
    EXPECT_EQ(it, end);
    EXPECT_NE(begin, end);
    self_iterator it2 = it++;
    EXPECT_NE(it2, begin);
    EXPECT_EQ(it2, end);
    EXPECT_NE(it, begin);
    EXPECT_NE(it, end);
    EXPECT_NE(begin, end);
  });
}

TEST(Xpath, ChildAxis) {
  test_dom([](CefRefPtr<CefDOMNode> r)
  {
    node root(r);
    child_iterator begin = root.child()->begin();
    child_iterator end = root.child()->end();
    EXPECT_EQ(begin, begin);
    EXPECT_EQ(end, end);
    EXPECT_NE(begin, end);

    child_iterator it = begin;
    ++it; ++it;
    begin = (*it).child()->begin();
    end = (*it).child()->end();
    
    EXPECT_EQ((*begin).tag_name(), "head");
    int cnt1 = 0;
    for (child_iterator it2 = begin; it2 != end; ++it2)
    {
      it = it2;
      ++cnt1;
    }
    EXPECT_EQ((*it).tag_name(), "body");
    int cnt2 = 0;
    for (child_iterator it3 = end; it3 != begin; --it3)
      ++cnt2;
    EXPECT_EQ(cnt1, cnt2);

    const child_iterator body = it;

    it = begin;
    begin = it->child()->begin();
    end = it->child()->end();

    EXPECT_EQ(
      body
      ->child()->begin()
      ->child()->begin()
      ->child()->begin()
      ->child()->begin()
      ->child()->begin()
      ->tag_name(),
      "img"
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
    descendant_iterator begin = 
      node(r).descendant()->begin();
    // TODO EXPECT_EQ(*begin, node(r));
    descendant_iterator end =  
      node(r).descendant()->end();

#if 0
    // out only tags
    std::copy_if(
      begin, 
      end, 
      std::ostream_iterator<node>(std::cout, "\n"),
      [](const node& n) { return n->IsElement(); }
    );
#endif

    const int n_tags = std::count_if(
      begin, 
      end, 
      [](const node& n) { return n->IsElement(); }
    );
    EXPECT_EQ(n_tags, 67);

    {
      ++begin; // at <!DOCTUPE ...>
      const int n_tags2 = std::count_if(
        begin->descendant()->begin(),
        begin->descendant()->end(),
        [](const node& n) { return n->IsElement(); }
      );
      EXPECT_EQ(n_tags2, 0);
    }

    {
    ++begin; ++begin;
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
