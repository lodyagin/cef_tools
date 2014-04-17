#include "xpath.h"
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
  return RUN_ALL_TESTS();
}
