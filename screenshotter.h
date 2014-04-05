// -*-coding: mule-utf-8-unix; fill-column: 58; -*-
/**
 * @file
 * Take web page screenshots.
 *
 * @author Sergei Lodyagin
 */

#ifndef OFFSCREEN_SCREENSHOTTER_H
#define OFFSCREEN_SCREENSHOTTER_H

#include "include/cef_base.h"

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
