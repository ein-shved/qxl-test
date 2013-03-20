#ifndef TEST_QXL_DEVICE_H
#define TEST_QXL_DEVICE_H

#define DEBUG

#include <spice.h>
#include <stdio.h>

#ifndef BOOL
#   define BOOL int
#endif //BOOL

#ifndef TRUE
#   define TRUE 1
#endif //TRUE

#ifndef FALSE
#   define FALSE 0
#endif //FALSE

#define dprint(qxl, lvl, fmt, ...) printf(fmt, __VA_ARGS__)
#define __FUNCTION__ __func__

struct _test_qxl_t {
    QXLInstance display_sin;
    QXLWorker *worker;
    SpiceCoreInterface *core;
    SpiceServer *spice_server;

    int worker_running;
};
typedef struct _test_qxl_t test_qxl_t;

QXLInterface *test_init_qxl(void);

#endif //TEST_QXL_DEVICE_H
