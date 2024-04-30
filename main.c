#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>

#define MAJOR 1
#define MINOR 1
#define PATCH 1


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
    event ev = { .type = EV_STARTUP};
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
            case EV_OK:
                msg.type = MSG_OK;
                break;
            case EV_ERROR:
                msg.type = MSG_ERROR;
                break;
            case EV_PIPE_IN_MESSAGE:
                react_to_message(&current_event);
                break;
            case EV_SEND_VERSION:
                msg.type = MSG_VERSION;
                msg.data.version.major = MAJOR;
                msg.data.version.minor = MINOR;
                msg.data.version.patch = PATCH;
                info("get version message set");
                break;
            case EV_CALCULATE_NEXT_PIXEL:
                compute_next_pixel();
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

void react_to_message(event* const current_event) {
    my_assert(current_event != NULL && current_event->type == EV_PIPE_IN_MESSAGE && current_event->data.msg, __func__, __LINE__, __FILE__);
    current_event->type = EV_TYPE_NUM;
    message* msg = current_event->data.msg;
    switch (msg->type) {
        case MSG_SET_COMPUTE:
            if (!currently_computing()) {
                // sets up the initial computation for the pixels
                set_up_computation(msg);
            } else {
                error("Computation set up failed: already computing");
                event ev = { .type = EV_ERROR };
                queue_push(ev);
            }
            break;
        case MSG_ABORT:
            if (!aborted_computation()) {
                // causes the next pixel computation to not push "compute_next_pixel"
                abort_computation();
            } else {
                error("Current computation already aborted");
            }
            break;
        case MSG_COMPUTE:
            if (!currently_computing()) {
                event ev = { .type = EV_OK };
                queue_push(ev);
                // sets up computation for the current chunk 
                set_up_chunk_computation(msg);
                // starts the "recursive" pixel computation
                compute_next_pixel();
            } else {
                error("Computation is already ongoing");
            }
            break;
        case MSG_GET_VERSION:
            info("Get version command received");
            event ev = { .type = EV_SEND_VERSION };
            queue_push(ev);
            break;
    }
    free(current_event->data.msg);
    current_event->data.msg = NULL;
}