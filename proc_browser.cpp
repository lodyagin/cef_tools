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
  shared::browser::Par par(url);
  if(command_line->HasSwitch("off-screen"))
    par.window_info.SetAsOffScreen(nullptr);
  shared::browser_repository::instance().create_object(par);
}

render::render(int width_, int height_)
  : width(width_), height(height_),
    buf(boost::extents[height_][width_])
{
  SCHECK(width > 10);
  SCHECK(height > 10);
}

bool render::GetViewRect(
  CefRefPtr<CefBrowser> browser,
  CefRect& rect
)
{
  rect.Set(0, 0, width, height);
  return true;
}

void render::OnPaint(
  CefRefPtr<CefBrowser> browser,
  CefRenderHandler::PaintElementType type,
  const CefRenderHandler::RectList& dirtyRects,
  const void* buffer,
  int width, 
  int height
)
{
  typedef point_buffer::index_range range;

  for (auto r : dirtyRects)
  {
    LOG_TRACE(log, 
      "render::OnPaint(" << r.x << ", " << r.y << ", " 
          << r.width << ", " << r.height << ")"
    );

    // select a rectangular write region
    point_buffer::array_view<2>::type dst =
      buf[point_buffer::index_gen()
        [range(r.y, r.y + r.height)]
        [range(r.x, r.x + r.width)]
      ];

    // write the buffer into the region
    const point * src = (const point*) buffer;
    int i = 0;

    for (point_buffer::index row = 0; row < r.height; row++)
    {
      for (point_buffer::index col = 0; col < r.width; col++)
      {
        dst[row][col] = src[i++];
      }
    }
  }
}

render::point_buffer
render::GetArea(int x, int y, int width, int height) const
{
  typedef point_buffer::index_range range;

  point_buffer::const_array_view<2>::type src =
    buf[point_buffer::index_gen()
      [range(y, y + height)]
      [range(x, x + width)]
    ];

#if 0
  for (point_buffer::index row = 0; row < r.height; row++)
  {
    for (point_buffer::index col = 0; col < r.width; col++)
    {
      dst[row][col] = src[i++];
    }
  }
#endif

  return src;
}

}}

