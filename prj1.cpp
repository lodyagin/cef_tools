// -*-coding: mule-utf-8-unix; fill-column: 58; -*-
/**
 * @file
 * The CEF Qt browser.
 *
 * @author Sergei Lodyagin
 */

#pragma GCC diagnostic ignored "-Wunused-local-typedefs"
#include <QtGui>
#pragma GCC diagnostic warning "-Wunused-local-typedefs"

namespace prj1 {
namespace window {

class browser : public QWidget
{
public:
//  using QWidget
};

}
}

using namespace prj1;

int main(int argc, char* argv[])
{
  QApplication app(argc, argv);

  window::browser browser;
  browser.show();

  return app.exec();
}
