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
#include "include/cef_load_handler.h"

#include "AutoRepository.hpp"
#include "REvent.h"

namespace shared {

DECLARE_AXIS(BrowserAxis, curr::StateAxis);

class browser 
  : public curr::RObjectWithEvents<BrowserAxis>
{
  friend std::ostream& 
  operator<<(std::ostream& out, const browser& br);

  DECLARE_EVENT(BrowserAxis, dom_ready);

public:

  //! @cond
  DECLARE_STATES(BrowserAxis, State);
  DECLARE_STATE_CONST(State, created);
  DECLARE_STATE_CONST(State, dom_ready);
  DECLARE_STATE_CONST(State, destroying);
  //! @endcond

  class Par
  {
    friend class browser;

  public:
    std::string url;
    CefWindowInfo window_info;
    CefBrowserSettings settings;

    //! Create a new CefBrowser
    Par(const std::string& url_);

    //! Register the existing CefBrowser
    Par(CefRefPtr<CefBrowser> br_) 
      : br(br_), br_id(br->GetIdentifier())
    {}

    int get_id(const curr::ObjectCreationInfo&) const;

    PAR_DEFAULT_MEMBERS(browser);

  protected:
    mutable CefRefPtr<CefBrowser> br;
    mutable int br_id = -1;
  };

  const int id;

  virtual ~browser();

  browser(const browser&) = delete;
  browser& operator=(const browser&) = delete;

  std::string universal_id() const
  {
    return curr::toString(id);
  }

  curr::CompoundEvent is_terminal_state() const override
  {
    // TODO
    return curr::CompoundEvent();
  }

  const std::string url;
  const CefRefPtr<CefBrowser> br;

protected:
  browser(const curr::ObjectCreationInfo& oi, 
          const Par& par);
};

std::ostream& 
operator<<(std::ostream& out, const browser& br);

#if 1
using browser_repository = 
  curr::AutoRepository<browser, int>;
#else
class browser_rep
  : public curr::AutoRepository<browser, int>
{
public:
  using Parent = curr::AutoRepository<browser, int>;

  browser* get_object_by_id(int id) override
  {
    try {
      return Parent::get_object_by_id(id);
    }
    catch (const NoSuchId&) {
      return create_object(browser::Par());
    }
  }
};
#endif

}

#endif
