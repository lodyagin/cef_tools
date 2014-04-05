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
  CefRefPtr<CefLoadHandler> GetLoadHandler() override
  {
    return new load;
  }

  void OnBrowserCreated
    (CefRefPtr<CefBrowser> br) override;

  void OnBrowserDestroyed
    (CefRefPtr<CefBrowser> br) override;

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
    return new renderer::handler::renderer;
  }

private:
  IMPLEMENT_REFCOUNTING(application);
};

}

int main(int argc, char* argv[])
{
  using namespace shared;

  std::cout << "main started: ";
  for (int i = 1; i < argc; i++)
    std::cout << argv[i] << " ";
  std::cout << std::endl;

  const CefMainArgs main_args(argc, argv);
  CefRefPtr<application> app(new application);

  // Subprocess executor
  const int sub_exit = CefExecuteProcess
    (main_args, app.get(), nullptr); // TODO without .get?
  if (sub_exit >= 0)
    // it was a sub process, it is done, return
    return sub_exit;

#if 0
  gtk_init(&argc, &argv);
#endif

  CefSettings settings;
  CefInitialize(main_args, settings, app.get(), nullptr);
  screenshot::take_delayed(20);
  CefRunMessageLoop();
  CefShutdown();
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
  const int browser_id = 
    browser_repository::instance().create_object
      (shared::browser::Par(br)) -> id;

  // Start the dom_ready wait thread
  StdThread::create<LightThread>([browser_id]()
  {
    CURR_WAIT(
      browser_repository::instance()
        . get_object_by_id(browser_id)
        -> is_dom_ready(),
      60001
    );

    // post the flash search task
    CefPostTask(
      TID_RENDERER, 
      new ::renderer::search::flash(browser_id)
    );
  })->start();
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


