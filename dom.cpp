// -*-coding: mule-utf-8-unix; fill-column: 58; -*-
/**
 * @file
 * The dom node for usage from any thread.
 *
 * @author Sergei Lodyagin
 */

#include "Repository.hpp"
#include "SSingleton.hpp"
#include "dom.h"

namespace renderer {

namespace dom_visitor {
namespace {

class DOMVisitor : public CefDOMVisitor
{
public:
  DOMVisitor(const query_base& q) : query(q) {}

  // DOM is valid only inside this function
  // do not store DOM externally!
  void Visit(CefRefPtr<CefDOMDocument> d) override
  {
    fun(d->GetDocument());
  }

protected:
  query_base& query;

private:
  IMPLEMENT_REFCOUNTING();
};
}}

}

node_repository::list_type node_repository
::query(
  int browser_id,
  const dom_visitor::query_base& q
)
{
  shared::browser_repository::instance()
    . get_object_by_id(browser_id) -> br
    -> GetMainFrame() -> VisitDOM(new DOMVisitor(q));
}

}
