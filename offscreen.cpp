// -*-coding: mule-utf-8-unix; fill-column: 58; -*-
/**
 * @file
 * The CEF off-screen browser.
 *
 * @author Sergei Lodyagin
 */

#include <iostream>
#include <assert.h>
#include "include/cef_app.h"
#include "include/cef_client.h"
#include "include/cef_task.h"

#include "Logging.h"
#include "RThread.h"

#include "offscreen.h"
#include "task.h"
#include "browser.h"
#include "proc_browser.h"
#include "dom_event.h"
#include "search.h"
#include "screenshotter.h"

using namespace curr;

//! The code used by a renderer process only
namespace renderer {
namespace handler {

class load : public CefLoadHandler
{
public:
  void OnLoadStart(CefRefPtr<CefBrowser> br,
                   CefRefPtr<CefFrame> fr) override;
private:
  IMPLEMENT_REFCOUNTING(load);
};

class renderer : public CefRenderProcessHandler
{
public:
  renderer(const std::function<void()>& on_browser_created)
    : on_created(on_browser_created)
  {
    assert(on_created);
  }

  CefRefPtr<CefLoadHandler> GetLoadHandler() override
  {
    return new load;
  }

  void OnBrowserCreated
    (CefRefPtr<CefBrowser> br) override;

  void OnBrowserDestroyed
    (CefRefPtr<CefBrowser> br) override;

protected:
  std::function<void()> on_created;

private:
  IMPLEMENT_REFCOUNTING(renderer);
  typedef Logger<renderer> log;
};

}
}

//! The code shared for all processes.
namespace shared {

class application : public CefApp
{
public:
  application(const std::function<void()>& render_thread)
    : th_render(render_thread)
  {}

  //! Gets CefBrowserProcessHandler from browser process
  //! only. Use CefRenderProcessHandler for access same
  //! things from a renderer process.
  CefRefPtr<CefBrowserProcessHandler> 
  GetBrowserProcessHandler() override
  {
    return new ::browser::handler::browser;
  }

  CefRefPtr<CefRenderProcessHandler> 
  GetRenderProcessHandler() override
  {
    return new renderer::handler::renderer(th_render);
  }

protected:
  std::function<void()> th_render;

private:
  IMPLEMENT_REFCOUNTING(application);
};

}

int offscreen(
  int argc, 
  char* argv[],
  const std::function<void()>& render_thread
)
{
  using namespace shared;

  const CefMainArgs main_args(argc, argv);
  CefRefPtr<application> app(
    new application(render_thread)
  );

  // Subprocess executor
  const int sub_exit = CefExecuteProcess
    (main_args, app.get(), nullptr); // TODO without .get?
  if (sub_exit >= 0)
    // it was a sub process, it is done, return
    return sub_exit;

  // main process only:

#if 0
  gtk_init(&argc, &argv);
#endif

  CefSettings settings;
  CefInitialize(main_args, settings, app.get(), nullptr);
#if 0
  screenshot::take_delayed(
    1, 
    CefRect(0, 0, 2700, 2700), 
    "test.png", 
    28
  );
#endif
  CefRunMessageLoop();
  CefShutdown();
  return 0;
}

namespace renderer { namespace handler {

void load::OnLoadStart
  (CefRefPtr<CefBrowser> br,
   CefRefPtr<CefFrame> fr)
{
  assert(br.get());
  assert(fr.get());

  struct Visitor : CefDOMVisitor
  {
    Visitor(shared::browser* br) : the_browser(br) 
    {
      SCHECK(br);
    }

    struct Listener : CefDOMEventListener
    {
      Listener(shared::browser* br) : the_browser(br) {}

      void HandleEvent(CefRefPtr<CefDOMEvent> ev) override
      {
        LOG_DEBUG(log, 
          "thread " 
          << RThread<std::thread>::current_pretty_id()
          << "> HandleEvent: ev=" << ev
          << ", dom="
          << ev->GetDocument()->GetBaseURL().ToString()
          << std::endl;
        );
        // FIXME ensure the_browser is not destroyed yet
        compare_and_move(
          *the_browser, 
          shared::browser::createdState,
          shared::browser::dom_readyState);
      }

      shared::browser* the_browser;
      IMPLEMENT_REFCOUNTING(Listener);
      typedef Logger<Listener> log;
    };

    void Visit(CefRefPtr<CefDOMDocument> d) override
    {
      if (auto root_node = d->GetDocument()) {
        root_node->AddEventListener
          (L"DOMContentLoaded", new Listener(the_browser), true);
      }
      else assert(false);
    }

    shared::browser* the_browser;

    IMPLEMENT_REFCOUNTING(Visitor);
  };

  REQUIRE_RENDERER_THREAD(); // for VisitDOM
  if (fr->IsMain()) {
    fr->VisitDOM(
      new Visitor(
        shared::browser_repository::instance()
          . get_object_by_id(br->GetIdentifier())
      )
    );
  }
}

void renderer::OnBrowserCreated(CefRefPtr<CefBrowser> br)
{
  using namespace shared;

  // register the new browser in the browser_repository
  browser_repository::instance().create_object
    (shared::browser::Par(br)) -> id;

  // Start the on_create thread
  if (on_created) {
    StdThread::create<LightThread>(
      on_created, "render_thread"
    )->start();
  }
}

void renderer::OnBrowserDestroyed(CefRefPtr<CefBrowser> br)
{
  using namespace shared;

  assert(br.get());
  const int br_id = br->GetIdentifier();
  assert(br_id > 0);
  
  // register the new browser in the browser_repository
  browser_repository::instance().delete_object_by_id
    (br_id, true);
}

}}


