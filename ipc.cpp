// -*-coding: mule-utf-8-unix; fill-column: 58; -*-
/**
 * @file
 * CEF IPC wrapper
 *
 * @author Sergei Lodyagin
 */

#include "ipc.h"

namespace process {
std::atomic<CefProcessId> current((cef_process_id_t)-1);
}

namespace ipc {
namespace receiver {

std::ostream&
operator<< (std::ostream& out, const entry_base& entr)
{
  return out << "entry[" << entr.universal_id() << "]";
}

bool repository::call(CefRefPtr<CefProcessMessage> msg)
{
  assert(msg.get());
  entry_base* entry = nullptr;
  try {
    entry = this->get_object_by_id(msg->GetName());
  }
  catch(const NoSuchId&) {
    return false;
  }
  assert(entry);
  entry->call(msg);
  return true;
}

} // receiver
} // ipc
