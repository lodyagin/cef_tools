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
  CefRefPtr<CefRenderHandler> GetRenderHandler() override
  {
    return new render;
  }
private:
  IMPLEMENT_REFCOUNTING(client);
};

}
}

#endif

