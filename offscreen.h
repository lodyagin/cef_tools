// -*-coding: mule-utf-8-unix; fill-column: 58; -*-
/**
 * @file
 *
 * @author Sergei Lodyagin
 */

#include <functional>

int offscreen(
  int argc, 
  char* argv[],
  const std::function<void()>& render_thread =
    std::function<void()>(),
  const std::function<void()>& browser_thread =
    std::function<void()>()
);

