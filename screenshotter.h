// -*-coding: mule-utf-8-unix; fill-column: 58; -*-
/**
 * @file
 * Take web page screenshots.
 *
 * @author Sergei Lodyagin
 */

#ifndef OFFSCREEN_SCREENSHOTTER_H
#define OFFSCREEN_SCREENSHOTTER_H

namespace screenshot {

//! Take a screenshot.
void take();

//! Take the screenshot after secs seconds. Creates the
//! thread for the remote take() call and returns
//! immediately.
void take_delayed(int secs);

}

#endif
