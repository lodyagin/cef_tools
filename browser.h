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
#include <map>
#include <assert.h>

#include <boost/multi_array.hpp>

#include "include/cef_browser.h"
#include "include/cef_client.h"
#include "include/cef_load_handler.h"

#include "AutoRepository.hpp"
#include "REvent.h"

namespace shared {

DECLARE_AXIS(BrowserAxis, curr::StateAxis);

class videobuffer
{
public:
  struct alignas (4) point {
    uint8_t red, green, blue, alpha;
  };

  typedef boost::multi_array<point, 2> point_buffer;

  videobuffer(int width, int height);

  void on_paint(
    int x, 
    int y, 
    int width, 
    int height,
    const point* buffer
  );

  //! Return the rectangular part of buf
  point_buffer get_area
    (int x, int y, int width, int height) const;

protected:
  point_buffer buf;
};

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
    Par(const std::string& url_) : url(url_) {}

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
  videobuffer vbuf;

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

class browser_repository_impl
  : public curr::Repository
      <browser, browser::Par, std::map, int>
{
  using Parent = curr::Repository
    <browser, browser::Par, std::map, int>;

public:
  browser* create_object(const browser::Par& p) override;

  // Return the browser object by CefBrowser ptr
  browser* get_object_by_cefbrowser(const CefBrowser* br) const
  {
    RLOCK(this->objectsM);
    return cefindex.at(br);
  }

protected:
  typedef std::map<const CefBrowser*, browser*> cefindex_t;
  cefindex_t cefindex;
};

using browser_repository = 
  curr::AutoRepository<browser, int>;


}

#endif
