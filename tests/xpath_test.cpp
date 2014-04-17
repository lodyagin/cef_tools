#include <string.h>
#include "include/cef_command_line.h"
#include "xpath.h"
#include "offscreen.h"
#include "gtest/gtest.h"

using namespace xpath;

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

  offscreen(argc2, argv2);

  return RUN_ALL_TESTS();
}
