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

namespace renderer {

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

}

#endif
