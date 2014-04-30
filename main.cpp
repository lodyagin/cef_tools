// -*-coding: mule-utf-8-unix; fill-column: 58; -*-
/**
 * @file
 *
 * @author Sergei Lodyagin
 */

#include <iostream>
#include <memory>
#include "RThread.h"
#include "RHolder.hpp"
#include "offscreen.h"
#include "browser.h"

// Get png of the first flash banner.
#include "task1.h"

using namespace curr;

static const int browser_id = 1;

namespace g_flags {
bool single_process_mode = false;
}

int main(int argc, char* argv[])
{
  std::cout << "main started: ";
  for (int i = 1; i < argc; i++)
    std::cout << argv[i] << " ";
  std::cout << std::endl;

  char** argv2 = new char*[argc+2];
  int argc2;
  for (argc2 = 0; argc2 < argc; argc2++)
    argv2[argc2] = argv[argc2];
#if 0
  // enable single process
  g_flags::single_process_mode = true;
  SCHECK(argv2[argc2++] = strdup("--single-process"));
#endif

  return offscreen(
    argc2, 
    argv2,
    []()
    {
      CURR_WAIT_L(
        Logger<LOG::Root>::logger(),
        RHolder<shared::browser>(1)->is_dom_ready(),
        60001
      );

      // post the flash search task
      CefPostTask(
        TID_RENDERER, 
        new ::renderer::get_png(browser_id, 1)
      );
    }
  );
}
