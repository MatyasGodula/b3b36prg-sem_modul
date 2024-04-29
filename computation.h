#ifndef __COMPUTATION_H__
#define __COMPUTATION_H__

#include "messages.h"

void set_up_computation(message* msg);
bool currently_computing();
bool aborted_computation();

#endif