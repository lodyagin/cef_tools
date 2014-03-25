// -*-coding: mule-utf-8-unix; fill-column: 58; -*-
/**
 * @file
 *
 * @author Sergei Lodyagin
 */

#ifndef OFFSCREEN_BROWSER_H
#define OFFSCREEN_BROWSER_H

#include <iostream>
#include <string>
#include <assert.h>

#include "include/cef_browser.h"
#include "include/cef_client.h"

#include "AutoRepository.hpp"

namespace offscreen {

class browser 
  //: public CefBrowser
    //public RObjectWithStates
{
  friend std::ostream& 
  operator<<(std::ostream& out, const browser& br);

public:
  struct Par
  {
    std::string url;
    CefWindowInfo window_info;
    CefBrowserSettings settings;

    Par(const std::string& url_) : url(url_) {}

    std::string get_id(const curr::ObjectCreationInfo&) const
    {
      return url;
    }

    PAR_DEFAULT_MEMBERS(browser);
  };

  virtual ~browser();

  browser(const browser&) = delete;
  browser& operator=(const browser&) = delete;

  std::string universal_id() const
  {
    return url;
  }

protected:
  class handler : public CefClient
  {
  private:
    IMPLEMENT_REFCOUNTING(application);
  };

  std::string url;

  browser(const curr::ObjectCreationInfo& oi, 
          const Par& par);

#if 0
public:
  browser(CefRefPtr<CefBrowser> br_) : br(br_) 
  {
    assert(br.get());
  }

  CefBrowser* operator->()
  {
    return br.get();
  }
#endif

protected:
  CefRefPtr<CefBrowser> br;
};

std::ostream& 
operator<<(std::ostream& out, const browser& br);

using browser_rep = 
  curr::AutoRepository<browser, std::string>;

}

#endif
