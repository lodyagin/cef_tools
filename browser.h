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
#include "RMutex.h"

namespace shared {

DECLARE_AXIS(BrowserAxis, curr::StateAxis);

class videobuffer
{
public:
  struct alignas (4) point {
    uint8_t blue, green, red, alpha;
  };

  typedef boost::multi_array<point, 2> point_buffer;

  const int width, height;

  videobuffer(int width_, int height_);

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
  curr::RMutex mx = { "videobuffer::mx" };

private:
  typedef curr::Logger<videobuffer> log;
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
    int width = 2700;
    int height = 2700;

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

  CefBrowser* get_cef_browser()
  {
    return br.get();
  }

  videobuffer& get_vbuf()
  {
    return vbuf;
  }

  void get_dims(int& width, int& height) const
  {
    width = vbuf.width;
    height = vbuf.height;
  }

  const std::string url;
  const CefRefPtr<CefBrowser> br;

protected:
  browser(const curr::ObjectCreationInfo& oi, 
          const Par& par);

private:
  typedef curr::Logger<browser> log;
};

std::ostream& 
operator<<(std::ostream& out, const browser& br);

#if 0
class browser_repository final
  : public curr::SAutoSingleton<browser_repository>,
    public curr::Repository
      <browser, browser::Par, std::map, int>
{
  using Parent = curr::Repository
    <browser, browser::Par, std::map, int>;

public:
  browser_repository();

  browser* create_object(const browser::Par& p) override;

  // Return the browser object by CefBrowser ptr
  browser* get_object_by_cefbrowser
    (const CefBrowser* br) const
  {
    RLOCK(this->objectsM);
    return cefindex.at(br);
  }

protected:
  typedef std::map<const CefBrowser*, browser*> cefindex_t;
  cefindex_t cefindex;

private:
  typedef curr::Logger<browser_repository> log;
};
#else
using browser_repository =
  curr::AutoRepository<browser, int>;
#endif

}

#endif
