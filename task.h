#ifndef OFFSCREEN_TASK_H
#define OFFSCREEN_TASK_H

#include "include/cef_task.h"

#define REQUIRE_UI_THREAD()   assert(CefCurrentlyOn(TID_UI));
#define REQUIRE_IO_THREAD()   assert(CefCurrentlyOn(TID_IO));
#define REQUIRE_FILE_THREAD() assert(CefCurrentlyOn(TID_FILE));
#define REQUIRE_RENDERER_THREAD() assert(CefCurrentlyOn(TID_RENDERER));

#endif
