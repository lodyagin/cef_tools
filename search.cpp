// -*-coding: mule-utf-8-unix; fill-column: 58; -*-
/**
 * @file
 * Search elements in DOM.
 *
 * @author Sergei Lodyagin
 */

#include "search.h"
#include "browser.h"
#include "xpath.h"
#include "string_utils.h"

using namespace curr;

namespace renderer { namespace search {

#if 0
void flash::Execute()
{
  using namespace shared;

  struct Visitor : CefDOMVisitor
  {
    void Visit(CefRefPtr<CefDOMDocument> d) override
    {
      LOG_DEBUG(log, 
        xpath::select("object", d->GetBody())
      );
    }
    IMPLEMENT_REFCOUNTING();
    typedef curr::Logger<Visitor> log;
  };

  browser_repository::instance()
    . get_object_by_id(browser_id) -> br
    -> GetMainFrame() -> VisitDOM(new Visitor);
}
#endif

}}
