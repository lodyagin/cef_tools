// -*-coding: mule-utf-8-unix; fill-column: 58; -*-
/**
 * @file
 *
 * @author Sergei Lodyagin
 */

#include <iostream>
#include "offscreen.h"

int main(int argc, char* argv[])
{
  std::cout << "main started: ";
  for (int i = 1; i < argc; i++)
    std::cout << argv[i] << " ";
  std::cout << std::endl;

  return offscreen(argc, argv);
}
