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
#include "include/cef_render_handler.h"
#include "Logging.h"
//#include "RHolder.h"
#include "browser.h"

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

class render : public CefRenderHandler
{
public:
  const int width;
  const int height;

  render(int w, int h);

  bool GetViewRect(
    CefRefPtr<CefBrowser> browser,
    CefRect& rect
  ) override;

  bool GetScreenPoint(
    CefRefPtr<CefBrowser> browser,
    int viewX,
    int viewY,
    int& screenX,
    int& screenY
  ) override
  {
    screenX = viewX;
    screenY = viewY;
    return true;
  }

  void OnPaint(
    CefRefPtr<CefBrowser> browser,
    PaintElementType type,
    const RectList& dirtyRects,
    const void* buffer,
    int width, 
    int height
  ) override;

private:
  typedef curr::Logger<render> log;
  IMPLEMENT_REFCOUNTING(client);
};

class client : public CefClient
{
public:
  const int width, height;

  client(const shared::browser::Par& par)
    : width(par.width), height(par.height)
  {}

  CefRefPtr<CefRenderHandler> GetRenderHandler() override
  {
    return new render(width, height);
  }

  bool OnProcessMessageReceived(
    CefRefPtr<CefBrowser> browser,
    CefProcessId source_proc_id,
    CefRefPtr<CefProcessMessage> msg
  ) override;

private:
  using log = curr::Logger<client>;
  IMPLEMENT_REFCOUNTING(client);
};

}
}

#endif

