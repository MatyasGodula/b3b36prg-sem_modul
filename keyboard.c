#include "keyboard.h"
#include "event_queue.h"
#include "utils.h"

#include <sys/select.h>

void* keyboard_thread(void* data)
{
    fprintf(stderr, "Keyboard thread started\n");
    call_termios(0);
    int c;
    event ev;
    while (!is_quit() && (c = getchar()) != 'q') { } // end of while loop
    set_quit();
    ev.type = EV_QUIT;
    queue_push(ev);
    call_termios(1); // restore terminal
    fprintf(stderr, "Keyboard thread ended\n");
    return NULL;
}