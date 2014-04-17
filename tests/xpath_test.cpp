#include <string.h>
#include <atomic>
#include <functional>
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

const int browser_id = 1;

using fun_t = std::function<void(CefRefPtr<CefDOMNode>)>;

class DOMVisitor : public CefDOMVisitor
{
public:
  DOMVisitor(const fun_t& f) : fun(f) {}

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

  char** argv2 = new char*[argc+1];
  int argc2;
  for (argc2 = 0; argc2 < argc; argc2++)
    argv2[argc2] = argv[argc2];
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
