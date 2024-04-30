#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <pthread.h>

#include "utils.h"
#include "prg_io_nonblock.h"
#include "messages.h"
#include "event_queue.h"
#include "main.h"
#include "computation.h"

void* read_pipe_thread(void*); 

#ifndef IO_READ_TIMEOUT_MS
#define IO_READ_TIMEOUT_MS 10
#endif

int main(int argc, char* argv[]) 
{
    int ret = EXIT_SUCCESS;
	
    /*
     * copied these lines from prgsem just changed pipe out and pipe in
     * also changed thread functions
     */
    const char* fname_pipe_out = argc > 1 ? argv[1] : "/tmp/computational_module.out";
    const char* fname_pipe_in = argc > 2 ? argv[2] : "/tmp/computational_module.in";

    int pipe_in = io_open_read(fname_pipe_in);
    int pipe_out = io_open_write(fname_pipe_out);

    printf("finished opening pipes\n");
    my_assert(pipe_in != -1 && pipe_out != -1, __func__, __LINE__, __FILE__);

    enum {READ_PIPE_THREAD, MAIN_THREAD, NUM_THREADS, COMPUTATION_THREAD};
    const char* thread_names[] = {"ReadPipe", "MainThread", "ComputationThread"};
    pthread_t threads[NUM_THREADS];
    void* (*thread_functions[])(void*) = {read_pipe_thread, main_thread, computation_thread};
    void* thread_data[NUM_THREADS] = {};
    thread_data[READ_PIPE_THREAD] = &pipe_in;
    thread_data[MAIN_THREAD] = &pipe_out;

	/*
	 *
	 * plan: make the calculation function so i can start debugging, the main.c program is 
	 * pretty well constructed but i had to change the handling of the messages
	 * 
	 */


    for (int i = 0; i < NUM_THREADS; ++i) {
		int r = pthread_create(&threads[i], NULL, thread_functions[i], thread_data[i]);
		printf("Create thread '%s' %s\r\n", thread_names[i], ( r == 0 ? "OK" : "FAIL") );
	}

	int *ex;
	for (int i = 0; i < NUM_THREADS; ++i) {
		printf("Call join to the thread %s\r\n", thread_names[i]);
		int r = pthread_join(threads[i], (void*)&ex);
		printf("Joining the thread %s has been %s\r\n", thread_names[i], (r == 0 ? "OK" : "FAIL"));
	}

    io_close(pipe_in);
    io_close(pipe_out);

    return ret;
}

void* read_pipe_thread(void* data)
{
	my_assert(data != NULL, __func__, __LINE__, __FILE__);
	int pipe_in = *(int*)data;
	fprintf(stderr, "read_pipe thread started\n");
	bool end = false;
	uint8_t msg_buf[sizeof(message)];
	int i = 0;
	int len = 0;

	unsigned char c;
	while (io_getc_timeout(pipe_in, IO_READ_TIMEOUT_MS, &c) > 0) {}; // discard garbage

	while (!end) {
		int ret = io_getc_timeout(pipe_in, IO_READ_TIMEOUT_MS, &c);
		if (ret > 0) { // char has been read
			if (i == 0) {
				if (get_message_size(c, &len)) {
					msg_buf[i++] = c;
				} else {
					fprintf(stderr, "Unknown message type detected 0x%x\n", c);
				}
			} else { // read remaining bytes of message
				msg_buf[i++] = c;
			}
			if (len  > 0 && i == len) {
				message* msg = my_alloc(sizeof(message));
				if (parse_message_buf(msg_buf, len, msg)) {
					event ev = { .type = EV_PIPE_IN_MESSAGE };
					ev.data.msg = msg;
					queue_push(ev);
				} else {
					fprintf(stderr, "Error: Cannot parse message type %d\n", msg_buf[0]);
					free(msg);
				}
				i = len = 0;
			}
		} else if (ret == 0) { // timeout happened

		} else { // error occurred
			fprintf(stderr, "Error: problem reading from a file\n");
			set_quit();
			event ev = { .type = EV_ABORT }; // deactivated for now
			/*
			 * added queue_push to push the quit into the queue 
			 */
			queue_push(ev);
		}
		end = is_quit();
	} // end of while cycle
	
	fprintf(stderr, "read_pipe thread ended\n");
    return NULL;
}
