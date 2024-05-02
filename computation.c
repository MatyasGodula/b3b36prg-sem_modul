#include <stdlib.h>
#include <stdbool.h>

#include "messages.h"
#include "event_queue.h"
#include "computation.h"
#include "utils.h"


static struct {

    double constant_real;
    double constant_imag;

    double real_increment; // the increment for each pixel on x axis
    double imag_increment; // the increment for each pixel on y axis

    uint8_t n; // number of iteration per pixel

    // current chunk computation
    uint8_t cid; // chunk id
    double start_real_chunk; // start of the real coordinates for this chunk
    double start_imag_chunk; // start of the imaginary coordinates for this chunk
    uint8_t n_real; // number of real cells
    uint8_t n_imag; // number of imaginary cells
    double end_real_chunk;
    double end_imag_chunk;
    int pixels_to_calculate;
    int pixels_calculated;

    // current pixel to be computed, it is changed at the end of computation
    double real_coords;
    uint8_t x_calculated;
    double imag_coords;
    uint8_t y_calculated;

    bool computing;
    bool done;
    bool aborted;
    bool set_up;

} data = {
    .computing = false,
    .done = false,
    .aborted = false,
    .set_up = false
};

void set_up_computation(message* msg_pipe_in, message* msg_pipe_out)
{
    data.constant_imag = msg_pipe_in->data.set_compute.c_im;
    data.constant_real = msg_pipe_in->data.set_compute.c_re;
    data.real_increment = msg_pipe_in->data.set_compute.d_re;
    data.imag_increment = msg_pipe_in->data.set_compute.d_im;
    data.n = msg_pipe_in->data.set_compute.n;
    data.set_up = true;
    msg_pipe_out->type = MSG_OK;
}

bool currently_computing() { return data.computing; }

void abort_computation(message* msg_pipe_out)
{
    data.computing = false;
    data.aborted = true;
    msg_pipe_out->type = MSG_ABORT;
}

bool aborted_computation() { return data.aborted; }

void set_up_chunk_computation(message* msg_pipe_in, message* msg_pipe_out)
{
    if (data.set_up && !data.computing) {
        data.cid = msg_pipe_in->data.compute.cid;
        data.start_real_chunk = msg_pipe_in->data.compute.re;
        data.start_imag_chunk = msg_pipe_in->data.compute.im;
        data.n_real = msg_pipe_in->data.compute.n_re;
        data.n_imag = msg_pipe_in->data.compute.n_im;
        data.end_real_chunk = data.start_real_chunk + (data.n_real * data.real_increment);
        data.end_imag_chunk = data.start_imag_chunk + (data.n_imag * data.imag_increment);
        // sets upp the coord numbers for sending in compute_data
        data.x_calculated = 0;
        data.y_calculated = 0;
        data.computing = true;
        data.aborted = false;
        data.done = false;
        data.real_coords = data.start_real_chunk;
        data.imag_coords = data.start_imag_chunk;
        data.pixels_calculated = 0;
        data.pixels_to_calculate = data.n_real * data.n_imag;
        msg_pipe_out->type = MSG_OK;
    } else {
        msg_pipe_out->type = MSG_ERROR;
    }
    
}

uint8_t iteration_calculation()
{
    double z_real = data.real_coords;
    double z_imag = data.imag_coords;
    double const_real = data.constant_real;
    double const_imag = data.constant_imag;
    uint8_t max_iters = data.n;
    double z_real2 = z_real * z_real;
    double z_imag2 = z_imag * z_imag;

    uint8_t iters = 0;

    while ((z_real2 + z_imag2) <= 4.0 && iters < max_iters) {
        double temp_real = z_real2 - z_imag2 + const_real;
        double temp_imag = 2.0 * z_real * z_imag + const_imag;

        z_real = temp_real;
        z_imag = temp_imag;

        z_real2 = z_real * z_real;
        z_imag2 = z_imag * z_imag;

        iters++;
    }
    return iters;
}

void compute_pixel(message* msg_pipe_out) 
{
    if (data.set_up && !data.aborted && data.computing && !data.done) {
        uint8_t iters = iteration_calculation();
        msg_pipe_out->type = MSG_COMPUTE_DATA;
        msg_pipe_out->data.compute_data.cid = data.cid;
        msg_pipe_out->data.compute_data.iter = iters;
        msg_pipe_out->data.compute_data.i_re = data.x_calculated;
        msg_pipe_out->data.compute_data.i_im = data.y_calculated;
        data.pixels_calculated += 1;
        data.real_coords += data.real_increment;
        data.x_calculated += 1;
        if (data.x_calculated >= data.n_real) {
            data.real_coords = data.start_real_chunk;
            data.x_calculated = 0;
            data.imag_coords += data.imag_increment;
            data.y_calculated += 1;
            if (data.y_calculated >= data.n_imag) {
                data.computing = false;
                data.done = true;
            }
        }
    }
}

bool done_computing() { return data.done; }

