#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>

#define MAJOR 1
#define MINOR 1
#define PATCH 1


#include "main.h"
#include "event_queue.h"
#include "utils.h"

// data is containing the pipe out for sending messages to prgsem
void* main_thread(void* data) 
{
    my_assert(data != NULL, __func__, __LINE__, __FILE__);
    int pipe_out = *(int*)data;
    message msg;
    uint8_t msg_buffer[sizeof(message)];
    int msg_len;
    bool quit = false;
    // inspired by prgsem, but will probably delete a lot of it since there are not 
    // many events happening in this module
    do {
        event current_event = queue_pop();
        msg.type = MSG_NBR;
        switch (current_event.type) {

        }
    } while (!quit);
}

void react_to_message(event* const current_event) {
    my_assert(current_event != NULL && current_event->type == EV_PIPE_IN_MESSAGE && current_event->data.msg, __func__, __LINE__, __FILE__);
    current_event->type = EV_TYPE_NUM;
    message* msg = current_event->data.msg;
    switch (msg->type) {
        case MSG_SET_COMPUTE:
            if (!currently_computing()) {
                event ev = { .type = EV_SET_COMPUTE };
                ev.data.msg = msg;
                queue_push(ev);
            } else {
                error("Computation set up failed: already computing");
            }
        case MSG_ABORT:
            if (!aborted_computation()) {
                event ev = { .type = EV_ABORT };
                queue_push(ev);
            } else {
                error("Current computation already aborted");
            }
        case MSG_COMPUTE:
            if (!currently_computing()) {
                event ev = { .type = EV_START_COMPUTING, };
                ev.data.msg = msg;
                queue_push(ev);
            } else {
                error("Computation is already initialized");
            }
        case MSG_GET_VERSION:
            event ev = { .type = EV_SEND_MESSAGE };
            msg->type = MSG_VERSION;
            msg->data.version.major = MAJOR;
            msg->data.version.minor = MINOR;
            msg->data.version.patch = PATCH;
            ev.data.msg = msg;
            queue_push(ev);
    }
}