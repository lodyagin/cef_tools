#include "browser.h"
#include "AutoRepository.hpp"

namespace offscreen {

browser::browser(const curr::ObjectCreationInfo& oi, 
                 const Par& par)
  : url(par.url)
{
  br = CefBrowserHost::CreateBrowserSync
    (par.window_info,
     new handler(),
     url,
     par.settings,
     nullptr);
  SCHECK(br.get());
}

browser::~browser()
{
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
