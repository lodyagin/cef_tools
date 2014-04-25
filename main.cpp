// -*-coding: mule-utf-8-unix; fill-column: 58; -*-
/**
 * @file
 *
 * @author Sergei Lodyagin
 */

#include <iostream>
#include <memory>
#include "RThread.h"
#include "offscreen.h"
#include "browser.h"

// Get png of the first flash banner.
#include "task1.h"

using namespace curr;

static const int browser_id = 1;

int main(int argc, char* argv[])
{
  std::cout << "main started: ";
  for (int i = 1; i < argc; i++)
    std::cout << argv[i] << " ";
  std::cout << std::endl;

  return offscreen(
    argc, 
    argv,
    []()
    {
      CURR_WAIT_L(
        Logger<LOG::Root>::logger(),
        shared::browser_repository::instance()
          . get_object_by_id(1)
          -> is_dom_ready(),
        60001
      );

      std::cout << "RENDER THREAD" << std::endl;
      // post the flash search task
      CefPostTask(
        TID_RENDERER, 
        new ::renderer::get_png(browser_id, 1)
      );
    }
  );
}
