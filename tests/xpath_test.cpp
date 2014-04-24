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
#include "dom.h"
#include "offscreen.h"
#include "browser.h"
#include "gtest/gtest.h"

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

template<class It>
void distance_test(
  It begin, 
  It end,
  typename It::difference_type l
)
{
    // distance
    EXPECT_EQ(0, begin - begin);
    EXPECT_EQ(0, end - end);
    EXPECT_EQ(l, end - begin);
    EXPECT_EQ(-l, begin - end);
    EXPECT_EQ(l, (end + 1) - (begin + 1));
    EXPECT_EQ(1-l, begin + 1 - end);
    EXPECT_EQ(1, begin + 1 - begin);
    EXPECT_EQ(-1, begin - 1 - begin);
    EXPECT_EQ(1, end + 1 - end);
    EXPECT_EQ(-1, end - 1 - end);
    EXPECT_EQ(l-1, end - 1 - begin);
    EXPECT_EQ(l-2, (end + 1) - (begin + 3));
    EXPECT_EQ(l-2, end - 1 - (begin + 1));
}

TEST(Xpath, SelfAxis) {
  test_dom([](CefRefPtr<CefDOMNode> r)
  {
    using namespace renderer::dom_visitor;

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

    distance_test(begin, end, 1);

    EXPECT_EQ(".", (std::string) begin.path());

    // ovf test
    {
      auto it = begin;
      --it; ++it;
      EXPECT_EQ(it, begin);
      auto i2 = end;
      ++it2; --it2;
      EXPECT_EQ(it2, end);
    }
  });
}

TEST(Xpath, SiblingAxes) {
  test_dom([](CefRefPtr<CefDOMNode> r)
  {
    using namespace renderer::dom_visitor;

    node root(r);

    // a root node has no siblings
    EXPECT_EQ(0, root.following_sibling()->size());
    EXPECT_EQ(0, root.preceding_sibling()->size());

    auto head = root.descendant()->begin() + 3;
    EXPECT_EQ("head", head->tag_name());
    EXPECT_EQ("2/0", (std::string) head.path());
    EXPECT_EQ(2, head->following_sibling()->size());
    EXPECT_EQ(0, head->preceding_sibling()->size());
    auto body = head->following_sibling()->begin() + 1;
    EXPECT_EQ("body", body->tag_name());
    EXPECT_EQ(".", (std::string) body.path());
    EXPECT_EQ(0, body->following_sibling()->size());
    EXPECT_EQ(2, body->preceding_sibling()->size());
    auto meta = head->child()->begin();
    EXPECT_EQ("meta", meta->tag_name());
#if 0
    std::copy(
      meta->following_sibling()->begin(),
      meta->following_sibling()->end(), 
      std::ostream_iterator<node>(std::cout, "\n")
    );
#endif
    EXPECT_EQ(23, meta->following_sibling()->size());
    auto last_script = meta->following_sibling()->end() - 2;
    EXPECT_EQ("script", last_script->tag_name());
    EXPECT_EQ(22, last_script->preceding_sibling()->size());

    distance_test(
      meta->following_sibling()->begin(),
      meta->following_sibling()->end(),
      23
    );
  });
}

TEST(Xpath, ChildAxis) {
  test_dom([](CefRefPtr<CefDOMNode> r)
  {
    using namespace renderer::dom_visitor;

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
    auto head = begin;
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

    distance_test(
      head->child()->begin(), 
      head->child()->end(),
      24
    );
  });
}

TEST(Xpath, DescendantAxis) {
  test_dom([](CefRefPtr<CefDOMNode> r)
  {
    using namespace renderer::dom_visitor;

    node::descendant_iterator begin = 
      node(r).descendant()->begin();
    // TODO EXPECT_EQ(*begin, node(r));
    node::descendant_iterator end =  
      node(r).descendant()->end();
    EXPECT_NE(begin, end);
    auto doctype = *begin;
    EXPECT_EQ("0", (std::string) begin.path());
#if 0
    std::copy(
      begin, 
      end, 
      std::ostream_iterator<node>(std::cout, "\n")
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
    EXPECT_EQ("2/0/0", (std::string) meta.path());
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

#if 0 // need to implement operator--
    distance_test(begin, end, 1); // FIXME 1
#endif
  });
}

TEST(Xpath, AttributeAxis) {
  test_dom([](CefRefPtr<CefDOMNode> r)
  {
    using namespace renderer::dom_visitor;

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
    EXPECT_EQ("head", i->tag_name());
    EXPECT_EQ("2/0", (std::string) i.path());
    const auto head = i;
    {
      EXPECT_EQ(i->n_attrs(), 0);
      const auto begin = i->attribute()->begin();
      const auto end = i->attribute()->end();
      EXPECT_EQ(begin, end);
    }

    // ovf test
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

    distance_test(
      html->attribute()->begin(),
      html->attribute()->end(),
      3
    );
  });
}

TEST(Xpath, NodeAttributes)
{
  test_dom([](CefRefPtr<CefDOMNode> r)
  {
    using namespace renderer::dom_visitor;

    // output all 'a' tags hrefs
    auto descendant = node(r).descendant();
    int cnt1 = 0, cnt2 = 0;
    for (const auto& n : *descendant) {
      if (n.tag_name() != "a") continue;
#if 0
      std::cout << n["href"] << std::endl;
#endif
      cnt1++;
      if (n["href"].substr(0, 4) == "http") cnt2++;
    }
    EXPECT_EQ(cnt1, 12);
    EXPECT_EQ(cnt2, 10);
  });
}

TEST(Xpath, XpathIterator)
{
  test_dom([](CefRefPtr<CefDOMNode> r)
  {
    using namespace std;
    using namespace renderer::dom_visitor;

    const auto html = node(r).child()->begin() + 2;
    EXPECT_EQ(html->tag_name(), "html");

    // self axis
    {
      const auto begin = html
        ->self<::xpath::test::constant>(false)->xbegin();
      const auto end = html
        -> self<::xpath::test::constant>(false)->xend();
      EXPECT_EQ(begin, end);
      //EXPECT_EQ(0, begin - end); //TODO check exception
      //EXPECT_EQ(0, end - begin);
      EXPECT_EQ(
        0, 
        html->self<::xpath::test::constant>(false)->xsize()
      );

      const auto begin1 = html
        -> self<::xpath::test::constant>(true)->xbegin();
      const auto end1 = html
        -> self<::xpath::test::constant>(true)->xend();
      EXPECT_NE(begin1, end1);
      EXPECT_EQ(-1, begin1 - end1);
      EXPECT_EQ(1, end1 - begin1);
      EXPECT_EQ(
        1, 
        html->self<::xpath::test::constant>(true)->xsize()
      );
    }

    // child axis
    {
      auto ax1 = html
        -> child<::xpath::test::node_type>(
             ::xpath::node_type::node
           );
      const auto begin = ax1->xbegin();
      const auto end = ax1->xend();
#if 0
      copy(
        begin, end, 
        ostream_iterator<node>(cout, "\n")
      );
#endif
      EXPECT_NE(begin, end);
      EXPECT_EQ(-2, begin - end);
      EXPECT_EQ(2, end - begin);
      EXPECT_EQ(2, ax1->xsize());
    }
  });
}

TEST(Xpath, Query)
{
  test_dom([](CefRefPtr<CefDOMNode> r)
  {
    using namespace std;
    using namespace renderer::dom_visitor;
    using namespace ::xpath::step;
    using namespace ::xpath;

    // double-checkin query
    auto q = 
    build_query<::xpath::test::name>(
      "a",
      build_query<axis::descendant, ::xpath::test::name>(
        renderer::dom_visitor::node(r),
        std::string("a"),
        true
      )
    );

#if 0
    using node = renderer::dom_visitor::node;
    copy(q.begin(), q.end(), 
      ostream_iterator<node>(cout, "\n")
    );
#endif

    EXPECT_EQ(12, size(q));
        
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
