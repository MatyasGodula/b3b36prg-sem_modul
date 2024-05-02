#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>

#define MAJOR 1
#define MINOR 2
#define PATCH 12


#include "main.h"
#include "event_queue.h"
#include "utils.h"
#include "computation.h"
#include "messages.h"

/*
 * message is statically allocated in main_thread, also the message from read pipe
 * is being freed in the react_to_message function
 */

// data is containing the pipe out for sending messages to prgsem
void* main_thread(void* data) 
{
    my_assert(data != NULL, __func__, __LINE__, __FILE__);
    int pipe_out = *(int*)data;
    message msg;
    uint8_t msg_buffer[sizeof(message)];
    int msg_len;
    bool quit = false;
    event ev = { .type = EV_STARTUP };
    queue_push(ev);
    // inspired by prgsem, but will probably delete a lot of it since there are not 
    // many events happening in this module
    do {
        event current_event = queue_pop();
        msg.type = MSG_NBR;
        switch (current_event.type) {
            case EV_STARTUP:
                debug("startup message set");
                msg.type = MSG_STARTUP;
                break;
            case EV_PIPE_IN_MESSAGE:
                react_to_message(&current_event, &msg);
                break;
            case EV_CALCULATE_NEXT_PIXEL:
                if (!aborted_computation() && !done_computing()) {
                    compute_pixel(&msg);
                    event ev = { .type = EV_CALCULATE_NEXT_PIXEL };
                    queue_push(ev);
                } else if (done_computing()) {
                    info("sending msg_done");
                    msg.type = MSG_DONE;
                }
                break;
            default:
                break;
        }
        if (msg.type != MSG_NBR) {
            my_assert(fill_message_buf(&msg, msg_buffer, sizeof(message), &msg_len), __func__, __LINE__, __FILE__);
            if (write(pipe_out, msg_buffer, msg_len) == msg_len) {
                debug("message successfully sent");
            } else {
                error("send message failed");
            }
        }
    } while (!quit);
    return NULL;
}

void react_to_message(event* const current_event, message* const msg_pipe_out) {
    my_assert(current_event != NULL && current_event->type == EV_PIPE_IN_MESSAGE && current_event->data.msg, __func__, __LINE__, __FILE__);
    current_event->type = EV_TYPE_NUM;
    message* msg_pipe_in = current_event->data.msg;
    switch (msg_pipe_in->type) {
        case MSG_SET_COMPUTE:
            if (!currently_computing()) {
                // sets up the initial computation for the pixels
                set_up_computation(msg_pipe_in, msg_pipe_out);
                debug("computation set up correctly");
            } else {
                error("Computation set up failed: already computing");
                msg_pipe_out->type = MSG_ERROR;
            }
            break;
        case MSG_ABORT:
            if (!aborted_computation()) {
                // causes the next pixel computation to not push "compute_next_pixel"
                abort_computation(msg_pipe_out);
                debug("abortion set up");
            } else {
                error("Current computation already aborted");
            }
            break;
        case MSG_COMPUTE:
            if (!currently_computing()) {
                // sets up computation for the current chunk 
                set_up_chunk_computation(msg_pipe_in, msg_pipe_out);
                // starts the "recursive" pixel computation
                event ev = { .type = EV_CALCULATE_NEXT_PIXEL };
                queue_push(ev);
            } else {
                error("Computation is already ongoing");
                msg_pipe_out->type = MSG_ERROR;
            }
            break;
        case MSG_GET_VERSION:
            msg_pipe_out->type = MSG_VERSION;
            msg_pipe_out->data.version.major = MAJOR;
            msg_pipe_out->data.version.minor = MINOR;
            msg_pipe_out->data.version.patch = PATCH;
            break;
    }
    free(current_event->data.msg);
    current_event->data.msg = NULL;
}