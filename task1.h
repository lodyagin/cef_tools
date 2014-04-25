// -*-coding: mule-utf-8-unix; fill-column: 58; -*-
/**
 * @file
 * Get png of the first flash banner.
 *
 * @author Sergei Lodyagin
 */

#ifndef OFFSCREEN_TASK1_H
#define OFFSCREEN_TASK1_H

#include <string>
#include "include/cef_task.h"
#include "Logging.h"

namespace renderer {

//! Get png of the flash_num flash in the
//! browser_id. 
//! Store as "tag_name" "tag_path" _ "screenshot_time".png
class get_png : public CefTask
{
public:
  get_png(
    int browser_id_,
    int flash_num_ //< 1-based
//    const std::string& fname_
  )
    : browser_id(browser_id_),
      flash_num(flash_num_)
//      fname(fname_)
  {}

  void Execute() override;

private:
  using log = curr::Logger<get_png>;

  int browser_id;
  int flash_num;
//  std::string fname;

  IMPLEMENT_REFCOUNTING();
};

} // renderer

#endif

