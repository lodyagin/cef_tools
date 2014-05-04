// -*-coding: mule-utf-8-unix; fill-column: 58; -*-
/**
 * @file
 * CEF IPC wrapper
 *
 * @author Sergei Lodyagin
 */

namespace ipc {
namespace receiver {

void repository::call(CefRefPtr<CefProcessMessage> msg)
{
  assert(msg.get());
  this->get_object_by_id(msg->GetName())->call(msg);
}

} // receiver
} // ipc
