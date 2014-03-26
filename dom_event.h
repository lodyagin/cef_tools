// -*-coding: mule-utf-8-unix; fill-column: 58; -*-
/**
 * @file
 * The CefDOMEvent utils.
 *
 * @author Sergei Lodyagin
 */

#ifndef OFFSCREEN_DOM_EVENT_H
#define OFFSCREEN_DOM_EVENT_H

#include <iostream>

std::ostream&
operator<< (std::ostream& out, 
            const CefRefPtr<CefDOMEvent>& ev);

#endif

