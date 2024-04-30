#ifndef __COMPUTATION_H__
#define __COMPUTATION_H__

#include "messages.h"

// needs to push ev_ok if successful
void set_up_computation(message* msg);
bool currently_computing();
// needs to push ev with msg_abort
void abort_computation();
bool aborted_computation();

// sets up important info for computing a chunk
void set_up_chunk_computation(message* msg);
void compute_next_pixel();


#endif