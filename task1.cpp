// -*-coding: mule-utf-8-unix; fill-column: 58; -*-
/**
 * @file
 * Get png of the first flash banner.
 *
 * @author Sergei Lodyagin
 */

#include "task1.h"
#include "dom.h"

using namespace curr;

namespace renderer {

void get_png::Execute()
{
  using namespace std;
  using namespace std::chrono;
  using namespace renderer;
  using namespace shared;

  auto list = node_repository::instance().query(
    browser_id,
    dom_visitor::build_query<::xpath::test::fun>(
      [](const dom_visitor::node::generic_iterator& it)
      {
        return (*it)["type"] == 
          "application/x-shockwave-flash";
      },
      dom_visitor::build_query
        <xpath::axis::descendant, xpath::test::name>
      (
        "object",
        true
      )
    )
  );

  LOG_INFO(log, 
    "task1: got " << list.size() << " objects");

  if (list.empty())
    return;

  for (auto ptr : list)
    LOG_INFO(log, ptr->universal_id() << '\t' << *ptr);

  auto flash = list.cbegin();
  for(int k = 1; k < flash_num; k++)
  {
    if (flash == list.cend()) {
      LOG_ERROR(log, "No flash no " << flash_num
        << " on the page, there are only "
        << list.size() << " flash objects"
      );
      return;
    }
    ++flash;
  }

  LOG_DEBUG(log, "task1: " << **flash << " is selected");

  string fname = sformat(
    (*flash)->GetElementTagName(), 
    '_', (*flash)->get_id(), ".png"
  );
  replace(fname.begin(), fname.end(), '/', '_');
  replace(fname.begin(), fname.end(), ':', '_');

  (*flash)->take_screenshot_delayed
    (fname, seconds(23), false);
}

}

