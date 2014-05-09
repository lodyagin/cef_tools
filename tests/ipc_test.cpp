//#include <atomic>
#include <string>
#include "include/cef_command_line.h"
#include "include/cef_task.h"
#include "include/cef_base.h"
#include "Logging.h"
#include "Event.h"
#include "ipc.h"
#include "offscreen.h"
#include "gtest/gtest.h"

using namespace curr;

namespace { 
using log = Logger<LOG::Root>; 
}

template<class...>
struct test_proc;

template<>
struct test_proc<int, double, std::string, CefRect>
{
  static Event ev;

  test_proc(int i, double f, const std::string& s, const CefRect& r)
  {
    EXPECT_EQ(10, i);
    EXPECT_EQ(13.2, f);
    EXPECT_EQ("abc", s);
    EXPECT_EQ(CefRect(21, 22, 30, 31), r);
    ev.set();
  }
};

Event test_proc<int, double, std::string, CefRect>::ev(
  "test_proc::ev", 
  false /* auto reset */
);

TEST(Xpath, CurrentProcessPid)
{
  EXPECT_TRUE(
       process::current == PID_BROWSER
    || process::current == PID_RENDERER
  );
}

TEST(Xpath, SendAndReceive)
{
  if (process::current == PID_RENDERER)
  {
//    sleep(5); // TODO receiver: register entries in On callback,
              // remove the delay
    ipc::send<test_proc>(
      10, 13.2, std::string("abc"), CefRect(21, 22, 30, 31)
    );
  }
  else if (process::current == PID_BROWSER)
  {
    ipc::receiver::repository::instance().reg<
      test_proc<int, double, std::string, CefRect>
    >();
    CURR_WAIT((test_proc<int, double, std::string, CefRect>::ev), 10);
  }
  else EXPECT_FALSE(true);
}

namespace g_flags{
bool single_process_mode = false;
}

int main(int argc, char* argv[])
{
  testing::InitGoogleTest(&argc, argv);

  char** argv2 = new char*[argc+2];
  int argc2;
  for (argc2 = 0; argc2 < argc; argc2++)
    argv2[argc2] = argv[argc2];
  SCHECK(argv2[argc2++] = strdup("--off-screen"));

  int renderer_test_result = 1;
  int browser_test_result = 1;
  offscreen(
    argc2, 
    argv2,
    [&renderer_test_result]() 
    { renderer_test_result = RUN_ALL_TESTS(); }/*,
    [&browser_test_result]() 
    { browser_test_result = RUN_ALL_TESTS(); }*/
  );
  if (renderer_test_result)
    return renderer_test_result;
  else
    return browser_test_result;
}
