// -*-coding: mule-utf-8-unix; fill-column: 58; -*-
/**
 * @file
 * CefString utils.
 *
 * @author Sergei Lodyagin
 */

#ifndef OFFSCREEN_STRING_H
#define OFFSCREEN_STRING_H

#include <iostream>
#include "include/cef_base.h"

std::ostream&
operator<< (std::ostream& out, const CefString& s);

#endif
