// -*-coding: mule-utf-8-unix; fill-column: 58; -*-
/**
 * @file
 * Search elements in DOM.
 *
 * @author Sergei Lodyagin
 */

#include <assert.h>
#include "include/cef_task.h"
#include "Logging.h"

namespace renderer { namespace search {

#if 0
//! Search flash plugin objects
class flash : public CefTask
{
public:
  //! \param br_id CEF browser id 
  flash(int br_id) 
    : browser_id(br_id)
  {
    assert(browser_id > 0);
  }

  void Execute() override;

  const int browser_id;

private:
//  typedef curr::Logger<flash> log;
  IMPLEMENT_REFCOUNTING();
};
#endif

}}
