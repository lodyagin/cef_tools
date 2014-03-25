// -*-coding: mule-utf-8-unix; fill-column: 58; -*-
/**
 * @file
 * The code for a browser process.
 *
 * @author Sergei Lodyagin
 */

#ifndef OFFSCREEN_PROC_BROWSER_H
#define OFFSCREEN_PROC_BROWSER_H

#include "include/cef_browser_process_handler.h"
#include "include/cef_client.h"
#include "Logging.h"

//! The code used by a browser process only
namespace browser {
namespace handler {

class browser : public CefBrowserProcessHandler
{
public:
  void OnContextInitialized() override;
private:
  typedef curr::Logger<browser> log;
  IMPLEMENT_REFCOUNTING(browser);
};

class client : public CefClient
{
public:
#if 0
  CefRefPtr<CefLoadHandler> GetLoadHandler() override
  {
    return new load(br);
  }
#endif
private:
  IMPLEMENT_REFCOUNTING(client);
};

}
}

#endif

