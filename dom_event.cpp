// -*-coding: mule-utf-8-unix; fill-column: 58; -*-
/**
 * @file
 * The CefDOMEvent utils.
 *
 * @author Sergei Lodyagin
 */

#include "include/cef_base.h"
#include "include/cef_dom.h"
#include "dom_event.h"

std::ostream&
operator<< (std::ostream& out, 
            const CefRefPtr<CefDOMEvent>& ev)
{
  out << "CefDOMEvent {"
      << "phase = " << ev->GetPhase()
      << "}";
  return out;
}

