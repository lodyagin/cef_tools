// -*-coding: mule-utf-8-unix; fill-column: 58; -*-
/**
 * @file
 * Take web page screenshots.
 *
 * @author Sergei Lodyagin
 */

#ifndef OFFSCREEN_SCREENSHOTTER_H
#define OFFSCREEN_SCREENSHOTTER_H

#include <boost/multi_array.hpp>
#include <png++/png.hpp>
#include "include/cef_base.h"

//! Save 2d BGRA point array to png::image
template<class point, class pixel>
png::image<pixel>&
operator<< (
  png::image<pixel>& img, 
  const boost::multi_array<point, 2> area
)
{
  for (int y = 0; y < img.get_height(); y++)
  {
    for (int x = 0; x < img.get_width(); x++)
    {
      const point& p = area[y][x];
      img[y][x] = png::rgba_pixel
        (p.red, p.green, p.blue, p.alpha);
    }
  }
  return img;
}

namespace screenshot {

//! Take a screenshot from browser_id of rect r into png
//! with the path fname
void take(
  int browser_id, 
  const CefRect& r, 
  const std::string fname
);

  //! Take the screenshot after secs seconds. Creates the
  //! thread for the remote take() call and returns
  //! immediately.
  void take_delayed(
    int browser_id, 
    const CefRect& r, 
    const std::string fname,
    int secs
  );

}

#endif
