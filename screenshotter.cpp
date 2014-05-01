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
#include <string>
#include "include/cef_task.h"
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

  static const char* time_format = "utc%y%m%d_%H%M%S";

  LOG_INFO(log, "Taking the screenshot");
  const CefRect& r = bounding_rect;

#ifndef HAS_PUT_TIME
  // GCC has no std::put_time
  std::ostringstream s;
  {
    const std::tm tmp = 
      curr::time::make_utc_tm(system_clock::now());
    using iterator = std::ostreambuf_iterator<char>;
    using time_put = std::time_put<char, iterator>;
    const time_put& tp = 
      std::use_facet<time_put>(s.getloc());
    const iterator end = tp.put(
      iterator(s.rdbuf()), 
      s, 
      s.fill(), 
      &tmp,
      time_format,
      time_format 
        + std::char_traits<char>::length(time_format)
    );

    if (end.failed())
      s.setstate(std::ios_base::badbit);
  }
#endif

  string png_name = prepend_timestamp
    ? sformat(
#ifdef HAS_PUT_TIME
        put_time(system_clock::now(), time_format),
#else
        s.str(),
#endif
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

    RHolder<shared::browser>(id.browser_id)
      -> get_cef_browser() 
      -> SendProcessMessage(PID_BROWSER, msg);
  }
  LOG_DEBUG(log, "msg is sent");
}

// TODO make a function wrapper like CefRunnableMethod
// but without Cef ref counting (is it possible?)
class tmp_task : public CefTask
{
public:
  tmp_task(
    node_obj& obj_,
    const std::string& fname_
  ) 
    : obj(obj_), 
      fname(fname_)
  {}

  void Execute() override
  {
    std::cout << "test_task::Execute()" << std::endl;
    obj.take_screenshot(fname, true);
  }

protected:
  node_obj& obj;
  std::string fname;

private:
  IMPLEMENT_REFCOUNTING(test_task);
};

void node_obj::take_screenshot_delayed(
  const std::string& fname,
  node_obj::duration delay,
  bool prepend_timestamp
)
{
  LOG_TRACE(log, "take_screenshot_delayed()");

  CefPostDelayedTask(
    TID_RENDERER, 
    new tmp_task(*this, fname),
    std::chrono::duration_cast<std::chrono::milliseconds>
      (delay).count()
  );
}

} // renderer
