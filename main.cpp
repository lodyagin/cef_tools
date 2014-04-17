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

using namespace curr;

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
#if 0
      // post the flash search task
      CefPostTask(
        TID_RENDERER, 
        new ::renderer::search::flash(browser_id)
      );
#endif
    }
  );
}
