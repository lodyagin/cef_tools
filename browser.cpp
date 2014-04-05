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
  : RObjectWithEvents(createdState),
    CONSTRUCT_EVENT(dom_ready),
    id(par.br->GetIdentifier()),
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

browser* browser_repository
::create_object(const browser::Par& p)
{
  RLOCK(this->objectsM);
  browser* res = Parent::create_object(p);
  assert(res);
  cefindex[res->br] = res;
}

browser::videobuffer::videobuffer(int width, int height)
  : buf(boost::extents[height][width])
{
  SCHECK(width > 0);
  SCHECK(height > 0);
}

void browser::videobuffer::on_paint(
  int x, 
  int y, 
  int width, 
  int height,
  const point* buffer
)
{
  typedef point_buffer::index_range range;

  // select a rectangular write region
  point_buffer::array_view<2>::type dst =
    buf[point_buffer::index_gen()
      [range(r.y, r.y + r.height)]
      [range(r.x, r.x + r.width)]
    ];

  // write the buffer into the region
  int i = 0;

  for (point_buffer::index row = 0; row < r.height; row++)
  {
    for (point_buffer::index col = 0; col < r.width; col++)
    {
      dst[row][col] = buffer[i++];
    }
  }
}

browser::videobuffer::point_buffer 
browser::videobuffer::get_area
  (int x, int y, int width, int height) const
{
  typedef point_buffer::index_range range;

  point_buffer::const_array_view<2>::type src =
    buf[point_buffer::index_gen()
      [range(y, y + height)]
      [range(x, x + width)]
    ];

  return src;
}

}
