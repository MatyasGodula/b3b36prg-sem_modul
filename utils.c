/*
 *
 * Author1: Jan Faigl
 * Author2: Matyas Godula
 *
 */

#include "utils.h"

#define RESET          "\033[0m"
#define RED            "\033[0;31m"
#define GREEN          "\033[0;32m"
#define YELLOW         "\033[0;33m"
#define BLUE           "\033[0;34m"
#define MAGENTA        "\033[0;35m"
#define CYAN           "\033[0;36m"
#define WHITE          "\033[0;37m"

#define BRIGHT_BLUE    "\033[0;94m"
#define BRIGHT_RED     "\033[0;91m"
#define BRIGHT_GREEN   "\033[0;92m"
#define BRIGHT_YELLOW  "\033[0;93m"
#define BRIGHT_BLUE    "\033[0;94m"
#define BRIGHT_MAGENTA "\033[0;95m"
#define BRIGHT_CYAN    "\033[0;96m"

#define BOLD_RED       "\033[1;31m"
#define BOLD_GREEN     "\033[1;32m"
#define BOLD_YELLOW    "\033[1;33m"
#define BOLD_BLUE      "\033[1;34m"
#define BOLD_MAGENTA   "\033[1;35m"
#define BOLD_CYAN      "\033[1;36m"
// these lines above were written by chatgpt

void my_assert(bool r, const char *func_name, int line, const char *file_name)
{
    if (!r) {
        fprintf(stderr, "\rError: %s() failed on line %d in file %s\n", func_name, line, file_name);
        exit(GENERIC_ERROR);
    }  
}

void* my_alloc(size_t size)
{
    void* ret = (void*)malloc(size);
    if (!ret) {
        fprintf(stderr, MALLOC_ERR_MESSAGE, __func__, __FILE__, __LINE__);
        exit(MALLOC_ERROR);
    }
    return ret;
}

void call_termios(int reset)
{
    static struct termios tio, tioOld;
    tcgetattr(STDIN_FILENO, &tio);
    if (reset) {
        tcsetattr(STDIN_FILENO, TCSANOW, &tioOld);
    } else {
        tioOld = tio; //backup 
        cfmakeraw(&tio);
        tio.c_oflag |= OPOST;
        tcsetattr(STDIN_FILENO, TCSANOW, &tio);
    }
}

void info(const char *str) 
{
    fprintf(stderr, BRIGHT_GREEN "INFO: " RESET);
    fprintf(stderr, "%s\n", str);
}

void debug(const char *str)
{
    fprintf(stderr, "DEBUG: ");
    fprintf(stderr, "%s\n", str);
}

void error(const char *str)
{
    fprintf(stderr, BRIGHT_RED "ERROR: " RESET);
    fprintf(stderr, "%s\n", str);
}

void warning(const char* str) 
{
    fprintf(stderr, RED "WARNING: " RESET);
    fprintf(stderr, "%s\n", str);
}

void display_module_ver(int major, int minor, int patch) 
{
    fprintf(stderr, BRIGHT_BLUE "MODULE VERSION: " RESET);
    fprintf(stderr, "%d.%d-p%d\n", major, minor, patch);
}

void report_cid(int cid) 
{
    fprintf(stderr, BRIGHT_CYAN "CID: " RESET "Calculating for cid %d\n", cid);
}