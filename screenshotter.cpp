// -*-coding: mule-utf-8-unix; fill-column: 58; -*-
/**
 * @file
 * Take web page screenshots.
 *
 * @author Sergei Lodyagin
 */

#include <iostream>
#include <chrono>
#include <thread>
#include "include/wrapper/cef_message_router.h"
#include "RThread.hpp"
#include "SCommon.h"
#include "types/time.h"
#include "screenshotter.h"
#include "browser.h"
#include "dom.h"

using namespace curr;

namespace renderer {

void node_obj::take_screenshot(
  const std::string& fname,
  bool prepend_timestamp
)
{
  LOG_TRACE(log, "take_screenshot()");

  using namespace std;
  using namespace std::chrono;
  using namespace curr::types;

  static const auto time_format = "%y%m%d_%H%M%S";

  LOG_INFO(log, "Taking the screenshot");
  const CefRect& r = bounding_rect;

  string png_name = prepend_timestamp
    ? sformat(
        put_time(system_clock::now(), time_format),
        '_', fname
      )
    : fname;

  if (r.width == 0 || r.height == 0) {
    LOG_ERROR(log, "the node " << *this
      << "area is empty, do not store " << png_name);
    return;
  }

  LOG_DEBUG(log, "sending the msg");
  {
    CefRefPtr<CefProcessMessage> msg =
      CefProcessMessage::Create("take_screenshot");
#if 0
    LOG_TRACE(log, "1");
    msg->GetArgumentList()->SetInt(0, id.browser_id);
    LOG_TRACE(log, "2");
    msg->GetArgumentList()->SetInt(1, r.x);
    LOG_TRACE(log, "3");
    msg->GetArgumentList()->SetInt(2, r.y);
    LOG_TRACE(log, "4");
    msg->GetArgumentList()->SetInt(3, r.width);
    LOG_TRACE(log, "5");
    msg->GetArgumentList()->SetInt(4, r.height);
    LOG_TRACE(log, "6");
    msg->GetArgumentList()->SetString(
      5, 
      CefString(png_name)
    );
    LOG_TRACE(log, "7");
#endif
     shared::browser_repository::instance()
      . get_object_by_id(id.browser_id)
      -> get_cef_browser() 
      -> SendProcessMessage(PID_BROWSER, msg);
  }
  LOG_DEBUG(log, "msg is sent");

#if 0
  png::image<png::rgba_pixel> img(r.width, r.height);
  (img << 
#if 1
     shared::browser_repository::instance()
      . get_object_by_id(id.browser_id)
#else
     RHolder<shared::browser>(id.browser_id)
#endif
     -> vbuf . get_area(r.x, r.y, r.width, r.height)
  ).write(png_name);
#endif
}

void node_obj::take_screenshot_delayed(
  const std::string& fname,
  node_obj::duration delay,
  bool prepend_timestamp
)
{
  LOG_TRACE(log, "take_screenshot_delayed()");
  StdThread::create<LightThread>(
  [this, fname, delay, prepend_timestamp]()
  {
    std::this_thread::sleep_for(delay);
    take_screenshot(fname, prepend_timestamp);
  })->start();
}

} // renderer
