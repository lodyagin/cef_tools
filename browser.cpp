// -*-coding: mule-utf-8-unix; fill-column: 58; -*-
/**
 * @file
 *
 * @author Sergei Lodyagin
 */

#define IMG

#ifdef IMG
#  include "screenshotter.h"
#endif

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
     new ::browser::handler::client(*this),
     url,
     settings,
     nullptr);

  SCHECK(br.get());
  br_id = br->GetIdentifier();
  SCHECK(br_id > 0);
  LOG_DEBUG(log, "set_browser_id");
  return br_id;
}

browser::browser(const curr::ObjectCreationInfo& oi, 
                 const Par& par)
  : RObjectWithEvents(createdState),
    CONSTRUCT_EVENT(dom_ready),
    id(par.br->GetIdentifier()),
    vbuf(par.width, par.height),
    url(par.url),
    br(par.br)
{
}

browser::~browser()
{
  move_to(*this, destroyingState);

  if(!CefCurrentlyOn(TID_RENDERER)) {
    br->GetHost()->CloseBrowser
      (true /* without asking a user */);
  }

  // TODO wait, states
}

std::ostream& 
operator<<(std::ostream& out, const browser& br)
{
  out << "browser{url=" << br.url << "}";
  return out;
}

#if 0
browser_repository::browser_repository()
  : Parent(type<browser_repository>::name(), 0)
{
  this->complete_construction();
}

browser* browser_repository
::create_object(const browser::Par& p)
{
  RLOCK(this->objectsM);
  browser* res = Parent::create_object(p);
  assert(res);
  LOG_DEBUG(log, "browser_repository::create_browser for ("
    << res->br.get() << ')');
  return cefindex[res->br.get()] = res;
  
}
#endif

videobuffer::videobuffer(int width_, int height_)
  : width(width_), height(height_),
    buf(boost::extents[height][width])
{
  SCHECK(width > 0);
  SCHECK(height > 0);
}

void videobuffer::on_paint(
  int x, 
  int y, 
  int w, 
  int h,
  const point* buffer
)
{
  static int img_cnt = 1;

  typedef point_buffer::index_range range;
  typedef boost::const_multi_array_ref<point, 2> 
    source_buf;

  source_buf src_buf(
    buffer, 
    boost::extents[height][width]
  );

  source_buf::const_array_view<2>::type src =
    src_buf[point_buffer::index_gen()
      [range(y, y + h)]
      [range(x, x + w)]
    ];

  // select a rectangular write region
  point_buffer::array_view<2>::type dst =
    buf[point_buffer::index_gen()
      [range(y, y + h)]
      [range(x, x + w)]
    ];

#ifdef IMG
  png::image<png::rgba_pixel> img(w, h);
#endif

  {
    RLOCK(mx);
#if 0
    dst = src;
#else
  // write the buffer into the region
//  int i = 0;

#if 0
  for (point_buffer::index row = 0; row < height; row++)
  {
    for (point_buffer::index col = 0; col < width; col++)
    {
      buf[row][col] = src_buf[row][col];
    }
  }
#else
  for (point_buffer::index row = 0; row < h; row++)
  {
    for (point_buffer::index col = 0; col < w; col++)
    {
      dst[row][col] = src[row][col];
    }
  }
#endif
#endif
#ifdef IMG
  ::operator<<(img, (videobuffer::point_buffer) dst);
  }
  const std::string fname = SFORMAT(
    "test" << img_cnt++ << ".png"
  );
  LOG_DEBUG(log, fname);
  img.write(fname);
#endif
}

videobuffer::point_buffer videobuffer::get_area
  (int x, int y, int width, int height) const
{
  RLOCK(mx);
  typedef point_buffer::index_range range;

  point_buffer::const_array_view<2>::type src =
    buf[point_buffer::index_gen()
      [range(y, y + height)]
      [range(x, x + width)]
    ];

  return src;
}

}
