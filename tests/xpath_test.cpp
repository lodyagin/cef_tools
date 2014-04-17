#include <string.h>
#include <atomic>
#include "include/cef_command_line.h"
#include "Logging.h"
#include "Event.h"
#include "xpath.h"
#include "offscreen.h"
#include "browser.h"
#include "gtest/gtest.h"

using namespace xpath;
using namespace curr;

TEST(XpathBasic, Wrap) {
  using namespace node_iterators;
  EXPECT_TRUE(true);
  // EXPECT_TRUE((bool) wrap(CefRefPtr<CefDOMNode(nullptr)));
  // TODO EXPECT_FALSE
}

int main(int argc, char* argv[])
{
  testing::InitGoogleTest(&argc, argv);

  char** argv2 = new char*[argc+1];
  int argc2;
  for (argc2 = 0; argc2 < argc; argc2++)
    argv2[argc2] = argv[argc2];
  SCHECK(argv2[argc2++] = strdup("--off-screen"));

  std::atomic<int> test_result(13);

  offscreen(
    argc2, 
    argv2,
    [&test_result]()
    {
      CURR_WAIT_L(
        Logger<LOG::Root>::logger(),
        shared::browser_repository::instance()
          . get_object_by_id(1)
          -> is_dom_ready(),
        60001
      );

      test_result = RUN_ALL_TESTS();
    }
  );
  return test_result;
}
