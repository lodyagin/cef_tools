// -*-coding: mule-utf-8-unix; fill-column: 58; -*-
/**
 * @file
 * Take web page screenshots.
 *
 * @author Sergei Lodyagin
 */

#include <iostream>
#include "RThread.hpp"
//#include "RHolder.hpp"
#include "screenshotter.h"
#include "browser.h"

using namespace curr;

namespace screenshot {

void take(
  int browser_id, 
  const CefRect& r, 
  const std::string fname
)
{
  LOG_INFO(Logger<LOG::Root>, "Taking the screenshot");
  png::image<png::rgba_pixel> img(r.width, r.height);
  (img << 
#if 1
     shared::browser_repository::instance()
      . get_object_by_id(browser_id)
#else
     RHolder<shared::browser>(browser_id)
#endif
     -> vbuf . get_area(r.x, r.y, r.width, r.height)
  ).write(fname);
}

void take_delayed(
  int browser_id, 
  const CefRect& r, 
  const std::string fname,
  int secs
)
{
  StdThread::create<LightThread>(
  [browser_id, r, fname, secs]()
  {
    sleep(secs);
    take(browser_id, r, fname);
  })->start();
}

}
