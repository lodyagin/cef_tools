// -*-coding: mule-utf-8-unix; fill-column: 58; -*-
/**
 * @file
 * The code for a browser process.
 *
 * @author Sergei Lodyagin
 */

//#include "RHolder.hpp"
#include "proc_browser.h"
#include "browser.h"
#include "task.h"

using namespace curr;

namespace g_flags {
extern bool single_process_mode;
}

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
  shared::browser::Par par(url);
  if(command_line->HasSwitch("off-screen"))
    par.window_info.SetAsOffScreen(nullptr);
  shared::browser_repository::instance().create_object(par);
}

render::render(int w, int h)
  : width(w), height(h)
{
  LOG_TRACE(log, "render::render");
  SCHECK(w > 10);
  SCHECK(h > 10);
}

bool render::GetViewRect(
  CefRefPtr<CefBrowser> browser,
  CefRect& rect
)
{
  LOG_TRACE(log, "render::GetViewRect");
  rect.Set(0, 0, width, height);
  return true;
}

void render::OnPaint(
  CefRefPtr<CefBrowser> browser,
  CefRenderHandler::PaintElementType type,
  const CefRenderHandler::RectList& dirtyRects,
  const void* buffer,
  int w, 
  int h
)
{
#if 1
  shared::browser* br = nullptr;
  try {
    br = shared::browser_repository::instance()
      . get_object_by_id(browser->GetIdentifier());
  }
  catch(const std::out_of_range&) {}
#else
  RHolder<shared::browser> br(browser->GetIdentifier());
#endif

  for (auto r : dirtyRects)
  {
      LOG_TRACE(log, 
        "render::OnPaint(" << r.x << ", " << r.y << ", " 
            << r.width << ", " << r.height << ")"
      );

      br -> get_vbuf().on_paint(
        r.x, r.y, r.width, r.height,
        static_cast
          <const shared::videobuffer::point*>(buffer)
      );
  }
}

}}

