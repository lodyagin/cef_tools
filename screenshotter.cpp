// -*-coding: mule-utf-8-unix; fill-column: 58; -*-
/**
 * @file
 * Take web page screenshots.
 *
 * @author Sergei Lodyagin
 */

#include <iostream>
#include <png++/png.hpp>
#include "RThread.hpp"
#include "screenshotter.h"
#include "browser.h"

using namespace curr;

//! Save 2d RGBA point array to png::image
template<class point, class pixel>
png::image<pixel>&
operator<< (
  png::image<pixel>& img, 
  const boost:multi_array<point, 2> area
)
{
  for (int y = 0; y < img.get_height(); y++)
  {
    for (int x = 0; x < img.get_width(); x++)
    {
      const point& p = area[y][x];
      img[y][x] = png::rgba_pixel(p.red, p.green, p.blue, p.alpha);
    }
  }
  return img;
}

namespace screenshot {

void take(
  int browser_id, 
  const CefRect& r, 
  const std::string fname
)
{
  (png::image<png::rgba_pixel> (r.width, r.height)
   << shared::browser_repository::instance()
    . get_object_by_id(browser_id)
    -> br -> vbuf . get_area(r.x, r.y, r.width, r.height)
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
