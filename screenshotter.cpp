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

void take_delayed(int secs)
{
  StdThread::create<LightThread>([secs]()
  {
    sleep(secs);
    take();
  })->start();
}

}
