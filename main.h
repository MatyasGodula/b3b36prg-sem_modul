#ifndef __MAIN_H__
#define __MAIN_H__

#include "event_queue.h"

void* main_thread(void* data);
void react_to_message(event* const current_event);

#endif