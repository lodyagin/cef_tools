// -*-coding: mule-utf-8-unix; fill-column: 58; -*-
/**
 * @file
 * Take web page screenshots.
 *
 * @author Sergei Lodyagin
 */

#include <iostream>
#include "RThread.hpp"
#include "screenshotter.h"

using namespace curr;

namespace screenshot {

void take(int browser_id)
{
}

void take_delayed(int browser_id, int secs)
{
  StdThread::create<LightThread>([browser_id, secs]()
  {
    sleep(secs);
    take(browser_id);
  })->start();
}

}
