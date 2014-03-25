#include "AutoRepository.hpp"

#include "browser.h"
#include "proc_browser.h"
#include "task.h"

namespace shared {

using namespace curr;

DEFINE_AXIS_NS(
  BrowserAxis,
  { "created",
    "dom_ready",
    "destroying"
  },
  { { "created", "dom_ready" },
//    { "created", "destroying" }, //?
    { "dom_ready", "destroying" }
  }
);

DEFINE_STATE_CONST(browser, State, created);
DEFINE_STATE_CONST(browser, State, dom_ready);
DEFINE_STATE_CONST(browser, State, destroying);

int browser::Par::
get_id(const curr::ObjectCreationInfo&) const
{
  // to get the id we need create the browser ...

  SCHECK(br_id != 0);
  if (br_id > 0)
    return br_id; // already created, not the first call

  assert(br.get() == nullptr);

  REQUIRE_UI_THREAD();
  br = CefBrowserHost::CreateBrowserSync
    (window_info,
     new ::browser::handler::client,
     url,
     settings,
     nullptr);

  SCHECK(br.get());
  br_id = br->GetIdentifier();
  SCHECK(br_id > 0);
  return br_id;
}

browser::browser(const curr::ObjectCreationInfo& oi, 
                 const Par& par)
  : RObjectWithStates(createdState),
    url(par.url),
    br(par.br)
{
}

browser::~browser()
{
  move_to(*this, destroyingState);
  br->GetHost()->CloseBrowser
    (true /* without asking a user */);

  // TODO wait, states
}

std::ostream& 
operator<<(std::ostream& out, const browser& br)
{
  out << "browser{url=" << br.url << "}";
  return out;
}

}
