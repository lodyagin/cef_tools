// -*-coding: mule-utf-8-unix; fill-column: 58; -*-
/**
 * @file
 * The code for a browser process.
 *
 * @author Sergei Lodyagin
 */

#include "proc_browser.h"
#include "browser.h"
#include "task.h"

using namespace curr;

namespace browser { namespace handler {

void browser::OnContextInitialized() 
{
  LOG_DEBUG(log, "OnContextInitialized");

  REQUIRE_UI_THREAD();

  std::string url;

  // Check if a "--url=" value was provided via the
  // command-line. If so, use that instead of the default
  // URL.
  CefRefPtr<CefCommandLine> command_line =
      CefCommandLine::GetGlobalCommandLine();
  url = command_line->GetSwitchValue("url");
  if (url.empty())
  url = "http://ibm.com";

  // Create the first browser window.
  shared::browser_repository::instance().create_object(url);
}

bool ::browser::handler::render::GetViewRect(
  CefRefPtr<CefBrowser> browser,
  CefRect& rect
)
{
  rect.Set(0, 0, 2700, 2700);
  return true;
}

void ::browser::handler::render::OnPaint(
  CefRefPtr<CefBrowser> browser,
  CefRenderHandler::PaintElementType type,
  const CefRenderHandler::RectList& dirtyRects,
  const void* buffer,
  int width, 
  int height
)
{
  for (auto r : dirtyRects)
    LOG_DEBUG(log, 
      "(" << r.x << ", " << r.y << ", " 
          << r.width << ", " << r.height << ")"
    );
}


}}

