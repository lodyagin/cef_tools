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
#include "browser.h"

#define REQUIRE_UI_THREAD()   assert(CefCurrentlyOn(TID_UI));
#define REQUIRE_IO_THREAD()   assert(CefCurrentlyOn(TID_IO));
#define REQUIRE_FILE_THREAD() assert(CefCurrentlyOn(TID_FILE));

namespace offscreen {

class application 
  : public CefApp, 
    public CefBrowserProcessHandler
{
public:
  /* CefApp */
  CefRefPtr<CefBrowserProcessHandler> 
  GetBrowserProcessHandler() override
  {
    return this;
  }

  /* CefBrowserProcessHandler */
  void OnContextInitialized() override;

private:
  IMPLEMENT_REFCOUNTING(application);
};

class handler : public CefClient
{
private:
  IMPLEMENT_REFCOUNTING(application);
};

}

using namespace offscreen;

int main(int argc, char* argv[])
{
  std::cout << "main started" << std::endl;
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
  CefRunMessageLoop();
  CefShutdown();
}

namespace offscreen {

void application::OnContextInitialized() 
{
  REQUIRE_UI_THREAD();

  std::string url;

  // Check if a "--url=" value was provided via the command-line. If so, use
  // that instead of the default URL.
  CefRefPtr<CefCommandLine> command_line =
      CefCommandLine::GetGlobalCommandLine();
  url = command_line->GetSwitchValue("url");
  if (url.empty())
  url = "http://ibm.com";

  // Create the first browser window.
  browser_rep::instance().create_object(url);
}

}


